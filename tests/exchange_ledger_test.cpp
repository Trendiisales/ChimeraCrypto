// exchange_ledger_test.cpp — Phase-2 items 1 & 3.
// Proves: holdings mutate ONLY from execution reports; partial fills + a cancel
// leave the EXACT position + fees; a rejected order leaves the ledger unchanged;
// distinct states are modelled.
#include "live/ExchangeLedger.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
static bool near(double a, double b, double e=1e-6){ return std::fabs(a-b) < e; }

int main() {
    // ---- partial fills + cancel ----
    {
        ExchangeLedger L; L.configure(/*cash*/100000.0, /*enforce*/true, /*fee*/0.001);
        std::string cid = "RIPBSOL-1";
        double adj = L.reserve_buy(cid, "SOLUSDT", "RIP", 10.0, 100.0);
        CHECK(near(adj, 10.0));                       // cash ample -> no resize
        CHECK(near(L.reserved_cash(), 10.0*100.0*1.001));

        ExecReport r; r.client_id = cid; r.symbol="SOLUSDT"; r.source="RIP"; r.is_buy=true;
        r.state = OrderState::PARTIAL; r.filled_qty=4.0; r.avg_price=100.0; L.apply_report(r);
        CHECK(near(L.position("SOLUSDT"), 4.0));
        r.state = OrderState::PARTIAL; r.filled_qty=3.0; r.avg_price=102.0; L.apply_report(r);
        CHECK(near(L.position("SOLUSDT"), 7.0));
        CHECK(near(L.avg_price("SOLUSDT"), (4*100.0+3*102.0)/7.0));
        // remaining 3 units cancelled -> position stays 7, reservation freed
        r.state = OrderState::CANCELLED; r.filled_qty=0; L.apply_report(r);
        CHECK(near(L.position("SOLUSDT"), 7.0));
        CHECK(near(L.reserved_cash(), 0.0));
        CHECK(near(L.fees_paid("SOLUSDT"), 4*100*0.001 + 3*102*0.001));
        CHECK(near(L.total_cash(), 100000.0 - (4*100+3*102) - (4*100*0.001+3*102*0.001)));
        CHECK(near(L.attributed_qty("RIP","SOLUSDT"), 7.0));
        CHECK(!L.has_pending(cid));                    // order closed out
    }
    // ---- rejected order leaves ledger unchanged ----
    {
        ExchangeLedger L; L.configure(50000.0, true, 0.001);
        double c0 = L.total_cash();
        std::string cid = "XBUYETH-9";
        double adj = L.reserve_buy(cid, "ETHUSDT", "X", 2.0, 2000.0);
        CHECK(near(adj, 2.0));
        CHECK(L.reserved_cash() > 0.0);
        ExecReport r; r.client_id=cid; r.symbol="ETHUSDT"; r.source="X"; r.is_buy=true;
        r.state = OrderState::REJECTED; L.apply_report(r);
        CHECK(near(L.position("ETHUSDT"), 0.0));
        CHECK(near(L.total_cash(), c0));               // cash restored
        CHECK(near(L.reserved_cash(), 0.0));
        CHECK(!L.has_pending(cid));
    }
    // ---- UNKNOWN never touches holdings (ambiguous) ----
    {
        ExchangeLedger L; L.configure(50000.0, true, 0.001);
        std::string cid = "XBUYBTC-3";
        L.reserve_buy(cid, "BTCUSDT", "X", 0.1, 40000.0);
        ExecReport r; r.client_id=cid; r.symbol="BTCUSDT"; r.source="X"; r.is_buy=true;
        r.state = OrderState::UNKNOWN; L.apply_report(r);
        CHECK(near(L.position("BTCUSDT"), 0.0));
        CHECK(L.has_pending(cid));                      // stays pending for recovery
        CHECK(L.pending_state(cid) == OrderState::UNKNOWN);
    }
    std::printf(fails==0 ? "PASS: ledger report-driven / partial+cancel / rejected / unknown\n" : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
