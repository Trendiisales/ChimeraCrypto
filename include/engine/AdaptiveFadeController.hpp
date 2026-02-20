#pragma once
#include <cmath>
#include <chrono>
#include <algorithm>
#include "core/Symbol.hpp"

namespace chimera {

struct MarketContext {
    double spread_bps;
    double volatility_bps;
    double imbalance;
    double signal_raw;
    double estimated_slippage_bps;
    bool regime_trending;
    bool regime_choppy;
};

struct PositionState {
    bool has_position = false;
    bool is_long = false;
    double entry_price = 0.0;
    double size = 0.0;
    std::chrono::steady_clock::time_point entry_time;
};

struct TradeDecision {
    bool enter_long = false;
    bool enter_short = false;
    bool exit_position = false;
    double size_multiplier = 1.0;
};

class AdaptiveFadeController {
public:
    AdaptiveFadeController(double entry_threshold,
                           double exit_threshold,
                           int min_hold_ms,
                           double fee_bps)
        : entry_threshold_(entry_threshold),
          exit_threshold_(exit_threshold),
          min_hold_ms_(min_hold_ms),
          fee_bps_(fee_bps)
    {}

    TradeDecision evaluate(const MarketContext& ctx, PositionState& pos)
    {
        TradeDecision decision;
        
        double conviction = compute_conviction(ctx);
        double cost_floor = compute_cost_floor(ctx);
        
        auto now = std::chrono::steady_clock::now();
        
        // ENTRY LOGIC
        if (!pos.has_position) {
            if (!regime_allows(ctx)) return decision;
            if (conviction < entry_threshold_) return decision;
            if (conviction < cost_floor) return decision;
            
            decision.size_multiplier = size_scaler(conviction);
            
            if (ctx.signal_raw > 0.0) {
                decision.enter_long = true;
            } else {
                decision.enter_short = true;
            }
            
            return decision;
        }
        
        // EXIT LOGIC
        auto held_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pos.entry_time).count();
        
        if (held_ms < min_hold_ms_) {
            return decision;
        }
        
        double directional_signal = pos.is_long ? ctx.signal_raw : -ctx.signal_raw;
        
        // Structured exit
        if (directional_signal < -exit_threshold_) {
            decision.exit_position = true;
            return decision;
        }
        
        // Cost bleed protection
        if (conviction < cost_floor * 0.8) {
            decision.exit_position = true;
            return decision;
        }
        
        return decision;
    }

private:
    double entry_threshold_;
    double exit_threshold_;
    int min_hold_ms_;
    double fee_bps_;
    
    double compute_conviction(const MarketContext& ctx)
    {
        double vol_weight = std::clamp(ctx.volatility_bps / 10.0, 0.5, 2.0);
        double imbalance_weight = std::abs(ctx.imbalance);
        double normalized_signal = std::abs(ctx.signal_raw);
        
        return normalized_signal * (1.0 + imbalance_weight) * vol_weight;
    }
    
    double compute_cost_floor(const MarketContext& ctx)
    {
        return (ctx.spread_bps + fee_bps_ + ctx.estimated_slippage_bps) * 1.5;
    }
    
    bool regime_allows(const MarketContext& ctx)
    {
        if (ctx.regime_trending) return false;
        if (ctx.spread_bps > 8.0) return false;
        if (ctx.volatility_bps > 60.0) return false;
        return true;
    }
    
    double size_scaler(double conviction)
    {
        double scaled = conviction / (entry_threshold_ * 1.5);
        return std::clamp(scaled, 0.5, 2.0);
    }
};

}
