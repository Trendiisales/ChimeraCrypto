#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// EthBtcLeadLagEngine.hpp
// Chimera — Phase 1 BTC-targeting engine: ETH→BTC 1-3 minute lead-lag scalp
//
// EDGE:
//   ETH and BTC have ~0.85-0.92 daily realised correlation (2024-2026), but on
//   1-5 minute timeframes ETH leads BTC ~55-65% of the time during US/EU
//   sessions. When ETH makes a meaningful directional move and BTC has not
//   yet caught up, BTC tends to mean-revert TO the ETH move within minutes.
//
//   This engine watches that gap. When ETH has moved >= ETH_LEAD_BP_MIN over
//   the last LEAD_WINDOW_MS milliseconds AND BTC's move over the same window
//   is less than BTC_LAG_BP_MAX (i.e. the lag is real, not a co-move), it
//   opens a half-size BTC spot LONG and exits on TP / trail / SL / timeout.
//
//   Spot-only. Long-only (we cannot short spot in shadow mode without margin
//   wiring, and the deep-dive's primary edge case is the upside lag).
//   Independent of PerpFeed — uses BinanceWSFeed (spot) only.
//
// ENTRY CONDITIONS (all must be true):
//   1. Both BTC and ETH have at least MIN_SAMPLES samples in their buffers.
//   2. ETH return over LEAD_WINDOW_MS >= ETH_LEAD_BP_MIN (positive, bullish).
//   3. BTC return over the same window is in [-BTC_LAG_BP_MAX, BTC_LAG_BP_MAX]
//      (i.e. BTC has not yet moved much in either direction).
//   4. Not in cooldown (COOLDOWN_MS after the previous exit).
//   5. No position currently open.
//   6. available_R >= MIN_AVAIL_R (placeholder until Tier-1 risk wrapper
//      lands; main.cpp passes 1.0).
//
// EXIT (whichever fires first):
//   - TP at +TP_BP (gross, before round-trip cost).
//   - Trail: arms at +TRAIL_ARM_BP MFE, trails TRAIL_DIST_BP below the peak.
//     Trail is monotone (only ratchets up, never gives ground).
//   - SL at -STOP_BP (gross).
//   - Timeout at MAX_HOLD_MS (if BTC hasn't followed ETH within this window
//     the signal is stale).
//
// COST MODEL:
//   Round-trip uses TradingConfig::MAKER_ROUND_TRIP_BP (15 bp with BNB
//   discount). Net P&L = (gross_bp - 15) * pos_size_R.
//
// SYMBOLS:
//   BTC (id=0) — the trade leg. ETH (id=1) — the trigger leg only; never
//   traded by this engine. All other symbols are ignored.
//
// MOVE 2 / SHADOW WIRING (mirrors FundingWindowEngine convention):
//   shadow_mode flag       : default true; gates any future executor call.
//   halted_ flag           : set by kill_all(); blocks new entries until clear_halt().
//   on_tick(...)            : adapter so main.cpp can call uniformly per tick.
//                             For BTC ticks it updates the buffer AND evaluates
//                             entry/manage. For ETH ticks it only updates the
//                             buffer. All other symbols are ignored.
//   kill_all(last_btc_price, now_ms)
//                           : flattens any open paper position at the supplied
//                             price, books P&L, sets halted_.
//   state_json(btc_px, eth_px)
//                           : returns JSON of internal Stats + flags + signal
//                             readouts for the GUI / /api/state2 endpoint.
//
// THREAD-SAFETY:
//   No internal locking. main.cpp serialises every call via g_engine_mtx the
//   same way it does for FundingWindow / BasisMomentum / OBI — see Move 2.
// ============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "core/SymbolIndex.hpp"
#include "live/BinanceWSFeed.hpp"
#include "risk/Tier1Risk.hpp"

namespace chimera {

class EthBtcLeadLagEngine {
public:
    // Tier1Risk identity (session 6 wiring) — engine trades BTC-only.
    static constexpr chimera::risk::EngineType ETYPE =
        chimera::risk::EngineType::ETH_BTC_LEADLAG;

    // ── Cost model (single source of truth in TradingConfig) ──────────────
    static constexpr double  ROUND_TRIP_COST_BP = TradingConfig::MAKER_ROUND_TRIP_BP; // 15bp

    // ── Signal window ──────────────────────────────────────────────────────
    // 90s sits in the middle of the deep-dive's "1-5 min" lead-lag window.
    // Buffer retains 120s of samples so we always have at least 90s of
    // history available even after a brief feed gap.
    static constexpr int64_t LEAD_WINDOW_MS    = 90000;   // 90 seconds
    static constexpr int64_t BUFFER_RETAIN_MS  = 120000;  // 120 seconds
    static constexpr int     MIN_SAMPLES       = 5;       // need ~5 ticks per leg

    // ── Entry thresholds ───────────────────────────────────────────────────
    // ETH must have made a meaningful directional move over the lookback.
    // 25 bp on ETH ~= a 1-2x ATR(1min) move during normal vol — large enough
    // to be a real impulse, small enough to fire a few times per session.
    static constexpr double  ETH_LEAD_BP_MIN   = 25.0;

    // BTC's move over the same window must be small. If BTC has already
    // moved more than half of ETH's threshold in the same direction the
    // lead-lag has already played out — no edge left. Symmetric range
    // because we also require BTC NOT to have moved sharply against the
    // ETH direction (that would be a true divergence, different signal).
    static constexpr double  BTC_LAG_BP_MAX    = 12.0;

    static constexpr double  MIN_AVAIL_R       = 0.5;     // skip if risk budget too low

    // ── Position management ────────────────────────────────────────────────
    static constexpr double  TP_BP             = 35.0;    // gross take-profit
    static constexpr double  STOP_BP           = 25.0;    // gross stop-loss (positive number)
    static constexpr double  TRAIL_ARM_BP      = 20.0;    // MFE level to arm the trail
    static constexpr double  TRAIL_DIST_BP     = 12.0;    // trail this far below peak
    static constexpr int64_t MAX_HOLD_MS       = 360000;  // 6 min — signal is stale beyond this
    static constexpr int64_t COOLDOWN_MS       = 480000;  // 8 min between trades

    // ── Sizing ─────────────────────────────────────────────────────────────
    // Phase 1 deep-dive specifies "half-size BTC follower". With the Tier-1
    // risk wrapper still pending, available_R is 1.0; we apply our own 0.5
    // multiplier here so the size in P&L bookkeeping is correct now and
    // remains correct once the wrapper lands and starts varying R.
    static constexpr double  SIZE_FRAC_OF_R    = 0.5;

    EthBtcLeadLagEngine() = default;

    // Tier1Risk integration setter (session 6 wiring) — null-safe.
    void set_risk(chimera::risk::Tier1Risk* r) { risk_ = r; }

    // ── shadow-mode gate (mirrors SwingEngine / FundingWindowEngine convention) ──
    // Default true so the engine paper-trades. Real-execution wiring is
    // intentionally deferred until Tier-1 risk wrapper exists.
    bool shadow_mode = true;

    // ── per-tick entry point ───────────────────────────────────────────────
    // Called from main.cpp's spot tick callback. We accept ANY symbol id
    // here so main.cpp can route every tick uniformly; the engine ignores
    // anything that isn't BTC or ETH.
    //
    // For ETH ticks: update the ETH price buffer only.
    // For BTC ticks: update the BTC price buffer AND evaluate entry/manage.
    // This means evaluation cadence is bound to BTC ticks — the asset we
    // actually trade — which is the correct anchor.
    void on_tick(int symbol_id,
                 const MarketTick& tick,
                 int64_t now_ms,
                 double available_R) {
        if (halted_) return;
        if (symbol_id != SYM_BTC && symbol_id != SYM_ETH) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        // Update the buffer for whichever leg this tick is.
        _push_price(symbol_id, price, now_ms);

        // Manage / evaluate only on BTC ticks (the trade leg).
        if (symbol_id != SYM_BTC) return;

        last_btc_px_ = price;
        last_now_ms_ = now_ms;

        if (pos_active_) {
            _manage(price, now_ms);
        } else {
            _try_enter(price, now_ms, available_R);
        }
    }

    // ── kill switch (mirrors FundingWindowEngine::kill_all) ────────────────
    void kill_all(double last_btc_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            const double exit_px = (last_btc_price > 0.0) ? last_btc_price
                                                          : (last_btc_px_ > 0.0 ? last_btc_px_
                                                                                : entry_price_);
            const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            ++total_trades_;
            if (net_bp > 0) ++wins_;

            std::printf("[ELL-KILL] BTC | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            trail_floor_bp_     = -9999.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_now_ms_) + COOLDOWN_MS;

            // Tier1Risk: release per-engine R for the killed BTC position.
            // main.cpp's /api/kill handler centralises the halt_all() call.
            if (risk_) risk_->on_position_close(ETYPE, net_bp);
        }
        halted_ = true;
        std::printf("[ELL-KILL] engine halted; clear_halt() to resume\n");
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    // ── stats ──────────────────────────────────────────────────────────────
    struct Stats {
        bool   active;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double trail_floor_bp;
        bool   trail_armed;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
        // Last-evaluated signal readouts (whether or not we entered)
        double last_eth_lead_bp;
        double last_btc_lag_bp;
    };

    Stats get_stats() const {
        return {
            pos_active_, entry_price_, pos_mfe_bp_, pos_mae_bp_,
            trail_floor_bp_, pos_mfe_bp_ >= TRAIL_ARM_BP,
            total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0,
            total_pnl_bp_, total_trades_,
            last_eth_lead_bp_, last_btc_lag_bp_
        };
    }

    int    total_trades()    const { return total_trades_; }
    double total_pnl_bp()    const { return total_pnl_bp_; }
    bool   position_active() const { return pos_active_; }

    // ── per-engine state JSON for GUI / /api/state2 ────────────────────────
    // Caller passes the latest BTC and ETH spot prices so the snapshot
    // reflects current market context (open-trade move_bp, current windowed
    // ETH lead / BTC lag readouts) rather than just internal counters.
    std::string state_json(double btc_price = 0.0,
                           double eth_price = 0.0) const {
        const Stats s = get_stats();

        const double move_bp = (pos_active_ && entry_price_ > 0.0 && btc_price > 0.0)
            ? (btc_price - entry_price_) / entry_price_ * 10000.0
            : 0.0;

        // Recompute current windowed leads against the latest prices so the
        // GUI shows live signal context, not the entry-time snapshot.
        double live_eth_bp = 0.0;
        double live_btc_bp = 0.0;
        _compute_window_returns(live_eth_bp, live_btc_bp);

        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{"
           << "\"engine\":\"eth_btc_leadlag\","
           << "\"trade_symbol\":\"btcusdt\","
           << "\"trigger_symbol\":\"ethusdt\","
           << "\"shadow_mode\":"      << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"           << (halted_     ? "true" : "false") << ","
           << "\"active\":"           << (s.active    ? "true" : "false") << ","
           << "\"entry_price\":"      << s.entry_price << ","
           << "\"btc_price\":"        << btc_price     << ","
           << "\"eth_price\":"        << eth_price     << ","
           << "\"move_bp\":"          << move_bp       << ","
           << "\"mfe_bp\":"           << s.mfe_bp      << ","
           << "\"mae_bp\":"           << s.mae_bp      << ","
           << "\"trail_floor_bp\":"   << s.trail_floor_bp << ","
           << "\"trail_armed\":"      << (s.trail_armed ? "true" : "false") << ","
           << "\"win_rate\":"         << s.win_rate    << ","
           << "\"total_pnl_bp\":"     << s.total_pnl_bp << ","
           << "\"total_trades\":"     << s.total_trades << ","
           << "\"size_R\":"           << pos_size_R_   << ","
           << "\"last_eth_lead_bp\":" << s.last_eth_lead_bp << ","
           << "\"last_btc_lag_bp\":"  << s.last_btc_lag_bp  << ","
           << "\"live_eth_window_bp\":" << live_eth_bp << ","
           << "\"live_btc_window_bp\":" << live_btc_bp << ","
           << "\"window_ms\":"        << LEAD_WINDOW_MS << ","
           << "\"eth_lead_threshold_bp\":" << ETH_LEAD_BP_MIN << ","
           << "\"btc_lag_threshold_bp\":"  << BTC_LAG_BP_MAX
           << "}";
        return js.str();
    }

private:
    struct PriceSample {
        double  price;
        int64_t ts_ms;
    };

    // ── price buffers ──────────────────────────────────────────────────────
    // Two buffers indexed by SYM_BTC (0) and SYM_ETH (1). We don't need
    // generality across MAX_SYMBOLS — the engine is BTC↔ETH specific by
    // design — so two named members keep the intent obvious.
    std::deque<PriceSample> btc_buf_;
    std::deque<PriceSample> eth_buf_;

    // ── position state ─────────────────────────────────────────────────────
    bool    pos_active_         = false;
    double  pos_size_R_         = 0.0;
    double  entry_price_        = 0.0;
    int64_t entry_ms_           = 0;
    double  pos_mfe_bp_         = 0.0;
    double  pos_mae_bp_         = 0.0;
    double  trail_floor_bp_     = -9999.0;
    int64_t cooldown_until_ms_  = 0;

    // ── kill-switch state ──────────────────────────────────────────────────
    bool    halted_             = false;

    // ── running counters ───────────────────────────────────────────────────
    int     wins_               = 0;
    int     total_trades_       = 0;
    double  total_pnl_bp_       = 0.0;

    // ── last-seen context (for kill / JSON readout) ────────────────────────
    double  last_btc_px_        = 0.0;
    int64_t last_now_ms_        = 0;

    // ── last-evaluated signal readout (sticky between ticks for GUI) ──────
    mutable double last_eth_lead_bp_ = 0.0;
    mutable double last_btc_lag_bp_  = 0.0;

    // ──────────────────────────────────────────────────────────────────────
    void _push_price(int symbol_id, double price, int64_t now_ms) {
        std::deque<PriceSample>& buf = (symbol_id == SYM_BTC) ? btc_buf_ : eth_buf_;
        buf.push_back({price, now_ms});
        while (!buf.empty() && (now_ms - buf.front().ts_ms) > BUFFER_RETAIN_MS) {
            buf.pop_front();
        }
    }

    // Look back LEAD_WINDOW_MS into a buffer and return the price that was
    // most recent at-or-before the cutoff. Returns 0.0 if buffer doesn't yet
    // span the lookback (i.e. signal can't fire yet).
    double _price_at_lookback(const std::deque<PriceSample>& buf,
                              int64_t now_ms) const {
        if ((int)buf.size() < MIN_SAMPLES) return 0.0;
        const int64_t cutoff = now_ms - LEAD_WINDOW_MS;
        if (buf.front().ts_ms > cutoff) return 0.0;  // not enough history span yet

        double best = 0.0;
        for (const auto& p : buf) {
            if (p.ts_ms <= cutoff) best = p.price;
            else break;
        }
        return best;
    }

    // Compute windowed bp returns for both legs at the most recent ts on
    // record. Writes 0 to outputs if either leg lacks enough history.
    // Used both by the entry gate and by the live-readout fields in JSON.
    void _compute_window_returns(double& eth_bp, double& btc_bp) const {
        eth_bp = 0.0;
        btc_bp = 0.0;
        if (btc_buf_.empty() || eth_buf_.empty()) return;
        const int64_t now = std::max(btc_buf_.back().ts_ms, eth_buf_.back().ts_ms);

        const double eth_now  = eth_buf_.back().price;
        const double btc_now  = btc_buf_.back().price;
        const double eth_then = _price_at_lookback(eth_buf_, now);
        const double btc_then = _price_at_lookback(btc_buf_, now);
        if (eth_then <= 0.0 || btc_then <= 0.0) return;

        eth_bp = (eth_now - eth_then) / eth_then * 10000.0;
        btc_bp = (btc_now - btc_then) / btc_then * 10000.0;
    }

    // ── entry gate ─────────────────────────────────────────────────────────
    void _try_enter(double btc_price, int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;

        double eth_bp = 0.0, btc_bp = 0.0;
        _compute_window_returns(eth_bp, btc_bp);
        last_eth_lead_bp_ = eth_bp;
        last_btc_lag_bp_  = btc_bp;

        if (eth_bp < ETH_LEAD_BP_MIN)            return;  // ETH didn't lead enough
        if (std::fabs(btc_bp) > BTC_LAG_BP_MAX)  return;  // BTC already moved (either way)

        // ── Enter BTC long ────────────────────────────────────────────────
        pos_active_     = true;
        entry_price_    = btc_price;
        entry_ms_       = now_ms;
        pos_mfe_bp_     = 0.0;
        pos_mae_bp_     = 0.0;
        trail_floor_bp_ = -9999.0;
        pos_size_R_     = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[ELL-SHADOW-ENTRY]" : "[ELL-ENTRY]";
        std::printf("%s BTC LONG | eth_lead=%.2fbp (>=%.1f) | btc_lag=%.2fbp (|.|<=%.1f) | "
                    "px=%.4f | size=%.2fR | window_ms=%lld\n",
            pfx,
            eth_bp, ETH_LEAD_BP_MIN,
            btc_bp, BTC_LAG_BP_MAX,
            btc_price, pos_size_R_,
            (long long)LEAD_WINDOW_MS);
        std::fflush(stdout);

        // Tier1Risk: register the BTC long position with the risk wrapper.
        if (risk_) risk_->on_position_open(ETYPE, SYM_BTC,
                                           /*is_long=*/true, pos_size_R_);
    }

    // ── manage open position ───────────────────────────────────────────────
    void _manage(double btc_price, int64_t now_ms) {
        const double move_bp = (btc_price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

        // Arm / ratchet trail
        if (pos_mfe_bp_ >= TRAIL_ARM_BP) {
            const double new_floor = pos_mfe_bp_ - TRAIL_DIST_BP;
            if (new_floor > trail_floor_bp_) trail_floor_bp_ = new_floor;
        }

        const bool tp      = move_bp >= TP_BP;
        const bool sl      = move_bp <= -STOP_BP;
        const bool trail   = (pos_mfe_bp_ >= TRAIL_ARM_BP) && (move_bp <= trail_floor_bp_);
        const bool timeout = (now_ms - entry_ms_) > MAX_HOLD_MS;

        if (!(tp || sl || trail || timeout)) return;

        const double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
        total_pnl_bp_ += net_bp;
        ++total_trades_;
        if (net_bp > 0) ++wins_;

        const char* reason = tp      ? "TP"
                           : trail   ? "TRAIL"
                           : sl      ? "SL"
                           :           "TIMEOUT";

        const char* pfx = shadow_mode ? "[ELL-SHADOW-EXIT]" : "[ELL-EXIT]";
        std::printf("%s BTC | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                    "mfe=%.1f mae=%.1f | hold_ms=%lld | total=%.1fbp wins=%d/%d\n",
            pfx,
            net_bp, move_bp, ROUND_TRIP_COST_BP,
            reason,
            pos_mfe_bp_, pos_mae_bp_,
            (long long)(now_ms - entry_ms_),
            total_pnl_bp_, wins_, total_trades_);
        std::fflush(stdout);

        pos_active_         = false;
        entry_price_        = 0.0;
        trail_floor_bp_     = -9999.0;
        cooldown_until_ms_  = now_ms + COOLDOWN_MS;

        // Tier1Risk: release per-engine R + feed daily-loss circuit.
        if (risk_) risk_->on_position_close(ETYPE, net_bp);
    }

    // ── Tier1Risk wiring (session 6) ───────────────────────────────────────
    chimera::risk::Tier1Risk* risk_ = nullptr;
};

} // namespace chimera
