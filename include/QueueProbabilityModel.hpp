#pragma once
#include <cmath>

namespace chimera {

class QueueProbabilityModel {
public:
    double estimateFillProb(double queueSize, double tradeRate, 
                           double volatility) {
        double base = tradeRate / (queueSize + 1e-9);
        double volAdj = 1.0 + volatility * 50.0;

        double prob = base * volAdj;

        if (prob > 1.0) {
            prob = 1.0;
        }

        return prob;
    }
};

} // namespace chimera
