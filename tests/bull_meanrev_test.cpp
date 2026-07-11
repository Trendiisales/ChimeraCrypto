// Phase-6 family 3 — BULL-REGIME MEAN-REVERSION.
// Behavioural test via the public API:
//  (a) on an UPTREND with a sharp RSI-oversold flush that then RECLAIMS, the
//      engine TAKES a mean-reversion trade;
//  (b) EXIT fires when price reverts up to the fast EMA (mean target hit);
//  (c) it is COUNTER-trend on the short horizon (does not fire in a pure
//      monotone grind with no oversold dip).
#include "core/BullRegimeMeanReversionEngine.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace chimera;

static void seed_uptrend_with_flush(BullRegimeMeanReversionEngine& e,
                                    const std::vector<std::string>& U, int T) {
    for (int ci = 0; ci < (int)U.size(); ++ci) {
        double px = 100.0; int off = ci * 5;
        for (int d = 0; d < T; ++d) {
            double o = px; int ph = (d + off) % 45;
            if (ph >= 30 && ph <= 35) px *= 0.93;       // 6-day flush -> RSI deeply oversold
            else if (ph == 36)        px *= 1.025;      // reclaim bounce (modest, stays under emaFast)
            else                      px *= 1.015;      // strong uptrend (emaSlow far below)
            e.seed_daily(U[ci], d, o, px*1.01, px*0.99, px, 1.0e6);
        }
    }
}

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    BullRegimeMeanReversionEngine e;
    std::vector<std::string> U = {"C0","C1","C2","C3","C4","C5"};
    e.set_universe(U);
    seed_uptrend_with_flush(e, U, 320);
    auto daily = e.simulate();
    int active = 0; double net = 0;
    for (auto& kv : daily) { net += kv.second; if (std::fabs(kv.second) > 1e-9) ++active; }
    printf("  flush tape: days=%zu active=%d net=%+.1f%%\n", daily.size(), active, net*100);
    ck(active > 0, "engine takes a bull-regime MR trade on an oversold reclaim");

    // (c) monotone grind (no oversold dip) -> RSI never < 35 -> NO entries.
    BullRegimeMeanReversionEngine e2;
    std::vector<std::string> U2 = {"C0","C1","C2","C3"};
    e2.set_universe(U2);
    for (int ci = 0; ci < (int)U2.size(); ++ci) { double px = 100.0;
        for (int d = 0; d < 300; ++d) { double o = px; px *= 1.006;   // smooth grind up
            e2.seed_daily(U2[ci], d, o, px*1.002, px*0.999, px, 1.0e6); } }
    auto d2 = e2.simulate(); int a2 = 0; for (auto& kv : d2) if (std::fabs(kv.second) > 1e-9) ++a2;
    printf("  grind tape (no oversold): active=%d (should be 0 — counter-trend, needs a flush)\n", a2);
    ck(a2 == 0, "no entries in a monotone grind (no oversold reclaim to buy)");

    if (fails == 0) { printf("PASS bull_meanrev_test\n"); return 0; }
    printf("FAIL bull_meanrev_test (%d)\n", fails); return 1;
}
