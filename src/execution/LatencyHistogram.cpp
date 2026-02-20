#include "execution/LatencyHistogram.hpp"
#include <algorithm>

namespace chimera {

LatencyHistogramOld::LatencyHistogramOld(size_t window_size)
    : window_size_(window_size), write_idx_(0) {
    samples_.reserve(window_size);
}

void LatencyHistogramOld::add_sample(double latency_ms) {
    if (samples_.size() < window_size_) {
        samples_.push_back(latency_ms);
    } else {
        samples_[write_idx_] = latency_ms;
        write_idx_ = (write_idx_ + 1) % window_size_;
    }
}

double LatencyHistogramOld::percentile(double p) const {
    if (samples_.empty()) return 0.0;
    
    std::vector<double> sorted = samples_;
    std::sort(sorted.begin(), sorted.end());
    
    size_t idx = static_cast<size_t>(p * sorted.size());
    if (idx >= sorted.size()) idx = sorted.size() - 1;
    
    return sorted[idx];
}

}
