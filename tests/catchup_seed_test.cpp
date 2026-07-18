// catchup_seed_test.cpp — unit test for the S-2026-07-18 BOUNDED CATCH-UP in
// UpJumpLadderCompanion::seed_det_ring_hist (Config::catchup_max_age_bars).
// Behavior cert lives in Crypto/backtest/catchup_outage_bt.cpp (surgical equivalence
// + grid); THIS test pins the arming preconditions (positive + every negative path).
// Exit 0 iff all checks pass.
#include <cstdio>
#include <vector>
#include <string>
#include "core/UpJumpLadderCompanion.hpp"
using chimera::UpJumpLadderCompanion;

static int g_rc = 0;
static void check(bool ok, const char* what) {
    std::printf("  %-58s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) g_rc = 1;
}

static UpJumpLadderCompanion::Config base_cfg(int W = 4, double thr = 0.015) {
    UpJumpLadderCompanion::Config c;
    c.parent_tag = "T-FEED"; c.tag = "T-CELL"; c.symbol = "tstusdt";
    c.det_w = W; c.det_thr = thr; c.tf_secs = 3600; c.round_trip_bp = 28.0;
    c.mimic_floor = true; c.mimic_giveback = 0.5;
    c.reclip_pct = 0.0; c.loss_cut_bp = 0.0;
    c.confirm_bp = 60.0; c.confirm_anchor_epx = true; c.be_floor = false;
    c.mimic_stagger = true; c.stagger_mode = 1; c.stagger_be_bp = 20.0;
    c.tight = {0.2, 0, 0.0, 0, 0.0}; c.wide = {0.2, 0, 0.0, 0, 0.0};
    c.cap = 8;
    c.catchup_max_age_bars = 24;
    return c;
}

// closes: flat 100.0 ring-fill, then a +2% jump, then `tail` bars at `after` px.
static std::vector<double> mk_closes(int W, int tail, double after, double jump_px = 102.0) {
    std::vector<double> cs;
    for (int i = 0; i < W + 4; ++i) cs.push_back(100.0);
    cs.push_back(jump_px);                       // j = +2% over W -> enters
    for (int i = 0; i < tail; ++i) cs.push_back(after);
    return cs;
}

// snapshot det_in via persist_det_state json ("det_in":0/1)
static bool det_in_of(UpJumpLadderCompanion& e) {
    std::string js = e.det_state_json();
    auto p = js.find("\"det_in\":");
    return p != std::string::npos && js[p + 9] == '1';
}

int main() {
    std::printf("catchup_seed_test\n");

    { // POSITIVE: recent jump, confirm never crossed -> window re-opened
        UpJumpLadderCompanion e(base_cfg());
        auto cs = mk_closes(4, 3, 102.2);        // after-jump closes +0.2% > jump (< 60bp confirm from 102)
        e.seed_det_ring_hist(cs, 1000);
        check(det_in_of(e), "arms: recent jump, confirm not crossed");
    }
    { // NEGATIVE: catchup off (default 0) -> flat
        auto c = base_cfg(); c.catchup_max_age_bars = 0;
        UpJumpLadderCompanion e(c);
        auto cs = mk_closes(4, 3, 102.2);
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: catchup_max_age_bars=0 (default off)");
    }
    { // NEGATIVE: confirm crossed during downtime (close >= entry*(1+60bp)) -> flat
        UpJumpLadderCompanion e(base_cfg());
        auto cs = mk_closes(4, 3, 102.0 * 1.0070);   // +70bp above jump close > 60bp confirm
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: confirm crossed in downtime (late-chase ban)");
    }
    { // NEGATIVE: pending (last) close crossed confirm -> flat
        UpJumpLadderCompanion e(base_cfg());
        auto cs = mk_closes(4, 2, 102.2);
        cs.push_back(102.0 * 1.0070);            // pending bar already past confirm
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: pending close past confirm");
    }
    { // NEGATIVE: jump older than the bound -> flat
        auto c = base_cfg(); c.catchup_max_age_bars = 5;
        UpJumpLadderCompanion e(c);
        auto cs = mk_closes(4, 9, 102.2);        // age 8 finalized bars > bound 5
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: jump older than bound");
    }
    { // NEGATIVE: reversal (j<=-thr) after the jump -> flat
        UpJumpLadderCompanion e(base_cfg());
        auto cs = mk_closes(4, 0, 0.0);
        for (int i = 0; i < 4; ++i) cs.push_back(102.0);
        cs.push_back(100.4);                     // j vs 102.0 = -1.57% <= -1.5% -> window exited
        cs.push_back(100.4);
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: reversal closed the window historically");
    }
    { // NEGATIVE: jump_floor cell (immediate-entry family) never catch-up-arms a window
        auto c = base_cfg(); c.mimic_stagger = false; c.mimic_floor = false;
        c.jump_floor = true; c.jf_giveback = 1.0;
        UpJumpLadderCompanion e(c);
        auto cs = mk_closes(4, 3, 102.2);
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: jump_floor excluded (no late immediate entry)");
    }
    { // NEGATIVE: confirm_bp=0 (unconfirmed-entry config) -> refused
        auto c = base_cfg(); c.confirm_bp = 0.0;
        UpJumpLadderCompanion e(c);
        auto cs = mk_closes(4, 3, 102.2);
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: min confirm 0 (immediate-open) refused");
    }
    { // NEGATIVE: retired book (arming_allowed_ false) -> flat
        auto c = base_cfg(); c.rank_out = true;
        UpJumpLadderCompanion e(c);
        auto cs = mk_closes(4, 3, 102.2);
        e.seed_det_ring_hist(cs, 1000);
        check(!det_in_of(e), "stays flat: rank_out/retired book");
    }
    { // POSITIVE + entry: after catch-up arm, leg opens ONLY at live confirm cross
        UpJumpLadderCompanion e(base_cfg());
        auto cs = mk_closes(4, 3, 102.2);
        e.seed_det_ring_hist(cs, 1000);
        // live bars below confirm level: leg must stay flat (books nothing)
        int64_t ts = 1001LL * 3600 * 1000;
        e.observe(true, 0.0, 102.3, ts);         // finalizes pending, inits legs
        e.observe(true, 0.0, 102.4, ts + 3600 * 1000);
        auto s1 = e.snapshot();
        check(!s1.open, "catch-up window: leg FLAT below confirm level");
        // cross confirm (102 * 1.006 = 102.612): leg opens on the live path
        e.observe(true, 0.0, 102.70, ts + 2LL * 3600 * 1000);
        e.observe(true, 0.0, 102.75, ts + 3LL * 3600 * 1000);
        auto s2 = e.snapshot();
        check(s2.open, "catch-up window: leg OPEN after live confirm cross");
    }

    std::printf(g_rc ? "catchup_seed_test: FAIL\n" : "catchup_seed_test: ALL PASS\n");
    return g_rc;
}
