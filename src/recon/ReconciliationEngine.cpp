#include "recon/ReconciliationEngine.hpp"

namespace chimera {

ReconciliationEngine::ReconciliationEngine(
    ExchangeTruth& truth,
    RiskGovernor& governor,
    EventSpine& spine,
    double tolerance,
    PositionLedger& ledger,
    OrderTracker& tracker)
    : truth_(truth),
      governor_(governor),
      spine_(spine),
      ledger_(ledger),
      tracker_(tracker),
      tolerance_(tolerance)
{
}

void ReconciliationEngine::start()
{
    running_ = true;
    worker_ = std::thread([this]() { loop(); });
}

void ReconciliationEngine::stop()
{
    running_ = false;
    if (worker_.joinable())
        worker_.join();
}

void ReconciliationEngine::loop()
{
    while (running_)
    {
        for (int i = 0; i < static_cast<int>(SymbolID::COUNT); ++i)
        {
            SymbolID id = static_cast<SymbolID>(i);
            double local_pos = ledger_.get_position(id);
            (void)local_pos;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

}
