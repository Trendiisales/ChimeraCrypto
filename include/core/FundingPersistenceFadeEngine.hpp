#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// FundingPersistenceFadeEngine.hpp
// Chimera — Multi-day BTC mean reversion via persistent funding rate extremes
//
// EDGE:
//   The 8-hour funding rate on Binance perp BTCUSDT is a published, real-time
//   measure of perp-vs-spot positioning imbalance. When funding stays
//   sustainedly negative over 24+ hours (multiple consecutive funding
//   periods), it indicates the SHORT side is paying premium to keep their
//   positions open — a classic capitulation / short-squeeze setup. Spot
//   tends to rally 3-7% over the following 3-7 days as funding normalises.
//
//   This is the multi-day complement to the existing FundingWindowEngine
//   (which trades the 3-minute window AROUND funding settlement). The two
//   engines are structurally orthogonal: same data, completely different
//   timeframes.
//
//   NOTE on positive-funding side: when funding is sustainedly positive
//   (longs paying), the historical play is to FADE THE LONGS — short the
//   perp. Spot-only Chimera cannot take that side, so this engine fires
//   only on negative funding extremes. Once a perp short executor lands,
//   the same buffer + signal logic is reusable for the symmetric trade.
//
// SIGNAL (multi-day):
//   Maintain a 26h rolling buffer of funding_rate samples (sampled on every
//   Binance BTC tick from PerpFeed::funding_rate(SYM_BTC)).
//
//   ENTRY (LONG BTC spot):
//     1. 24h-avg funding_rate <= FUNDING_TRIGGER (negative; -10 bp/8h)
//     2. Recent 8h: every sample <= FUNDING_RECENT_MAX (-3 bp/8h) — extreme
//        is sustained, not a single funding-period blip
//     3. Buffer ready: >= MIN_SAMPLES samples spanning >= MIN_BUFFER_SPAN_MS
//     4. No active position, not in cooldown, available_R >= MIN_AVAIL_R
//
//   EXIT (whichever fires first):
//     - Funding-revert: 24h-avg funding_rate >= FUNDING_REVERT (back to
//       neutral or positive; the imbalance has resolved)
//     - TP: +TP_BP gross
//     - SL: -STOP_BP gross
//     - Timeout: MAX_HOLD_MS (7 days)
//
// CURRENT-STATE CAVEAT:
//   PerpFeed::funding_rate(id) currently returns 0.0 on the Tokyo VPS due
//   to the perp-WS data block diagnosed 2026-05-10. While that's the case
//   the engine sees a flat-zero funding rate, never crosses the trigger,
//   and never fires. As soon as perp data is restored — either via VPS
//   migration or a REST fallback inside PerpFeed — this engine starts
//   evaluating signal automatically with no further code change.
//
// COST MODEL:
//   Round-trip = TradingConfig::MAKER_ROUND_TRIP_BP (15 bp).
//   Net P&L = (gross_bp - 15) * size_R.
//
// SHADOW WIRING:
//   shadow_mode = true by default. Paper-only via printf log.
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

namespace chimera {

class FundingPersistenceFadeEngine {
public:
    // ── Cost model ─────────────────────────────────────────────────────────
    static constexpr double  ROUND_TRIP_COST_BP  = TradingConfig::MAKER_ROUND_TRIP_BP; // 15 bp

    // ── Sampling / buffer geometry ─────────────────────────────────────────
    static constexpr int64_t BUFFER_RETAIN_MS    = 26LL * 60 * 60 * 1000; // 26h
    static constexpr int64_t LOOKBACK_24H_MS     = 24LL * 60 * 60 * 1000; // 24h
    static constexpr int64_t RECENT_WINDOW_MS    =  8LL * 60 * 60 * 1000; //  8h
    static constexpr int64_t MIN_BUFFER_SPAN_MS  = 23LL * 60 * 60 * 1000; // need >=23h
    static constexpr int     MIN_SAMPLES         = 200;
    static constexpr int64_t SAMPLE_INTERVAL_MS  = 60000;                 // throttle to 1/min

    // ── Entry thresholds (funding_rate is fractional; bp = rate*10000) ─────
    // Trigger: 24h-avg <= -10 bp / 8h (sustained short-side payment)
    static constexpr double  FUNDING_TRIGGER     = -0.0010;   // -10 bp/8h fraction
    // Recent guard: every sample in last 8h <= -3 bp / 8h
    static constexpr double  FUNDING_RECENT_MAX  = -0.0003;   //  -3 bp/8h fraction
    // Revert exit: 24h-avg back >= 0
    static constexpr double  FUNDING_REVERT      =  0.0;
    static constexpr double  MIN_AVAIL_R         =  0.5;

    // ── Position management ────────────────────────────────────────────────
    static constexpr double  TP_BP               = 500.0;   // +5% gross
    static constexpr double  STOP_BP             = 250.0;   // -2.5% gross
    static constexpr int64_t MAX_HOLD_MS         =  7LL * 24 * 60 * 60 * 1000;  // 7 days
    static constexpr int64_t COOLDOWN_MS         =  3LL * 24 * 60 * 60 * 1000;  // 3 days

    // ── Sizing ─────────────────────────────────────────────────────────────
    static constexpr double  SIZE_FRAC_OF_R      = 0.5;

    FundingPersistenceFadeEngine() = default;

    bool shadow_mode = true;

    // ── Per-tick entry point ───────────────────────────────────────────────
    // Called from main.cpp's Binance spot tick callback (BTC ticks only).
    // funding_rate is read from PerpFeed::funding_rate(SYM_BTC) by main.cpp
    // and passed in (placeholder 0.0 while perp WS is silent — engine
    // self-disables via the trigger logic).
    void on_tick(int symbol_id,
                 const MarketTick& tick,
                 int64_t now_ms,
                 double funding_rate,
                 double available_R) {
        if (halted_) return;
        if (symbol_id != SYM_BTC) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        last_btc_price_ = price;
        last_now_ms_    = now_ms;

        // Throttle the funding-sample stream to 1/min so the buffer's
        // statistical weight per sample is even (Binance ticks fire many
        // times per second; we don't want one minute over-sampled).
        if (now_ms - last_sample_ts_ >= SAMPLE_INTERVAL_MS) {
            last_sample_ts_ = now_ms;
            buffer_.push_back({funding_rate, now_ms});
            while (!buffer_.empty() && (now_ms - buffer_.front().ts_ms) > BUFFER_RETAIN_MS) {
                buffer_.pop_front();
            }
        }

        if (pos_active_) _manage(price, now_ms, funding_rate);
        else             _try_enter(price, now_ms, available_R);
    }

    // ── kill switch ────────────────────────────────────────────────────────
    void kill_all(double last_btc_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            const double exit_px = (last_btc_price > 0.0) ? last_btc_price
                                                          : (last_btc_price_ > 0.0 ? last_btc_price_
                                                                                   : entry_price_);
            const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            ++total_trades_;
            if (net_bp > 0) ++wins_;

            std::printf("[FUND-PERSIST-KILL] BTC | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_now_ms_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[FUND-PERSIST-KILL] engine halted; clear_halt() to resume\n");
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    int    total_trades() const { return total_trades_; }
    double total_pnl_bp() const { return total_pnl_bp_; }
    bool   position_active() const { return pos_active_; }

    // ── /api/state2 JSON ──────────────────────────────────────────────────
    std::string state_json(double btc_price       = 0.0,
                           double funding_rate_now = 0.0) const {
        const double move_bp = (pos_active_ && entry_price_ > 0.0 && btc_price > 0.0)
            ? (btc_price - entry_price_) / entry_price_ * 10000.0
            : 0.0;
        const double avg24      = _avg_window(LOOKBACK_24H_MS);
        const double recent_max = _max_window(RECENT_WINDOW_MS);

        std::ostringstream js;
        js << std::fixed << std::setprecision(6);
        js << "{"
           << "\"engine\":\"funding_persistence_fade\","
           << "\"trade_symbol\":\"btcusdt\","
           << "\"shadow_mode\":"      << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"           << (halted_     ? "true" : "false") << ","
           << "\"active\":"           << (pos_active_ ? "true" : "false") << ","
           << "\"entry_price\":"      << entry_price_ << ","
           << "\"btc_price\":"        << btc_price    << ","
           << "\"move_bp\":"          << move_bp      << ","
           << "\"mfe_bp\":"           << pos_mfe_bp_  << ","
           << "\"mae_bp\":"           << pos_mae_bp_  << ","
           << "\"win_rate\":"
                << (total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0) << ","
           << "\"total_pnl_bp\":"     << total_pnl_bp_ << ","
           << "\"total_trades\":"     << total_trades_ << ","
           << "\"size_R\":"           << pos_size_R_   << ","
           << "\"funding_rate_now\":" << funding_rate_now << ","
           << "\"funding_rate_now_bp\":" << (funding_rate_now * 10000.0) << ","
           << "\"avg_24h_funding\":"  << avg24 << ","
           << "\"avg_24h_funding_bp\":" << (avg24 * 10000.0) << ","
           << "\"recent_8h_max_funding\":" << recent_max << ","
           << "\"recent_8h_max_funding_bp\":" << (recent_max * 10000.0) << ","
           << "\"buffer_samples\":"   << (int)buffer_.size() << ","
           << "\"buffer_span_ms\":"
                << (buffer_.empty() ? 0LL
                                    : (int64_t)(buffer_.back().ts_ms - buffer_.front().ts_ms)) << ","
           << "\"funding_trigger_bp\":"        << (FUNDING_TRIGGER * 10000.0) << ","
           << "\"funding_recent_max_bp\":"     << (FUNDING_RECENT_MAX * 10000.0) << ","
           << "\"funding_revert_bp\":"         << (FUNDING_REVERT * 10000.0)
           << "}";
        return js.str();
    }

private:
    struct Sample {
        double  funding_rate;   // fractional
        int64_t ts_ms;
    };

    std::deque<Sample> buffer_;

    bool    pos_active_         = false;
    double  pos_size_R_         = 0.0;
    double  entry_price_        = 0.0;
    int64_t entry_ms_           = 0;
    double  pos_mfe_bp_         = 0.0;
    double  pos_mae_bp_         = 0.0;
    int64_t cooldown_until_ms_  = 0;

    bool    halted_             = false;
    int     wins_               = 0;
    int     total_trades_       = 0;
    double  total_pnl_bp_       = 0.0;

    double  last_btc_price_     = 0.0;
    int64_t last_now_ms_        = 0;
    int64_t last_sample_ts_     = 0;

    // ──────────────────────────────────────────────────────────────────────
    double _avg_window(int64_t window_ms) const {
        if (buffer_.empty()) return 0.0;
        const int64_t now    = buffer_.back().ts_ms;
        const int64_t cutoff = now - window_ms;
        double sum = 0.0;
        int    n   = 0;
        for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
            if (it->ts_ms < cutoff) break;
            sum += it->funding_rate;
            ++n;
        }
        if (n < MIN_SAMPLES) return 0.0;
        return sum / (double)n;
    }

    double _max_window(int64_t window_ms) const {
        if (buffer_.empty()) return 0.0;
        const int64_t now    = buffer_.back().ts_ms;
        const int64_t cutoff = now - window_ms;
        double mx = -1e9;
        bool any = false;
        for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
            if (it->ts_ms < cutoff) break;
            if (it->funding_rate > mx) mx = it->funding_rate;
            any = true;
        }
        return any ? mx : 0.0;
    }

    bool _buffer_ready() const {
        if ((int)buffer_.size() < MIN_SAMPLES) return false;
        const int64_t span = buffer_.back().ts_ms - buffer_.front().ts_ms;
        return span >= MIN_BUFFER_SPAN_MS;
    }

    void _try_enter(double btc_price, int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;
        if (!_buffer_ready())            return;

        const double avg24      = _avg_window(LOOKBACK_24H_MS);
        const double recent_max = _max_window(RECENT_WINDOW_MS);

        if (avg24      > FUNDING_TRIGGER)    return;  // 24h-avg not negative enough
        if (recent_max > FUNDING_RECENT_MAX) return;  // recent window has a non-extreme sample

        pos_active_   = true;
        entry_price_  = btc_price;
        entry_ms_     = now_ms;
        pos_mfe_bp_   = 0.0;
        pos_mae_bp_   = 0.0;
        pos_size_R_   = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[FUND-PERSIST-SHADOW-ENTRY]" : "[FUND-PERSIST-ENTRY]";
        std::printf("%s BTC LONG | avg24h_fund=%.4f%% (=%.2fbp/8h, <=%.2fbp) | "
                    "recent8h_max=%.4f%% (=%.2fbp/8h, <=%.2fbp) | "
                    "px=%.4f | size=%.2fR | buf_n=%d span_ms=%lld\n",
            pfx,
            avg24 * 100.0, avg24 * 10000.0, FUNDING_TRIGGER * 10000.0,
            recent_max * 100.0, recent_max * 10000.0, FUNDING_RECENT_MAX * 10000.0,
            btc_price, pos_size_R_,
            (int)buffer_.size(),
            (long long)(buffer_.back().ts_ms - buffer_.front().ts_ms));
        std::fflush(stdout);
    }

    void _manage(double btc_price, int64_t now_ms, double /*funding_rate_now*/) {
        const double move_bp = (btc_price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

        const double avg24 = _avg_window(LOOKBACK_24H_MS);

        const bool tp        = move_bp >= TP_BP;
        const bool sl        = move_bp <= -STOP_BP;
        const bool reverted  = avg24 >= FUNDING_REVERT;
        const bool timeout   = (now_ms - entry_ms_) > MAX_HOLD_MS;

        if (!(tp || sl || reverted || timeout)) return;

        const double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
        total_pnl_bp_ += net_bp;
        ++total_trades_;
        if (net_bp > 0) ++wins_;

        const char* reason = tp        ? "TP"
                           : sl        ? "SL"
                           : reverted  ? "FUND_REVERT"
                           :             "TIMEOUT";

        const char* pfx = shadow_mode ? "[FUND-PERSIST-SHADOW-EXIT]" : "[FUND-PERSIST-EXIT]";
        std::printf("%s BTC | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                    "avg24h_fund_bp=%.2f | mfe=%.1f mae=%.1f | hold_h=%.1f | "
                    "total=%.1fbp wins=%d/%d\n",
            pfx,
            net_bp, move_bp, ROUND_TRIP_COST_BP,
            reason, avg24 * 10000.0,
            pos_mfe_bp_, pos_mae_bp_,
            (double)(now_ms - entry_ms_) / 3600000.0,
            total_pnl_bp_, wins_, total_trades_);
        std::fflush(stdout);

        pos_active_         = false;
        entry_price_        = 0.0;
        cooldown_until_ms_  = now_ms + COOLDOWN_MS;
    }
};

} // namespace chimera
