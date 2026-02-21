#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * MicroEdgeEngine - Liquidity Vacuum & Microstructure Imbalance Detection
 * 
 * Consumes order book state and computes microstructure edge.
 * Outputs impulse_bias and maker_bias multipliers.
 * 
 * Detects:
 * - Liquidity vacuums (depth collapse + spread expansion)
 * - Book imbalance prediction
 * - Funding distortion bias
 */
class MicroEdgeEngine {
public:

    struct BookState {
        double bid_depth;
        double ask_depth;
        double spread_bps;
        double mid_price;
        double short_range;
        double long_range;
        double funding_rate;
        double latency_ms;
    };

private:

    double vacuum_score_ = 0.0;
    double imbalance_score_ = 0.0;
    double funding_score_ = 0.0;
    double vol_ratio_prev_ = 1.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    void update(const BookState& s) {

        double total_depth = std::max(s.bid_depth + s.ask_depth, 1e-6);
        double imbalance = (s.bid_depth - s.ask_depth) / total_depth;

        imbalance_score_ = ema(imbalance_score_, imbalance, 0.1);

        double vol_ratio = s.short_range / std::max(s.long_range, 1e-6);
        double vol_accel = vol_ratio - vol_ratio_prev_;
        vol_ratio_prev_ = vol_ratio;

        double depth_collapse = clamp(1.0 - total_depth, 0.0, 1.0);

        double spread_expand = clamp(s.spread_bps / 5.0, 0.0, 2.0);

        vacuum_score_ = ema(
            vacuum_score_,
            depth_collapse * spread_expand * vol_accel,
            0.1
        );

        if (std::abs(s.funding_rate) > 0.0005)
            funding_score_ = 0.7;
        else
            funding_score_ = 1.0;
    }

    double impulse_bias() const {

        double imbalance_component =
            clamp(std::abs(imbalance_score_), 0.0, 1.0);

        double vacuum_component =
            clamp(vacuum_score_, 0.0, 2.0);

        double raw =
            0.5
          + imbalance_component * 0.4
          + vacuum_component * 0.6;

        return clamp(raw, 0.1, 1.5) * funding_score_;
    }

    double maker_bias() const {

        double compression =
            clamp(1.0 - std::abs(imbalance_score_), 0.0, 1.0);

        double stable_book =
            clamp(1.0 - std::abs(vacuum_score_), 0.0, 1.0);

        double raw =
            0.5
          + compression * 0.6
          + stable_book * 0.4;

        return clamp(raw, 0.1, 1.5);
    }
};

} // namespace chimera
