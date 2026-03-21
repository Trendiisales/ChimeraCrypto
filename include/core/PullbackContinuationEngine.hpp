#pragma once
// ============================================================================
// PullbackContinuationEngine.hpp
// Chimera -- Pullback Continuation momentum engine
//
// SIGNAL: When price has displaced significantly from anchor (trending) AND
//         a pullback occurs (price retraces toward anchor), enter in the
//         direction of the original trend.
//
//   Trend UP  (displacement_bp > 0): wait for price to pull back toward
//             anchor (displacement shrinks), then re-enter LONG.
//
// DATA USED: displacement_bp, vol_ratio, spread_bps, acceleration_bp
// HOLD: 4000ms max, TP=20bp, SL=12bp
// SIZE: 0.5-1.0R
// SPOT ONLY: long side only
// ============================================================================

#include <cmath>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace chimera {

class PullbackContinuationEngine {
public:
    struct Stats {
        bool   active;
        double size_R;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double win_rate;
        double total_pnl_bp;
        int    total_trades;
    };

    explicit PullbackContinuationEngine(const std::string& sym) : symbol_(sym) {}
    PullbackContinuationEngine() = default;

    void evaluate(
        double   price,
        double   displacement_bp,    // current displacement from rolling anchor
        double   acceleration_bp,    // price change acceleration
        double   spread_bps,
        double   vol_ratio,
        int64_t  ts,
        double   available_R
    ) {
        if (cooldown_ticks_ > 0) { cooldown_ticks_--; return; }

        if (!pos_active_) {

            // Need a clear uptrend (displacement > 20bp above anchor)
            // and a pullback signal (acceleration negative = price decelerating)
            if (displacement_bp < 20.0)  return;  // not trending enough up
            if (acceleration_bp > -2.0)  return;  // no pullback deceleration yet
            if (vol_ratio < 1.2)         return;  // needs elevated vol
            if (spread_bps > 3.0)        return;  // tight spread required
            if (available_R < 0.5)       return;

            pos_active_  = true;
            entry_price_ = price;
            pos_size_R_  = std::min(1.0, available_R);
            entry_ts_    = ts;
            pos_mfe_bp_  = 0.0;
            pos_mae_bp_  = 0.0;

            std::printf("[PCE-ENTRY] %s | disp=%.1fbp | accel=%.2fbp | vol=%.2f | size=%.1fR\n",
                symbol_.c_str(), displacement_bp, acceleration_bp, vol_ratio, pos_size_R_);
            std::fflush(stdout);
        }
        else {
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;

            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            bool tp      = move_bp >= 20.0;
            bool sl      = move_bp <= -12.0;
            bool timeout = (ts - entry_ts_) > 4000;

            if (tp || sl || timeout) {
                total_pnl_bp_ += move_bp * pos_size_R_;
                total_trades_++;
                if (move_bp > 0) wins_++;
                const char* reason = tp ? "TP" : (sl ? "SL" : "TIMEOUT");
                std::printf("[PCE-EXIT] %s | pnl=%.2fbp | reason=%s | total=%.1fbp\n",
                    symbol_.c_str(), move_bp, reason, total_pnl_bp_);
                std::fflush(stdout);
                pos_active_     = false;
                cooldown_ticks_ = 60;
            }
        }
    }

    Stats get_stats() const {
        return {
            pos_active_, pos_size_R_, entry_price_, pos_mfe_bp_, pos_mae_bp_,
            total_trades_ > 0 ? (double)wins_ / total_trades_ : 0.0,
            total_pnl_bp_, total_trades_
        };
    }

    bool   pos_active_     = false;
    double pos_size_R_     = 0.0;
    int    cooldown_ticks_ = 0;

private:
    std::string symbol_;
    double  entry_price_ = 0.0;
    int64_t entry_ts_    = 0;
    double  pos_mfe_bp_  = 0.0;
    double  pos_mae_bp_  = 0.0;

    int    wins_         = 0;
    int    total_trades_ = 0;
    double total_pnl_bp_ = 0.0;
};

} // namespace chimera
