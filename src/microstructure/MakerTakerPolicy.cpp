#include "microstructure/MakerTakerPolicy.hpp"

namespace chimera {

MakerTakerPolicy::MakerTakerPolicy(double latency_threshold_ms, double sweep_intensity_threshold)
    : latency_threshold_(latency_threshold_ms), sweep_threshold_(sweep_intensity_threshold) {}

ExecMode MakerTakerPolicy::decide(double rtt_ms, const VacuumSignal& sweep) const {
    if (rtt_ms > latency_threshold_)
        return ExecMode::TAKER;

    if (sweep.active && sweep.intensity > sweep_threshold_)
        return ExecMode::TAKER;

    return ExecMode::MAKER;
}

}
