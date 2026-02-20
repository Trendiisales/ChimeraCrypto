#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class LiquidityVacuum {
public:
    void record(double bid_qty, double ask_qty);
    double score() const;

private:
    std::deque<double> bid_hist_;
    std::deque<double> ask_hist_;
    size_t max_samples_ = 20;
};

}
