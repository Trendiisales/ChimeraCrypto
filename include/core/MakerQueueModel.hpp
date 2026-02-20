#pragma once
#include <cmath>

namespace chimera {

class MakerQueueModel {
public:
    inline double expected_fill_probability(double imbalance, double volatility_bps) const {
        double pressure = imbalance * 0.6 + volatility_bps * 0.04;
        if (pressure > 1.0) pressure = 1.0;
        if (pressure < 0.0) pressure = 0.0;
        return pressure;
    }

    inline bool should_post(double imbalance, double volatility_bps) const {
        return expected_fill_probability(imbalance, volatility_bps) > 0.45;
    }
};

}
