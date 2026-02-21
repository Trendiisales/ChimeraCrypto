#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * ExecutionOptimizer - Real-Time Execution Decision Engine
 * 
 * Sits BELOW signal and ABOVE order submission.
 * Decides in real-time:
 * - Post-only vs market order
 * - Limit offset distance
 * - TP compression/expansion
 * - Stop distance
 * - Aggression multiplier
 * - Entry side bias
 * 
 * All based on:
 * - Liquidity vacuum detection
 * - Book imbalance
 * - Vol acceleration
 * - Latency regime
 * - Spread conditions
 * - Funding distortion
 */
class ExecutionOptimizer {
public:

    struct Inputs {
        double spread_bps;
        double latency_ms;
        double short_range;
        double long_range;
        double imbalance;
        double queue_density;
        double funding_rate;
        bool   net_clean;
    };

    struct Decision {
        bool   use_market;
        double limit_offset_bps;
        double tp_bps;
        double stop_bps;
        double aggression_mult;
    };

private:

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

public:

    Decision optimize(const Inputs& in) {

        Decision d;

        double vol_ratio =
            in.short_range / std::max(in.long_range, 1e-6);

        double vol_accel =
            clamp(vol_ratio - 1.1, 0.0, 3.0);

        double imbalance_strength =
            clamp(std::abs(in.imbalance), 0.0, 1.0);

        double spread_factor =
            clamp(in.spread_bps / 3.0, 0.0, 2.0);

        double latency_penalty =
            in.net_clean ? 1.0 : 0.7;

        double funding_penalty =
            std::abs(in.funding_rate) > 0.0005 ? 0.8 : 1.0;

        double vacuum_score =
            vol_accel
          * imbalance_strength
          * spread_factor;

        bool strong_impulse =
            vacuum_score > 0.6 && in.latency_ms < 8.0;

        d.use_market = strong_impulse;

        double base_offset =
            0.5 * spread_factor;

        double queue_bias =
            clamp(1.0 - in.queue_density, 0.5, 1.5);

        if (strong_impulse) {
            d.limit_offset_bps = 0.0;
        } else {
            d.limit_offset_bps =
                clamp(base_offset * queue_bias, 0.1, 2.0);
        }

        double tp_base =
            6.0 + vol_accel * 6.0;

        double stop_base =
            4.0 + vol_accel * 4.0;

        if (in.spread_bps < 1.0)
            tp_base *= 0.8;

        d.tp_bps =
            clamp(tp_base * latency_penalty, 3.0, 25.0);

        d.stop_bps =
            clamp(stop_base * funding_penalty, 2.0, 20.0);

        d.aggression_mult =
            clamp(
                1.0
              + vol_accel * 0.4
              + imbalance_strength * 0.3,
              0.7,
              2.0
            );

        return d;
    }
};

} // namespace chimera
