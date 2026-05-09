#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// CoinbasePremiumMRevEngine.hpp
// Chimera — Multi-day BTC mean reversion via cross-venue (US-vs-rest) premium
//
// EDGE:
//   The Coinbase BTC-USD vs Binance BTC-USDT premium is the cleanest publicly
//   observable proxy for US-domiciled institutional positioning bias. When
//   Coinbase trades sustainedly BELOW Binance ("Coinbase discount"), US
//   institutions (and ETF authorized participants) are net selling into
//   Asian/European bid; this has historically marked medium-term local
//   bottoms with 3-10% bounces over 3-7 days. The signal works because the
//   actors creating it aren't trading the premium — they're trading their
//   underlying mandate. The premium is a byproduct, so it isn't arbed flat
//   on a 24h timeframe even though it's well-known.
//
// SIGNAL (multi-day):
//   premium_bp = (cb_btc_usd - bn_btc_usdt) / bn_btc_usdt * 10000
//   Maintain a 24h rolling buffer of premium samples (sampled on every
//   Coinbase price update + every Binance BTC tick).
//
//   ENTRY (LONG BTC spot):
//     1. 24h-avg premium <= -PREMIUM_TRIGGER_BP (sustained discount)
//     2. Last RECENT_WINDOW_MS (4h) all samples <= PREMIUM_RECENT_MAX_BP
//        (the discount isn't a single outlier — it persists)
//     3. Both Coinbase and Binance buffers each have >= MIN_SAMPLES samples
//        spanning >= MIN_BUFFER_SPAN_MS history
//     4. No active position, not in cooldown, available_R >= MIN_AVAIL_R
//
//   EXIT (whichever fires first):
//     - Premium-revert: 24h-avg premium >= PREMIUM_REVERT_BP (mean reversion
//       complete; take the structural exit even if price hasn't moved)
//     - TP: +TP_BP gross
//     - SL: -STOP_BP gross
//     - Timeout: MAX_HOLD_MS (10 days)
//
// COST MODEL:
//   Round-trip cost = TradingConfig::MAKER_ROUND_TRIP_BP (15 bp).
//   Net P&L = (gross_bp - 15) * size_R.
//
// SIZING:
//   0.5 * available_R per entry. Multi-day holds — sized conservatively
//   until Tier-1 risk wrapper lands and proper portfolio-level sizing
//   replaces the placeholder.
//
// SHADOW WIRING:
//   shadow_mode = true by default. Paper trades via printf log only;
//   no executor wired. Mirrors FundingWindow / Phase 1 conventions.
//
// THREAD-SAFETY:
//   No internal locking. main.cpp serialises calls via g_engine_mtx.
//   The Coinbase WS feed runs on a separate lws thread, so main.cpp must
//   acquire g_engine_mtx in the Coinbase callback before calling
//   update_coinbase_btc() — same way it does for the Binance tick callback.
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

class CoinbasePremiumMRevEngine {
public:
    // ── Cost model ─────────────────────────────────────────────────────────
    static constexpr double  ROUND_TRIP_COST_BP  = TradingConfig::MAKER_ROUND_TRIP_BP; // 15 bp

    // ── Sampling / buffer geometry ─────────────────────────────────────────
    // Sample premium on every venue update; retain 26 hours of history so
    // the 24h rolling window is always covered even with brief feed gaps.
    static constexpr int64_t BUFFER_RETAIN_MS    = 26LL * 60 * 60 * 1000;   // 26h
    static constexpr int64_t LOOKBACK_24H_MS     = 24LL * 60 * 60 * 1000;   // 24h
    static constexpr int64_t RECENT_WINDOW_MS    =  4LL * 60 * 60 * 1000;   //  4h
    static constexpr int64_t MIN_BUFFER_SPAN_MS  = 23LL * 60 * 60 * 1000;   // need >=23h history before firing
    static constexpr int     MIN_SAMPLES         = 200;                     // ~one per ~7min over 24h

    // ── Entry thresholds (premium = (cb - bn) / bn * 10000, bp) ────────────
    static constexpr double  PREMIUM_TRIGGER_BP    = -25.0;  // 24h-avg <= -25 bp
    static constexpr double  PREMIUM_RECENT_MAX_BP = -10.0;  // last 4h all <= -10 bp
    static constexpr double  MIN_AVAIL_R           = 0.5;

    // ── Exit thresholds ────────────────────────────────────────────────────
    static constexpr double  PREMIUM_REVERT_BP     =   0.0;  // mean-revert exit
    static constexpr double  TP_BP                 = 600.0;  // +6% gross
    static constexpr double  STOP_BP               = 300.0;  // -3% gross (positive number)
    static constexpr int64_t MAX_HOLD_MS           = 10LL * 24 * 60 * 60 * 1000;  // 10 days
    static constexpr int64_t COOLDOWN_MS           =  3LL * 24 * 60 * 60 * 1000;  // 3 days

    // ── Sizing ─────────────────────────────────────────────────────────────
    static constexpr double  SIZE_FRAC_OF_R        = 0.5;

    CoinbasePremiumMRevEngine() = default;

    bool shadow_mode = true;

    // ── Inputs ─────────────────────────────────────────────────────────────
    // Called from main.cpp's Binance spot tick callback (BTC ticks only).
    // Updates the Binance leg of the premium and evaluates entry / manage.
    void update_binance_btc(int symbol_id,
                            const MarketTick& tick,
                            int64_t now_ms,
                            double available_R) {
        if (halted_) return;
        if (symbol_id != SYM_BTC) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        last_bn_price_ = price;
        last_bn_ts_    = now_ms;
        _push_premium_sample(now_ms);

        if (pos_active_) _manage(price, now_ms);
        else             _try_enter(price, now_ms, available_R);
    }

    // Called from main.cpp's Coinbase feed callback. Only updates the
    // Coinbase leg; entry/manage evaluation happens on Binance ticks (the
    // trade leg). Caller must hold g_engine_mtx.
    void update_coinbase_btc(double cb_price, int64_t now_ms) {
        if (cb_price <= 0.0) return;
        last_cb_price_ = cb_price;
        last_cb_ts_    = now_ms;
        _push_premium_sample(now_ms);
    }

    // ── kill switch (mirrors FundingWindowEngine::kill_all) ────────────────
    void kill_all(double last_btc_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            const double exit_px = (last_btc_price > 0.0) ? last_btc_price
                                                          : (last_bn_price_ > 0.0 ? last_bn_price_
                                                                                  : entry_price_);
            const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            ++total_trades_;
            if (net_bp > 0) ++wins_;

            std::printf("[CBPREM-KILL] BTC | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_bn_ts_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[CBPREM-KILL] engine halted; clear_halt() to resume\n");
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    int    total_trades() const { return total_trades_; }
    double total_pnl_bp() const { return total_pnl_bp_; }
    bool   position_active() const { return pos_active_; }

    // ── /api/state2 JSON ──────────────────────────────────────────────────
    std::string state_json(double btc_price = 0.0,
                           double cb_price  = 0.0) const {
        const double move_bp = (pos_active_ && entry_price_ > 0.0 && btc_price > 0.0)
            ? (btc_price - entry_price_) / entry_price_ * 10000.0
            : 0.0;

        const double live_premium_bp =
            (cb_price > 0.0 && btc_price > 0.0)
                ? (cb_price - btc_price) / btc_price * 10000.0
                : 0.0;
        const double avg24_bp  = _avg_window_bp(LOOKBACK_24H_MS);
        const double recentMax = _max_window_bp(RECENT_WINDOW_MS);  // closest to zero (least-negative)

        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{"
           << "\"engine\":\"coinbase_premium_mrev\","
           << "\"trade_symbol\":\"btcusdt\","
           << "\"shadow_mode\":"           << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"                << (halted_     ? "true" : "false") << ","
           << "\"active\":"                << (pos_active_ ? "true" : "false") << ","
           << "\"entry_price\":"           << entry_price_ << ","
           << "\"btc_price\":"             << btc_price    << ","
           << "\"cb_price\":"              << cb_price     << ","
           << "\"move_bp\":"               << move_bp      << ","
           << "\"mfe_bp\":"                << pos_mfe_bp_  << ","
           << "\"mae_bp\":"                << pos_mae_bp_  << ","
           << "\"win_rate\":"
                << (total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0) << ","
           << "\"total_pnl_bp\":"          << total_pnl_bp_ << ","
           << "\"total_trades\":"          << total_trades_ << ","
           << "\"size_R\":"                << pos_size_R_   << ","
           << "\"live_premium_bp\":"       << live_premium_bp << ","
           << "\"avg_24h_premium_bp\":"    << avg24_bp        << ","
           << "\"recent_4h_max_bp\":"      << recentMax       << ","
           << "\"buffer_samples\":"        << (int)buffer_.size() << ","
           << "\"buffer_span_ms\":"
                << (buffer_.empty() ? 0LL
                                    : (int64_t)(buffer_.back().ts_ms - buffer_.front().ts_ms)) << ","
           << "\"premium_trigger_bp\":"    << PREMIUM_TRIGGER_BP << ","
           << "\"premium_recent_max_bp\":" << PREMIUM_RECENT_MAX_BP << ","
           << "\"premium_revert_bp\":"     << PREMIUM_REVERT_BP
           << "}";
        return js.str();
    }

private:
    struct Sample {
        double  premium_bp;
        int64_t ts_ms;
    };

    // ── current venue state ────────────────────────────────────────────────
    double  last_bn_price_      = 0.0;
    int64_t last_bn_ts_         = 0;
    double  last_cb_price_      = 0.0;
    int64_t last_cb_ts_         = 0;

    std::deque<Sample> buffer_;

    // ── position state ─────────────────────────────────────────────────────
    bool    pos_active_         = false;
    double  pos_size_R_         = 0.0;
    double  entry_price_        = 0.0;
    int64_t entry_ms_           = 0;
    double  pos_mfe_bp_         = 0.0;
    double  pos_mae_bp_         = 0.0;
    int64_t cooldown_until_ms_  = 0;

    // ── kill switch + counters ─────────────────────────────────────────────
    bool    halted_             = false;
    int     wins_               = 0;
    int     total_trades_       = 0;
    double  total_pnl_bp_       = 0.0;

    // ──────────────────────────────────────────────────────────────────────
    void _push_premium_sample(int64_t now_ms) {
        if (last_bn_price_ <= 0.0 || last_cb_price_ <= 0.0) return;
        // Require both venue feeds reasonably current (within 60s) so we
        // don't pollute the buffer with stale-leg premium readings.
        const int64_t MAX_LEG_AGE_MS = 60000;
        if (now_ms - last_bn_ts_ > MAX_LEG_AGE_MS) return;
        if (now_ms - last_cb_ts_ > MAX_LEG_AGE_MS) return;

        const double premium_bp =
            (last_cb_price_ - last_bn_price_) / last_bn_price_ * 10000.0;

        buffer_.push_back({premium_bp, now_ms});
        while (!buffer_.empty() && (now_ms - buffer_.front().ts_ms) > BUFFER_RETAIN_MS) {
            buffer_.pop_front();
        }
    }

    // Average premium over the last `window_ms` of buffered samples.
    // Returns 0.0 if buffer doesn't span the window with enough samples.
    double _avg_window_bp(int64_t window_ms) const {
        if (buffer_.empty()) return 0.0;
        const int64_t now    = buffer_.back().ts_ms;
        const int64_t cutoff = now - window_ms;
        double sum   = 0.0;
        int    n     = 0;
        for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
            if (it->ts_ms < cutoff) break;
            sum += it->premium_bp;
            ++n;
        }
        if (n < MIN_SAMPLES) return 0.0;
        return sum / (double)n;
    }

    // Maximum (closest-to-zero or most-positive) premium in the last
    // `window_ms`. Used to verify the recent window has not had a single
    // sample exceed PREMIUM_RECENT_MAX_BP.
    double _max_window_bp(int64_t window_ms) const {
        if (buffer_.empty()) return 0.0;
        const int64_t now    = buffer_.back().ts_ms;
        const int64_t cutoff = now - window_ms;
        double mx = -1e9;
        bool any = false;
        for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
            if (it->ts_ms < cutoff) break;
            if (it->premium_bp > mx) mx = it->premium_bp;
            any = true;
        }
        return any ? mx : 0.0;
    }

    bool _buffer_ready() const {
        if ((int)buffer_.size() < MIN_SAMPLES) return false;
        const int64_t span = buffer_.back().ts_ms - buffer_.front().ts_ms;
        return span >= MIN_BUFFER_SPAN_MS;
    }

    // ── entry gate ─────────────────────────────────────────────────────────
    void _try_enter(double btc_price, int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;
        if (!_buffer_ready())            return;

        const double avg24_bp     = _avg_window_bp(LOOKBACK_24H_MS);
        const double recent_max   = _max_window_bp(RECENT_WINDOW_MS);

        if (avg24_bp     >  PREMIUM_TRIGGER_BP)    return;  // 24h-avg not deep enough
        if (recent_max   >  PREMIUM_RECENT_MAX_BP) return;  // discount not sustained in recent window

        pos_active_     = true;
        entry_price_    = btc_price;
        entry_ms_       = now_ms;
        pos_mfe_bp_     = 0.0;
        pos_mae_bp_     = 0.0;
        pos_size_R_     = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[CBPREM-SHADOW-ENTRY]" : "[CBPREM-ENTRY]";
        std::printf("%s BTC LONG | avg24h_premium=%.2fbp (<=%.1f) | recent4h_max=%.2fbp (<=%.1f) | "
                    "px=%.4f | size=%.2fR | cb=%.4f bn=%.4f | buf_n=%d span_ms=%lld\n",
            pfx,
            avg24_bp, PREMIUM_TRIGGER_BP,
            recent_max, PREMIUM_RECENT_MAX_BP,
            btc_price, pos_size_R_,
            last_cb_price_, last_bn_price_,
            (int)buffer_.size(),
            (long long)(buffer_.back().ts_ms - buffer_.front().ts_ms));
        std::fflush(stdout);
    }

    // ── manage open position ───────────────────────────────────────────────
    void _manage(double btc_price, int64_t now_ms) {
        const double move_bp = (btc_price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

        const double avg24_bp = _avg_window_bp(LOOKBACK_24H_MS);

        const bool tp        = move_bp >= TP_BP;
        const bool sl        = move_bp <= -STOP_BP;
        const bool reverted  = avg24_bp >= PREMIUM_REVERT_BP;
        const bool timeout   = (now_ms - entry_ms_) > MAX_HOLD_MS;

        if (!(tp || sl || reverted || timeout)) return;

        const double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
        total_pnl_bp_ += net_bp;
        ++total_trades_;
        if (net_bp > 0) ++wins_;

        const char* reason = tp        ? "TP"
                           : sl        ? "SL"
                           : reverted  ? "REVERT"
                           :             "TIMEOUT";

        const char* pfx = shadow_mode ? "[CBPREM-SHADOW-EXIT]" : "[CBPREM-EXIT]";
        std::printf("%s BTC | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                    "avg24h_premium=%.2fbp | mfe=%.1f mae=%.1f | hold_h=%.1f | "
                    "total=%.1fbp wins=%d/%d\n",
            pfx,
            net_bp, move_bp, ROUND_TRIP_COST_BP,
            reason, avg24_bp,
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
