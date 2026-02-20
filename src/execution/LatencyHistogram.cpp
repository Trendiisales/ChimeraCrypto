#include "execution/LatencyHistogram.hpp"

namespace chimera {

void LatencyHistogram::record(double latency_ms)
{
    samples_.push_back(latency_ms);
    if (samples_.size() > max_samples_)
        samples_.erase(samples_.begin());
}

double LatencyHistogram::p50() const
{
    if (samples_.empty()) return 0.0;
    auto temp = samples_;
    std::sort(temp.begin(), temp.end());
    return temp[temp.size() / 2];
}

double LatencyHistogram::p95() const
{
    if (samples_.empty()) return 0.0;
    auto temp = samples_;
    std::sort(temp.begin(), temp.end());
    size_t idx = static_cast<size_t>(temp.size() * 0.95);
    if (idx >= temp.size())
        idx = temp.size() - 1;
    return temp[idx];
}

}
