// Phase-7 test 4 — OBSERVATION-ONLY invariant + would-be size multiplier.
// Phase 7's whole point: derivatives data is a SIGNAL only, NEVER traded, and in
// the shipped wiring it changes NOTHING (all 3 filters REJECTED by backtest). This
// pins: (a) eval() is a PURE READ (identical result on repeat, no state mutation);
// (b) the documented size_mult is 1.0 in the neutral/healthy case and only applies
// the (inert, recorded) haircuts on crowded-long / perp-led — proving the layer is
// a size MODIFIER at most, never a new entry and never a short.
#include "core/DerivativesSignals.hpp"
#include <cstdio>
#include <cmath>

using namespace chimera;

int main() {
    DerivativesSignalBook b;
    int64_t t0 = 1'700'000'000'000LL;
    // healthy: mid funding (spread -5bp..+5bp, newest lands mid-range), spot-led, flat basis
    for (int i = 0; i < 100; ++i) b.on_funding("X", t0 + i*28'800'000LL, -0.0005 + 0.0010*(i/99.0));
    b.on_funding("X", t0 + 200*28'800'000LL, 0.0000);   // newest = middle of the range => pct ~0.5
    for (int i = 0; i < 24; ++i) {
        b.on_spot_h1("X", t0 + i*3'600'000LL, 100.0, 100.0, 70.0);   // spot-led
        b.on_perp_h1("X", t0 + i*3'600'000LL, 100.0, 100.0, 40.0);
    }
    auto d1 = b.eval("X");
    auto d2 = b.eval("X");   // pure read — must be identical
    if (d1.funding_pct != d2.funding_pct || d1.cvd_div != d2.cvd_div || d1.basis_pct != d2.basis_pct) {
        printf("  FAIL: eval() not idempotent (mutated state)\n"); return 1; }
    double mh = d1.size_mult(b.params());
    printf("  healthy: pct=%.2f spot_led=%d size_mult=%.2f (idempotent read)\n", d1.funding_pct, d1.spot_led?1:0, mh);
    if (std::fabs(mh - 1.0) > 1e-9) { printf("  FAIL: healthy size_mult should be 1.0\n"); return 1; }

    // crowded-long + perp-led => documented (inert) double haircut 0.75*0.75
    DerivativesSignalBook c;
    for (int i = 0; i < 100; ++i) c.on_funding("X", t0 + i*28'800'000LL, -0.0005 + 0.0010*(i/99.0));
    c.on_funding("X", t0 + 200*28'800'000LL, 0.0009);   // newest = near-max => high pct
    for (int i = 0; i < 24; ++i) {
        c.on_spot_h1("X", t0 + i*3'600'000LL, 100.0, 100.0, 30.0);   // perp-led
        c.on_perp_h1("X", t0 + i*3'600'000LL, 100.0, 100.0, 70.0);
    }
    auto dc = c.eval("X");
    double mc = dc.size_mult(c.params());
    printf("  crowded+perp-led: pct=%.2f spot_led=%d size_mult=%.2f\n", dc.funding_pct, dc.spot_led?1:0, mc);
    if (!(mc < mh)) { printf("  FAIL: crowded/perp-led should haircut below healthy\n"); return 1; }
    if (!(std::fabs(mc - 0.5625) < 1e-6)) { printf("  FAIL: expected 0.75*0.75=0.5625\n"); return 1; }

    printf("  PASS observation-only: eval pure-read; size_mult=1.0 healthy, haircut when crowded/perp-led (never a short)\n");
    return 0;
}
