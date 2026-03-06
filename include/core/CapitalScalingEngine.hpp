#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * CapitalScalingEngine - Structural Compounding System
 * 
 * Asymmetric capital scaling:
 * - Scale up SLOW when expectancy positive and stable
 * - Scale down FAST when expectancy negative or slip expanding
 * 
 * Prevents emotional overtrading and protects capital during regime shifts.
 */
class CapitalScalingEngine {
public:

    struct Metrics {
        double net_expectancy;
        double gross_expectancy;
        double structural_cost;
        double win_rate;
        double slip_cost;
        double drawdown_bps;
    };

private:

    double capital_mult_ = 1.0;
    double stability_ema_ = 0.0;
    double last_net_ = 0.0;

    double max_mult_ = 3.0;
    double min_mult_ = 0.3;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

public:

    void update(const Metrics& m) {

        // Stability = net edge minus cost pressure
        double structural_margin =
            m.gross_expectancy - m.structural_cost;

        double stability_score =
            m.net_expectancy * 0.6
          + structural_margin * 0.3
          + (m.win_rate - 0.5) * 5.0;

        stability_ema_ = ema(stability_ema_, stability_score, 0.03);

        // Scale up slowly (compound 2% when stable)
        if (stability_ema_ > 0.5
        && m.net_expectancy > 0.5
        && structural_margin > 0.5)
        {
            capital_mult_ *= 1.02;
        }

        // Mild negative → reduce gently
        if (m.net_expectancy < 0.0) {
            capital_mult_ *= 0.95;
        }

        // Structural collapse → cut hard
        if (m.net_expectancy < -1.0
        || structural_margin < -0.5
        || m.drawdown_bps < -20.0)
        {
            capital_mult_ *= 0.7;
        }

        capital_mult_ = clamp(capital_mult_, min_mult_, max_mult_);

        last_net_ = m.net_expectancy;
    }

    double multiplier() const {
        return capital_mult_;
    }
    
    double stability() const {
        return stability_ema_;
    }
};

} // namespace chimera
