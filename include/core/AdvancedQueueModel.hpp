#pragma once
#include <cmath>

namespace chimera {

class AdvancedQueueModel {
public:
    inline double queue_priority_score(double imbalance, double spread_bps, double volatility_bps, double latency_ms) const {
        double score = imbalance * 0.5 + (1.0 / (1.0 + spread_bps)) * 0.2 + (volatility_bps * 0.03) - (latency_ms * 0.05);
        if (score < 0.0) score = 0.0;
        if (score > 1.0) score = 1.0;
        return score;
    }

    inline bool should_post(double imbalance, double spread_bps, double volatility_bps, double latency_ms) const {
        return queue_priority_score(imbalance, spread_bps, volatility_bps, latency_ms) > 0.55;
    }
};

}
