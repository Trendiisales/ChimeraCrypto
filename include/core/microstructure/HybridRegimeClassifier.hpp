#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * HybridRegimeClassifier - Predictive Micro Regime Detection
 * 
 * Switches automatically between:
 * - BURST (join expansion momentum)
 * - REVERT (fade exhaustion)
 * - NEUTRAL (low conviction → reduce size)
 * 
 * This sits ABOVE ToxicFlow and BELOW Execution.
 * It does not trade directly - it classifies micro regime for capital routing.
 */
class HybridRegimeClassifier {
public:

    enum Regime {
        REGIME_NEUTRAL = 0,
        REGIME_BURST   = 1,
        REGIME_REVERT  = 2
    };

    struct Input {
        double short_range;
        double long_range;
        double trade_volume;
        double aggressive_buy_volume;
        double aggressive_sell_volume;
        double bid_depth;
        double ask_depth;
        double spread_bps;
        double latency_ms;
    };

private:

    double vol_ema_ = 1.0;
    double imbalance_ema_ = 0.0;
    double volume_ema_ = 0.0;
    double depth_ema_ = 1.0;
    double regime_score_ = 0.0;
    Regime regime_ = REGIME_NEUTRAL;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    void update(const Input& in) {

        double vol_ratio =
            in.short_range / std::max(in.long_range, 1e-6);

        double total_depth =
            std::max(in.bid_depth + in.ask_depth, 1e-6);

        double imbalance =
            (in.aggressive_buy_volume - in.aggressive_sell_volume)
            / std::max(in.trade_volume, 1e-6);

        vol_ema_      = ema(vol_ema_, vol_ratio, 0.05);
        imbalance_ema_ = ema(imbalance_ema_, imbalance, 0.1);
        volume_ema_   = ema(volume_ema_, in.trade_volume, 0.05);
        // Seed depth_ema_ on first tick to avoid depth_collapse=1.0 at startup
        if (depth_ema_ < 1e-9) depth_ema_ = total_depth;
        else depth_ema_ = ema(depth_ema_, total_depth, 0.05);

        double vol_expansion =
            clamp(vol_ema_ - 1.15, 0.0, 3.0);

        double aggressive_pressure =
            clamp(std::abs(imbalance_ema_), 0.0, 1.0);

        // depth_collapse: ratio vs own baseline, not absolute value
        double depth_ratio_vs_baseline = (depth_ema_ > 1e-9)
            ? total_depth / depth_ema_ : 1.0;
        double depth_collapse =
            clamp(1.0 - depth_ratio_vs_baseline, 0.0, 1.0);

        double spread_expansion =
            clamp(in.spread_bps / 5.0, 0.0, 2.0);

        double burst_score =
            vol_expansion * 0.5
          + aggressive_pressure * 0.5
          + depth_collapse * 0.4
          + spread_expansion * 0.2;

        double exhaustion_score =
            clamp(aggressive_pressure * 0.6
                + (1.0 - depth_collapse) * 0.4
                - vol_expansion * 0.5,
                0.0,
                3.0);

        regime_score_ =
            clamp(burst_score - exhaustion_score, -3.0, 3.0);

        if (regime_score_ > 0.6)
            regime_ = REGIME_BURST;
        else if (regime_score_ < -0.6)
            regime_ = REGIME_REVERT;
        else
            regime_ = REGIME_NEUTRAL;
    }

    Regime regime() const {
        return regime_;
    }

    double regime_score() const {
        return regime_score_;
    }

    double impulse_multiplier() const {
        if (regime_ == REGIME_BURST)
            return 1.6;
        if (regime_ == REGIME_REVERT)
            return 0.6;
        return 1.0;
    }

    double maker_multiplier() const {
        if (regime_ == REGIME_REVERT)
            return 1.5;
        if (regime_ == REGIME_BURST)
            return 0.5;
        return 1.0;
    }

    double tp_adjustment() const {
        if (regime_ == REGIME_BURST)
            return 1.4;
        if (regime_ == REGIME_REVERT)
            return 0.7;
        return 1.0;
    }

    double stop_adjustment() const {
        if (regime_ == REGIME_BURST)
            return 1.3;
        if (regime_ == REGIME_REVERT)
            return 0.8;
        return 1.0;
    }

    const char* regime_name() const {
        if (regime_ == REGIME_BURST) return "BURST";
        if (regime_ == REGIME_REVERT) return "REVERT";
        return "NEUTRAL";
    }
};

} // namespace chimera
