#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * SelfThrottlingGovernor - Automatic De-Risk System
 * 
 * Progressively reduces risk when structural expectancy degrades.
 * Does NOT flip system off instantly - degrades in measured tiers.
 * 
 * Tier 0: Normal operation
 * Tier 1: Soft throttle (reduce aggression 30%)
 * Tier 2: Disable losing regimes
 * Tier 3: Disable losing latency bands
 * Tier 4: Hard stop (all trading halted)
 */
class SelfThrottlingGovernor {
public:

    enum Tier {
        TIER_NORMAL = 0,
        TIER_SOFT_THROTTLE = 1,
        TIER_REGIME_DISABLE = 2,
        TIER_LATENCY_DISABLE = 3,
        TIER_HARD_STOP = 4
    };

private:

    Tier tier_ = TIER_NORMAL;

    double aggression_mult_ = 1.0;
    bool disable_regime_[3] = {false, false, false};
    bool disable_latency_band_[3] = {false, false, false};

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

public:

    void evaluate(double net_expectancy,
                  double gross_expectancy,
                  double structural_cost,
                  double win_rate,
                  double regime_expectancy[3],
                  double latency_expectancy[3])
    {
        // Reset flags
        for (int i = 0; i < 3; i++) {
            disable_regime_[i] = false;
            disable_latency_band_[i] = false;
        }

        if (net_expectancy > 0) {
            tier_ = TIER_NORMAL;
            aggression_mult_ = 1.0;
            return;
        }

        // Tier 1: Net negative but gross positive (slip problem)
        if (net_expectancy < 0 && gross_expectancy > 0) {
            tier_ = TIER_SOFT_THROTTLE;
            aggression_mult_ = 0.7;
        }

        // Tier 2: Disable losing regimes
        for (int r = 0; r < 3; r++) {
            if (regime_expectancy[r] < 0)
                disable_regime_[r] = true;
        }

        if (disable_regime_[0] || disable_regime_[1] || disable_regime_[2]) {
            tier_ = TIER_REGIME_DISABLE;
            aggression_mult_ = 0.6;
        }

        // Tier 3: Disable losing latency bands
        for (int b = 0; b < 3; b++) {
            if (latency_expectancy[b] < 0)
                disable_latency_band_[b] = true;
        }

        if (disable_latency_band_[0]
        || disable_latency_band_[1]
        || disable_latency_band_[2]) {
            tier_ = TIER_LATENCY_DISABLE;
            aggression_mult_ = 0.5;
        }

        // Tier 4: Structural collapse
        if (net_expectancy < -1.0
        && gross_expectancy < structural_cost) {
            tier_ = TIER_HARD_STOP;
            aggression_mult_ = 0.0;
        }
    }

    double aggression_multiplier() const {
        return aggression_mult_;
    }

    bool regime_allowed(int r) const {
        if (tier_ >= TIER_REGIME_DISABLE && r >= 0 && r < 3)
            return !disable_regime_[r];
        return true;
    }

    bool latency_allowed(int band) const {
        if (tier_ >= TIER_LATENCY_DISABLE && band >= 0 && band < 3)
            return !disable_latency_band_[band];
        return true;
    }

    bool hard_stop() const {
        return tier_ == TIER_HARD_STOP;
    }

    Tier current_tier() const {
        return tier_;
    }
    
    const char* tier_name() const {
        switch (tier_) {
            case TIER_NORMAL: return "NORMAL";
            case TIER_SOFT_THROTTLE: return "SOFT_THROTTLE";
            case TIER_REGIME_DISABLE: return "REGIME_DISABLE";
            case TIER_LATENCY_DISABLE: return "LATENCY_DISABLE";
            case TIER_HARD_STOP: return "HARD_STOP";
            default: return "UNKNOWN";
        }
    }
};

} // namespace chimera
