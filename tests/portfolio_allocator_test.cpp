// portfolio_allocator_test.cpp — Phase-3 item 15 (SpotPortfolioAllocator).
// Proves the R2 worked example EXACTLY: XSec SOL $4000 + Mimic $2000 + pullback
// $1500, cap $5000, existing $1200, pending $500 -> final BUY $3300. Plus a
// portfolio-collision test (3 strategies -> ONE merged, capped order) and that
// the 32-cell GRID (strategies that never register a target) is untouched.
//
// Item-15 is the pure MERGE/CAP/NET layer; the exposure overlays (items 16-19)
// are toggled OFF here so the netting arithmetic is isolated (each overlay has its
// own dedicated regression test).
#include "live/SpotPortfolioAllocator.hpp"
#include <cstdio>
#include <cmath>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
#define NEAR(a,b) (std::fabs((a)-(b)) < 1e-6)

int main() {
    // ── R2 worked example ────────────────────────────────────────────────────
    // Ledger holds SOL: existing $1200 (12 @ $100) + a pending BUY of $500 (5 @ $100).
    ExchangeLedger L; L.configure(/*cash*/0.0, /*enforce*/false, /*fee*/0.0);  // track-only ledger
    {   // existing position: a filled buy of 12 SOL @ 100
        ExecReport r; r.client_id="seed"; r.symbol="SOLUSDT"; r.source="X"; r.is_buy=true;
        r.state=OrderState::FILLED; r.filled_qty=12.0; r.avg_price=100.0; r.fee=0.0;
        L.apply_report(r);
    }
    L.reserve_buy("pend","SOLUSDT","X",5.0,100.0);    // pending buy $500 in-flight

    CHECK(NEAR(L.position_value("SOLUSDT",100.0), 1200.0));   // existing
    CHECK(NEAR(L.pending_buy_value("SOLUSDT"), 500.0));       // pending

    SpotPortfolioAllocator A;
    A.configure(/*enforce*/false, /*symbol_cap*/5000.0, /*momentum_cap*/0.0,
                /*target_vol*/0.0, /*cluster_frac*/0.0, /*beta*/0.0);
    A.ref_px = [](const std::string&){ return 100.0; };
    // isolate the netting layer: overlays OFF (family mult = dd = risk = 1.0).
    A.set_regime_overlay(false); A.set_drawdown_overlay(false);
    A.set_momentum_overlay(false); A.set_risk_overlay(false);

    A.set_target("XSEC",     "SOLUSDT", 4000.0, Factor::MOMENTUM, Family::XSEC);
    A.set_target("MIMIC",   "SOLUSDT", 2000.0, Factor::MOMENTUM, Family::MIMIC);
    A.set_target("PULLBACK", "SOLUSDT", 1500.0, Factor::OTHER,    Family::EDGE);

    auto deltas = A.plan(&L);
    CHECK(deltas.size() == 1);
    const AllocDelta& d = deltas[0];
    std::printf("[info] merged=$%.2f capped=$%.2f held=$%.2f -> %s $%.2f\n",
                d.merged_usd, d.capped_usd, d.held_usd, d.is_buy?"BUY":"SELL", d.usd);
    CHECK(NEAR(d.merged_usd, 7500.0));   // 4000+2000+1500
    CHECK(NEAR(d.capped_usd, 5000.0));   // ONE symbol cap after merge
    CHECK(NEAR(d.held_usd, 1700.0));     // existing 1200 + pending 500
    CHECK(d.is_buy);
    CHECK(NEAR(d.usd, 3300.0));          // final BUY $3300  <<< the R2 example
    CHECK(NEAR(d.qty, 33.0));            // $3300 / $100

    // ── portfolio-collision: 3 strategies same coin -> ONE merged capped order ──
    SpotPortfolioAllocator B;
    B.configure(false, 5000.0, 0.0, 0.0, 0.0, 0.0);
    B.ref_px = [](const std::string&){ return 50.0; };
    B.set_regime_overlay(false); B.set_drawdown_overlay(false);
    B.set_momentum_overlay(false); B.set_risk_overlay(false);
    B.set_target("A","LINKUSDT", 3000.0, Factor::MOMENTUM, Family::XSEC);
    B.set_target("B","LINKUSDT", 3000.0, Factor::MOMENTUM, Family::MIMIC);
    B.set_target("C","LINKUSDT", 3000.0, Factor::MOMENTUM, Family::EDGE);
    ExchangeLedger L2; L2.configure(0.0,false,0.0);
    auto d2 = B.plan(&L2);
    CHECK(d2.size() == 1);                // ONE order, not three
    CHECK(NEAR(d2[0].merged_usd, 9000.0));
    CHECK(NEAR(d2[0].capped_usd, 5000.0));  // capped once after merge
    CHECK(NEAR(d2[0].usd, 5000.0));         // nothing held -> buy the whole capped target

    // ── GRID preservation: cells that never register a target produce NO delta ──
    // (the 32-cell Mimic threshold grid keeps its own book; it never calls
    //  set_target, so the allocator emits nothing for those symbols.)
    CHECK(B.num_targets() == 3);          // 3 strategy targets, all on LINKUSDT
    bool any_non_link = false;            // no delta for any coin nobody targeted
    for (auto& x : d2) if (x.symbol != "LINKUSDT") any_non_link = true;
    CHECK(!any_non_link);
    // an untargeted grid symbol yields zero deltas from the allocator.
    B.clear_target("A","LINKUSDT"); B.clear_target("B","LINKUSDT"); B.clear_target("C","LINKUSDT");
    CHECK(B.num_targets() == 0);
    CHECK(B.plan(&L2).empty());           // no targets registered -> allocator emits nothing

    // ── a target that shrinks below the held book -> a SELL delta (rebalance out) ─
    SpotPortfolioAllocator C;
    C.configure(false, 5000.0, 0.0, 0.0, 0.0, 0.0);
    C.ref_px = [](const std::string&){ return 100.0; };
    C.set_regime_overlay(false); C.set_drawdown_overlay(false);
    C.set_momentum_overlay(false); C.set_risk_overlay(false);
    ExchangeLedger L3; L3.configure(0.0,false,0.0);
    { ExecReport r; r.client_id="s"; r.symbol="ETHUSDT"; r.source="X"; r.is_buy=true;
      r.state=OrderState::FILLED; r.filled_qty=40.0; r.avg_price=100.0; L3.apply_report(r); }  // hold $4000
    C.set_target("XSEC","ETHUSDT", 1000.0, Factor::MOMENTUM, Family::XSEC);   // want only $1000
    auto d3 = C.plan(&L3);
    CHECK(d3.size()==1 && !d3[0].is_buy && NEAR(d3[0].usd, 3000.0));          // SELL $3000

    std::printf(fails==0 ? "PASS: allocator merge/cap/net — R2 $3300 example exact\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
