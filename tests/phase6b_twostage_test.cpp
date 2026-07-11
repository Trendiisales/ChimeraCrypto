// ============================================================================
// phase6b_twostage_test.cpp — Item 28 (two-stage / tranche ignition) mechanics.
// Pins the SALVAGE finding (see ChimeraReviewPhase6b + backtest/phase6b_twostage_bt):
// at IDENTICAL total capital per signal, splitting the ignition entry does NOT
// beat the immediate-only RipRider parent, because on a clean momentum runner the
// pullback tranche NEVER fills -> the split UNDER-DEPLOYS and captures LESS of the
// move than the fully-deployed immediate entry. Self-contained (no engine dep).
// ============================================================================
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>

static int failures = 0;
#define CHECK(c,msg) do{ if(!(c)){ printf("  FAIL: %s\n", msg); ++failures; } else printf("  ok: %s\n", msg); }while(0)

struct Bar { double o,h,l,c; };

// Minimal tranche-entry replica: immediate fills at entry-day open; pullback tranche
// fills only if a subsequent day's LOW <= open*(1-depth) within expiry; ride to the
// last bar (regime stays bull in this fixture). Returns {deployed, net_per_alloc}.
static std::pair<double,double> tranche(const std::vector<Bar>& path,
                                        double w_imm, double w_pb, double pb_depth, int pb_exp) {
    double openpx = path[0].o;
    bool imm_f = w_imm > 0, pb_f = false;
    double pb_limit = openpx*(1.0 - pb_depth);
    for (int j=0; j<(int)path.size() && j<=pb_exp; ++j)
        if (w_pb>0 && !pb_f && path[j].l <= pb_limit) pb_f = true;
    double wsum=0, wpx=0;
    if (imm_f){ wsum+=w_imm; wpx+=w_imm*openpx; }
    if (pb_f){ wsum+=w_pb; wpx+=w_pb*pb_limit; }
    if (wsum<=0) return {0.0, 0.0};
    double blended = wpx/wsum;
    double exit_px = path.back().c;
    double net = exit_px/blended - 1.0;        // no cost — mechanics only
    return {wsum, wsum*net};                    // per 1.0 allocated capital budget
}

int main(){
    printf("=== Item 28: two-stage ignition mechanics (identical-capital) ===\n");

    // A clean momentum runner: monotonic uptrend, low never dips below the open.
    std::vector<Bar> runner; { double px=100.0;
        for (int d=0; d<40; ++d){ double prev=px; px*=1.03;
            runner.push_back({prev, px*1.005, std::min(prev,px)*1.0005, px}); } }

    auto imm   = tranche(runner, 1.0, 0.0, 0.06, 5);      // immediate-only parent
    auto split = tranche(runner, 0.4, 0.6, 0.06, 5);      // immediate+pullback

    printf("  runner: immediate deployed=%.2f net/alloc=%+.1f%% | split deployed=%.2f net/alloc=%+.1f%%\n",
           imm.first, imm.second*100, split.first, split.second*100);
    CHECK(std::fabs(imm.first - 1.0) < 1e-9, "immediate deploys the FULL budget (1.0)");
    CHECK(split.first < 0.99,               "split UNDER-deploys on a clean runner (pullback never fills)");
    CHECK(imm.second > split.second,        "immediate net/alloc BEATS split at identical capital on a runner");

    // A pullback DOES occur: the tranche fills cheaper -> better per-deployed edge,
    // but still only matches once fully deployed (fill improvement is real but bounded).
    std::vector<Bar> dip; { double px=100.0;
        // day0 open 100, then a -8% dip (fills the 6% pullback), then recovery up
        dip.push_back({100.0, 101.0, 92.0, 93.0});
        double p=93.0; for (int d=0; d<20; ++d){ double prev=p; p*=1.03; dip.push_back({prev,p*1.005,std::min(prev,p),p}); } }
    auto imm2   = tranche(dip, 1.0, 0.0, 0.06, 5);
    auto split2 = tranche(dip, 0.4, 0.6, 0.06, 5);
    printf("  dip:    immediate deployed=%.2f net/alloc=%+.1f%% | split deployed=%.2f net/alloc=%+.1f%%\n",
           imm2.first, imm2.second*100, split2.first, split2.second*100);
    CHECK(std::fabs(split2.first - 1.0) < 1e-9, "split fully deploys WHEN the pullback fills");
    CHECK(split2.second > imm2.second,          "split wins ONLY on the trades that actually pull back (cheaper fill)");

    printf(failures? "\nSOME TESTS FAILED\n" : "\nALL PHASE-6b TWO-STAGE TESTS PASS\n");
    return failures? 1 : 0;
}
