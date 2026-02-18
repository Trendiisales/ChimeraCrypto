#pragma once
#include <cmath>

namespace chimera {

class MicroburstDetector {
public:
    void update(double tradeVolume) {
        volEma_ = 0.95 * volEma_ + 0.05 * tradeVolume;
        double ratio = tradeVolume / (volEma_ + 1e-9);
        burstScore_ = 0.9 * burstScore_ + 0.1 * ratio;
    }

    double score() const {
        return burstScore_;
    }

    bool burst() const {
        return burstScore_ > 2.5;
    }

private:
    double volEma_ = 0.0;
    double burstScore_ = 0.0;
};

} // namespace chimera
