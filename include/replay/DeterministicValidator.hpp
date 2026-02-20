#pragma once
#include <vector>
#include <string>
#include <functional>

namespace chimera {

struct ReplayEvent {
    std::string symbol;
    double bid;
    double ask;
    double bid_size;
    double ask_size;
};

class DeterministicValidator {
public:
    void record(const ReplayEvent& e);

    bool validate(
        std::function<void(const ReplayEvent&)> apply_event,
        std::function<double()> final_state_snapshot);

private:
    std::vector<ReplayEvent> events_;
};

}
