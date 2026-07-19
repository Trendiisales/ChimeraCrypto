// regime_exposure_test.cpp — Phase-3 item 19.
// Breadth -> a CONTINUOUS exposure multiplier (R2 bands), hysteresis-smoothed so
// it does not oscillate at a band boundary; a severe alarm forces 0%. Family-
// specific gates: RipRider needs a STRONG regime (flat below a floor); Mimic
// permits a WEAKER macro but at reduced size.
#include "live/RegimeExposure.hpp"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace chimera;
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL: %s (line %d)\n", #c, __LINE__); ++fails; } }while(0)

int main() {
    // ── continuous + monotonic curve across the R2 bands ────────────────────
    double prev = -1;
    for (double b = 0.0; b <= 1.0001; b += 0.05) {
        double e = RegimeExposure::raw_curve(b);
        CHECK(e >= -1e-9 && e <= 0.95 + 1e-9);       // bounded, never 100% deployed
        CHECK(e >= prev - 1e-9);                      // monotonic non-decreasing
        prev = e;
    }
    // representative band levels (R2): low breadth ~ small, high breadth ~ large.
    CHECK(RegimeExposure::raw_curve(0.10) < 0.15);    // <25% band -> single digits
    CHECK(RegimeExposure::raw_curve(0.55) > 0.45 && RegimeExposure::raw_curve(0.55) < 0.75); // 45-65%
    CHECK(RegimeExposure::raw_curve(0.90) > 0.80);    // >65% band -> 80-95%

    // ── hysteresis: alternating breadth ACROSS the 0.45 boundary must NOT
    //    oscillate the exposure (dead-band = 0.05 > the 0.02 wiggle). ──────────
    RegimeExposure R; R.configure(/*hyst*/0.05);
    std::vector<double> seq = {0.44, 0.46, 0.44, 0.46, 0.44, 0.46};
    double first = R.update(seq[0], false);
    int flips = 0; double last = first, lastdir = 0;
    for (size_t i = 1; i < seq.size(); ++i) {
        double e = R.update(seq[i], false);
        double dir = e - last;
        if (std::fabs(dir) > 1e-12 && lastdir != 0 && ((dir>0) != (lastdir>0))) ++flips;
        if (std::fabs(dir) > 1e-12) lastdir = dir;
        last = e;
    }
    std::printf("[info] boundary-straddle exposure held=%.4f flips=%d\n", first, flips);
    CHECK(flips == 0);                                // no oscillation at the boundary
    // with a 0.05 dead-band and 0.02 wiggles, exposure stays put after the anchor.
    CHECK(std::fabs(R.exposure() - first) < 1e-9);

    // a REAL breadth move (beyond the dead-band) DOES move exposure.
    double up = R.update(0.80, false);
    CHECK(up > first);

    // ── severe alarm -> hard 0% override ────────────────────────────────────
    CHECK(R.update(0.90, /*severe*/true) == 0.0);

    // ── family-specific gates ───────────────────────────────────────────────
    RegimeExposure F; F.configure(0.0);
    // RipRider: STRONG requirement -> flat below the breadth floor, live above.
    double rip_lo = F.family_exposure(Family::RIPRIDER, 0.30, 0.5, false);   // below 0.45 floor
    double rip_hi = F.family_exposure(Family::RIPRIDER, 0.70, 0.5, false);   // above floor
    std::printf("[info] RipRider lo=%.3f hi=%.3f\n", rip_lo, rip_hi);
    CHECK(rip_lo == 0.0);
    CHECK(rip_hi > 0.0);
    // Mimic: permits a weaker macro but at REDUCED size vs the raw curve.
    double uj = F.family_exposure(Family::MIMIC, 0.55, 0.5, false);
    double raw = RegimeExposure::raw_curve(0.55);
    std::printf("[info] Mimic=%.3f raw=%.3f\n", uj, raw);
    CHECK(uj < raw);                                   // reduced size
    CHECK(uj > 0.0);
    // Mimic still fires at a low breadth where RipRider is flat (weaker macro OK).
    double uj_lo = F.family_exposure(Family::MIMIC, 0.30, 0.5, false);
    CHECK(uj_lo > 0.0 && F.family_exposure(Family::RIPRIDER, 0.30, 0.5, false) == 0.0);
    // XSec needs dispersion: low dispersion -> smaller exposure than high dispersion.
    double xs_lowdisp  = F.family_exposure(Family::XSEC, 0.70, 0.05, false);
    double xs_highdisp = F.family_exposure(Family::XSEC, 0.70, 0.50, false);
    CHECK(xs_lowdisp < xs_highdisp);
    // severe alarm zeroes every family.
    CHECK(F.family_exposure(Family::XSEC, 0.90, 0.9, true) == 0.0);

    std::printf(fails==0 ? "PASS: continuous regime exposure + hysteresis + family gates\n"
                         : "FAILED (%d)\n", fails);
    return fails==0?0:1;
}
