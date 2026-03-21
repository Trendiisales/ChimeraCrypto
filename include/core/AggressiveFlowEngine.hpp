#pragma once
// ============================================================================
// AggressiveFlowEngine.hpp
// Chimera -- Aggressive Trade Flow momentum engine
//
// SIGNAL: When buy flow significantly exceeds sell flow over recent ticks
//         AND vol is elevated, follow the aggressor direction.
//         Uses agg_buy_volume / agg_sell_volume EMA ratio from MarketTick.
//
// DATA USED: tick.agg_buy_volume, tick.agg_sell_volume, vol_ratio
// HOLD: 3000ms max, TP=18bp, SL=12bp
// SIZE: 0.5-1.2R
// SPOT ONLY: long side only (buy flow dominant)
// ============================================================================

#include <cmath>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace chimera {

class AggressiveFlowEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = 8.0;
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

    explicit AggressiveFlowEngine(const std::string& sym) : symbol_(sym) {}
    AggressiveFlowEngine() = default;

    // buy_ema / sell_ema: short-window EMAs of agg_buy_volume / agg_sell_volume
    // These are maintained externally in QuadEngine per-symbol state
    void evaluate(
        double   price,
        double   buy_ema,
        double   sell_ema,
        double   spread_bps,
        double   vol_ratio,
        int64_t  ts,
        double   available_R
    ) {
        if (cooldown_ticks_ > 0) { cooldown_ticks_--; return; }

        if (!pos_active_) {

            if (vol_ratio < 1.1)  return;
            if (spread_bps > 3.0) return;
            if (available_R < 0.5) return;

            double total = buy_ema + sell_ema;
            if (total < 1e-9) return;

            double flow_ratio = (buy_ema - sell_ema) / total;

            // Spot only: only enter LONG when buy flow dominates strongly
            if (flow_ratio < 0.30) return;  // buy must be >65% of total flow

            pos_active_  = true;
            entry_price_ = price;
            pos_size_R_  = std::min(1.2, available_R);
            pos_dir_     = 1;  // always long
            entry_ts_    = ts;
            pos_mfe_bp_  = 0.0;
            pos_mae_bp_  = 0.0;

            std::printf("[AFE-ENTRY] %s | flow_ratio=%.2f | vol=%.2f | spread=%.2fbp | size=%.1fR\n",
                symbol_.c_str(), flow_ratio, vol_ratio, spread_bps, pos_size_R_);
            std::fflush(stdout);
        }
        else {
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;

            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            bool tp      = move_bp >= 18.0;
            bool sl      = move_bp <= -12.0;
            bool timeout = (ts - entry_ts_) > 3000;

            if (tp || sl || timeout) {
                double net_bp = move_bp - ROUND_TRIP_COST_BP;
                total_pnl_bp_ += net_bp * pos_size_R_;
                total_trades_++;
                if (net_bp > 0) wins_++;
                const char* reason = tp ? "TP" : (sl ? "SL" : "TIMEOUT");
                std::printf("[AFE-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | reason=%s | total=%.1fbp\n",
                    symbol_.c_str(), net_bp, move_bp, ROUND_TRIP_COST_BP, reason, total_pnl_bp_);
                std::fflush(stdout);
                pos_active_     = false;
                cooldown_ticks_ = 50;
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
    int     pos_dir_     = 0;
    int64_t entry_ts_    = 0;
    double  pos_mfe_bp_  = 0.0;
    double  pos_mae_bp_  = 0.0;

    int    wins_         = 0;
    int    total_trades_ = 0;
    double total_pnl_bp_ = 0.0;
};

} // namespace chimera
