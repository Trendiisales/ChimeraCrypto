#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class VolatilityAllocator {
public:
    VolatilityAllocator(double target_vol = 0.02);

    void record(double price);

    void record_price(double price)
    {
        record(price);
    }

    double volatility() const;
    double size_multiplier() const;

private:
    std::deque<double> returns_;
    double last_price_ = 0.0;
    size_t max_samples_ = 100;
    double target_vol_;
};

}
