// Phase-6 shared invariant — NO 200DMA + BREADTH regime gate.
// The HARD operator rule: the REGIME gate must be a breadth participation ratio,
// never a 200-day moving average. This test proves, on the shared base:
//  (a) a UNIFORMLY-RISING universe is deployed (bull=true) with FAR fewer than
//      200 bars of history (min_history=120) — no 200-day requirement exists;
//  (b) a UNIFORMLY-FALLING universe is CASH (bull=false) — long-only sits out the
//      bear via BREADTH, not a price MA;
//  (c) the deploy/cash decision responds to breadth, not to any 200-bar anchor
//      (flip achieved with only ~150 bars total).
#include "core/TrendPullbackReclaimEngine.hpp"
#include "core/CompressionBreakoutDailyEngine.hpp"
#include "core/BullRegimeMeanReversionEngine.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace chimera;

template <class Eng>
static bool bull_on_rising(const char* name, int T) {
    Eng e; std::vector<std::string> U = {"C0","C1","C2","C3","C4","C5"};
    e.set_universe(U);
    for (int ci = 0; ci < (int)U.size(); ++ci) { double px = 100.0;
        for (int d = 0; d < T; ++d) { double o = px; px *= 1.010;
            e.seed_daily(U[ci], d, o, px*1.01, px*0.99, px, 1.0e6); } }
    size_t i = e.num_days() - 1; bool bull; e.compute_target_weights(i, bull);
    printf("  %-14s rising(%d bars): breadth=%.2f bull=%d\n", name, T, e.breadth_latest(), bull?1:0);
    return bull;
}
template <class Eng>
static bool bull_on_falling(const char* name, int T) {
    Eng e; std::vector<std::string> U = {"C0","C1","C2","C3","C4","C5"};
    e.set_universe(U);
    for (int ci = 0; ci < (int)U.size(); ++ci) { double px = 100.0;
        for (int d = 0; d < T; ++d) { double o = px; px *= 0.990;
            e.seed_daily(U[ci], d, o, px*1.01, px*0.99, px, 1.0e6); } }
    size_t i = e.num_days() - 1; bool bull; e.compute_target_weights(i, bull);
    printf("  %-14s falling(%d bars): breadth=%.2f bull=%d\n", name, T, e.breadth_latest(), bull?1:0);
    return bull;
}

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    const int BARS = 150;   // deliberately < 200 to prove no 200DMA anchor
    // (a) rising -> deployed, with only 150 bars
    ck(bull_on_rising<TrendPullbackReclaimEngine>("TREND-PB", BARS),   "trend-pb bull on rising (150 bars)");
    ck(bull_on_rising<CompressionBreakoutDailyEngine>("COMPRESSION", BARS), "compression bull on rising (150 bars)");
    ck(bull_on_rising<BullRegimeMeanReversionEngine>("BULL-MR", BARS), "bull-mr bull on rising (150 bars)");
    // (b) falling -> CASH (breadth gate, not a price MA)
    ck(!bull_on_falling<TrendPullbackReclaimEngine>("TREND-PB", BARS),   "trend-pb CASH on falling");
    ck(!bull_on_falling<CompressionBreakoutDailyEngine>("COMPRESSION", BARS), "compression CASH on falling");
    ck(!bull_on_falling<BullRegimeMeanReversionEngine>("BULL-MR", BARS), "bull-mr CASH on falling");

    if (fails == 0) { printf("PASS no_200dma_breadth_test\n"); return 0; }
    printf("FAIL no_200dma_breadth_test (%d)\n", fails); return 1;
}
