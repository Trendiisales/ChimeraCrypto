#pragma once
#include <cstdint>

namespace chimera {

struct CancelBurstSignal {
    bool active;
    double ratio;
    uint64_t timestamp_ns;
};

class CancelBurstDetector {
public:
    CancelBurstDetector(double cancel_ratio_threshold);
    CancelBurstSignal update(double previous_depth, double current_depth, uint64_t timestamp_ns);

private:
    double threshold_;
};

}
