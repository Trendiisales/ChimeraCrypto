#pragma once

namespace chimera {

class AdaptiveStopEngine {
public:
    double compute(double entry, double volatility, 
                  double latencyMs, bool aggressive) {
        double stopFactor = volatility * 2.5;

        if (latencyMs > 10.0) {
            stopFactor *= 1.3;
        }

        if (aggressive) {
            stopFactor *= 0.8;
        }

        return entry * stopFactor;
    }
};

} // namespace chimera
