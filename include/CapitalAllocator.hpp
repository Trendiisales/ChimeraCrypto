#pragma once
#include "RegimeClassifier.hpp"

namespace chimera {

class CapitalAllocator {
public:
    double scale(MarketRegime regime, double baseSize,
                bool latencyUnstable, bool shock) {
        if (shock) {
            return 0.0;
        }

        if (latencyUnstable) {
            baseSize *= 0.5;
        }

        switch (regime) {
            case MarketRegime::CALM: 
                return baseSize * 1.2;
            case MarketRegime::NORMAL: 
                return baseSize;
            case MarketRegime::VOLATILE: 
                return baseSize * 0.6;
            case MarketRegime::CHAOTIC: 
                return 0.0;
        }

        return baseSize;
    }
};

} // namespace chimera
