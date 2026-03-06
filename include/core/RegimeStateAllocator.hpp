#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>
#include <cstdio>

namespace chimera {

// =============================================================
// REGIME STATE ALLOCATOR
// Volatility State → Capital Allocation Engine
// Dynamically scales exposure budget per symbol
// Works ABOVE Structural / Convex / Compression
// Does NOT generate signals — allocates capital
// =============================================================

enum class VolState {
    DEAD = 0,
    COMPRESSION = 1,
    EXPANSION = 2,
    SHOCK = 3
};

class RegimeStateAllocator {
public:
    explicit RegimeStateAllocator(const std::string& sym)
        : symbol(sym) {}

    // Evaluate state and compute allowed_R multiplier
    void evaluate(double vol_ratio_smooth,
                  double short_vol,
                  double long_vol_ema,
                  double displacement_bp,
                  double acceleration_bp)
    {
        // NOISE FILTER: Ignore tiny moves
        if (std::abs(displacement_bp) < 5.0) {
            return; // Don't react to micro noise
        }
        
        // EMA smoothing to prevent flipping
        if (smoothed_vol == 0.0) smoothed_vol = vol_ratio_smooth;
        smoothed_vol = 0.8 * smoothed_vol + 0.2 * vol_ratio_smooth;
        
        VolState prev = state;

        // State classification with higher thresholds
        if (smoothed_vol < 0.75) {
            state = VolState::DEAD;
        }
        else if (smoothed_vol < 1.2) {
            state = VolState::COMPRESSION;
        }
        else if (smoothed_vol < 1.8) {
            state = VolState::EXPANSION;
        }
        else {
            state = VolState::SHOCK;
        }
        
        // HYSTERESIS: Prevent flipping on tiny changes
        if (std::abs(smoothed_vol - last_transition_vol) < 0.15) {
            state = prev; // Stay in current state
        }

        if (state != prev) {
            last_transition_vol = smoothed_vol;
            const char* state_names[] = {"DEAD", "COMPRESSION", "EXPANSION", "SHOCK"};
            std::printf("[ALLOCATOR-STATE] %s | %s -> %s | smoothed_vol=%.2f | raw_vol=%.2f | disp=%.1fbp\n",
                        symbol.c_str(),
                        state_names[(int)prev],
                        state_names[(int)state],
                        smoothed_vol,
                        vol_ratio_smooth,
                        displacement_bp);
            std::fflush(stdout);
        }

        compute_multiplier();
    }

    double allowed_R(double base_symbol_cap_R) const {
        return base_symbol_cap_R * multiplier;
    }

    VolState get_state() const {
        return state;
    }

    double get_multiplier() const {
        return multiplier;
    }

    const char* get_state_name() const {
        const char* state_names[] = {"DEAD", "COMPRESSION", "EXPANSION", "SHOCK"};
        return state_names[(int)state];
    }

private:
    std::string symbol;
    VolState state = VolState::DEAD;
    double multiplier = 0.0;
    
    // Smoothing and hysteresis
    double smoothed_vol = 0.0;
    double last_transition_vol = 0.0;

    void compute_multiplier() {
        switch (state) {
        case VolState::DEAD:
            multiplier = 0.0;      // No trading
            break;

        case VolState::COMPRESSION:
            multiplier = 0.5;      // Conservative sizing
            break;

        case VolState::EXPANSION:
            multiplier = 1.0;      // Normal sizing
            break;

        case VolState::SHOCK:
            multiplier = 1.5;      // Aggressive scaling allowed
            break;
        }
    }
};

} // namespace chimera
