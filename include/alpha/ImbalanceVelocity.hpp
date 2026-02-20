#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class ImbalanceVelocity {
public:
    void record(double bid_qty, double ask_qty);
    double velocity_score() const;

private:
    std::deque<double> imbalance_hist_;
    size_t max_samples_ = 25;
};

}
