#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class ExecutionGovernor {
public:
    ExecutionGovernor(double latency_limit_ms = 20.0,
                      double slippage_limit_bps = 15.0);

    void record(double latency_ms,
                double slippage_bps);

    bool allow_trading() const;

private:
    std::deque<double> latency_samples_;
    std::deque<double> slippage_samples_;

    size_t max_samples_ = 50;

    double latency_limit_ms_;
    double slippage_limit_bps_;

    double avg(const std::deque<double>& d) const;
};

}
