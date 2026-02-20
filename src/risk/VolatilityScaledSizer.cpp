#include "risk/VolatilityScaledSizer.hpp"

namespace chimera {

VolatilityScaledSizer::VolatilityScaledSizer(double base_size)
    : base_(base_size) {}

void VolatilityScaledSizer::record(double price)
{
    if (!returns_.empty())
        returns_.push_back(price - returns_.back());
    else
        returns_.push_back(price);

    if (returns_.size() > window_)
        returns_.pop_front();
}

double VolatilityScaledSizer::size_multiplier() const
{
    if (returns_.size() < 10) return 1.0;

    double mean = 0.0;
    for (double r : returns_)
        mean += r;
    mean /= returns_.size();

    double var = 0.0;
    for (double r : returns_)
        var += (r - mean)*(r - mean);
    var /= returns_.size();

    double vol = std::sqrt(var);

    if (vol > 0.5) return 1.5;
    if (vol > 0.2) return 1.2;
    return 0.8;
}

}
