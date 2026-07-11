// execution_gateway_test.cpp — Phase-1 regression test for the ExecutionGateway.
// Proves: DISABLED never trades; kill-switch blocks ENTRIES but never EXITS;
// min-notional + qty sanity enforced; a valid entry forwards to the executor.
// Build (needs curl/openssl headers for OrderResult in BinanceREST.hpp):
//   g++ -std=c++20 -I../include -I<curl>/include -I<openssl>/include \
//       execution_gateway_test.cpp -o execution_gateway_test
#include "live/ExecutionGateway.hpp"
#include <cstdio>
using namespace chimera;

// Mock executor with a PUBLIC execute() — records the last forwarded order.
struct MockExec {
    int calls = 0;
    std::string last_sym; bool last_buy = false; double last_qty = 0, last_px = 0;
    OrderResult execute(const std::string& sym, bool is_buy, double qty, double px) {
        ++calls; last_sym = sym; last_buy = is_buy; last_qty = qty; last_px = px;
        OrderResult r; r.ok = true; r.shadow = true; r.status = "FILLED";
        r.executed_qty = qty; r.avg_price = px; return r;
    }
};

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    // 1. DISABLED mode: nothing reaches the executor.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::DISABLED);
        auto r = gw.submit({ "BTCUSDT", true, 1.0, 100.0, false, "TEST" });
        CHECK(!r.ok && ex.calls == 0);
    }
    // 2. SHADOW mode, kill-switch active: ENTRY blocked, EXIT still passes.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        bool halted = true;
        gw.kill_switch_active = [&]{ return halted; };
        auto entry = gw.submit({ "BTCUSDT", true, 1.0, 100.0, /*is_exit*/false, "TEST" });
        CHECK(!entry.ok && ex.calls == 0);                 // entry blocked by halt
        auto exit = gw.submit({ "BTCUSDT", false, 1.0, 100.0, /*is_exit*/true, "TEST" });
        CHECK(exit.ok && ex.calls == 1 && !ex.last_buy);   // risk-reducing exit NEVER blocked
    }
    // 3. min-notional + qty sanity.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        CHECK(!gw.submit({ "BTCUSDT", true, 0.0,  100.0, false, "T" }).ok); // qty<=0
        CHECK(!gw.submit({ "BTCUSDT", true, 1.0,  0.0,   false, "T" }).ok); // px<=0
        CHECK(!gw.submit({ "BTCUSDT", true, 0.001, 1.0,  false, "T" }).ok); // $0.001 < MIN_NOTIONAL
        CHECK(ex.calls == 0);
    }
    // 4. Valid entry forwards through to the executor.
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        auto r = gw.submit({ "ETHUSDT", true, 2.0, 50.0, false, "TEST" });
        CHECK(r.ok && ex.calls == 1 && ex.last_sym == "ETHUSDT"
              && ex.last_buy && ex.last_qty == 2.0 && ex.last_px == 50.0);
    }
    // 5. exposure hook rejects an entry (but not an exit).
    {
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        gw.exposure_ok = [](const OrderIntent&){ return false; };
        CHECK(!gw.submit({ "ETHUSDT", true, 2.0, 50.0, false, "T" }).ok && ex.calls == 0);
        CHECK(gw.submit({ "ETHUSDT", false, 2.0, 50.0, true, "T" }).ok && ex.calls == 1);
    }

    std::printf(fails == 0 ? "PASS: gateway mode/kill-switch/filters/exit-passthrough\n" : "FAILED (%d)\n", fails);
    return fails == 0 ? 0 : 1;
}
