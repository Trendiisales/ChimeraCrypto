#include "risk/AdaptiveSizer.hpp"
#include <algorithm>
#include <cmath>

namespace chimera {

AdaptiveSizer::AdaptiveSizer(double base_size, double max_multiplier)
    : base_(base_size), max_mult_(max_multiplier) {}

double AdaptiveSizer::size(double sweep_intensity, double imbalance_accel) const {
    double multiplier = 1.0 + std::abs(sweep_intensity) + std::abs(imbalance_accel);
    multiplier = std::min(multiplier, max_mult_);
    return base_ * multiplier;
}

}
