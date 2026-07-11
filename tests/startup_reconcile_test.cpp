// startup_reconcile_test.cpp — Phase-2 item 9.
// Proves: on restart, an open order found on the exchange is ADOPTED into the
// ledger + id-registry so the strategy does NOT resubmit it (no duplicate);
// a position mismatch BLOCKS trading; a clean match passes.
#include "live/StartupReconciler.hpp"
#include "live/ExecutionGateway.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

struct MockExec {
    int calls=0;
    OrderResult execute(const std::string&, bool, double qty, double px, const std::string& = "") {
        ++calls; OrderResult r; r.ok=true; r.shadow=true; r.status="FILLED";
        r.executed_qty=qty; r.avg_price=px; return r;
    }
};

int main() {
    // --- restart with a working order: adopt, then prove no duplicate submit ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        OrderIdRegistry reg;
        StartupReconciler R;
        // The order the (now-restarted) RIP sleeve had already placed. Its id is
        // the DETERMINISTIC id RIP would regenerate for the same signal.
        std::string cid = OrderIdRegistry::make_client_id("RIP", 777, "SOLUSDT", true);
        ExchangeSnapshot snap; snap.ok = true;
        snap.open_orders.push_back({ cid, "SOLUSDT", "RIP", true, 5.0, 100.0 });
        // balances: none held yet (order still working) -> matches empty ledger
        auto res = R.reconcile(snap, L, &reg, /*now*/1000);
        CHECK(res.passed && res.adopted_orders == 1);
        CHECK(L.has_pending(cid) && reg.in_flight(cid));

        // The strategy re-fires the SAME signal after restart. Gateway must NOT
        // submit a duplicate — the id is already in flight -> recover first.
        MockExec ex; ExecutionGatewayT<MockExec> gw(ex, RuntimeMode::SHADOW);
        gw.set_ledger(&L); gw.set_id_registry(&reg);
        auto r = gw.submit({ "SOLUSDT", true, 5.0, 100.0, false, "RIP", /*signal_id*/777 });
        CHECK(!r.ok && r.error == "recover-first");
        CHECK(ex.calls == 0);                          // NO duplicate order
    }
    // --- position mismatch blocks trading ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        StartupReconciler R;
        ExchangeSnapshot snap; snap.ok = true;
        snap.base_balances["BTCUSDT"] = 1.5;           // exchange says 1.5 held
        // ledger thinks 0 -> mismatch -> must block
        auto res = R.reconcile(snap, L);
        CHECK(!res.passed && res.position_mismatches == 1);
    }
    // --- failed snapshot fetch blocks trading ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        StartupReconciler R; ExchangeSnapshot snap; snap.ok = false;
        CHECK(!R.reconcile(snap, L).passed);
    }
    // --- clean match passes ---
    {
        ExchangeLedger L; L.configure(100000.0, true, 0.001);
        // ledger already holds 0.5 BTC; exchange agrees
        ExecReport r; r.client_id="x"; r.symbol="BTCUSDT"; r.source="RIP"; r.is_buy=true;
        r.state=OrderState::FILLED; r.filled_qty=0.5; r.avg_price=40000.0; L.apply_report(r);
        StartupReconciler R; ExchangeSnapshot snap; snap.ok=true;
        snap.base_balances["BTCUSDT"] = 0.5;
        CHECK(R.reconcile(snap, L).passed);
    }
    std::printf(fails==0 ? "PASS: restart adopt (no dup) / mismatch block / fetch-fail block / match pass\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
