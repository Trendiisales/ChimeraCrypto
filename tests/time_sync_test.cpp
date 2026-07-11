// time_sync_test.cpp — Phase-2 item 6.
// Proves: with an injected clock offset beyond threshold, SIGNED (LIVE) ordering
// halts; within tolerance it proceeds; a risk-reducing exit is never blocked.
#include "live/ExecutionGateway.hpp"
#include <cstdio>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

struct MockExec {
    int calls=0;
    OrderResult execute(const std::string&, bool, double qty, double px, const std::string& = "") {
        ++calls; OrderResult r; r.ok=true; r.status="FILLED"; r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    // --- pure halt logic ---
    {
        ExchangeTimeSync c; c.set_threshold_ms(1000);
        CHECK(c.signed_trading_halted());        // never synced -> halt
        c.record_offset(200);  CHECK(!c.signed_trading_halted());
        c.record_offset(-200); CHECK(!c.signed_trading_halted());
        c.record_offset(1500); CHECK(c.signed_trading_halted());   // drift
        c.record_offset(-1500);CHECK(c.signed_trading_halted());
    }
    // --- gateway LIVE enforcement ---
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::LIVE);
        ExchangeTimeSync c; c.set_threshold_ms(1000); gw.set_clock(&c);

        // unsynced: entry halted
        auto e0 = gw.submit({ "BTCUSDT", true, 0.001, 40000.0, false, "T" });
        CHECK(!e0.ok && ex.calls==0);
        // in tolerance: entry proceeds
        c.record_offset(100);
        auto e1 = gw.submit({ "BTCUSDT", true, 0.001, 40000.0, false, "T" });
        CHECK(e1.ok && ex.calls==1);
        // drift: entry halted again, but a risk-reducing EXIT still passes
        c.record_offset(5000);
        auto e2 = gw.submit({ "BTCUSDT", true, 0.001, 40000.0, false, "T" });
        CHECK(!e2.ok && ex.calls==1);
        auto x  = gw.submit({ "BTCUSDT", false, 0.001, 40000.0, /*is_exit*/true, "T" });
        CHECK(x.ok && ex.calls==2);
    }
    std::printf(fails==0 ? "PASS: clock-drift halts signed entries, exits pass\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
