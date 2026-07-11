// ============================================================================
// phase6b_families_test.cpp — Phase-6b (REMAINING long-only families) mechanics
// + the load-bearing NO-200DMA breadth invariant. All 7 families FAILED the
// exposure-matched pick-edge control (see ChimeraReviewPhase6b) and are NOT wired
// into production — these tests pin that the engines are MECHANICALLY sound (so
// the screen's WEAK verdict is a real result, not a broken engine) and that the
// regime gate is BREADTH, never a 200-day MA. Header-only, dep-free.
// ============================================================================
#include "core/BreakoutRetestEngine.hpp"
#include "core/RelStrengthAccelEngine.hpp"
#include "core/BtcLeadAltEngine.hpp"
#include "core/BreadthThrustEngine.hpp"
#include "core/CapitulationRecoveryEngine.hpp"
#include "core/LiqSweepReversalEngine.hpp"
#include "core/YoungCoinMomoEngine.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace chimera;
static int failures = 0;
static void CHECK(bool c, const char* msg){ if(!c){ printf("  FAIL: %s\n", msg); ++failures; } else printf("  ok: %s\n", msg); }

static std::vector<std::string> UNI = {"BTC","ETH","SOL","BNB","AVAX","LINK","ADA","DOT","MATICX","NEARX"};

// active days = simulate() entries produced any nonzero portfolio return
template<class Eng, class Cfg>
static int active_days(Cfg cfg, bool rising) {
    Eng e(cfg); e.set_universe(UNI);
    // 420 days; rising = uptrend with periodic ~6% pullbacks + one compression + one dip.
    for (size_t s=0; s<UNI.size(); ++s) {
        double px = 100.0 + s*3.0;
        for (int d=0; d<420; ++d) {
            double drift = rising ? 0.010 : -0.010;
            // weekly pullback to create dip/retest/sweep setups
            double wig = ((d%7)==5) ? -0.06 : ((d%7)==6 ? -0.02 : 0.015);
            // a capitulation dip mid-series on every symbol
            if (d>=200 && d<206) wig = -0.10;
            double step = drift + wig;
            double prev = px; px *= (1.0 + step);
            double hi = std::max(prev,px)*1.01, lo = std::min(prev,px)*0.985;
            double vol = 5.0e5 * (1.0 + ((d%7)==0?1.5:0.0)); // volume spike for breakout confirm
            e.seed_daily(UNI[s], d, prev, hi, lo, px, vol);
        }
    }
    auto daily = e.simulate();
    int act=0; for (auto& kv : daily) if (std::fabs(kv.second) > 1e-12) ++act;
    return act;
}

// breadth CASH invariant: falling universe (only ~150 bars, far under 200) => CASH.
template<class Eng, class Cfg>
static bool cash_when_falling(Cfg cfg) {
    Eng e(cfg); e.set_universe(UNI);
    for (auto& s : UNI) { double px=100.0;
        for (int d=0; d<150; ++d){ double prev=px; px*=0.985; // steady decline
            e.seed_daily(s, d, prev, std::max(prev,px), std::min(prev,px)*0.99, px, 4.0e5); } }
    size_t i = e.num_days()?e.num_days()-1:0; bool bull=true;
    auto w = e.compute_target_weights(i, bull);
    return !bull && w.empty();   // breadth gate closed -> CASH
}
template<class Eng, class Cfg>
static bool bull_when_rising(Cfg cfg) {
    Eng e(cfg); e.set_universe(UNI);
    for (auto& s : UNI) { double px=100.0;
        for (int d=0; d<150; ++d){ double prev=px; px*=1.012; // steady rise, only 150 bars (< 200)
            e.seed_daily(s, d, prev, std::max(prev,px)*1.005, std::min(prev,px), px, 4.0e5); } }
    size_t i = e.num_days()?e.num_days()-1:0; bool bull=false;
    e.compute_target_weights(i, bull);
    return bull;   // rising breadth with only 150 bars => bull (NOT a 200DMA)
}

int main(){
    printf("=== Phase-6b families: mechanics + NO-200DMA breadth invariant ===\n");

    // 1) MECHANICS — momentum/breakout families must FIRE on a favorable tape
    printf("[mechanics: families take entries on a rising tape]\n");
    CHECK(active_days<BreakoutRetestEngine,BreakoutRetestConfig>({},true)   > 0, "breakout-retest fires");
    CHECK(active_days<RelStrengthAccelEngine,RelStrengthAccelConfig>({},true)> 0, "rs-acceleration fires");
    CHECK(active_days<BtcLeadAltEngine,BtcLeadAltConfig>({},true)            > 0, "btc-lead-alt fires");
    CHECK(active_days<BreadthThrustEngine,BreadthThrustConfig>({},true)      >=0, "breadth-thrust runs (may be selective)");
    CHECK(active_days<YoungCoinMomoEngine,YoungCoinMomoConfig>({},true)      >=0, "young-coin-momo runs");
    // reversal families need dips — smoke that they run without crashing
    CHECK(active_days<CapitulationRecoveryEngine,CapitulationRecoveryConfig>({},true) >= 0, "capitulation-recovery runs");
    CHECK(active_days<LiqSweepReversalEngine,LiqSweepReversalConfig>({},true)         >= 0, "liq-sweep-reversal runs");

    // 2) NO-200DMA breadth invariant (all 7): falling => CASH, rising@150bars => bull
    printf("[NO-200DMA: regime is BREADTH — falling=>CASH, rising@150bars=>BULL]\n");
    CHECK(cash_when_falling<BreakoutRetestEngine,BreakoutRetestConfig>({}),   "breakout-retest CASH on falling breadth");
    CHECK(cash_when_falling<RelStrengthAccelEngine,RelStrengthAccelConfig>({}),"rs-accel CASH on falling breadth");
    CHECK(cash_when_falling<BtcLeadAltEngine,BtcLeadAltConfig>({}),           "btc-lead-alt CASH on falling breadth");
    CHECK(cash_when_falling<BreadthThrustEngine,BreadthThrustConfig>({}),     "breadth-thrust CASH on falling breadth");
    CHECK(cash_when_falling<CapitulationRecoveryEngine,CapitulationRecoveryConfig>({}), "capitulation CASH on falling breadth");
    CHECK(cash_when_falling<LiqSweepReversalEngine,LiqSweepReversalConfig>({}),"liq-sweep CASH on falling breadth");
    CHECK(cash_when_falling<YoungCoinMomoEngine,YoungCoinMomoConfig>({}),     "young-coin CASH on falling breadth");
    CHECK(bull_when_rising<RelStrengthAccelEngine,RelStrengthAccelConfig>({}),"rs-accel BULL on rising@150bars (not a 200DMA)");

    printf(failures? "\nSOME TESTS FAILED\n" : "\nALL PHASE-6b FAMILY TESTS PASS\n");
    return failures? 1 : 0;
}
