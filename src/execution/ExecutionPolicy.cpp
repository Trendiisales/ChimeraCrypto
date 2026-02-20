#include "execution/ExecutionPolicy.hpp"

namespace chimera {

void ExecutionPolicy::update_latency(double rtt_ms) {
    last_rtt_ = rtt_ms;
}

void ExecutionPolicy::update_spread(double spread) {
    last_spread_ = spread;
}

void ExecutionPolicy::record_reject() {
    reject_count_++;
}

bool ExecutionPolicy::allow_trade() const {
    if (last_rtt_ > max_latency_ms_)
        return false;

    if (last_spread_ > max_spread_)
        return false;

    if (reject_count_ > max_rejects_)
        return false;

    return true;
}

}
