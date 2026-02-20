#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>

namespace chimera {

enum NetRegime {
    NET_CLEAN,
    NET_UNSTABLE,
    NET_BROKEN
};

class LatencyGovernor {
public:
    LatencyGovernor() : regime_(NET_CLEAN), last_flip_ts_(0) {}

    void update(double latency_ms, int64_t ts) {
        samples_.push_back(latency_ms);
        if (samples_.size() > 500)
            samples_.erase(samples_.begin());

        if (samples_.size() < 100)
            return;

        std::vector<double> tmp = samples_;
        std::sort(tmp.begin(), tmp.end());
        double p95 = tmp[(int)(tmp.size() * 0.95)];

        NetRegime new_regime = classify(p95);

        if (new_regime != regime_) {
            if (ts - last_flip_ts_ > 2000) {
                regime_ = new_regime;
                last_flip_ts_ = ts;
            }
        }
    }

    bool allow_entry(int64_t ts) const {
        if (regime_ == NET_BROKEN)
            return false;
        return true;
    }

    bool allow_micro() const {
        return regime_ == NET_CLEAN;
    }

    bool allow_impulse() const {
        return regime_ != NET_BROKEN;
    }

    NetRegime regime() const {
        return regime_;
    }

private:
    NetRegime classify(double p95) {
        if (p95 <= 8.0)
            return NET_CLEAN;
        if (p95 <= 20.0)
            return NET_UNSTABLE;
        return NET_BROKEN;
    }

private:
    std::vector<double> samples_;
    NetRegime regime_;
    int64_t last_flip_ts_;
};

}
