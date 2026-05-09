#pragma once
#include "config/TradingConfig.hpp"
// ============================================================================
// VolCompressionBreakoutEngine.hpp
// Chimera — Intermediate-timeframe BTC volatility-compression breakout
//
// EDGE:
//   Realised volatility regimes mean-revert. After a sustained low-vol
//   "coil" period, BTC tends to make a directional break that runs for
//   days. Trading the BREAKOUT (not the compression itself) captures the
//   expansion phase while the vol-compression filter rejects mid-regime
//   breakouts that fail.
//
//   This engine measures vol via the standard deviation of 1-minute log
//   returns over rolling windows entirely from in-memory tick data — no
//   REST seeding, no persistence, just 24h of price samples accumulated
//   after startup.
//
// SIGNAL (intermediate timeframe — 8 to 72 hours):
//   Maintain a 24h ring of mid-price samples at SAMPLE_INTERVAL_MS (60s)
//   resolution. From these samples derive 1-minute log returns.
//
//   Vol metrics:
//     short_vol = stddev of last RECENT_WINDOW_MS (6h) of returns
//     long_vol  = stddev of full 24h of returns
//
//   ENTRY (LONG BTC spot):
//     1. short_vol / long_vol <= COMPRESSION_RATIO (0.50)
//     2. Current price > max of last LOOKBACK_24H_MS prices (Donchian-24h)
//     3. Buffer ready: >= MIN_SAMPLES samples spanning >= MIN_BUFFER_SPAN_MS
//     4. No active position, not in cooldown, available_R >= MIN_AVAIL_R
//
//   EXIT (whichever fires first):
//     - TP: +TP_BP gross
//     - Trail: arms at +TRAIL_ARM_BP MFE, ratchets TRAIL_DIST_BP below peak
//     - SL: -STOP_BP gross
//     - Timeout: MAX_HOLD_MS (72 hours)
//
//   Long-only — the signal is symmetric in principle but spot-only Chimera
//   can't take a short on a downside breakout. Once a perp short executor
//   lands, the negative side of the signal is reusable trivially.
//
// COST MODEL:
//   Round-trip = TradingConfig::MAKER_ROUND_TRIP_BP (15 bp).
//   Net P&L = (gross_bp - 15) * size_R.
//
// SHADOW WIRING:
//   shadow_mode = true by default. Paper-only via printf log.
//
// WARM-UP:
//   Engine cannot fire until ~23 hours after startup (needs to span the
//   24h buffer). After that, signal evaluation runs continuously.
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

class VolCompressionBreakoutEngine {
public:
    // ── Cost model ─────────────────────────────────────────────────────────
    static constexpr double  ROUND_TRIP_COST_BP  = TradingConfig::MAKER_ROUND_TRIP_BP; // 15 bp

    // ── Sampling / buffer geometry ─────────────────────────────────────────
    static constexpr int64_t SAMPLE_INTERVAL_MS  = 60000;                 // 1 min
    static constexpr int64_t LOOKBACK_24H_MS     = 24LL * 60 * 60 * 1000; // 24h
    static constexpr int64_t RECENT_WINDOW_MS    =  6LL * 60 * 60 * 1000; //  6h
    static constexpr int64_t BUFFER_RETAIN_MS    = 26LL * 60 * 60 * 1000; // 26h
    static constexpr int64_t MIN_BUFFER_SPAN_MS  = 23LL * 60 * 60 * 1000; // need >=23h
    static constexpr int     MIN_SAMPLES         = 200;                   // at 60s sampling

    // ── Entry thresholds ───────────────────────────────────────────────────
    static constexpr double  COMPRESSION_RATIO   = 0.50;   // 6h_vol/24h_vol must be <= this
    static constexpr double  MIN_AVAIL_R         = 0.5;

    // ── Position management ────────────────────────────────────────────────
    static constexpr double  TP_BP               = 400.0;  // +4% gross
    static constexpr double  STOP_BP             = 200.0;  // -2% gross
    static constexpr double  TRAIL_ARM_BP        = 200.0;  // arm trail at +2% MFE
    static constexpr double  TRAIL_DIST_BP       = 100.0;  // trail 1% below peak
    static constexpr int64_t MAX_HOLD_MS         = 72LL * 60 * 60 * 1000;  // 72h
    static constexpr int64_t COOLDOWN_MS         = 12LL * 60 * 60 * 1000;  // 12h

    // ── Sizing ─────────────────────────────────────────────────────────────
    static constexpr double  SIZE_FRAC_OF_R      = 0.5;

    VolCompressionBreakoutEngine() = default;

    bool shadow_mode = true;

    // ── Per-tick entry point ───────────────────────────────────────────────
    // Called from main.cpp's spot tick callback. Reacts to BTC ticks only;
    // any other symbol_id is ignored.
    void on_tick(int symbol_id,
                 const MarketTick& tick,
                 int64_t now_ms,
                 double available_R) {
        if (halted_) return;
        if (symbol_id != SYM_BTC) return;

        const double price = tick.mid_price > 0.0 ? tick.mid_price
                                                  : tick.last_price;
        if (price <= 0.0) return;

        last_btc_price_ = price;
        last_now_ms_    = now_ms;

        // Throttle samples to 1/min. The 24h Donchian uses the BUFFERED
        // samples (not every tick), which keeps the high-water mark stable
        // and prevents micro-spikes from triggering breakouts.
        if (now_ms - last_sample_ts_ >= SAMPLE_INTERVAL_MS) {
            last_sample_ts_ = now_ms;
            buffer_.push_back({price, now_ms});
            while (!buffer_.empty() && (now_ms - buffer_.front().ts_ms) > BUFFER_RETAIN_MS) {
                buffer_.pop_front();
            }
        }

        if (pos_active_) _manage(price, now_ms);
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

            std::printf("[VOLCMP-KILL] BTC | net=%.2fbp (gross=%.2f cost=%.1f) | "
                        "exit_px=%.4f entry=%.4f | mfe=%.1f mae=%.1f | total=%.1fbp\n",
                net_bp, move_bp, ROUND_TRIP_COST_BP,
                exit_px, entry_price_, pos_mfe_bp_, pos_mae_bp_, total_pnl_bp_);
            std::fflush(stdout);

            pos_active_         = false;
            entry_price_        = 0.0;
            trail_floor_bp_     = -9999.0;
            cooldown_until_ms_  = (now_ms > 0 ? now_ms : last_now_ms_) + COOLDOWN_MS;
        }
        halted_ = true;
        std::printf("[VOLCMP-KILL] engine halted; clear_halt() to resume\n");
        std::fflush(stdout);
    }

    void clear_halt() { halted_ = false; }
    bool is_halted() const { return halted_; }

    int    total_trades() const { return total_trades_; }
    double total_pnl_bp() const { return total_pnl_bp_; }
    bool   position_active() const { return pos_active_; }

    // ── /api/state2 JSON ──────────────────────────────────────────────────
    std::string state_json(double btc_price = 0.0) const {
        const double move_bp = (pos_active_ && entry_price_ > 0.0 && btc_price > 0.0)
            ? (btc_price - entry_price_) / entry_price_ * 10000.0
            : 0.0;

        double short_vol = 0.0, long_vol = 0.0;
        const bool vols_ok = _compute_vols(short_vol, long_vol);
        const double ratio = (vols_ok && long_vol > 0.0) ? short_vol / long_vol : 0.0;

        double don_24h_high = 0.0;
        for (const auto& s : buffer_) if (s.price > don_24h_high) don_24h_high = s.price;

        std::ostringstream js;
        js << std::fixed << std::setprecision(6);
        js << "{"
           << "\"engine\":\"vol_compression_breakout\","
           << "\"trade_symbol\":\"btcusdt\","
           << "\"shadow_mode\":"        << (shadow_mode ? "true" : "false") << ","
           << "\"halted\":"             << (halted_     ? "true" : "false") << ","
           << "\"active\":"             << (pos_active_ ? "true" : "false") << ","
           << "\"entry_price\":"        << entry_price_ << ","
           << "\"btc_price\":"          << btc_price    << ","
           << "\"move_bp\":"            << move_bp      << ","
           << "\"mfe_bp\":"             << pos_mfe_bp_  << ","
           << "\"mae_bp\":"             << pos_mae_bp_  << ","
           << "\"trail_floor_bp\":"     << trail_floor_bp_ << ","
           << "\"trail_armed\":"        << (pos_mfe_bp_ >= TRAIL_ARM_BP ? "true" : "false") << ","
           << "\"win_rate\":"
                << (total_trades_ > 0 ? (double)wins_ / (double)total_trades_ : 0.0) << ","
           << "\"total_pnl_bp\":"       << total_pnl_bp_ << ","
           << "\"total_trades\":"       << total_trades_ << ","
           << "\"size_R\":"             << pos_size_R_   << ","
           << "\"short_vol_6h\":"       << short_vol << ","
           << "\"long_vol_24h\":"       << long_vol  << ","
           << "\"vol_ratio\":"          << ratio     << ","
           << "\"donchian_24h_high\":"  << don_24h_high << ","
           << "\"buffer_samples\":"     << (int)buffer_.size() << ","
           << "\"buffer_span_ms\":"
                << (buffer_.empty() ? 0LL
                                    : (int64_t)(buffer_.back().ts_ms - buffer_.front().ts_ms)) << ","
           << "\"compression_ratio_threshold\":" << COMPRESSION_RATIO
           << "}";
        return js.str();
    }

private:
    struct Sample {
        double  price;
        int64_t ts_ms;
    };

    std::deque<Sample> buffer_;

    bool    pos_active_         = false;
    double  pos_size_R_         = 0.0;
    double  entry_price_        = 0.0;
    int64_t entry_ms_           = 0;
    double  pos_mfe_bp_         = 0.0;
    double  pos_mae_bp_         = 0.0;
    double  trail_floor_bp_     = -9999.0;
    int64_t cooldown_until_ms_  = 0;

    bool    halted_             = false;
    int     wins_               = 0;
    int     total_trades_       = 0;
    double  total_pnl_bp_       = 0.0;

    double  last_btc_price_     = 0.0;
    int64_t last_now_ms_        = 0;
    int64_t last_sample_ts_     = 0;

    // ──────────────────────────────────────────────────────────────────────
    // Compute stddev of 1-min log returns over the last 6h (short) and the
    // full 24h buffer (long). Returns false if buffer doesn't yet span the
    // required windows or has insufficient samples.
    bool _compute_vols(double& short_vol, double& long_vol) const {
        short_vol = 0.0;
        long_vol  = 0.0;

        if ((int)buffer_.size() < MIN_SAMPLES) return false;
        const int64_t span = buffer_.back().ts_ms - buffer_.front().ts_ms;
        if (span < MIN_BUFFER_SPAN_MS) return false;

        // Build 1-minute log-return series from consecutive samples.
        // (Samples are throttled to 1/min so consecutive deltas already
        // approximate 1-min log returns.)
        // First pass: compute 24h (long) variance.
        double sum_l = 0.0;
        double sumsq_l = 0.0;
        int    n_l = 0;
        double prev = -1.0;
        for (const auto& s : buffer_) {
            if (prev > 0.0 && s.price > 0.0) {
                const double r = std::log(s.price / prev);
                sum_l   += r;
                sumsq_l += r * r;
                ++n_l;
            }
            prev = s.price;
        }
        if (n_l < 30) return false;
        const double mean_l = sum_l / (double)n_l;
        const double var_l  = sumsq_l / (double)n_l - mean_l * mean_l;
        long_vol = (var_l > 0.0) ? std::sqrt(var_l) : 0.0;

        // Second pass: 6h (short) variance — last RECENT_WINDOW_MS of returns.
        const int64_t now    = buffer_.back().ts_ms;
        const int64_t cutoff = now - RECENT_WINDOW_MS;
        double sum_s = 0.0;
        double sumsq_s = 0.0;
        int    n_s = 0;
        prev = -1.0;
        for (const auto& s : buffer_) {
            if (s.ts_ms >= cutoff && prev > 0.0 && s.price > 0.0) {
                const double r = std::log(s.price / prev);
                sum_s   += r;
                sumsq_s += r * r;
                ++n_s;
            }
            prev = s.price;
        }
        if (n_s < 30) return false;
        const double mean_s = sum_s / (double)n_s;
        const double var_s  = sumsq_s / (double)n_s - mean_s * mean_s;
        short_vol = (var_s > 0.0) ? std::sqrt(var_s) : 0.0;

        return true;
    }

    double _donchian_24h_high() const {
        double mx = 0.0;
        for (const auto& s : buffer_) if (s.price > mx) mx = s.price;
        return mx;
    }

    void _try_enter(double btc_price, int64_t now_ms, double available_R) {
        if (now_ms < cooldown_until_ms_) return;
        if (available_R < MIN_AVAIL_R)   return;

        double short_vol = 0.0, long_vol = 0.0;
        if (!_compute_vols(short_vol, long_vol)) return;
        if (long_vol <= 0.0) return;

        const double ratio = short_vol / long_vol;
        if (ratio > COMPRESSION_RATIO) return;     // not compressed

        // Donchian-24h breakout: current price strictly greater than the
        // highest sampled price in the buffer. Note this excludes the
        // current price itself because we sample on the same tick boundary.
        const double don_high = _donchian_24h_high();
        if (don_high <= 0.0) return;
        if (btc_price <= don_high) return;

        pos_active_     = true;
        entry_price_    = btc_price;
        entry_ms_       = now_ms;
        pos_mfe_bp_     = 0.0;
        pos_mae_bp_     = 0.0;
        trail_floor_bp_ = -9999.0;
        pos_size_R_     = available_R * SIZE_FRAC_OF_R;

        const char* pfx = shadow_mode ? "[VOLCMP-SHADOW-ENTRY]" : "[VOLCMP-ENTRY]";
        std::printf("%s BTC LONG | vol_6h=%.6f vol_24h=%.6f ratio=%.3f (<=%.2f) | "
                    "px=%.4f > don24h_high=%.4f | size=%.2fR | buf_n=%d span_ms=%lld\n",
            pfx,
            short_vol, long_vol, ratio, COMPRESSION_RATIO,
            btc_price, don_high, pos_size_R_,
            (int)buffer_.size(),
            (long long)(buffer_.back().ts_ms - buffer_.front().ts_ms));
        std::fflush(stdout);
    }

    void _manage(double btc_price, int64_t now_ms) {
        const double move_bp = (btc_price - entry_price_) / entry_price_ * 10000.0;
        if (move_bp > pos_mfe_bp_) pos_mfe_bp_ = move_bp;
        if (move_bp < pos_mae_bp_) pos_mae_bp_ = move_bp;

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

        const char* pfx = shadow_mode ? "[VOLCMP-SHADOW-EXIT]" : "[VOLCMP-EXIT]";
        std::printf("%s BTC | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | "
                    "mfe=%.1f mae=%.1f | hold_h=%.1f | total=%.1fbp wins=%d/%d\n",
            pfx,
            net_bp, move_bp, ROUND_TRIP_COST_BP,
            reason,
            pos_mfe_bp_, pos_mae_bp_,
            (double)(now_ms - entry_ms_) / 3600000.0,
            total_pnl_bp_, wins_, total_trades_);
        std::fflush(stdout);

        pos_active_         = false;
        entry_price_        = 0.0;
        trail_floor_bp_     = -9999.0;
        cooldown_until_ms_  = now_ms + COOLDOWN_MS;
    }
};

} // namespace chimera
