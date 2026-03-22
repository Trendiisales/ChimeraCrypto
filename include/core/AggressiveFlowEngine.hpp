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
// HOLD: 3000ms max, TP=30bp gross(+22bp net), SL=8bp gross(-16bp net)
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
    static constexpr double ROUND_TRIP_COST_BP = 15.0; // 7.5bp/side with BNB discount (0.075% per side)
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
        double   perp_flow_ratio,
        int      regime,         // 0=DEAD,1=GRIND,2=BUILDUP,3=BREAKOUT
        int64_t  ts,
        double   available_R
    ) {
        if (ts < cooldown_until_ms_) return;
        // AFE is momentum: only valid in BUILDUP or BREAKOUT (vol expanding)
        if (regime < 2) return;  // require BUILDUP or BREAKOUT

        if (!pos_active_) {

            if (vol_ratio < 1.2)  return;  // raised 1.1->1.2: need genuine vol surge
            if (spread_bps > 3.0) return;
            if (available_R < 0.5) return;

            double total = buy_ema + sell_ema;
            if (total < 1e-9) return;

            double flow_ratio = (buy_ema - sell_ema) / total;

            // Spot only: only enter LONG when buy flow dominates strongly
            if (flow_ratio < 0.40) return;  // raised 0.30->0.40: need strong conviction

            // Perp confirmation: perp flow should also be buy-dominated (or neutral)
            // If perp is selling aggressively while spot buys, spot pump likely short-lived
            if (perp_flow_ratio < -0.20) return;

            pos_active_  = true;
            entry_price_ = price;
            pos_size_R_  = std::min(1.2, available_R);
            pos_dir_     = 1;  // always long
            entry_ts_    = ts;
            pos_mfe_bp_  = 0.0;
            pos_mae_bp_  = 0.0;

            std::printf("[AFE-ENTRY] %s | spot_flow=%.2f | perp_flow=%.2f | vol=%.2f | spread=%.2fbp | size=%.1fR\n",
                symbol_.c_str(), flow_ratio, perp_flow_ratio, vol_ratio, spread_bps, pos_size_R_);
            std::fflush(stdout);
        }
        else {
            double move_bp = (price - entry_price_) / entry_price_ * 10000.0;

            pos_mfe_bp_ = std::max(pos_mfe_bp_, move_bp);
            pos_mae_bp_ = std::min(pos_mae_bp_, move_bp);

            bool tp      = move_bp >= 30.0;  // raised 18->30bp: net +22bp after 8bp cost
            bool sl      = move_bp <= -8.0;   // tightened 12->8bp: net -16bp after 8bp cost
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
                cooldown_until_ms_ = ts + 60000;  // 60s cooldown
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
    int64_t cooldown_until_ms_ = 0;

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
