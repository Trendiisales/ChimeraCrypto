// Phase-7 test 1 — funding percentile.
// The funding-crowding signal is a trailing-window percentile of the 8h rate.
// Verify: an extreme-high current rate lands near pct 1.0, a low rate near 0.0,
// and the not-ready guard holds before any funding is seen. (Backtest verdict:
// funding-extreme filter = NEUTRAL/REJECT — this test pins MECHANICS only.)
#include "core/DerivativesSignals.hpp"
#include <cstdio>
#include <cmath>

using namespace chimera;

int main() {
    DerivativesSignalBook b;
    // not ready before any data
    if (b.eval("BTCUSDT").ready) { printf("  FAIL: ready with no data\n"); return 1; }
    // need spot+perp too for eval() readiness; feed a trivial CVD bar each
    int64_t t0 = 1'700'000'000'000LL;
    // seed 100 funding samples spread -5bp..+5bp, then a high and a low probe
    for (int i = 0; i < 100; ++i) {
        double r = (-0.0005) + 0.0010 * (i / 99.0);
        b.on_funding("BTCUSDT", t0 + i*28'800'000LL, r);
    }
    b.on_spot_h1("BTCUSDT", t0, 100.0, 10.0, 6.0);
    b.on_perp_h1("BTCUSDT", t0, 100.0, 10.0, 5.0);

    // current (last) funding is the max (+5bp) => percentile should be ~1.0
    auto d = b.eval("BTCUSDT");
    if (!d.ready) { printf("  FAIL: not ready after seeding\n"); return 1; }
    printf("  last funding=%.4f%% pct=%.2f\n", d.funding_rate*100, d.funding_pct);
    if (d.funding_pct < 0.95) { printf("  FAIL: extreme-high rate should be high pct\n"); return 1; }

    // now push a very LOW rate as the newest sample => percentile near 0
    b.on_funding("BTCUSDT", t0 + 200*28'800'000LL, -0.0020);
    auto d2 = b.eval("BTCUSDT");
    printf("  after low probe: funding=%.4f%% pct=%.2f\n", d2.funding_rate*100, d2.funding_pct);
    if (d2.funding_pct > 0.10) { printf("  FAIL: low rate should be low pct\n"); return 1; }

    printf("  PASS funding percentile monotone (extreme->1, low->0), not-ready guard holds\n");
    return 0;
}
