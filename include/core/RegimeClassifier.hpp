#pragma once
#include <cstdio>
#include <cmath>
#include "config/TradingConfig.hpp"

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
    bool should_print = (++diag_counter % TradingConfig::REGIME_DIAG_INTERVAL == 0);
    
    // Safeguard against division by zero
    if (long_avg < TradingConfig::REGIME_MIN_LONG_AVG) {
        if (should_print) {
            std::printf("[REGIME-DIAG] long_avg=%.4f < %.4f → REGIME_DEAD (insufficient baseline)\n", 
                       long_avg, TradingConfig::REGIME_MIN_LONG_AVG);
            std::fflush(stdout);
        }
        return REGIME_DEAD;
    }
    
    double ratio = short_avg / long_avg;
    
    Regime result;
    const char* reason;
    
    if (ratio < TradingConfig::REGIME_DEAD_THRESHOLD) {
        result = REGIME_DEAD;
        reason = "ratio < DEAD_THRESHOLD";
    } else if (ratio < TradingConfig::REGIME_GRIND_THRESHOLD) {
        result = REGIME_GRIND;
        reason = "DEAD <= ratio < GRIND (MICRO allowed)";
    } else if (ratio < TradingConfig::REGIME_BUILDUP_THRESHOLD) {
        result = REGIME_BUILDUP;
        reason = "GRIND <= ratio < BUILDUP";
    } else {
        result = REGIME_BREAKOUT;
        reason = "ratio >= BUILDUP (BREAKOUT)";
    }
    
    if (should_print) {
        std::printf("[REGIME-DIAG] short_avg=%.4f | long_avg=%.4f | ratio=%.2f | %s → %s\n",
                   short_avg, long_avg, ratio, reason, regime_name(result));
        std::fflush(stdout);
    }
    
    return result;
}

} // namespace chimera
