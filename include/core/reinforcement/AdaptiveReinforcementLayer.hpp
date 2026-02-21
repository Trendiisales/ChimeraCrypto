#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * AdaptiveReinforcementLayer - Self-Tuning Multipliers
 * 
 * Bounded, deterministic reinforcement (no ML black box).
 * Adjusts multipliers based on realized performance per regime.
 * 
 * Adapts:
 * - Impulse multiplier per regime
 * - Maker multiplier per regime
 * - TP scaling (global)
 * - Stop scaling (global)
 * - Aggression scaling
 * 
 * No parameter guessing. No curve fitting. Continuous adaptation.
 */
class AdaptiveReinforcementLayer {
public:

    struct TradeResult {
        int regime;           // 0 neutral, 1 burst, 2 revert
        double pnl_bps;
        double mfe_bps;
        double mae_bps;
    };

private:

    double burst_edge_ema_   = 0.0;
    double revert_edge_ema_  = 0.0;
    double neutral_edge_ema_ = 0.0;

    double burst_mult_   = 1.0;
    double revert_mult_  = 1.0;
    double neutral_mult_ = 1.0;

    double tp_mult_   = 1.0;
    double stop_mult_ = 1.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    void record_trade(const TradeResult& tr) {

        double quality =
            tr.pnl_bps
          + (tr.mfe_bps * 0.2)
          - (tr.mae_bps * 0.1);

        if (tr.regime == 1)
            burst_edge_ema_ =
                ema(burst_edge_ema_, quality, 0.05);

        else if (tr.regime == 2)
            revert_edge_ema_ =
                ema(revert_edge_ema_, quality, 0.05);

        else
            neutral_edge_ema_ =
                ema(neutral_edge_ema_, quality, 0.05);

        update_multipliers();
    }

private:

    void update_multipliers() {

        burst_mult_ =
            clamp(1.0 + burst_edge_ema_ * 0.02, 0.6, 1.8);

        revert_mult_ =
            clamp(1.0 + revert_edge_ema_ * 0.02, 0.6, 1.8);

        neutral_mult_ =
            clamp(1.0 + neutral_edge_ema_ * 0.02, 0.7, 1.3);

        double global_edge =
            (burst_edge_ema_
           + revert_edge_ema_
           + neutral_edge_ema_) / 3.0;

        tp_mult_ =
            clamp(1.0 + global_edge * 0.015, 0.8, 1.5);

        stop_mult_ =
            clamp(1.0 - global_edge * 0.01, 0.8, 1.4);
    }

public:

    double regime_multiplier(int regime) const {
        if (regime == 1) return burst_mult_;
        if (regime == 2) return revert_mult_;
        return neutral_mult_;
    }

    double tp_multiplier() const {
        return tp_mult_;
    }

    double stop_multiplier() const {
        return stop_mult_;
    }

    // Getters for monitoring
    double burst_edge() const { return burst_edge_ema_; }
    double revert_edge() const { return revert_edge_ema_; }
    double neutral_edge() const { return neutral_edge_ema_; }
};

} // namespace chimera
