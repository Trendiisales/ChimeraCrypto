#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * DynamicCostSignalGate - Adaptive Required Move Calculator
 * 
 * Prevents trading when expected move < structural cost.
 * 
 * Required move scales dynamically with:
 * - Structural cost (spread + slip + fees)
 * - Volatility regime (easier to capture during expansion)
 * - Toxic flow (harder during informed flow)
 * - Liquidity depth (harder when thin)
 * 
 * This single gate fixes expectancy math by rejecting low-probability trades.
 */
class DynamicCostSignalGate {
public:

    struct MarketState {
        double spread_bps;
        double latency_ms;
        double taker_fee_bps;
        double short_range;
        double long_range;
        double book_depth_norm;   // 0 thin → 1 deep
        double toxicity_score;    // 0 calm → 3 extreme
    };

private:

    double base_multiplier_ = 1.4;
    double min_floor_ = 3.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double estimate_slippage(double latency_ms, double spread_bps) {
        double base = spread_bps * 0.5;
        double latency_penalty =
            clamp(latency_ms / 10.0, 0.0, 3.0);
        return base + latency_penalty;
    }

public:

    void set_base_multiplier(double v) {
        base_multiplier_ = v;
    }

    void set_min_floor(double v) {
        min_floor_ = v;
    }

    bool approve(double expected_move_bps,
                 const MarketState& m,
                 double& required_out,
                 double& cost_out)
    {
        double vol_ratio =
            m.short_range / std::max(m.long_range, 1e-6);

        double slip =
            estimate_slippage(m.latency_ms, m.spread_bps);

        double structural_cost =
            m.spread_bps
          + slip
          + m.taker_fee_bps;

        cost_out = structural_cost;

        // Volatility scaling (easier during expansion)
        double vol_scale =
            clamp(vol_ratio, 0.8, 2.5);

        // Toxicity scaling (harder during toxic flow)
        double tox_scale =
            clamp(1.0 + m.toxicity_score * 0.3, 1.0, 2.0);

        // Thin book scaling (harder when liquidity poor)
        double liquidity_scale =
            clamp(1.0 + (1.0 - m.book_depth_norm) * 0.5,
                  1.0,
                  1.8);

        double dynamic_multiplier =
            base_multiplier_
          * tox_scale
          * liquidity_scale
          / vol_scale;

        double required_move =
            std::max(
                structural_cost * dynamic_multiplier,
                min_floor_
            );

        required_out = required_move;

        if (expected_move_bps < required_move)
            return false;

        return true;
    }
};

} // namespace chimera
