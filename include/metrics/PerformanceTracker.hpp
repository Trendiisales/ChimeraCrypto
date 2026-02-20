#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class PerformanceTracker {
public:
    void record(double pnl);
    double sharpe() const;
    double avg_return() const;

private:
    std::deque<double> returns_;
    size_t max_samples_ = 200;
};

}
