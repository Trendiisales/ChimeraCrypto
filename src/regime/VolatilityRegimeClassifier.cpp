#include "regime/VolatilityRegimeClassifier.hpp"
#include <cmath>

namespace chimera {

VolatilityRegimeClassifier::VolatilityRegimeClassifier(size_t window, double high_threshold, double low_threshold)
    : window_(window), high_(high_threshold), low_(low_threshold) {}

Regime VolatilityRegimeClassifier::update(double return_pct) {
    returns_.push_back(return_pct);
    if (returns_.size() > window_)
        returns_.pop_front();

    if (returns_.size() < window_)
        return Regime::NORMAL;

    double var = 0.0;
    for (double r : returns_)
        var += r * r;

    var /= returns_.size();
    double vol = std::sqrt(var);

    if (vol > high_)
        return Regime::HIGH_VOL;
    if (vol < low_)
        return Regime::LOW_VOL;

    return Regime::NORMAL;
}

}
