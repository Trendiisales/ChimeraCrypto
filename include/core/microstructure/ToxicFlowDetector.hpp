#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * ToxicFlowDetector - Informed Order Flow Detection
 * 
 * Toxic flow = when aggressive participants are informed and liquidity 
 * is about to shift violently.
 * 
 * If you provide liquidity during toxic flow → you get run over
 * If you join impulse early during toxic flow → you capture displacement
 * 
 * This layer decides:
 * - When maker should shut down
 * - When impulse should scale
 * - When to widen stops
 * - When to avoid trading entirely
 */
class ToxicFlowDetector {
public:

    struct TickInput {
        double trade_volume;
        double aggressive_buy_volume;
        double aggressive_sell_volume;
        double bid_depth;
        double ask_depth;
        double spread_bps;
        double short_range;
        double long_range;
    };

private:

    double volume_ema_ = 0.0;
    double imbalance_ema_ = 0.0;
    double depth_ema_ = 0.0;
    double volatility_ema_ = 0.0;
    double toxicity_score_ = 0.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    void update(const TickInput& t) {

        double total_depth =
            std::max(t.bid_depth + t.ask_depth, 1e-6);

        double imbalance =
            (t.aggressive_buy_volume - t.aggressive_sell_volume)
            / std::max(t.trade_volume, 1e-6);

        double vol_ratio =
            t.short_range / std::max(t.long_range, 1e-6);

        volume_ema_     = ema(volume_ema_, t.trade_volume, 0.05);
        imbalance_ema_  = ema(imbalance_ema_, imbalance, 0.1);
        // depth_ema_ tracks the baseline depth level (absolute units)
        // depth_collapse = how much depth has SHRUNK vs its own baseline
        // If depth_ema_ is uninitialized (0), seed it first tick
        if (depth_ema_ < 1e-9) depth_ema_ = total_depth;
        else depth_ema_ = ema(depth_ema_, total_depth, 0.05);
        volatility_ema_ = ema(volatility_ema_, vol_ratio, 0.05);

        double aggressive_pressure =
            clamp(std::abs(imbalance_ema_), 0.0, 1.0);

        // depth_collapse: 0 = depth is normal or growing, 1 = depth totally gone
        // Use ratio: how much below baseline is current depth?
        double depth_ratio_vs_baseline = (depth_ema_ > 1e-9)
            ? total_depth / depth_ema_
            : 1.0;
        double depth_collapse =
            clamp(1.0 - depth_ratio_vs_baseline, 0.0, 1.0);

        double vol_expansion =
            clamp(volatility_ema_ - 1.1, 0.0, 3.0);

        toxicity_score_ =
            clamp(
                aggressive_pressure * 0.5
              + depth_collapse * 0.3
              + vol_expansion * 0.4,
              0.0,
              3.0
            );
    }

    double toxicity() const {
        return toxicity_score_;
    }

    bool toxic_regime() const {
        return toxicity_score_ > 0.8;
    }

    double maker_suppression() const {
        return clamp(1.0 - toxicity_score_ * 0.7, 0.0, 1.0);
    }

    double impulse_boost() const {
        return clamp(1.0 + toxicity_score_ * 0.6, 1.0, 2.0);
    }

    double stop_widening_factor() const {
        return clamp(1.0 + toxicity_score_ * 0.5, 1.0, 2.0);
    }
};

} // namespace chimera
