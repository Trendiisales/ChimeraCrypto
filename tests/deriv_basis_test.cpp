// Phase-7 test 3 — perp-spot basis.
// basis_pct = (perp_close - spot_close)/spot_close. Verify sign + magnitude.
// (Backtest verdict: basis-extreme filter = SUSPECT/REJECT — high-basis entries
// were WINNERS, so a veto suppresses winners. Test pins the MECHANIC only.)
#include "core/DerivativesSignals.hpp"
#include <cstdio>
#include <cmath>

using namespace chimera;

static double basis_of(double spot, double perp) {
    DerivativesSignalBook b;
    int64_t t0 = 1'700'000'000'000LL;
    b.on_funding("X", t0, 0.0001);
    for (int i = 0; i < 24; ++i) {
        b.on_spot_h1("X", t0 + i*3'600'000LL, spot, 100.0, 55.0);
        b.on_perp_h1("X", t0 + i*3'600'000LL, perp, 100.0, 55.0);
    }
    return b.eval("X").basis_pct;
}

int main() {
    double pos = basis_of(100.0, 100.5);   // perp premium +0.5%
    double neg = basis_of(100.0,  99.5);   // perp discount -0.5%
    printf("  premium basis=%+.3f%%  discount basis=%+.3f%%\n", pos*100, neg*100);
    if (!(std::fabs(pos - 0.005) < 1e-6)) { printf("  FAIL: expected +0.5%% basis\n"); return 1; }
    if (!(std::fabs(neg + 0.005) < 1e-6)) { printf("  FAIL: expected -0.5%% basis\n"); return 1; }
    printf("  PASS basis sign+magnitude correct (perp premium>0, discount<0)\n");
    return 0;
}
