// Phase-5 item 26 — CORE + CHALLENGER + CASH.
// (a) with many strong coins, the core (top-3 picks) holds ~core_frac of the
//     sleeve and the challengers hold the remainder; (b) a challenger below
//     challenger_min_z is NOT bought (no auto-buy of a weak 4th/5th); (c) when
//     too few coins are positive, the sleeve sits in CASH (weights sum < 1).
#include "core/CrossSectionalMomentum2Engine.hpp"
#include <cstdio>
#include <cmath>
#include <set>

using namespace chimera;

// deterministic small noise so realized vol is well-defined (a pure exponential
// has zero variance -> degenerate vol/vol-adj; real coins are noisy).
static uint64_t g_rng = 7;
static double noise() { g_rng = g_rng*6364136223846793005ULL + 1442695040888963407ULL;
    return ((double)((g_rng>>33)&0xFFFF)/65535.0 - 0.5) * 0.03; }

static double sum_w(const std::map<std::string,double>& w) {
    double s = 0; for (auto& kv : w) s += kv.second; return s;
}
static int n_pos(const std::map<std::string,double>& w) {
    int n = 0; for (auto& kv : w) if (kv.second > 0) ++n; return n;
}

int main() {
    int fails = 0;
    auto ck = [&](bool c, const char* m){ if (!c) { printf("  FAIL: %s\n", m); ++fails; } };
    int N = 320;

    // (a) ten trending coins -> core 3 + challenger 2; core ~= core_frac.
    {
        XSec2Config cfg; cfg.min_dollar_vol = 0.0; cfg.breadth_gate = false;
        cfg.core_frac = 0.78; cfg.challenger_min_z = -9.0; cfg.corr_pen_w = 0.0;
        CrossSectionalMomentum2Engine e(cfg);
        std::vector<std::string> U;
        for (int k = 0; k < 10; ++k) U.push_back(std::string("C") + char('0'+k));
        e.set_universe(U);
        // two clusters: 5 STRONG (positive composite) + 5 WEAK (negative) so the
        // pool has ~5 positive names to fill core(3)+challenger(2).
        for (int d = 0; d < N; ++d) for (int k = 0; k < 10; ++k) {
            double drift = (k < 5) ? (1.0080 + k*0.0004) : (0.9975 - (k-5)*0.0004);
            e.seed_daily(U[k], d, 100.0*std::pow(drift, d)*(1.0+noise()), 5e7);
        }
        bool bull; auto w = e.compute_target_weights(N-1, bull, {}, 1);
        auto elig = e.eligible_set(N-1); auto comp = e.composite(N-1, elig);
        auto picks = e.select(N-1, elig, comp, {});
        double core = 0, chal = 0;
        for (size_t p = 0; p < picks.size(); ++p) (p < 3 ? core : chal) += w[picks[p]];
        printf("  10-strong: positions=%d picks=%zu core=%.2f challenger=%.2f total=%.2f\n",
               n_pos(w), picks.size(), core, chal, sum_w(w));
        ck(n_pos(w) >= 3 && n_pos(w) <= 5, "3..5 positions (core+challenger, watchlist unfunded)");
        ck(core > chal, "core holds the majority of the sleeve");
        ck(sum_w(w) <= 1.0 + 1e-9, "weights sum <= 1 (rest cash)");
        if (picks.size() == 5) ck(std::fabs(core - 0.78) < 0.06, "core ~= core_frac when challengers funded");
    }

    // (b) challenger below the cost-separation bar is NOT bought.
    {
        XSec2Config cfg; cfg.min_dollar_vol = 0.0; cfg.breadth_gate = false;
        cfg.challenger_min_z = 5.0;   // unreachable bar -> no challenger qualifies
        cfg.corr_pen_w = 0.0;
        CrossSectionalMomentum2Engine e(cfg);
        std::vector<std::string> U = {"A","B","C","D","E","F","G","H"};
        e.set_universe(U);
        // 5 strong + 3 weak: pool has >3 positive so challenger SLOTS exist, but
        // the unreachable min_z blocks them -> exactly the 3 core get funded.
        for (int d = 0; d < N; ++d) for (int k = 0; k < 8; ++k) {
            double drift = (k < 5) ? (1.0080 + k*0.0004) : (0.9970 - (k-5)*0.0004);
            e.seed_daily(U[k], d, 100.0*std::pow(drift, d)*(1.0+noise()), 5e7);
        }
        bool bull; auto w = e.compute_target_weights(N-1, bull, {}, 1);
        printf("  unreachable challenger bar: positions=%d total=%.2f\n", n_pos(w), sum_w(w));
        ck(n_pos(w) == 3, "only the 3 core funded (no weak challenger auto-buy)");
        ck(std::fabs(sum_w(w) - 1.0) < 1e-6, "core takes full sleeve when no challenger deployed");
    }

    // (c) too few positive -> CASH.
    {
        XSec2Config cfg; cfg.min_dollar_vol = 0.0; cfg.breadth_gate = false; cfg.min_positive = 2;
        CrossSectionalMomentum2Engine e(cfg);
        std::vector<std::string> U = {"A","B","C","D"};
        e.set_universe(U);
        for (int d = 0; d < N; ++d) {
            e.seed_daily("A", d, 100.0*std::pow(1.010, d)*(1.0+noise()), 5e7);
            e.seed_daily("B", d, 100.0*std::pow(0.995, d)*(1.0+noise()), 5e7);
            e.seed_daily("C", d, 100.0*std::pow(0.994, d)*(1.0+noise()), 5e7);
            e.seed_daily("D", d, 100.0*std::pow(0.993, d)*(1.0+noise()), 5e7);
        }
        bool bull; auto w = e.compute_target_weights(N-1, bull, {}, 1);
        printf("  1-positive vs min_positive=2: positions=%d total=%.2f\n", n_pos(w), sum_w(w));
        ck(n_pos(w) == 0, "sits in CASH when < min_positive coins are positive");
    }

    if (fails == 0) { printf("PASS core_challenger_test\n"); return 0; }
    printf("FAIL core_challenger_test (%d)\n", fails); return 1;
}
