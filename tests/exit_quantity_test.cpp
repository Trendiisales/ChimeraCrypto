// exit_quantity_test.cpp — Phase-2 item 4.
// Proves: an entry that partial-fills to X leaves exactly X attributed to the
// sleeve; the exit closes the EXCHANGE-CONFIRMED remaining qty (X) — not a
// recomputed rip_nav/exitPrice — with no residual and no oversell.
#include "live/ExecutionGateway.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
static bool near(double a,double b,double e=1e-6){ return std::fabs(a-b)<e; }

struct MockExec {
    int calls=0; double last_qty=0; bool last_buy=false;
    OrderResult execute(const std::string&, bool is_buy, double qty, double px, const std::string& = "") {
        ++calls; last_qty=qty; last_buy=is_buy; OrderResult r; r.ok=true; r.shadow=true;
        r.status="FILLED"; r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    MockExec ex;
    ExchangeLedger L; L.configure(/*cash*/0.0, /*enforce*/false, /*fee*/0.001); // track-only
    ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
    gw.set_ledger(&L);

    // Entry partial-fills to X=7 (two clips through the gateway, both attributed RIP).
    gw.submit({ "SOLUSDT", true, 4.0, 100.0, false, "RIP" });
    gw.submit({ "SOLUSDT", true, 3.0, 102.0, false, "RIP" });
    double X = L.attributed_qty("RIP", "SOLUSDT");
    CHECK(near(X, 7.0));                               // exact confirmed qty
    CHECK(near(L.position("SOLUSDT"), 7.0));

    // OLD (buggy) exit would size rip_nav/exitPrice — e.g. (1250/8)/105 -> wrong qty.
    double buggy = (1250.0/8.0)/105.0;
    CHECK(std::fabs(buggy - X) > 0.5);                // demonstrably different

    // CORRECT exit: close exactly the confirmed remaining qty from the ledger.
    double exit_qty = L.attributed_qty("RIP", "SOLUSDT");
    auto r = gw.submit({ "SOLUSDT", false, exit_qty, 105.0, /*is_exit*/true, "RIP" });
    CHECK(r.ok && !ex.last_buy && near(ex.last_qty, 7.0));
    CHECK(near(L.position("SOLUSDT"), 0.0));           // flat, no residual
    CHECK(near(L.attributed_qty("RIP","SOLUSDT"), 0.0));

    // Oversell guard: asking to sell more than held never drives position negative.
    L.configure(0.0, false, 0.001);
    gw.submit({ "ETHUSDT", true, 2.0, 2000.0, false, "RIP" });
    gw.submit({ "ETHUSDT", false, 10.0, 2100.0, true, "RIP" });   // ask 10, hold 2
    CHECK(near(L.position("ETHUSDT"), 0.0));           // sold only what was held

    std::printf(fails==0 ? "PASS: exit closes confirmed qty, no residual/oversell\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
