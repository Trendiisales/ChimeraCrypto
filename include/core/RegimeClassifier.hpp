#pragma once
#include <cstdio>
#include <cmath>

namespace chimera {

enum Regime {
    REGIME_DEAD = 0,
    REGIME_GRIND = 1,
    REGIME_BUILDUP = 2,
    REGIME_BREAKOUT = 3
};

inline const char* regime_name(Regime r) {
    switch (r) {
        case REGIME_DEAD: return "DEAD";
        case REGIME_GRIND: return "GRIND";
        case REGIME_BUILDUP: return "BUILDUP";
        case REGIME_BREAKOUT: return "BREAKOUT";
        default: return "UNKNOWN";
    }
}

// DIAGNOSTIC VERSION - prints why regime is assigned
inline Regime classify_regime(double short_avg, double long_avg) {
    static int diag_counter = 0;
    bool should_print = (++diag_counter % 500 == 0); // Print every 500 calls
    
    // Safeguard against division by zero - LOWERED from 0.01 to 0.004
    // Most crypto microstructure shows long_avg in 0.004-0.009 range during normal conditions
    if (long_avg < 0.004) {
        if (should_print) {
            std::printf("[REGIME-DIAG] long_avg=%.4f < 0.004 → REGIME_DEAD (insufficient baseline)\n", long_avg);
            std::fflush(stdout);
        }
        return REGIME_DEAD;
    }
    
    double ratio = short_avg / long_avg;
    
    // Regime classification thresholds - UPDATED to allow GRIND/BUILDUP more easily
    // DEAD: ratio < 0.6 (market extremely quiet - was 0.8, too strict)
    // GRIND: 0.6 <= ratio < 1.5 (normal market, MICRO strategy allowed)
    // BUILDUP: 1.5 <= ratio < 2.5 (increasing volatility)
    // BREAKOUT: ratio >= 2.5 (high volatility)
    
    Regime result;
    const char* reason;
    
    if (ratio < 0.6) {
        result = REGIME_DEAD;
        reason = "ratio < 0.6";
    } else if (ratio < 1.5) {
        result = REGIME_GRIND;
        reason = "0.6 <= ratio < 1.5 (MICRO allowed)";
    } else if (ratio < 2.5) {
        result = REGIME_BUILDUP;
        reason = "1.5 <= ratio < 2.5";
    } else {
        result = REGIME_BREAKOUT;
        reason = "ratio >= 2.5";
    }
    
    if (should_print) {
        std::printf("[REGIME-DIAG] short_avg=%.4f | long_avg=%.4f | ratio=%.2f | %s → %s\n",
                   short_avg, long_avg, ratio, reason, regime_name(result));
        std::fflush(stdout);
    }
    
    return result;
}

} // namespace chimera
