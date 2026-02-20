#pragma once
#include <cmath>

namespace chimera {

class QueueDecay {
public:
    inline double survival_probability(double queue_position, double volatility_bps, double imbalance) {
        double decay = std::exp(-queue_position * 0.01 - volatility_bps * 0.05);
        double pressure = imbalance * 0.6;
        return decay * pressure;
    }

    inline bool worth_waiting(double queue_position, double volatility_bps, double imbalance) {
        return survival_probability(queue_position, volatility_bps, imbalance) > 0.35;
    }
};

}
