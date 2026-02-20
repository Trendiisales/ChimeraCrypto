#pragma once
#include <unordered_map>
#include <deque>
#include <string>

namespace chimera {

struct HeatmapStats {
    double avg_latency_ms = 0.0;
    double avg_slippage_bps = 0.0;
    size_t samples = 0;
};

class ExecutionHeatmap {
public:
    void record(const std::string& symbol,
                double latency_ms,
                double slippage_bps);

    HeatmapStats stats(
        const std::string& symbol) const;

private:
    struct Bucket {
        std::deque<double> latency;
        std::deque<double> slippage;
    };

    std::unordered_map<std::string,
                       Bucket> buckets_;

    size_t max_samples_ = 100;

    double avg(const std::deque<double>& d) const;
};

}
