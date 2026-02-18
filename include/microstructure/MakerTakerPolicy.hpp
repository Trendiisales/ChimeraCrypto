#pragma once
#include "VacuumTypes.hpp"

namespace chimera {

enum class ExecMode {
    MAKER,
    TAKER
};

class MakerTakerPolicy {
public:
    MakerTakerPolicy(double latency_threshold_ms, double sweep_intensity_threshold);
    ExecMode decide(double rtt_ms, const VacuumSignal& sweep) const;

private:
    double latency_threshold_;
    double sweep_threshold_;
};

}
