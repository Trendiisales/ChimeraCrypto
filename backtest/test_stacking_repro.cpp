// S54 stacking reproduction + fix validation.
//
// THE ISSUE (live, 30-May): two engines on the SAME symbol both entered on the
// SAME bar (XLM TSMOM-H4 + ICHI-H4), then one -196.5bp adverse move hit BOTH ->
// effective ~-393bp single-name loss in one bar. The per-trade -170 hard floor
// caps each engine, but NOTHING capped the correlated SUM. Root cause:
// CLUSTER_MAX_PER_SYMBOL=2 permitted two simultaneous longs on one coin, which
// on a long-only book is pure 2x leverage + double fees, zero diversification.
//
// This test mirrors main.cpp's per-symbol concurrency gate (sequential counter,
// see main.cpp tick_with_cluster_gate ~6863) over a shared adverse price path,
// and reports the COMBINED realized single-name loss under cap=2 (the bug) vs
// cap=1 (the S54 fix). The single-engine backtest harness cannot reproduce this
// because the pathology is a PORTFOLIO interaction across engines.
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <vector>
#include <string>

using chimera::EdgeEngine;

static EdgeEngine::Config mk_cfg(const char* tag, const char* sym) {
    EdgeEngine::Config c;
    c.tag = tag; c.symbol = sym;
    c.kind = chimera::StrategyKind::TSMOM;
    c.tf_secs = 60; c.lookback = 3; c.hold_bars = 200;
    c.atr_period = 3; c.max_history = 64; c.round_trip_bp = 20.0;
    c.sl_atr_mult = 2.5;
    return c;
}

// Drive N engines on ONE symbol through a shared price path, enforcing the
// per-symbol concurrency cap exactly as main.cpp does: gate set from the live
// open-count BEFORE each tick, counter updated AFTER. Returns combined net bp.
static double run_one_symbol(int cap, double hard_floor_bp) {
    std::vector<EdgeEngine> engs;
    engs.emplace_back(mk_cfg("STACK-A", "xlmusdt"));
    engs.emplace_back(mk_cfg("STACK-B", "xlmusdt"));
    for (auto& e : engs) {
        e.apply_safety_preset();
        e.set_hard_floor_bp(hard_floor_bp);
        e.set_realistic_gap_fill(false);   // clean floor fill (= live dense-tick case)
    }

    int open_sym = 0;                 // mirrors g_cluster_open_sym[sid]
    double combined_before = 0.0;
    for (auto& e : engs) combined_before += e.total_bp();

    // Shared path: a short ramp UP to ARM+ENTER both TSMOM engines on the same
    // bar, then an IMMEDIATE crash before MFE arms the ratchet -> both ride to
    // the -170 hard floor (dead-on-arrival, the live failure mode). No extended
    // run-up, so the staged-ratchet never locks a profit.
    // Ramp ends exactly at the entry trigger; the crash starts on the very next
    // tick so price never reaches entry+trail_arm -> MFE stays ~0, the trail
    // never arms, and both engines ride straight to the stop (dead-on-arrival).
    std::vector<double> path;
    for (int i = 0; i < 4; ++i) path.push_back(100.0 + i * 0.6);    // ramp 100->101.8 (triggers entry, no arm)
    for (int i = 1; i <= 16; ++i) path.push_back(101.8 - i * 1.3);  // immediate crash past the floor

    int64_t t = 0;
    for (double px : path) {
        t += 60 * 1000;
        for (auto& e : engs) {
            bool was_in = e.in_position();
            if (!was_in) e.set_cluster_gate(open_sym < cap);   // per-symbol cap
            else         e.set_cluster_gate(true);
            e.on_tick(px, t);
            bool now_in = e.in_position();
            if (!was_in && now_in)      open_sym++;
            else if (was_in && !now_in) { if (open_sym > 0) open_sym--; }
        }
    }
    // Force-flatten any still-open (capture realized loss in total_bp).
    for (auto& e : engs) e.kill_all(95.0, t + 60000);

    double combined_after = 0.0;
    int n_in = 0;
    for (auto& e : engs) { combined_after += e.total_bp(); }
    (void)combined_before; (void)n_in;
    return combined_after;
}

int main() {
    int failures = 0;
    printf("=== S54 STACKING REPRODUCTION + FIX VALIDATION ===\n\n");

    // REPRODUCE the bug: cap=2 lets BOTH engines stack on xlmusdt.
    double loss_cap2 = run_one_symbol(/*cap=*/2, /*floor=*/-170.0);
    printf("[REPRO] cap=2 (the bug): combined single-name net = %+.1fbp\n", loss_cap2);

    // VALIDATE the fix: cap=1 admits only ONE engine -> ~half the single-name loss.
    double loss_cap1 = run_one_symbol(/*cap=*/1, /*floor=*/-170.0);
    printf("[FIX  ] cap=1 (S54):     combined single-name net = %+.1fbp\n", loss_cap1);

    printf("\n");
    // Assertion 1: cap=2 actually stacked (combined loss materially worse than a
    // single floored trade, i.e. < ~-250bp -> proves 2 engines took the hit).
    if (loss_cap2 < -250.0) {
        printf("[PASS] bug reproduced: cap=2 combined loss %.1fbp < -250 (two engines stacked)\n", loss_cap2);
    } else {
        printf("[FAIL] cap=2 did NOT stack (%.1fbp) — repro broken\n", loss_cap2); failures++;
    }
    // Assertion 2: cap=1 roughly halves the single-name loss vs cap=2.
    if (loss_cap1 > loss_cap2 * 0.60) {
        printf("[PASS] fix validated: cap=1 loss %.1fbp is <=60%% of cap=2 loss %.1fbp\n",
               loss_cap1, loss_cap2);
    } else {
        printf("[FAIL] cap=1 did not reduce stacking loss enough (%.1f vs %.1f)\n",
               loss_cap1, loss_cap2); failures++;
    }

    printf("\n%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
