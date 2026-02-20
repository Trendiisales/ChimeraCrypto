#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class SpreadCompression {
public:
    void record(double bid, double ask);
    double compression_score() const;

private:
    std::deque<double> spread_hist_;
    size_t max_samples_ = 30;
};

}
