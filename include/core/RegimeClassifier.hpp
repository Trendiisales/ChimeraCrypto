#pragma once

namespace chimera {

enum Regime {
    REGIME_DEAD,
    REGIME_GRIND,
    REGIME_BUILDUP,
    REGIME_BREAKOUT
};

inline Regime classify_regime(double short_avg, double long_avg) {
    if (short_avg < 3.0)
        return REGIME_DEAD;
    
    if (short_avg < 6.0)
        return REGIME_GRIND;
    
    if (short_avg < 10.0)
        return REGIME_BUILDUP;
    
    return REGIME_BREAKOUT;
}

inline double regime_size_multiplier(Regime r) {
    switch(r) {
        case REGIME_GRIND:     return 0.75;
        case REGIME_BUILDUP:   return 1.0;
        case REGIME_BREAKOUT:  return 2.0;
        default:               return 0.0;
    }
}

inline const char* regime_name(Regime r) {
    switch(r) {
        case REGIME_DEAD: return "DEAD";
        case REGIME_GRIND: return "GRIND";
        case REGIME_BUILDUP: return "BUILDUP";
        case REGIME_BREAKOUT: return "BREAKOUT";
        default: return "UNKNOWN";
    }
}

}
