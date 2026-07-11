// Phase-6 family 1 — TREND-PULLBACK / RECLAIM.
// Behavioural test via the public API (simulate / compute_target_weights):
//  (a) on an engineered uptrend-with-pullbacks tape the engine TAKES trades
//      (active days > 0) and is net-positive;
//  (b) NO 200DMA dependency — it signals with < 200 bars of history;
//  (c) EXIT fires on a trend break (a held name is dropped when close < emaSlow).
#include "core/TrendPullbackReclaimEngine.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace chimera;

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    TrendPullbackReclaimEngine e;
    std::vector<std::string> U = {"C0","C1","C2","C3","C4","C5","C6","C7"};
    e.set_universe(U);
    int T = 300;
    for (int ci = 0; ci < (int)U.size(); ++ci) {
        double px = 100.0; int phase0 = ci * 3;    // stagger pullback phases per coin
        for (int d = 0; d < T; ++d) {
            int ph = (d + phase0) % 25;
            double o = px;
            if (ph == 20 || ph == 21 || ph == 22) px *= 0.965;      // 3-day pullback
            else if (ph == 23)                    px *= 1.060;      // reclaim pop
            else                                  px *= 1.012;      // uptrend
            double hi = px * 1.015, lo = px * 0.985;
            if (ph == 20 || ph == 21 || ph == 22) lo = px * 0.955;  // dig low under EMA
            e.seed_daily(U[ci], d, o, hi, lo, px, 1.0e6);            // ample $-vol
        }
    }
    auto daily = e.simulate();
    int active = 0; double net = 0; int first_active = -1;
    for (int i = 0; i < (int)daily.size(); ++i) { net += daily[i].second;
        if (std::fabs(daily[i].second) > 1e-9) { ++active; if (first_active < 0) first_active = i; } }
    printf("  simulate: days=%zu active=%d net=%+.1f%% first_active_idx=%d\n",
           daily.size(), active, net*100, first_active);
    ck(active > 0, "engine takes trades on an uptrend-with-pullback tape");
    ck(net > 0, "net positive on a rising tape");
    ck(first_active >= 0 && first_active < 200, "signals BEFORE 200 bars (NO 200DMA dependency)");

    // (c) EXIT on trend break: uptrend for 120d then a sharp break below emaSlow.
    TrendPullbackReclaimEngine e2;
    std::vector<std::string> U2 = {"X"};
    for (auto& s : {"F0","F1","F2"}) U2.push_back(s);   // filler for breadth
    e2.set_universe(U2);
    for (int ci = 0; ci < (int)U2.size(); ++ci) {
        double px = 100.0;
        for (int d = 0; d < 160; ++d) {
            double o = px;
            if (d < 130) px *= 1.010;                 // long uptrend
            else         px *= 0.94;                  // hard break (falls under emaSlow)
            e2.seed_daily(U2[ci], d, o, px*1.01, px*0.99, px, 1.0e6);
        }
    }
    size_t iend = e2.num_days() - 1; bool bull;
    LongOnlyDailyBase::Position held; held.entry_price = 100.0; held.highest_close = 300.0;
    held.bars_held = 30; held.entry_i = 130;
    std::map<std::string,LongOnlyDailyBase::Position> h; h["X"] = held;
    auto w = e2.compute_target_weights(iend, bull, h);
    printf("  post-break weights has X? %d (should be 0 — trend break exit)\n", (int)w.count("X"));
    ck(w.count("X") == 0, "held name dropped after close < emaSlow (trend-break exit)");

    if (fails == 0) { printf("PASS trend_pullback_test\n"); return 0; }
    printf("FAIL trend_pullback_test (%d)\n", fails); return 1;
}
