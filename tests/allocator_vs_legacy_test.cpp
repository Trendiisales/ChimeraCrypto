// allocator_vs_legacy_test.cpp — permanent CI matrix item: ALLOCATOR-VS-LEGACY
// (diffs traceable).
//
// Phase-3 runs the SpotPortfolioAllocator TRACK-ONLY (Stage-1 shadow-compare):
// the legacy sleeves still trade (each fires its OWN raw target as a separate
// order), while the allocator merely RECORDS what it WOULD do (merge -> cap ->
// net). The go-live requirement is that the difference between the two is fully
// TRACEABLE — every dollar of the legacy-vs-allocator gap is reconstructable
// from the AllocDelta terms, and no allocator action leaks into the live books
// while enforce=false.
//
// This pins that reconciliation on EXISTING behaviour (no engine change):
//   legacy_total  = sum of the raw per-sleeve targets (what legacy fires as N orders)
//   d.merged_usd  = the allocator's pre-cap merged target      (== legacy_total)
//   cap_saving    = d.merged_usd - d.capped_usd                (double-count removed)
//   net_vs_held   = d.capped_usd - d.held_usd                  (delta vs exchange truth)
//   d.usd         = |net_vs_held|                              (the ONE netted order)
// so an operator can decompose legacy(N orders) -> allocator(1 order) exactly.
#include "live/SpotPortfolioAllocator.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
#define NEAR(a,b) (std::fabs((a)-(b)) < 1e-6)

int main() {
    // Three legacy momentum sleeves each independently want SOL (the overbooked
    // legacy path = 3 separate orders). Ledger already holds $1200 + $500 pending.
    ExchangeLedger L; L.configure(/*cash*/0.0, /*enforce*/false, /*fee*/0.0);
    { ExecReport r; r.client_id="seed"; r.symbol="SOLUSDT"; r.source="X"; r.is_buy=true;
      r.state=OrderState::FILLED; r.filled_qty=12.0; r.avg_price=100.0; r.fee=0.0; L.apply_report(r); }
    L.reserve_buy("pend","SOLUSDT","X",5.0,100.0);          // $500 pending

    struct Leg { const char* id; double usd; };
    Leg legs[] = { {"XSEC",4000.0}, {"UPJUMP",2000.0}, {"PULLBACK",1500.0} };
    double legacy_total = 0.0; int legacy_orders = 0;
    for (auto& g : legs) { legacy_total += g.usd; ++legacy_orders; }   // legacy fires 3 raw orders
    CHECK(NEAR(legacy_total, 7500.0) && legacy_orders == 3);

    SpotPortfolioAllocator A;
    A.configure(/*enforce*/false, /*symbol_cap*/5000.0, 0.0, 0.0, 0.0, 0.0);  // TRACK-ONLY
    A.ref_px = [](const std::string&){ return 100.0; };
    A.set_regime_overlay(false); A.set_drawdown_overlay(false);
    A.set_momentum_overlay(false); A.set_risk_overlay(false);
    int emitted = 0;
    A.emit = [&](const AllocDelta&, Factor, Family){ ++emitted; };   // must NEVER fire (track-only)
    A.set_target("XSEC",     "SOLUSDT", 4000.0, Factor::MOMENTUM, Family::XSEC);
    A.set_target("UPJUMP",   "SOLUSDT", 2000.0, Factor::MOMENTUM, Family::UPJUMP);
    A.set_target("PULLBACK", "SOLUSDT", 1500.0, Factor::OTHER,    Family::EDGE);

    auto deltas = A.plan(&L);

    // (1) allocator collapses the legacy N orders into ONE netted delta.
    CHECK(deltas.size() == 1);
    CHECK((int)A.num_targets() == legacy_orders);           // all 3 legacy sleeves accounted for
    const AllocDelta& d = deltas[0];

    // (2) the legacy-vs-allocator diff is fully TRACEABLE from AllocDelta terms.
    CHECK(NEAR(d.merged_usd, legacy_total));                // merged == exactly what legacy would fire
    double cap_saving  = d.merged_usd - d.capped_usd;       // double-count the symbol-cap removes
    double net_vs_held = d.capped_usd - d.held_usd;         // delta vs the exchange-truth ledger
    CHECK(NEAR(cap_saving, 2500.0));                        // 7500 merged -> 5000 capped
    CHECK(NEAR(d.held_usd, 1700.0));                        // existing 1200 + pending 500
    CHECK(NEAR(net_vs_held, 3300.0));                       // capped 5000 - held 1700
    CHECK(d.is_buy && NEAR(d.usd, 3300.0));                 // the ONE order the allocator would place
    CHECK(d.usd < legacy_total);                            // strictly less than the overbooked legacy sum
    std::printf("[trace] legacy=%d orders $%.0f  ->  allocator 1 order: "
                "merged=$%.0f cap_saving=$%.0f held=$%.0f net=$%.0f\n",
                legacy_orders, legacy_total, d.merged_usd, cap_saving, d.held_usd, net_vs_held);

    // (3) TRACK-ONLY: the allocator emitted NOTHING — legacy books/grid undisturbed.
    CHECK(emitted == 0);
    CHECK(!A.enforce());

    // (4) a second symbol nobody double-targets nets independently (per-symbol traceable).
    A.set_target("XSEC", "LINKUSDT", 800.0, Factor::MOMENTUM, Family::XSEC);
    auto d2 = A.plan(&L);
    CHECK(d2.size() == 2);                                   // SOL + LINK, each its own delta
    bool link_ok = false;
    for (auto& x : d2) if (x.symbol=="LINKUSDT")
        link_ok = NEAR(x.merged_usd,800.0) && NEAR(x.held_usd,0.0) && NEAR(x.usd,800.0) && x.is_buy;
    CHECK(link_ok);
    CHECK(emitted == 0);                                    // still track-only

    std::printf(fails==0
        ? "PASS: allocator-vs-legacy diff fully traceable; track-only emits nothing\n"
        : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
