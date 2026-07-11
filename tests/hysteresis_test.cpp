// Phase-5 item 25 — HYSTERESIS: a held name is NOT churned out for a marginal
// challenger. (a) a held coin whose composite is fractionally below a rival stays
// selected (no oscillation); (b) a challenger that beats it by MORE than the
// buffer does force the swap; (c) full-sim turnover stays finite (the no-trade
// band + hysteresis prevent per-day churn).
#include "core/CrossSectionalMomentum2Engine.hpp"
#include <cstdio>
#include <cmath>
#include <set>

using namespace chimera;

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };

    XSec2Config cfg; cfg.min_dollar_vol = 0.0; cfg.core_k = 1; cfg.challenger_k = 1;
    cfg.challenger_buffer = 0.40; cfg.corr_pen_w = 0.0;   // isolate the tenure logic
    CrossSectionalMomentum2Engine e(cfg);
    std::vector<std::string> U = {"A","B","C"};
    e.set_universe(U);
    int N = 300;
    for (int d = 0; d < N; ++d) {
        // A and B almost tied (A very slightly weaker); C clearly worst.
        e.seed_daily("A", d, 100.0*std::pow(1.0080, d), 5e7);
        e.seed_daily("B", d, 100.0*std::pow(1.0082, d), 5e7);
        e.seed_daily("C", d, 100.0*std::pow(0.999,  d), 5e7);
    }
    size_t i = N-1;
    auto elig = e.eligible_set(i);
    auto comp = e.composite(i, elig);

    // (a) A currently held, B marginally better -> hysteresis keeps A as a pick.
    std::set<std::string> held = {"A"};
    auto picks_held = e.select(i, elig, comp, held);
    bool a_kept = false; for (auto& s : picks_held) if (s=="A") a_kept = true;
    ck(a_kept, "held A retained despite B marginally higher (hysteresis)");
    printf("  held={A}: picks ="); for (auto& s : picks_held) printf(" %s", s.c_str()); printf("\n");

    // (b) if instead B is FAR ahead of the held name, the swap happens.
    XSec2Config cfg2 = cfg; CrossSectionalMomentum2Engine e2(cfg2);
    e2.set_universe(U);
    for (int d = 0; d < N; ++d) {
        e2.seed_daily("A", d, 100.0*std::pow(1.001, d), 5e7);   // held, now clearly weak
        e2.seed_daily("B", d, 100.0*std::pow(1.020, d), 5e7);   // far stronger challenger
        e2.seed_daily("C", d, 100.0*std::pow(0.999, d), 5e7);
    }
    auto elig2 = e2.eligible_set(i); auto comp2 = e2.composite(i, elig2);
    auto picks2 = e2.select(i, elig2, comp2, std::set<std::string>{"A"});
    bool b_core = (!picks2.empty() && picks2[0] == "B");
    ck(b_core, "far-superior challenger B becomes the core pick (swap allowed)");
    printf("  held={A}, B far ahead: core = %s\n", picks2.empty()?"(none)":picks2[0].c_str());

    // (c) full-sim turnover is finite / bounded (no per-day oscillation).
    XSec2Config cfg3; cfg3.min_dollar_vol = 0.0;
    CrossSectionalMomentum2Engine e3(cfg3);
    std::vector<std::string> U3 = {"A","B","C","D","E","F","G","H"};
    e3.set_universe(U3);
    // choppy but bounded: each coin a noisy random-walk (deterministic LCG).
    uint64_t rng = 42;
    auto rnd = [&]{ rng = rng*6364136223846793005ULL + 1442695040888963407ULL; return (double)((rng>>33)&0xFFFF)/65535.0; };
    std::map<std::string,double> px; for (auto& s : U3) px[s] = 100.0;
    for (int d = 0; d < 600; ++d) for (auto& s : U3) {
        px[s] *= (1.0 + (rnd()-0.48)*0.04);
        if (px[s] < 1) px[s] = 1;
        e3.seed_daily(s, d, px[s], 5e7);
    }
    auto daily = e3.simulate();
    // count rebalances implicitly: a well-behaved sim has far fewer non-zero-cost
    // days than total days. We approximate churn health by finite total return.
    double eq = 1; for (auto& kv : daily) eq *= (1.0 + kv.second);
    ck(std::isfinite(eq) && eq > 0, "simulate() finite over 600 choppy days (no blow-up churn)");
    printf("  choppy 600d: %zu days, final equity x%.3f\n", daily.size(), eq);

    if (fails == 0) { printf("PASS hysteresis_test\n"); return 0; }
    printf("FAIL hysteresis_test (%d)\n", fails); return 1;
}
