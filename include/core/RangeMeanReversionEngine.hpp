#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// RangeMeanReversionEngine.hpp
// Chimera — Short-timeframe range mean reversion (BTC + ETH spot, long-only)
//
// EDGE:
//   On the 1-minute timeframe crypto majors spend the bulk of their hours
//   inside narrow ranges. When price pokes the lower edge of a recent
//   range AND momentum is at an oversold extreme, mean reversion to the
//   middle of the range happens within minutes to a couple of hours, with
//   high frequency. This engine fades those dips.
//
//   It is intentionally the highest-frequency strategy in the portfolio.
//   The slow trio (CoinbasePremium / FundingPersistence / VolCompression)
//   fires once or twice a week; the perp-aware engines fire a handful of
//   times a day; this one is designed to fire 5-20 times per symbol per
//   day under normal liquidity. Combined with FundingPersistenceFade's
//   3-7 day holds, the portfolio's trade-rate distribution becomes much
//   more even.
//
// SCOPE:
//   BTC + ETH spot, LONG-ONLY. The signal is symmetric in principle (fade
//   upper-band touches when RSI is overbought) but Chimera is spot-only,
//   so we only take the long side. Adding more symbols is a one-line
//   change in main.cpp once we decide to extend it to SOL/BNB/etc.
//
// SIGNAL (1-minute timeframe):
//   Maintain a 30-sample ring of 1-minute mid-prices, sampled on a 60s
//   boundary. From these samples derive:
//     mean   = simple moving average over the 30 samples
//     stddev = sample standard deviation
//     upper  = mean + BB_K * stddev    (BB_K defaults to 2.0)
//     lower  = mean - BB_K * stddev
//
//   Maintain an RSI(14) on the same 1-minute close stream using Wilder's
//   smoothing (the standard formulation).
//
//   ENTRY (LONG):
//     1. price <= lower_band                       (fade the dip)
//     2. RSI <= RSI_ENTRY_MAX                      (oversold confirmation)
//     3. stddev / mean in [MIN_VOL_FRAC, MAX_VOL_FRAC]
//          (range is non-trivial but not trending — kills both dead-flat
//           and breakout regimes which this strategy actively fails in)
//     4. Buffer ready: 30 samples spanning >= MIN_BUFFER_SPAN_MS (28 min)
//        and RSI primed (>= RSI_PERIOD samples)
//     5. No active position, not in cooldown, available_R >= MIN_AVAIL_R
//
//   EXIT (whichever fires first):
//     - Mid-band reclaim: price >= mean             (the natural target)
//     - TP : +TP_BP gross                           (cap on big snap-backs)
//     - SL : -STOP_BP gross                         (range broke down)
//     - RSI overshoot: RSI >= RSI_EXIT_MIN          (momentum has flipped)
//     - Timeout: MAX_HOLD_MS                        (reverts that take this
//                                                    long have probably failed)
//
// COST MODEL:
//   Round-trip = TradingConfig::MAKER_ROUND_TRIP_BP (15 bp).
//   Net P&L per trade = (gross_bp - 15) * size_R.
//
//   With TP=+60bp and SL=-40bp:
//     win  net = +45bp * size_R
//     loss net = -55bp * size_R
//   Break-even win rate = 55/(45+55) ≈ 55% — achievable for mean reversion
//   in normal liquidity regimes; underperforms in trend regimes (which is
//   why the vol-fraction band gates entries out of trends).
//
// SHADOW WIRING:
//   shadow_mode = true by default. Paper-only via printf log lines. No
//   executor wiring — this engine never sends an order. When promoted to
//   live, the spot LONG fill goes through the same SpotExecutor that
//   SwingEngine uses (or a separate per-engine executor; an
//   architecture decision for that future session).
//
// WARM-UP:
//   Cannot fire until the buffer holds 30 samples over >=28 min of
//   wall-clock time, AND RSI has at least RSI_PERIOD samples. So
//   approximately the first 30 minutes after startup are silent.
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

class RangeMeanReversionEngine {
public:
    // ── Cost model ─────────────────────────────────────────────────────────
    static constexpr double  ROUND_TRIP_COST_BP  = TradingConfig::MAKER_ROUND_TRIP_BP; // 15 bp

    // ── Sampling / buffer geometry ─────────────────────────────────────────
    static constexpr int64_t SAMPLE_INTERVAL_MS  = 60000;              // 1 min
    static constexpr int     BB_WINDOW           = 30;                 // 30 1-min closes
    static constexpr int64_t MIN_BUFFER_SPAN_MS  = 28LL * 60 * 1000;   // require 28+ min span

    // ── Bollinger geometry ─────────────────────────────────────────────────
    static constexpr double  BB_K                = 2.0;                // band width = mean ± 2σ

    // Range-vol gate: stddev / mean (a unitless "fractional vol" measure).
    // 8  bp  =  0.0008 — below this is dead-flat, MR has no edge
    // 120 bp =  0.0120 — above this is trending, MR loses
    static constexpr double  MIN_VOL_FRAC        = 0.0008;
    static constexpr double  MAX_VOL_FRAC        = 0.0120;

    // ── RSI ────────────────────────────────────────────────────────────────
    static constexpr int     RSI_PERIOD          = 14;
    static constexpr double  RSI_ENTRY_MAX       = 30.0;
    static constexpr double  RSI_EXIT_MIN        = 60.0;

    // ── Entry sizing / risk gate ───────────────────────────────────────────
    static constexpr double  MIN_AVAIL_R         = 0.5;
    static constexpr double  SIZE_FRAC_OF_R      = 0.4;                // smaller per-trade size
                                                                       // (offset by higher freq)

    // ── Position management ────────────────────────────────────────────────
    static constexpr double  TP_BP               =  60.0;              // +0.60% gross
    static constexpr double  STOP_BP             =  40.0;              // -0.40% gross
    static constexpr int64_t MAX_HOLD_MS         =  4LL * 60 * 60 * 1000; //  4h
    static constexpr int64_t COOLDOWN_MS         = 15LL * 60 * 1000;   // 15 min

    // ── Construction ───────────────────────────────────────────────────────
    explicit RangeMeanReversionEngine(const std::string& sym = "")
        : symbol_(sym) {}

    bool shadow_mode = true;

    const std::string& symbol() const { return symbol_; }

    // ── Per-tick entry point ───────────────────────────────────────────────
    // Called from main.cpp for ticks of THIS engine's symbol only. The caller
    // is responsible for routing the right symbol to the right instance in
    // the per-symbol array.
    void on_tick(const MarketTick& tick,
                 int64_t now_ms,
                 double available_R) {
        if (halted_) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        last_price_ = price;
        last_now_ms_ = now_ms;

        // Throttle to 1 sample / minute. The Bollinger / RSI both consume
        // these throttled "1-min closes". We use the latest mid-price seen
        // before the boundary, which is a reasonable proxy for a 1-min close.
        if (now_ms - last_sample_ts_ >= SAMPLE_INTERVAL_MS) {
            last_sample_ts_ = now_ms;
            _ingest_sample(price, now_ms);
        }

        if (pos_active_) _manage(price, now_ms);
        else             _try_enter(price, now_ms, available_R);
    }

    // ── Kill switch (flatten + halt) ───────────────────────────────────────
    void kill_all(double last_known_price = 0.0, int64_t now_ms = 0) {
        if (pos_active_) {
            const double exit_px = (last_known_price > 0.0)
                ? last_known_price
                : (last_price_ > 0.0 ? last_price_ : entry_price_);
            const double move_bp = (exit_px - entry_price_) / entry_price_ * 10000.0;
            const double net_bp  = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
            total_pnl_bp_ += net_bp;
            ++total_trades_;
            if (net_bp > 0) ++wins_;

            std::printf("[RMR-KILL] %s | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                symbol_.c_str(),
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_now_ms_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[RMR-KILL] %s | engine halted; clear_halt() to resume\n",
                    symbol_.c_str());
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    int    total_trades()    const { return total_trades_; }
    double total_pnl_bp()    const { return total_pnl_bp_; }
    bool   position_active() const { return pos_active_; }

    // ── /api/state2 JSON ──────────────────────────────────────────────────
    std::string state_json(double current_price = 0.0) const {
        const double px = (current_price > 0.0) ? current_price : last_price_;

        double mean = 0.0, stdev = 0.0, upper = 0.0, lower = 0.0;
        const bool bb_ok = _compute_bb(mean, stdev, upper, lower);
        const double vol_frac = (mean > 0.0) ? (stdev / mean) : 0.0;

        const double rsi = _rsi_value();

        const double move_bp = (pos_active_ && entry_price_ > 0.0 && px > 0.0)
            ? (px - entry_price_) / entry_price_ * 10000.0
            : 0.0;

        const int64_t buf_span = buffer_.empty() ? 0LL
            : (int64_t)(buffer_.back().ts_ms - buffer_.front().ts_ms);

        std::ostringstream js;
        js << std::fixed << std::setprecision(6);
        js << "{"
           << "\"engine\":\"range_mean_reversion\","
           << "\"trade_symbol\":\""        << symbol_                       << "\","
           << "\"shadow_mode\":"           << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"                << (halted_     ? "true" : "false") << ","
           << "\"active\":"                << (pos_active_ ? "true" : "false") << ","
           << "\"entry_price\":"           << entry_price_                  << ","
           << "\"price\":"                 << px                            << ","
           << "\"move_bp\":"               << move_bp                       << ","
           << "\"mfe_bp\":"                << pos_mfe_bp_                   << ","
           << "\"mae_bp\":"                << pos_mae_bp_                   << ","
           << "\"win_rate\":"
                << (total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0) << ","
           << "\"total_pnl_bp\":"          << total_pnl_bp_                 << ","
           << "\"total_trades\":"          << total_trades_                 << ","
           << "\"size_R\":"                << pos_size_R_                   << ","
           << "\"bb_ready\":"              << (bb_ok ? "true" : "false")    << ","
           << "\"bb_mean\":"               << mean                          << ","
           << "\"bb_stdev\":"              << stdev                         << ","
           << "\"bb_upper\":"              << upper                         << ","
           << "\"bb_lower\":"              << lower                         << ","
           << "\"vol_frac\":"              << vol_frac                      << ","
           << "\"vol_frac_min\":"          << MIN_VOL_FRAC                  << ","
           << "\"vol_frac_max\":"          << MAX_VOL_FRAC                  << ","
           << "\"rsi\":"                   << rsi                           << ","
           << "\"rsi_entry_max\":"         << RSI_ENTRY_MAX                 << ","
           << "\"rsi_exit_min\":"          << RSI_EXIT_MIN                  << ","
           << "\"buffer_samples\":"        << (int)buffer_.size()           << ","
           << "\"buffer_span_ms\":"        << buf_span                      << ","
           << "\"cooldown_remaining_ms\":"
                << (cooldown_until_ms_ > last_now_ms_
                       ? (int64_t)(cooldown_until_ms_ - last_now_ms_) : 0LL)
           << "}";
        return js.str();
    }

private:
    struct Sample {
        double  price;
        int64_t ts_ms;
    };

    std::string         symbol_;

    // Buffer of 1-min closes (size capped at BB_WINDOW).
    std::deque<Sample>  buffer_;

    // RSI(14) state — Wilder smoothing.
    double              prev_close_      = -1.0;
    double              rsi_avg_gain_    = 0.0;
    double              rsi_avg_loss_    = 0.0;
    int                 rsi_samples_     = 0;     // grows up to RSI_PERIOD then stays

    // Position state.
    bool                pos_active_         = false;
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
    double              total_pnl_bp_       = 0.0;

    // Last-seen tick.
    double              last_price_         = 0.0;
    int64_t             last_now_ms_        = 0;
    int64_t             last_sample_ts_     = 0;

    // ──────────────────────────────────────────────────────────────────────
    // Sample ingestion: append a 1-min "close" to the buffer (capped at
    // BB_WINDOW), and update Wilder-smoothed RSI(14) on the close-to-close
    // delta.
    void _ingest_sample(double price, int64_t now_ms) {
        buffer_.push_back({price, now_ms});
        while ((int)buffer_.size() > BB_WINDOW) buffer_.pop_front();

        if (prev_close_ > 0.0) {
            const double delta = price - prev_close_;
            const double gain  = (delta > 0.0) ?  delta : 0.0;
            const double loss  = (delta < 0.0) ? -delta : 0.0;

            if (rsi_samples_ < RSI_PERIOD) {
                // Simple moving average for the first PERIOD samples — this
                // matches the standard Wilder warmup convention.
                rsi_avg_gain_ = (rsi_avg_gain_ * rsi_samples_ + gain) / (rsi_samples_ + 1);
                rsi_avg_loss_ = (rsi_avg_loss_ * rsi_samples_ + loss) / (rsi_samples_ + 1);
                ++rsi_samples_;
            } else {
                // Wilder smoothing: avg = (prev_avg * (PERIOD-1) + current) / PERIOD
                rsi_avg_gain_ = (rsi_avg_gain_ * (RSI_PERIOD - 1) + gain) / RSI_PERIOD;
                rsi_avg_loss_ = (rsi_avg_loss_ * (RSI_PERIOD - 1) + loss) / RSI_PERIOD;
            }
        }
        prev_close_ = price;
    }

    // RSI value derived from current Wilder-smoothed averages. Returns 50.0
    // (neutral) if not yet primed — that means it cannot accidentally satisfy
    // the entry gate (RSI <= 30) or the exit gate (RSI >= 60) during warmup.
    double _rsi_value() const {
        if (rsi_samples_ < RSI_PERIOD) return 50.0;
        if (rsi_avg_loss_ <= 0.0) {
            // No losses observed in the window → RSI = 100 (max).
            return (rsi_avg_gain_ <= 0.0) ? 50.0 : 100.0;
        }
        const double rs  = rsi_avg_gain_ / rsi_avg_loss_;
        return 100.0 - 100.0 / (1.0 + rs);
    }

    // Compute Bollinger stats over the buffer. Returns false if we do not
    // yet have BB_WINDOW samples spanning at least MIN_BUFFER_SPAN_MS.
    bool _compute_bb(double& mean, double& stdev, double& upper, double& lower) const {
        mean  = 0.0;
        stdev = 0.0;
        upper = 0.0;
        lower = 0.0;

        if ((int)buffer_.size() < BB_WINDOW) return false;
        const int64_t span = buffer_.back().ts_ms - buffer_.front().ts_ms;
        if (span < MIN_BUFFER_SPAN_MS) return false;

        double sum = 0.0;
        for (const auto& s : buffer_) sum += s.price;
        mean = sum / (double)buffer_.size();
        if (mean <= 0.0) return false;

        double sumsq = 0.0;
        for (const auto& s : buffer_) {
            const double d = s.price - mean;
            sumsq += d * d;
        }
        // Sample stddev (n-1). Standard for Bollinger.
        const double var = sumsq / (double)(buffer_.size() - 1);
        stdev = (var > 0.0) ? std::sqrt(var) : 0.0;

        upper = mean + BB_K * stdev;
        lower = mean - BB_K * stdev;
        return true;
    }

    bool _buffer_ready() const {
        if ((int)buffer_.size() < BB_WINDOW) return false;
        const int64_t span = buffer_.back().ts_ms - buffer_.front().ts_ms;
        if (span < MIN_BUFFER_SPAN_MS) return false;
        if (rsi_samples_ < RSI_PERIOD) return false;
        return true;
    }

    void _try_enter(double price, int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;
        if (!_buffer_ready())            return;

        double mean = 0.0, stdev = 0.0, upper = 0.0, lower = 0.0;
        if (!_compute_bb(mean, stdev, upper, lower)) return;
        if (mean <= 0.0)                              return;

        const double vol_frac = stdev / mean;
        if (vol_frac < MIN_VOL_FRAC) return;   // dead-flat
        if (vol_frac > MAX_VOL_FRAC) return;   // trending

        if (price > lower) return;             // need to be at-or-below the lower band

        const double rsi = _rsi_value();
        if (rsi > RSI_ENTRY_MAX) return;       // not oversold enough

        pos_active_     = true;
        entry_price_    = price;
        entry_ms_       = now_ms;
        pos_mfe_bp_     = 0.0;
        pos_mae_bp_     = 0.0;
        pos_size_R_     = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[RMR-SHADOW-ENTRY]" : "[RMR-ENTRY]";
        std::printf("%s %s LONG | px=%.4f <= lower=%.4f (mean=%.4f stdev=%.4f vol_frac=%.5f) | "
                    "rsi=%.1f (<=%.1f) | size=%.2fR | buf_n=%d\n",
            pfx,
            symbol_.c_str(),
            price, lower, mean, stdev, vol_frac,
            rsi, RSI_ENTRY_MAX,
            pos_size_R_, (int)buffer_.size());
        std::fflush(stdout);
    }

    void _manage(double price, int64_t now_ms) {
        const double move_bp = (price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

        // Recompute mean for the mid-band reclaim test (cheap; BB_WINDOW is small).
        double mean = 0.0, stdev = 0.0, upper = 0.0, lower = 0.0;
        const bool bb_ok = _compute_bb(mean, stdev, upper, lower);

        const double rsi = _rsi_value();

        const bool tp        = move_bp >= TP_BP;
        const bool sl        = move_bp <= -STOP_BP;
        const bool reclaimed = bb_ok && (price >= mean);
        const bool rsi_flip  = rsi >= RSI_EXIT_MIN;
        const bool timeout   = (now_ms - entry_ms_) > MAX_HOLD_MS;

        if (!(tp || sl || reclaimed || rsi_flip || timeout)) return;

        const double net_bp = (move_bp - ROUND_TRIP_COST_BP) * pos_size_R_;
        total_pnl_bp_ += net_bp;
        ++total_trades_;
        if (net_bp > 0) ++wins_;

        const char* reason = tp        ? "TP"
                           : sl        ? "SL"
                           : reclaimed ? "MID_RECLAIM"
                           : rsi_flip  ? "RSI_FLIP"
                           :             "TIMEOUT";

        const char* pfx = shadow_mode ? "[RMR-SHADOW-EXIT]" : "[RMR-EXIT]";
        std::printf("%s %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                    "rsi=%.1f mid=%.4f px=%.4f | mfe=%.1f mae=%.1f | hold_min=%.1f | "
                    "total=%.1fbp wins=%d/%d\n",
            pfx,
            symbol_.c_str(),
            net_bp, move_bp, ROUND_TRIP_COST_BP,
            reason,
            rsi, mean, price,
            pos_mfe_bp_, pos_mae_bp_,
            (double)(now_ms - entry_ms_) / 60000.0,
            total_pnl_bp_, wins_, total_trades_);
        std::fflush(stdout);

        pos_active_         = false;
        entry_price_        = 0.0;
        cooldown_until_ms_  = now_ms + COOLDOWN_MS;
    }
};

} // namespace chimera
