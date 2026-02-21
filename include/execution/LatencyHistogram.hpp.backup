#pragma once
#include <vector>
#include <algorithm>

namespace chimera {

class LatencyHistogram {
public:
    void record(double latency_ms);
    double p50() const;
    double p95() const;

private:
    std::vector<double> samples_;
    size_t max_samples_ = 200;
};

}
