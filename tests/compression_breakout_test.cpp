// Phase-6 family 2 — COMPRESSION BREAKOUT.
// Behavioural test via the public API:
//  (a) on a tape that goes VOLATILE -> COMPRESSED -> BREAKOUT-with-volume the
//      engine TAKES a trade (bandwidth percentile low + close breaks range high);
//  (b) EXIT fires when close falls back below the Bollinger mid (failed breakout).
#include "core/CompressionBreakoutDailyEngine.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace chimera;

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    CompressionBreakoutDailyEngine e;
    std::vector<std::string> U = {"C0","C1","C2","C3","C4","C5"};
    e.set_universe(U);
    // 0..150 volatile (HIGH bandwidth), 150..175 tight (LOW bandwidth -> low
    // percentile), 176 breakout +9% with a volume spike, then ride up.
    for (int ci = 0; ci < (int)U.size(); ++ci) {
        double px = 100.0; int off = ci;
        for (int d = 0; d < 230; ++d) {
            double o = px, vol = 1.0e6;
            if (d < 150)      { double s = ((d+off) % 2 == 0) ? 1.06 : 0.945; px *= s; }  // volatile
            else if (d < 176) { double s = ((d+off) % 2 == 0) ? 1.004 : 0.997; px *= s; } // compressed
            else if (d == 176){ px *= 1.09; vol = 4.0e6; }                                // breakout + volume
            else              { px *= 1.010; }                                            // ride
            e.seed_daily(U[ci], d, o, px*1.01, px*0.99, px, vol);
        }
    }
    auto daily = e.simulate();
    int active = 0; double net = 0;
    for (auto& kv : daily) { net += kv.second; if (std::fabs(kv.second) > 1e-9) ++active; }
    printf("  simulate: days=%zu active=%d net=%+.1f%%\n", daily.size(), active, net*100);
    ck(active > 0, "engine takes a compression-breakout trade");

    // (b) EXIT on failed breakout: build compression+breakout then a drop back
    // under the mid band; a held name should be dropped.
    CompressionBreakoutDailyEngine e2;
    std::vector<std::string> U2 = {"X","F0","F1","F2"};
    e2.set_universe(U2);
    for (int ci = 0; ci < (int)U2.size(); ++ci) {
        double px = 100.0;
        for (int d = 0; d < 210; ++d) {
            double o = px;
            if (d < 150)      px *= ((d%2==0)?1.05:0.955);
            else if (d < 200) px *= ((d%2==0)?1.003:0.998);
            else              px *= 0.90;                    // collapse below mid band
            e2.seed_daily(U2[ci], d, o, px*1.01, px*0.99, px, 1.0e6);
        }
    }
    size_t iend = e2.num_days() - 1; bool bull;
    LongOnlyDailyBase::Position held; held.entry_price = 100.0; held.highest_close = 150.0;
    held.bars_held = 5; held.entry_i = 200;
    std::map<std::string,LongOnlyDailyBase::Position> h; h["X"] = held;
    auto w = e2.compute_target_weights(iend, bull, h);
    printf("  post-collapse weights has X? %d (should be 0 — failed-breakout exit)\n", (int)w.count("X"));
    ck(w.count("X") == 0, "held name dropped after close < Bollinger mid (failed-breakout exit)");

    if (fails == 0) { printf("PASS compression_breakout_test\n"); return 0; }
    printf("FAIL compression_breakout_test (%d)\n", fails); return 1;
}
