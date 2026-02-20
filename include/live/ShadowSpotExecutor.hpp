#pragma once
#include <cstddef>
#include "core/Symbol.hpp"
#include "core/PositionLedger.hpp"
#include "core/OrderTracker.hpp"
#include "risk/RiskGovernor.hpp"

namespace chimera {

template<typename Derived>
class ExecutorBase {
public:
    inline void execute(
        SymbolID id,
        bool is_buy,
        double price,
        double size)
    {
        static_cast<Derived*>(this)
            ->execute_impl(id, is_buy, price, size);
    }
};

class ShadowSpotExecutor :
    public ExecutorBase<ShadowSpotExecutor>
{
public:
    ShadowSpotExecutor(
        bool enabled,
        RiskGovernor& governor,
        PositionLedger& ledger,
        OrderTracker& tracker)
        : enabled_(enabled),
          governor_(governor),
          ledger_(ledger),
          tracker_(tracker)
    {
    }

    inline void execute_impl(
        SymbolID id,
        bool is_buy,
        double price,
        double size)
    {
        if (!enabled_) return;
        if (!governor_.allow_global()) return;

        ledger_.apply_trade(id, is_buy, size);
        tracker_.track(id, is_buy, price, size);
    }

private:
    bool enabled_;
    RiskGovernor& governor_;
    PositionLedger& ledger_;
    OrderTracker& tracker_;
};

}
