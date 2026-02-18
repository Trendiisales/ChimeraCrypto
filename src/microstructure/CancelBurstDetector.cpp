#include "microstructure/CancelBurstDetector.hpp"

namespace chimera {

CancelBurstDetector::CancelBurstDetector(double cancel_ratio_threshold)
    : threshold_(cancel_ratio_threshold) {}

CancelBurstSignal CancelBurstDetector::update(double previous_depth, double current_depth, uint64_t timestamp_ns) {
    CancelBurstSignal sig{};
    sig.active = false;
    sig.ratio = 0.0;
    sig.timestamp_ns = timestamp_ns;

    if (previous_depth <= 0.0)
        return sig;

    double drop = (previous_depth - current_depth) / previous_depth;

    if (drop > threshold_) {
        sig.active = true;
        sig.ratio = drop;
    }

    return sig;
}

}
