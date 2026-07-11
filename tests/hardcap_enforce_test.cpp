// hardcap_enforce_test.cpp — Phase-8A Stage-2 (SpotPortfolioAllocator::govern_entry).
//
// Stage-2 = HARD SAFETY CAPS, not full sizing authority. Engines still PROPOSE
// their own quantity; the allocator can only REDUCE (to the remaining headroom)
// or REJECT (no headroom) a BUY that GENUINELY breaches a hard cap. An in-limit
// order MUST return byte-identical to what the sleeve proposed — the promotion
// requirement is "no erroneous rejections". This suite proves:
//   (A) symbol-cap:   in-limit -> unchanged; over -> reduced; full -> rejected
//   (B) momentum-cap: aggregate momentum exposure reduces/rejects; symbol cap ok
//   (C) drawdown-ladder: dd scale tightens both caps; HALT rejects all entries
//   (D) KEY no-erroneous-rejection: a whole in-limit order STREAM through mode
//       OFF (track-only) vs HARDCAP submits byte-identical qty for every order.
//   (E) mode gating: OFF and FULL never touch a proposed order via govern_entry.
#include "live/SpotPortfolioAllocator.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)
#define NEAR(a,b) (std::fabs((a)-(b)) < 1e-6)

// helper: seed a FILLED position into a ledger (held exposure the caps see).
static void seed_hold(ExchangeLedger& L, const char* sym, double qty, double px) {
    ExecReport r; r.client_id=std::string("seed-")+sym; r.symbol=sym; r.source="X";
    r.is_buy=true; r.state=OrderState::FILLED; r.filled_qty=qty; r.avg_price=px; r.fee=0.0;
    L.apply_report(r);
}

int main() {
    // ── (A) SYMBOL-CAP enforcement ──────────────────────────────────────────
    {
        SpotPortfolioAllocator A;
        A.configure(/*enforce*/false, /*symbol_cap*/10000.0, /*momentum_cap*/0.0,
                    0.0, 0.0, 0.0);
        A.set_enforce_mode(EnforceMode::HARDCAP);
        A.set_drawdown_overlay(false);              // dd=1.0 (isolate symbol cap)
        A.ref_px = [](const std::string&){ return 100.0; };
        ExchangeLedger L; L.configure(0.0,false,0.0);
        seed_hold(L, "SOLUSDT", 40.0, 100.0);       // $4000 already held

        // in-limit: held 4000 + order 3000 = 7000 <= 10000 -> UNCHANGED
        auto d1 = A.govern_entry("SOLUSDT", 30.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(d1.approved && !d1.reduced);
        CHECK(NEAR(d1.approved_qty, 30.0));
        CHECK(NEAR(d1.approved_usd, 3000.0));

        // over-cap: held 4000 + order 8000 = 12000 > 10000 -> REDUCE to $6000 headroom
        auto d2 = A.govern_entry("SOLUSDT", 80.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(d2.approved && d2.reduced);
        CHECK(NEAR(d2.approved_usd, 6000.0));       // 10000 - 4000 held
        CHECK(NEAR(d2.approved_qty, 60.0));
        CHECK(std::string(d2.reason) == "symbol-cap");

        // already at/over cap: seed to full, any BUY -> REJECT
        seed_hold(L, "SOLUSDT", 60.0, 100.0);       // now $10000 held
        auto d3 = A.govern_entry("SOLUSDT", 5.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(!d3.approved);
        CHECK(NEAR(d3.approved_usd, 0.0));

        // a DIFFERENT symbol with room is unaffected -> UNCHANGED
        auto d4 = A.govern_entry("ETHUSDT", 20.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(d4.approved && !d4.reduced && NEAR(d4.approved_qty, 20.0));
    }

    // ── (B) MOMENTUM-FACTOR aggregate cap ────────────────────────────────────
    {
        SpotPortfolioAllocator A;
        A.configure(false, /*symbol_cap*/100000.0, /*momentum_cap*/10000.0, 0.0,0.0,0.0);
        A.set_enforce_mode(EnforceMode::HARDCAP);
        A.set_drawdown_overlay(false);
        A.ref_px = [](const std::string&){ return 100.0; };
        ExchangeLedger L; L.configure(0.0,false,0.0);
        // two momentum names held: SOL $4000 + LINK $3000 = $7000 aggregate momentum.
        seed_hold(L, "SOLUSDT",  40.0, 100.0);
        seed_hold(L, "LINKUSDT", 30.0, 100.0);
        // the allocator must know these are momentum symbols -> declare targets.
        A.set_target("XSEC","SOLUSDT",  4000.0, Factor::MOMENTUM, Family::XSEC);
        A.set_target("RIP", "LINKUSDT", 3000.0, Factor::MOMENTUM, Family::RIPRIDER);

        // in-limit momentum add: 7000 + 2000 = 9000 <= 10000 -> UNCHANGED
        auto b1 = A.govern_entry("AVAXUSDT", 20.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(b1.approved && !b1.reduced && NEAR(b1.approved_usd, 2000.0));

        // over the momentum cap: 7000 + 6000 = 13000 -> REDUCE to $3000 headroom
        auto b2 = A.govern_entry("AVAXUSDT", 60.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(b2.approved && b2.reduced);
        CHECK(NEAR(b2.approved_usd, 3000.0));       // 10000 - 7000 momentum held
        CHECK(std::string(b2.reason) == "momentum-cap");

        // a NON-momentum (OTHER) order is NOT subject to the momentum cap (symbol
        // cap is huge here) -> UNCHANGED even though momentum book is near cap.
        auto b3 = A.govern_entry("AVAXUSDT", 60.0, 100.0, Factor::OTHER, &L);
        CHECK(b3.approved && !b3.reduced && NEAR(b3.approved_qty, 60.0));
    }

    // ── (C) DRAWDOWN ladder tightens the caps; HALT rejects ──────────────────
    {
        SpotPortfolioAllocator A;
        A.configure(false, /*symbol_cap*/10000.0, /*momentum_cap*/0.0, 0.0,0.0,0.0);
        A.set_enforce_mode(EnforceMode::HARDCAP);   // dd overlay ON (default)
        A.ref_px = [](const std::string&){ return 100.0; };
        ExchangeLedger L; L.configure(0.0,false,0.0);   // nothing held

        // dd=1.0 (no drawdown): $9000 order under $10000 cap -> UNCHANGED
        A.drawdown().update_equity(100000.0);
        auto c1 = A.govern_entry("BTCUSDT", 90.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(c1.approved && !c1.reduced && NEAR(c1.approved_usd, 9000.0));

        // draw down 12% -> exposure_scale 0.50 -> effective symbol cap $5000.
        A.drawdown().update_equity(88000.0);
        CHECK(NEAR(A.drawdown().exposure_scale(), 0.50));
        auto c2 = A.govern_entry("BTCUSDT", 90.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(c2.approved && c2.reduced && NEAR(c2.approved_usd, 5000.0));

        // HALT (>=20% dd) -> exposure_scale 0 -> ALL entries rejected.
        A.drawdown().update_equity(75000.0);
        CHECK(A.drawdown().halted());
        auto c3 = A.govern_entry("BTCUSDT", 1.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(!c3.approved);
        CHECK(std::string(c3.reason) == "drawdown-halt" || std::string(c3.reason) == "symbol-cap");
    }

    // ── (D) KEY: NO ERRONEOUS REJECTION — in-limit STREAM byte-identical ─────
    // Replay a realistic promoted-sleeve order stream (all well within caps)
    // through OFF (track-only) and HARDCAP and prove every submitted qty matches.
    {
        struct Ord { const char* sym; double qty; double px; };
        std::vector<Ord> stream = {
            {"BTCUSDT", 0.05, 60000.0}, {"ETHUSDT", 1.20, 3000.0},
            {"SOLUSDT", 20.0, 100.0},   {"LINKUSDT", 100.0, 15.0},
            {"AVAXUSDT", 50.0, 30.0},   {"BTCUSDT", 0.02, 60000.0},
            {"XRPUSDT", 2000.0, 0.60},  {"DOGEUSDT", 10000.0, 0.12},
        };
        auto run = [&](EnforceMode m){
            SpotPortfolioAllocator A;
            // production-shape caps: symbol $10k, momentum $40k (never breached by
            // this small stream). dd healthy.
            A.configure(false, 10000.0, 40000.0, 0.0, 0.50, 0.0);
            A.set_enforce_mode(m);
            A.ref_px = [](const std::string&){ return 0.0; };  // px comes from the order
            A.drawdown().update_equity(100000.0);
            ExchangeLedger L; L.configure(0.0,false,0.0);
            std::vector<double> qtys;
            for (auto& o : stream) {
                // small orders (all << $10k symbol cap, aggregate << $40k momentum)
                double usd = o.qty * o.px;
                if (usd > 3000.0) { o.qty = 3000.0 / o.px; }   // keep the stream in-limit
                auto d = A.govern_entry(o.sym, o.qty, o.px, Factor::MOMENTUM, &L);
                CHECK(d.approved);            // NEVER rejected on an in-limit stream
                CHECK(!d.reduced);            // NEVER reduced on an in-limit stream
                qtys.push_back(d.approved_qty);
                // simulate the shadow fill so held exposure accumulates (still in-limit)
                seed_hold(L, o.sym, d.approved_qty, o.px);
                A.set_target("S", o.sym, d.approved_usd, Factor::MOMENTUM, Family::XSEC);
            }
            return qtys;
        };
        auto off  = run(EnforceMode::OFF);
        auto hard = run(EnforceMode::HARDCAP);
        CHECK(off.size() == hard.size());
        bool identical = true;
        for (size_t i = 0; i < off.size(); ++i) if (!NEAR(off[i], hard[i])) identical = false;
        CHECK(identical);   // <<< the no-erroneous-rejection PROOF: OFF == HARDCAP
        std::printf("[info] no-erroneous-rejection stream: %zu orders, OFF==HARDCAP=%d\n",
                    off.size(), identical?1:0);
    }

    // ── (E) mode gating: OFF and FULL never touch a proposed order ───────────
    {
        SpotPortfolioAllocator A;
        A.configure(false, /*symbol_cap*/10000.0, 0.0, 0.0,0.0,0.0);
        A.set_drawdown_overlay(false);
        A.ref_px = [](const std::string&){ return 100.0; };
        ExchangeLedger L; L.configure(0.0,false,0.0);
        seed_hold(L, "SOLUSDT", 200.0, 100.0);      // $20000 held — WAY over the $10k cap

        A.set_enforce_mode(EnforceMode::OFF);       // track-only: no governance
        auto e1 = A.govern_entry("SOLUSDT", 50.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(e1.approved && !e1.reduced && NEAR(e1.approved_qty, 50.0));

        A.set_enforce_mode(EnforceMode::FULL);      // full: plan() emits; govern_entry inert
        auto e2 = A.govern_entry("SOLUSDT", 50.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(e2.approved && !e2.reduced && NEAR(e2.approved_qty, 50.0));

        A.set_enforce_mode(EnforceMode::HARDCAP);   // only HARDCAP bites -> REJECT (over cap)
        auto e3 = A.govern_entry("SOLUSDT", 50.0, 100.0, Factor::MOMENTUM, &L);
        CHECK(!e3.approved);
    }

    std::printf(fails==0 ? "PASS: hardcap enforce — symbol/momentum/DD caps + no-erroneous-rejection\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
