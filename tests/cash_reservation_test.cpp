// cash_reservation_test.cpp — Phase-2 item 2.
// Proves: with a limited cash budget, 3 buy intents that TOGETHER exceed cash are
// full / resized / rejected respectively — the book is never overbooked.
#include "live/ExecutionGateway.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

struct MockExec {
    int calls=0; double last_qty=0;
    OrderResult execute(const std::string&, bool, double qty, double px, const std::string& = "") {
        ++calls; last_qty=qty; OrderResult r; r.ok=true; r.shadow=true; r.status="FILLED";
        r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    MockExec ex;
    ExchangeLedger L; L.configure(/*cash*/1000.0, /*enforce*/true, /*fee*/0.001);
    ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
    gw.set_ledger(&L);

    // Each intent wants 6 @ $100 = $600 (+fee). Budget is $1000.
    auto r1 = gw.submit({ "SOLUSDT", true, 6.0, 100.0, false, "A" });
    CHECK(r1.ok && std::fabs(ex.last_qty - 6.0) < 1e-9);      // 1st: full

    auto r2 = gw.submit({ "SOLUSDT", true, 6.0, 100.0, false, "B" });
    CHECK(r2.ok && ex.last_qty > 0.0 && ex.last_qty < 6.0);   // 2nd: RESIZED to fit

    auto r3 = gw.submit({ "SOLUSDT", true, 6.0, 100.0, false, "C" });
    CHECK(!r3.ok);                                            // 3rd: REJECTED (no cash)
    CHECK(ex.calls == 2);                                     // only 2 orders reached the exchange

    // Never overbooked: total spent <= budget, cash never negative.
    CHECK(L.total_cash() >= -1e-6);
    CHECK(L.total_cash() < 1000.0);                           // some was spent
    std::printf("[info] final cash=%.4f reserved=%.4f pos=%.6f\n",
                L.total_cash(), L.reserved_cash(), L.position("SOLUSDT"));

    std::printf(fails==0 ? "PASS: cash reservation full/resize/reject, no overbook\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
