#pragma once
#include <thread>
#include <atomic>
#include <cmath>
#include "recon/ExchangeTruth.hpp"
#include "risk/RiskGovernor.hpp"
#include "spine/EventSpine.hpp"
#include "core/PositionLedger.hpp"
#include "core/OrderTracker.hpp"

namespace chimera {

class ReconciliationEngine {
public:
    ReconciliationEngine(ExchangeTruth& truth,
                         RiskGovernor& governor,
                         EventSpine& spine,
                         double tolerance,
                         PositionLedger& ledger,
                         OrderTracker& tracker);

    void start();
    void stop();

private:
    void loop();

    ExchangeTruth& truth_;
    RiskGovernor& governor_;
    EventSpine& spine_;
    PositionLedger& ledger_;
    OrderTracker& tracker_;
    double tolerance_;

    std::thread worker_;
    std::atomic<bool> running_{false};
};

}
