#pragma once
#include <deque>
#include <cmath>

namespace chimera {

class VolatilityScaledSizer {
public:
    explicit VolatilityScaledSizer(double base_size);

    void record(double price);
    double size_multiplier() const;

private:
    std::deque<double> returns_;
    double base_;
    const size_t window_ = 50;
};

}
