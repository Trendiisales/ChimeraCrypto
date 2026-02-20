#pragma once
#include <string>
#include "l2/L2Bootstrapper.hpp"
#include "risk/RiskGovernor.hpp"
#include "spine/EventSpine.hpp"

namespace chimera {

class DepthManager {
public:
    DepthManager(const std::string& symbol,
                 RiskGovernor& governor,
                 EventSpine& spine);

    void handle_snapshot(const Snapshot& snap);
    void handle_ws_delta(const DepthEvent& ev);

    const L2Book& book() const;
    bool ready() const;

private:
    L2Bootstrapper bootstrap_;
};

}
