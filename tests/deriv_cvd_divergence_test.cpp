// Phase-7 test 2 — spot-vs-perp CVD divergence (the STRONGEST-of-three signal).
// CVD per bar = (2*taker_buy_base - volume)/volume. cvd_div = spot_cvd - perp_cvd.
// Verify: spot buying-dominant while perp selling-dominant => cvd_div>0 (spot-led);
// the reverse => perp-led. (Backtest verdict: CVD does NOT rank quality monotonically
// => REJECTED as a filter, wired observation-only. This test pins the MECHANIC.)
#include "core/DerivativesSignals.hpp"
#include <cstdio>

using namespace chimera;

static DerivSignal build(double spot_tbb_frac, double perp_tbb_frac) {
    DerivativesSignalBook b;
    int64_t t0 = 1'700'000'000'000LL;
    b.on_funding("X", t0, 0.0001);
    for (int i = 0; i < 24; ++i) {
        double vol = 100.0;
        b.on_spot_h1("X", t0 + i*3'600'000LL, 100.0, vol, vol*spot_tbb_frac);
        b.on_perp_h1("X", t0 + i*3'600'000LL, 100.0, vol, vol*perp_tbb_frac);
    }
    return b.eval("X");
}

int main() {
    // spot heavily bought (80% taker-buy), perp heavily sold (20% taker-buy) => spot-led
    auto led = build(0.80, 0.20);
    printf("  spot-led case : cvd_div=%+.3f spot_led=%d\n", led.cvd_div, led.spot_led?1:0);
    if (!(led.cvd_div > 0 && led.spot_led)) { printf("  FAIL: expected spot-led (cvd_div>0)\n"); return 1; }

    // reverse: perp heavily bought, spot heavily sold => perp-led
    auto perp = build(0.20, 0.80);
    printf("  perp-led case : cvd_div=%+.3f spot_led=%d\n", perp.cvd_div, perp.spot_led?1:0);
    if (!(perp.cvd_div < 0 && !perp.spot_led)) { printf("  FAIL: expected perp-led (cvd_div<0)\n"); return 1; }

    // balanced => near zero
    auto bal = build(0.50, 0.50);
    printf("  balanced case : cvd_div=%+.3f\n", bal.cvd_div);
    if (!(bal.cvd_div > -0.01 && bal.cvd_div < 0.01)) { printf("  FAIL: balanced should be ~0\n"); return 1; }

    printf("  PASS spot-led/perp-led CVD divergence sign correct\n");
    return 0;
}
