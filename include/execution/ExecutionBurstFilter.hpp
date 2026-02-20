#pragma once

namespace chimera {

class ExecutionBurstFilter {
public:
    bool should_block(double latency_mean, double latency_p95) const {
        if (latency_p95 > 2.0 * latency_mean && latency_p95 > 5.0) {
            return true;
        }
        return false;
    }
};

}
