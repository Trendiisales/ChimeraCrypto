#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

enum class Regime {
    LOW_VOL,
    NORMAL,
    HIGH_VOL
};

class VolatilityRegimeClassifier {
public:
    VolatilityRegimeClassifier(size_t window, double high_threshold, double low_threshold);
    Regime update(double return_pct);

private:
    size_t window_;
    double high_;
    double low_;
    std::deque<double> returns_;
};

}
