// S54 low-MFE protection regression.
//
// THE GAP (live 31-May): the staged ratchet only engages at mfe >= ratchet_start
// (= round-trip cost ~22bp). A trade that popped +8..+22bp then reversed had ZERO
// stop-tightening and rode to the -170 hard floor. Real losers that day:
//   XLM-TSMOM-H4  mfe +19.8 -> -196.5     DOT-TSMOM-H3 mfe +8.3 -> -80.2
//   XLM-ICHI-H4   mfe +19.8 -> -196.5     ENA-TSMOM-H3 mfe +22.5 -> -66.9
// all peaked BELOW the +22 ratchet gate -> unprotected.
//
// FIX: ratchet_start 22->8, be_arm 32->15. The Stage-2 ramp now rescues these
// "almost made it" trades. This test drives ONE engine through a +15bp-pop-then-
// reverse path under OLD (22/32) vs NEW (8/15) config and asserts NEW caps the
// loss far tighter while OLD rides toward the floor.
#include "core/EdgeEngine.hpp"
#include <cstdio>
#include <vector>

using chimera::EdgeEngine;

static EdgeEngine::Config mk_cfg() {
    EdgeEngine::Config c;
    c.tag = "PROT"; c.symbol = "xlmusdt";
    c.kind = chimera::StrategyKind::TSMOM;
    c.tf_secs = 60; c.lookback = 3; c.hold_bars = 200;
    c.atr_period = 3; c.max_history = 64; c.round_trip_bp = 20.0;
    c.sl_atr_mult = 2.5;
    return c;
}

// Run the shared pop-then-reverse path under a given ratchet config; return net bp.
static double run(double ratchet_start, double be_arm) {
    EdgeEngine e(mk_cfg());
    e.apply_safety_preset();
    e.set_hard_floor_bp(-170.0);
    e.set_realistic_gap_fill(false);
    e.set_ratchet_start_bp(ratchet_start);
    e.set_be_arm_bp(be_arm);

    std::vector<double> path;
    for (int i = 0; i < 4; ++i) path.push_back(100.0 + i * 0.6);   // ramp 100->101.8 (entry)
    path.push_back(101.95);                                         // pop to ~+15bp MFE
    path.push_back(101.92);
    for (int i = 1; i <= 16; ++i) path.push_back(101.8 - i * 1.3);  // reverse, crash past floor

    int64_t t = 0;
    for (double px : path) { t += 60000; e.on_tick(px, t); }
    e.kill_all(95.0, t + 60000);
    return e.total_bp();
}

int main() {
    printf("=== S54 LOW-MFE PROTECTION REGRESSION ===\n\n");
    double old_cfg = run(22.0, 32.0);   // live pre-S54
    double new_cfg = run(8.0, 15.0);    // S54 fix
    printf("[OLD 22/32] +15bp-pop-then-reverse net = %+.1fbp (rode unprotected)\n", old_cfg);
    printf("[NEW  8/15] +15bp-pop-then-reverse net = %+.1fbp (ratchet engaged)\n", new_cfg);

    int failures = 0;
    printf("\n");
    if (old_cfg < -100.0)
        printf("[PASS] gap reproduced: OLD config rode to %.1fbp (< -100, near floor)\n", old_cfg);
    else { printf("[FAIL] OLD config didn't ride down (%.1fbp) — repro broken\n", old_cfg); failures++; }

    if (new_cfg > old_cfg + 100.0)
        printf("[PASS] fix works: NEW config %.1fbp is >100bp better than OLD %.1fbp\n", new_cfg, old_cfg);
    else { printf("[FAIL] NEW config didn't protect (%.1f vs %.1f)\n", new_cfg, old_cfg); failures++; }

    printf("\n%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
