#include "microstructure/ImbalanceAcceleration.hpp"
#include <cmath>

namespace chimera {

ImbalanceAcceleration::ImbalanceAcceleration(size_t window, double threshold)
    : window_(window), threshold_(threshold) {}

ImbalanceAccelSignal ImbalanceAcceleration::update(double imbalance, uint64_t timestamp_ns) {
    ImbalanceAccelSignal sig{};
    sig.active = false;
    sig.acceleration = 0.0;
    sig.timestamp_ns = timestamp_ns;

    history_.push_back(imbalance);
    if (history_.size() > window_)
        history_.pop_front();

    if (history_.size() < window_)
        return sig;

    double first = history_.front();
    double last = history_.back();
    double accel = last - first;

    if (std::abs(accel) > threshold_) {
        sig.active = true;
        sig.acceleration = accel;
    }

    return sig;
}

}
