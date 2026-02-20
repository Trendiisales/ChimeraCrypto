#include "risk/VolatilityAllocator.hpp"
#include <cmath>
#include <algorithm>

namespace chimera {

VolatilityAllocator::VolatilityAllocator(double target_vol)
    : target_vol_(target_vol)
{
}

void VolatilityAllocator::record(double price)
{
    if (last_price_ > 0.0) {
        double r = (price - last_price_) / last_price_;
        returns_.push_back(r);
        if (returns_.size() > max_samples_)
            returns_.pop_front();
    }
    last_price_ = price;
}

double VolatilityAllocator::volatility() const
{
    if (returns_.empty()) return 0.0;
    
    double sum = 0.0;
    for (double r : returns_)
        sum += r * r;
    
    return std::sqrt(sum / returns_.size());
}

double VolatilityAllocator::size_multiplier() const
{
    double vol = volatility();
    
    // Prevent division by very small numbers
    if (vol < 0.001) vol = 0.001;
    
    double mult = target_vol_ / vol;
    
    // Clamp to reasonable range
    return std::max(0.1, std::min(mult, 2.0));
}

}
