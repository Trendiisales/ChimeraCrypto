#include "depth/DepthManager.hpp"

namespace chimera {

DepthManager::DepthManager(const std::string& symbol,
                           RiskGovernor& governor,
                           EventSpine& spine)
    : bootstrap_(symbol, governor, spine)
{}

void DepthManager::handle_snapshot(const Snapshot& snap)
{
    bootstrap_.on_snapshot(snap);
}

void DepthManager::handle_ws_delta(const DepthEvent& ev)
{
    bootstrap_.on_ws_delta(ev);
}

const L2Book& DepthManager::book() const
{
    return bootstrap_.book();
}

bool DepthManager::ready() const
{
    return bootstrap_.ready();
}

}
