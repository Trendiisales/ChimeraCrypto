#include "replay/DeterministicValidator.hpp"

namespace chimera {

void DeterministicValidator::record(
    const ReplayEvent& e)
{
    events_.push_back(e);
}

bool DeterministicValidator::validate(
    std::function<void(const ReplayEvent&)> apply_event,
    std::function<double()> final_state_snapshot)
{
    double initial_snapshot = final_state_snapshot();

    for (const auto& e : events_)
        apply_event(e);

    double replay_snapshot = final_state_snapshot();

    return std::abs(replay_snapshot -
                    initial_snapshot) < 1e-6;
}

}
