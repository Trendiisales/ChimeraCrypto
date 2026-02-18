#pragma once
#include <cmath>

namespace chimera {

enum class MarketRegime {
    CALM,
    NORMAL,
    VOLATILE,
    CHAOTIC
};

class RegimeClassifier {
public:
    void update(double mid) {
        if (lastMid_ == 0.0) {
            lastMid_ = mid;
            return;
        }

        double ret = std::fabs((mid - lastMid_) / lastMid_);
        volEma_ = 0.96 * volEma_ + 0.04 * ret;
        lastMid_ = mid;
    }

    MarketRegime regime() const {
        if (volEma_ < 0.0002) return MarketRegime::CALM;
        if (volEma_ < 0.0008) return MarketRegime::NORMAL;
        if (volEma_ < 0.0020) return MarketRegime::VOLATILE;
        return MarketRegime::CHAOTIC;
    }

    double volatility() const {
        return volEma_;
    }

private:
    double lastMid_ = 0.0;
    double volEma_ = 0.0;
};

} // namespace chimera
