#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <mutex>

namespace Chimera {

class LatencyTracker {
public:
    void record(double ms) {
        std::lock_guard<std::mutex> lock(mtx_);
        samples_.push_back(ms);
        if (samples_.size() > 200)
            samples_.erase(samples_.begin());
    }

    double latest() const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (samples_.empty()) return 0.0;
        return samples_.back();
    }

    double mean() const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (samples_.empty()) return 0.0;
        double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
        return sum / samples_.size();
    }

    double p95() const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (samples_.empty()) return 0.0;
        std::vector<double> copy = samples_;
        std::sort(copy.begin(), copy.end());
        size_t idx = static_cast<size_t>(copy.size() * 0.95);
        if (idx >= copy.size()) idx = copy.size() - 1;
        return copy[idx];
    }

private:
    mutable std::mutex mtx_;
    std::vector<double> samples_;
};

struct EngineSnapshot {
    double equity = 0.0;
    double unrealized = 0.0;
    double realized = 0.0;
    double latency_ms = 0.0;
    double latency_avg = 0.0;
    double latency_p95 = 0.0;
    double slippage_bps = 0.0;
    double fill_probability = 0.0;
    double conviction = 0.0;
    double cost_floor = 0.0;
    double edge_score = 0.0;
    double heat_score = 0.0;
    int orders_sent = 0;
    int fills = 0;
    int rejects = 0;
    double exposure_usd = 0.0;
    bool governor_blocked = false;

    std::string to_json() const {
        std::ostringstream oss;
        oss << "{";
        oss << "\"equity\":" << equity << ",";
        oss << "\"unrealized\":" << unrealized << ",";
        oss << "\"realized\":" << realized << ",";
        oss << "\"latency_ms\":" << latency_ms << ",";
        oss << "\"latency_avg\":" << latency_avg << ",";
        oss << "\"latency_p95\":" << latency_p95 << ",";
        oss << "\"slippage_bps\":" << slippage_bps << ",";
        oss << "\"fill_probability\":" << fill_probability << ",";
        oss << "\"conviction\":" << conviction << ",";
        oss << "\"cost_floor\":" << cost_floor << ",";
        oss << "\"edge_score\":" << edge_score << ",";
        oss << "\"heat_score\":" << heat_score << ",";
        oss << "\"orders_sent\":" << orders_sent << ",";
        oss << "\"fills\":" << fills << ",";
        oss << "\"rejects\":" << rejects << ",";
        oss << "\"exposure_usd\":" << exposure_usd << ",";
        oss << "\"governor_blocked\":" << (governor_blocked ? "true" : "false");
        oss << "}";
        return oss.str();
    }
};

}
