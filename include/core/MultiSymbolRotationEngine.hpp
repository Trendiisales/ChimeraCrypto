#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// MultiSymbolRotationEngine.hpp
// Chimera — Cross-sectional momentum rotation across the 6 currently-unused
//          spot symbols (SOL, BNB, AVAX, LINK, XRP, DOGE)
//
// EDGE:
//   In a small basket of mid-large-cap altcoins, the strongest 4-hour
//   relative-momentum performer tends to keep outperforming for at least
//   a few hours. Capture that persistence by holding the leader long and
//   rotating into the new leader when leadership changes by enough margin
//   to overcome the round-trip transaction cost.
//
//   This is the natural complement to RangeMeanReversionEngine (which
//   trades BTC + ETH on the minute timescale): MSR trades on the
//   hour-to-day timescale and activates the 6 symbols that no other
//   Chimera engine currently touches. It deliberately avoids BTC and
//   ETH so it doesn't compete with the existing engines for capital.
//
// SCOPE:
//   Symbols: SYM_SOL (2), SYM_BNB (3), SYM_AVAX (4), SYM_LINK (5),
//            SYM_XRP (6), SYM_DOGE (7).
//   Direction: LONG-ONLY spot. (Cross-sectional momentum has a viable
//              short side too — fade the WORST performer — but Chimera
//              is spot-only, so we trade only the long side.)
//   Position: Singleton — one position at a time, in the current leader.
//
// SIGNAL (4h timescale, evaluated every 60s):
//   For each of the 6 basket symbols:
//     - Sample mid-price every SAMPLE_INTERVAL_MS (60 s)
//     - Retain BUFFER_RETAIN_MS (5 h) of samples
//     - Compute MOMENTUM_BP = log(price_now / price_4h_ago) * 10000
//     - Compute VOL_FRAC = stddev(prices_last_4h) / mean
//
//   Build a leaderboard sorted by momentum_bp descending.
//
//   ENTRY (LONG top symbol):
//     1. All 6 buffers span >= LOOKBACK_4H_MS — basket is fully primed
//     2. top.momentum_bp >= MIN_MOMENTUM_BP                    (100 bp / 1.00%)
//     3. top.momentum_bp - 2nd.momentum_bp >= MIN_LEAD_BP      (30 bp / 0.30%)
//     4. top.vol_frac <= MAX_VOL_FRAC                          (0.05 / 5.00%)
//        (kills parabolic / dump-and-spike scenarios where the
//         "leader" is just an artefact of recent extreme volatility)
//     5. No active position, not in cooldown, available_R >= MIN_AVAIL_R
//
//   POSITION MANAGEMENT (evaluated every tick once active):
//     - Re-evaluate leaderboard every EVAL_INTERVAL_MS (60 s).
//     - If a different symbol now leads by >= MIN_LEAD_BP at the next
//       eval boundary, EXIT current with reason=ROTATION, then start
//       cooldown. The new entry happens after cooldown clears, on the
//       next evaluation that still satisfies the entry rules.
//     - TP            : +TP_BP gross                          (300 bp)
//     - SL            : -STOP_BP gross                        (150 bp)
//     - Mom collapse  : current_symbol.momentum_bp < MIN_HOLD_BP  (50 bp)
//     - Timeout       : > MAX_HOLD_MS                         (24 h)
//
// COST MODEL:
//   Round-trip = TradingConfig::MAKER_ROUND_TRIP_BP (15 bp).
//   Net P&L per trade = (gross_bp - 15) * size_R.
//
//   With TP=+300bp / SL=-150bp:
//     win  net = +285bp * size_R
//     loss net = -165bp * size_R
//   Break-even win rate ≈ 36% — a reasonable bar for a momentum strategy
//   that trades clear-leader regimes only.
//
// SHADOW WIRING:
//   shadow_mode = true by default. Paper-only via printf log. No
//   executor wiring. Promotion to live trading requires a per-symbol
//   spot executor (the existing SpotExecutor is BTC/ETH-focused but
//   mechanically supports all 8 SymbolIndex entries).
//
// WARM-UP:
//   Cannot enter until ALL 6 basket symbols have a buffer span >=
//   LOOKBACK_4H_MS. That's roughly 4 hours after startup under normal
//   data flow — slightly longer if any symbol's WS is intermittent.
// ============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <array>

#include "core/SymbolIndex.hpp"
#include "live/BinanceWSFeed.hpp"

namespace chimera {

class MultiSymbolRotationEngine {
public:
    // ── Cost model ─────────────────────────────────────────────────────────
    static constexpr double  ROUND_TRIP_COST_BP   = TradingConfig::MAKER_ROUND_TRIP_BP; // 15 bp

    // ── Basket geometry ────────────────────────────────────────────────────
    // Indices into SymbolIndex.hpp. SYM_SOL=2 .. SYM_DOGE=7 inclusive.
    static constexpr int     BASKET_FIRST_ID      = SYM_SOL;   // 2
    static constexpr int     BASKET_LAST_ID       = SYM_DOGE;  // 7
    static constexpr int     BASKET_SIZE          = BASKET_LAST_ID - BASKET_FIRST_ID + 1; // 6

    // ── Sampling / buffer geometry ─────────────────────────────────────────
    static constexpr int64_t SAMPLE_INTERVAL_MS   = 60000;                  //  1 min
    static constexpr int64_t LOOKBACK_4H_MS       =  4LL * 60 * 60 * 1000;  //  4 h
    static constexpr int64_t BUFFER_RETAIN_MS     =  5LL * 60 * 60 * 1000;  //  5 h

    // ── Leaderboard evaluation cadence ─────────────────────────────────────
    static constexpr int64_t EVAL_INTERVAL_MS     = 60000;                  //  1 min

    // ── Entry thresholds ───────────────────────────────────────────────────
    static constexpr double  MIN_MOMENTUM_BP      = 100.0;   // top must have >=+1.00% over 4h
    static constexpr double  MIN_LEAD_BP          =  30.0;   // lead 2nd by >=0.30%
    static constexpr double  MAX_VOL_FRAC         =  0.05;   // top.vol must be <=5%
    static constexpr double  MIN_AVAIL_R          =   0.5;

    // ── Exit thresholds ────────────────────────────────────────────────────
    static constexpr double  TP_BP                = 300.0;   // +3.00% gross
    static constexpr double  STOP_BP              = 150.0;   // -1.50% gross
    static constexpr double  MIN_HOLD_BP          =  50.0;   // mom collapse if active sym <0.50%
    static constexpr int64_t MAX_HOLD_MS          = 24LL * 60 * 60 * 1000; // 24 h
    static constexpr int64_t COOLDOWN_MS          =  2LL * 60 * 60 * 1000; //  2 h

    // ── Sizing ─────────────────────────────────────────────────────────────
    static constexpr double  SIZE_FRAC_OF_R       =  0.5;

    MultiSymbolRotationEngine() = default;

    bool shadow_mode = true;

    // ── Per-tick entry point ───────────────────────────────────────────────
    // Called from main.cpp's spot tick callback for ALL spot symbols. The
    // engine self-routes: any symbol_id outside the basket is ignored.
    void on_tick(int symbol_id,
                 const MarketTick& tick,
                 int64_t now_ms,
                 double available_R) {
        if (halted_) return;
        if (symbol_id < BASKET_FIRST_ID || symbol_id > BASKET_LAST_ID) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        const int slot = symbol_id - BASKET_FIRST_ID;
        last_price_[slot] = price;
        last_now_ms_      = now_ms;

        // 1-min sample cadence per slot.
        if (now_ms - last_sample_ts_[slot] >= SAMPLE_INTERVAL_MS) {
            last_sample_ts_[slot] = now_ms;
            buffer_[slot].push_back({price, now_ms});
            while (!buffer_[slot].empty()
                   && (now_ms - buffer_[slot].front().ts_ms) > BUFFER_RETAIN_MS) {
                buffer_[slot].pop_front();
            }
        }

        // Per-tick management of the active position (cheap; no allocations).
        if (pos_active_) _manage(price, now_ms, symbol_id);

        // Leaderboard re-eval throttled to EVAL_INTERVAL_MS.
        if (now_ms - last_eval_ms_ >= EVAL_INTERVAL_MS) {
            last_eval_ms_ = now_ms;
            if (pos_active_) _maybe_rotate(now_ms);
            else             _try_enter (now_ms, available_R);
        }
    }

    // ── Kill switch (flatten + halt) ───────────────────────────────────────
    void kill_all(double last_known_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            const int active_slot = active_symbol_id_ - BASKET_FIRST_ID;
            const double exit_px = (last_known_price > 0.0)
                ? last_known_price
                : (active_slot >= 0 && active_slot < BASKET_SIZE && last_price_[active_slot] > 0.0
                       ? last_price_[active_slot]
                       : entry_price_);
            const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            ++total_trades_;
            if (net_bp > 0) ++wins_;

            std::printf("[MSR-KILL] %s | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                sym_full(active_symbol_id_),
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_now_ms_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[MSR-KILL] engine halted; clear_halt() to resume\n");
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    int    total_trades()    const { return total_trades_; }
    double total_pnl_bp()    const { return total_pnl_bp_; }
    int    rotations()       const { return rotations_; }
    bool   position_active() const { return pos_active_; }

    // ── /api/state2 JSON ──────────────────────────────────────────────────
    std::string state_json() const {
        // Build a per-slot snapshot. Sort by momentum descending.
        struct Row {
            int     id;
            double  px;
            double  mom_bp;
            double  vol_frac;
            int64_t span_ms;
            int     samples;
        };
        std::array<Row, BASKET_SIZE> rows{};
        for (int slot = 0; slot < BASKET_SIZE; ++slot) {
            rows[slot].id      = BASKET_FIRST_ID + slot;
            rows[slot].px      = last_price_[slot];
            double mom = 0.0, vf = 0.0;
            int64_t span = 0; int n = 0;
            _slot_metrics(slot, mom, vf, span, n);
            rows[slot].mom_bp   = mom;
            rows[slot].vol_frac = vf;
            rows[slot].span_ms  = span;
            rows[slot].samples  = n;
        }
        std::sort(rows.begin(), rows.end(),
                  [](const Row& a, const Row& b){ return a.mom_bp > b.mom_bp; });

        const bool basket_ready = _basket_ready();

        const int    active_slot = pos_active_ ? (active_symbol_id_ - BASKET_FIRST_ID) : -1;
        const double active_px   = (active_slot >= 0 && active_slot < BASKET_SIZE)
                                       ? last_price_[active_slot] : 0.0;
        const double move_bp     = (pos_active_ && entry_price_ > 0.0 && active_px > 0.0)
            ? (active_px - entry_price_) / entry_price_ * 10000.0
            : 0.0;

        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{"
           << "\"engine\":\"multi_symbol_rotation\","
           << "\"shadow_mode\":"     << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"          << (halted_     ? "true" : "false") << ","
           << "\"active\":"          << (pos_active_ ? "true" : "false") << ","
           << "\"basket_ready\":"    << (basket_ready ? "true" : "false") << ","
           << "\"current_symbol\":\""
              << (pos_active_ ? sym_full(active_symbol_id_) : "")        << "\","
           << "\"current_symbol_id\":"
              << (pos_active_ ? active_symbol_id_ : -1)                  << ","
           << "\"entry_price\":"     << entry_price_                     << ","
           << "\"spot_price\":"      << active_px                        << ","
           << "\"move_bp\":"         << move_bp                          << ","
           << "\"mfe_bp\":"          << pos_mfe_bp_                      << ","
           << "\"mae_bp\":"          << pos_mae_bp_                      << ","
           << "\"size_R\":"          << pos_size_R_                      << ","
           << "\"win_rate\":"
              << (total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0) << ","
           << "\"total_pnl_bp\":"    << total_pnl_bp_                    << ","
           << "\"total_trades\":"    << total_trades_                    << ","
           << "\"rotations\":"       << rotations_                       << ","
           << "\"min_momentum_bp\":" << MIN_MOMENTUM_BP                  << ","
           << "\"min_lead_bp\":"     << MIN_LEAD_BP                      << ","
           << "\"max_vol_frac\":"    << MAX_VOL_FRAC                     << ","
           << "\"cooldown_remaining_ms\":"
              << (cooldown_until_ms_ > last_now_ms_
                     ? (int64_t)(cooldown_until_ms_ - last_now_ms_) : 0LL) << ","
           << "\"leaderboard\":[";
        for (int i = 0; i < (int)rows.size(); ++i) {
            if (i > 0) js << ",";
            js << "{"
               << "\"symbol\":\""    << sym_full(rows[i].id) << "\","
               << "\"price\":"       << rows[i].px           << ","
               << "\"momentum_bp\":" << rows[i].mom_bp       << ","
               << "\"vol_frac\":"    << rows[i].vol_frac     << ","
               << "\"buffer_span_ms\":" << rows[i].span_ms   << ","
               << "\"buffer_samples\":" << rows[i].samples
               << "}";
        }
        js << "]}";
        return js.str();
    }

private:
    struct Sample {
        double  price;
        int64_t ts_ms;
    };

    // Per-slot state (slot = symbol_id - BASKET_FIRST_ID).
    std::deque<Sample>  buffer_[BASKET_SIZE]{};
    double              last_price_[BASKET_SIZE]{};
    int64_t             last_sample_ts_[BASKET_SIZE]{};

    // Position state.
    bool                pos_active_         = false;
    int                 active_symbol_id_   = -1;
    double              pos_size_R_         = 0.0;
    double              entry_price_        = 0.0;
    int64_t             entry_ms_           = 0;
    double              pos_mfe_bp_         = 0.0;
    double              pos_mae_bp_         = 0.0;
    int64_t             cooldown_until_ms_  = 0;

    // Engine state.
    bool                halted_             = false;
    int                 wins_               = 0;
    int                 total_trades_       = 0;
    int                 rotations_          = 0;
    double              total_pnl_bp_       = 0.0;
    int64_t             last_eval_ms_       = 0;
    int64_t             last_now_ms_        = 0;

    // ──────────────────────────────────────────────────────────────────────
    // Per-slot momentum + vol-fraction over the 4h lookback. Returns false
    // (mom=0, vol_frac=0, span/samples reflect actual buffer) if the slot
    // doesn't yet have >= LOOKBACK_4H_MS span.
    bool _slot_metrics(int slot,
                       double& momentum_bp,
                       double& vol_frac,
                       int64_t& span_ms_out,
                       int& samples_out) const {
        momentum_bp = 0.0;
        vol_frac    = 0.0;
        span_ms_out = 0;
        samples_out = 0;
        const auto& buf = buffer_[slot];
        if (buf.size() < 2) return false;
        span_ms_out = buf.back().ts_ms - buf.front().ts_ms;
        samples_out = (int)buf.size();
        if (span_ms_out < LOOKBACK_4H_MS) return false;

        // Find the sample closest to (now - 4h). buf is in chronological order.
        const int64_t target_ts = buf.back().ts_ms - LOOKBACK_4H_MS;
        const Sample* anchor = nullptr;
        // Linear scan; basket is small and 4h is well within retain window.
        for (const auto& s : buf) {
            if (s.ts_ms >= target_ts) { anchor = &s; break; }
            anchor = &s;  // last sample <= target_ts
        }
        if (!anchor || anchor->price <= 0.0) return false;
        const double last_px = buf.back().price;
        if (last_px <= 0.0) return false;
        momentum_bp = std::log(last_px / anchor->price) * 10000.0;

        // Vol-fraction: stddev / mean over the LAST 4h slice (for consistency
        // with the entry rule, which uses the same window as momentum).
        double sum = 0.0;
        int    n   = 0;
        for (const auto& s : buf) {
            if (s.ts_ms < target_ts) continue;
            sum += s.price;
            ++n;
        }
        if (n < 10) return false;
        const double mean = sum / (double)n;
        if (mean <= 0.0) return false;
        double sumsq = 0.0;
        for (const auto& s : buf) {
            if (s.ts_ms < target_ts) continue;
            const double d = s.price - mean;
            sumsq += d * d;
        }
        const double var   = sumsq / (double)(n - 1);
        const double stdev = (var > 0.0) ? std::sqrt(var) : 0.0;
        vol_frac           = stdev / mean;
        return true;
    }

    bool _basket_ready() const {
        for (int slot = 0; slot < BASKET_SIZE; ++slot) {
            const auto& buf = buffer_[slot];
            if (buf.size() < 2) return false;
            const int64_t span = buf.back().ts_ms - buf.front().ts_ms;
            if (span < LOOKBACK_4H_MS) return false;
        }
        return true;
    }

    // Identify the current top + 2nd basket members. Returns true if both
    // are valid (i.e. all metrics computable).
    bool _top_two(int& top_slot,
                  double& top_mom,  double& top_vol,
                  int& second_slot,
                  double& second_mom) const {
        top_slot = -1; second_slot = -1;
        top_mom  = -1e18; second_mom = -1e18;
        top_vol  = 0.0;
        for (int slot = 0; slot < BASKET_SIZE; ++slot) {
            double mom = 0.0, vf = 0.0; int64_t sp = 0; int n = 0;
            if (!_slot_metrics(slot, mom, vf, sp, n)) return false;
            if (mom > top_mom) {
                second_slot = top_slot; second_mom = top_mom;
                top_slot = slot; top_mom = mom; top_vol = vf;
            } else if (mom > second_mom) {
                second_slot = slot; second_mom = mom;
            }
        }
        return (top_slot >= 0 && second_slot >= 0);
    }

    void _try_enter(int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;
        if (!_basket_ready())            return;

        int top_slot = -1, second_slot = -1;
        double top_mom = 0, second_mom = 0, top_vol = 0;
        if (!_top_two(top_slot, top_mom, top_vol, second_slot, second_mom)) return;

        if (top_mom < MIN_MOMENTUM_BP)              return;  // no clear winner
        if ((top_mom - second_mom) < MIN_LEAD_BP)   return;  // not separated enough
        if (top_vol > MAX_VOL_FRAC)                 return;  // top is too volatile

        const int    sym_id   = BASKET_FIRST_ID + top_slot;
        const double price    = last_price_[top_slot];
        if (price <= 0.0) return;

        pos_active_       = true;
        active_symbol_id_ = sym_id;
        entry_price_      = price;
        entry_ms_         = now_ms;
        pos_mfe_bp_       = 0.0;
        pos_mae_bp_       = 0.0;
        pos_size_R_       = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[MSR-SHADOW-ENTRY]" : "[MSR-ENTRY]";
        std::printf("%s %s LONG | mom=%.2fbp 2nd=%.2fbp lead=%.2fbp | "
                    "vol_frac=%.4f (<=%.3f) | px=%.4f | size=%.2fR\n",
            pfx,
            sym_full(sym_id),
            top_mom, second_mom, top_mom - second_mom,
            top_vol, MAX_VOL_FRAC,
            price, pos_size_R_);
        std::fflush(stdout);
    }

    void _maybe_rotate(int64_t now_ms) {
        // Re-evaluate the leaderboard. If a different symbol now leads by
        // >= MIN_LEAD_BP, exit the current position with reason=ROTATION.
        // Re-entry happens after cooldown clears in the next _try_enter.
        int top_slot = -1, second_slot = -1;
        double top_mom = 0, second_mom = 0, top_vol = 0;
        if (!_top_two(top_slot, top_mom, top_vol, second_slot, second_mom)) return;

        const int new_top_id = BASKET_FIRST_ID + top_slot;
        if (new_top_id == active_symbol_id_) return;            // current still leads
        if ((top_mom - second_mom) < MIN_LEAD_BP) return;       // tie/noise — don't churn

        // The new leader differs and the lead is decisive — rotate.
        const int active_slot = active_symbol_id_ - BASKET_FIRST_ID;
        const double active_px = (active_slot >= 0 && active_slot < BASKET_SIZE)
                                     ? last_price_[active_slot] : 0.0;
        if (active_px <= 0.0) return;
        _close_position(active_px, now_ms, "ROTATION", new_top_id);
        ++rotations_;
    }

    void _manage(double tick_price, int64_t now_ms, int tick_symbol_id) {
        // Only price updates from the active symbol move the position book-keeping.
        if (tick_symbol_id != active_symbol_id_) return;

        const double move_bp = (tick_price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

        const bool tp      = move_bp >= TP_BP;
        const bool sl      = move_bp <= -STOP_BP;
        const bool timeout = (now_ms - entry_ms_) > MAX_HOLD_MS;
        bool collapse = false;
        // Momentum collapse — needs a leaderboard recompute, but we don't
        // need to do it every tick; the EVAL cadence gate in on_tick handles
        // that frequency. Here we cheaply recompute just the active slot.
        const int active_slot = active_symbol_id_ - BASKET_FIRST_ID;
        if (active_slot >= 0 && active_slot < BASKET_SIZE) {
            double mom = 0, vf = 0; int64_t sp = 0; int n = 0;
            if (_slot_metrics(active_slot, mom, vf, sp, n)) {
                if (mom < MIN_HOLD_BP) collapse = true;
            }
        }

        if (!(tp || sl || timeout || collapse)) return;

        const char* reason = tp        ? "TP"
                           : sl        ? "SL"
                           : collapse  ? "MOM_COLLAPSE"
                           :             "TIMEOUT";
        _close_position(tick_price, now_ms, reason, /*next_top_id=*/-1);
    }

    void _close_position(double exit_px, int64_t now_ms,
                         const char* reason, int next_top_id_for_log) {
        if (!pos_active_) return;
        const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
        const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
        total_pnl_bp_ += net_bp;
        ++total_trades_;
        if (net_bp > 0) ++wins_;

        const char* pfx = shadow_mode ? "[MSR-SHADOW-EXIT]" : "[MSR-EXIT]";
        if (next_top_id_for_log >= 0) {
            std::printf("%s %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s -> %s | "
                        "mfe=%.1f mae=%.1f | hold_h=%.1f | total=%.1fbp wins=%d/%d rot=%d\n",
                pfx,
                sym_full(active_symbol_id_),
                net_bp, move_bp, ROUND_TRIP_COST_BP, reason,
                sym_full(next_top_id_for_log),
                pos_mfe_bp_, pos_mae_bp_,
                (double)(now_ms - entry_ms_) / 3600000.0,
                total_pnl_bp_, wins_, total_trades_, rotations_);
        } else {
            std::printf("%s %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                        "mfe=%.1f mae=%.1f | hold_h=%.1f | total=%.1fbp wins=%d/%d rot=%d\n",
                pfx,
                sym_full(active_symbol_id_),
                net_bp, move_bp, ROUND_TRIP_COST_BP, reason,
                pos_mfe_bp_, pos_mae_bp_,
                (double)(now_ms - entry_ms_) / 3600000.0,
                total_pnl_bp_, wins_, total_trades_, rotations_);
        }
        std::fflush(stdout);

        pos_active_         = false;
        active_symbol_id_   = -1;
        entry_price_        = 0.0;
        cooldown_until_ms_  = now_ms + COOLDOWN_MS;
    }
};

} // namespace chimera
