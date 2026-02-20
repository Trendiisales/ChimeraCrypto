#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class SlippageModel {
public:
    void record(double edge_bps, double realized_bps);
    double predict(double edge_bps) const;

private:
    std::deque<double> errors_;
    size_t max_samples_ = 50;
};

}
