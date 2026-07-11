// Phase-5 item 24 — COMPOSITE score.
// (a) the coin with the strongest vol-adjusted momentum across 7/30/90d ranks
//     highest; (b) the liquidity penalty demotes an otherwise-identical coin that
//     trades far less $-volume. Deterministic synthetic universe.
#include "core/CrossSectionalMomentum2Engine.hpp"
#include <cstdio>
#include <cmath>

using namespace chimera;
static const int64_t DAY = 1;   // day index units (engine keys on day, spacing irrelevant here)

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    XSec2Config cfg; cfg.min_dollar_vol = 0.0;  // focus on the score, not the liq gate
    cfg.min_history_days = 120; cfg.liq_window = 30;
    CrossSectionalMomentum2Engine e(cfg);
    std::vector<std::string> U = {"STRONG","MID","WEAK","FLAT"};
    e.set_universe(U);

    int N = 300;
    for (int d = 0; d < N; ++d) {
        // STRONG: steady strong uptrend; MID: mild up; WEAK: down; FLAT: flat.
        double base = 100.0;
        e.seed_daily("STRONG", d, base * std::pow(1.010, d), 5e7);
        e.seed_daily("MID",    d, base * std::pow(1.004, d), 5e7);
        e.seed_daily("WEAK",   d, base * std::pow(0.997, d), 5e7);
        e.seed_daily("FLAT",   d, base,                       5e7);
    }
    size_t i = N - 1;
    auto elig = e.eligible_set(i);
    ck(elig.size() == 4, "all 4 coins eligible with long history");
    auto comp = e.composite(i, elig);
    printf("  composite: STRONG=%.2f MID=%.2f WEAK=%.2f FLAT=%.2f\n",
           comp["STRONG"], comp["MID"], comp["WEAK"], comp["FLAT"]);
    ck(comp["STRONG"] > comp["MID"],  "STRONG > MID");
    ck(comp["MID"]    > comp["WEAK"], "MID > WEAK");
    ck(comp["STRONG"] > 0,            "STRONG composite positive");
    ck(comp["WEAK"]   < 0,            "WEAK composite negative");

    // (b) liquidity penalty — clone STRONG's price path at 100x LOWER $-volume.
    XSec2Config cfg2 = cfg; cfg2.min_dollar_vol = 0.0; cfg2.liq_pen_w = 0.8;
    CrossSectionalMomentum2Engine e2(cfg2);
    std::vector<std::string> U2 = {"LIQ","ILLIQ","MID","WEAK"};
    e2.set_universe(U2);
    for (int d = 0; d < N; ++d) {
        double px = 100.0 * std::pow(1.010, d);
        e2.seed_daily("LIQ",   d, px, 5e8);     // same path, high volume
        e2.seed_daily("ILLIQ", d, px, 5e6);     // same path, 100x lower volume
        e2.seed_daily("MID",   d, 100.0*std::pow(1.004,d), 5e7);
        e2.seed_daily("WEAK",  d, 100.0*std::pow(0.997,d), 5e7);
    }
    auto elig2 = e2.eligible_set(N-1);
    auto comp2 = e2.composite(N-1, elig2);
    printf("  liq penalty: LIQ=%.2f  ILLIQ=%.2f (same price path)\n", comp2["LIQ"], comp2["ILLIQ"]);
    ck(comp2["LIQ"] > comp2["ILLIQ"], "liquidity penalty demotes the illiquid clone");

    if (fails == 0) { printf("PASS composite_score_test\n"); return 0; }
    printf("FAIL composite_score_test (%d)\n", fails); return 1;
}
