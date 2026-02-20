#pragma once
#include <vector>
#include <cstdint>

namespace chimera {

class LatencyHistogramOld {
public:
    LatencyHistogramOld(size_t window_size = 1000);
    void add_sample(double latency_ms);
    double percentile(double p) const;
    double p50() const { return percentile(0.50); }
    double p75() const { return percentile(0.75); }
    double p95() const { return percentile(0.95); }
    double p99() const { return percentile(0.99); }
    size_t sample_count() const { return samples_.size(); }

private:
    std::vector<double> samples_;
    size_t window_size_;
    size_t write_idx_;
};

}
