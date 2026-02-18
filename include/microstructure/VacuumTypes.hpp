#pragma once
#include <cstdint>

namespace chimera {

enum class SweepSide {
    NONE = 0,
    BUY_SWEEP,
    SELL_SWEEP
};

struct VacuumSignal {
    bool active;
    SweepSide side;
    double intensity;
    double imbalance;
    uint64_t timestamp_ns;
};

}
