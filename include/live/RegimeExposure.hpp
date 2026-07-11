#pragma once
// ============================================================================
// RegimeExposure — item 19 (Phase-3 portfolio unification, 2026-07-11).
//
// BEFORE: ONE binary macro rule for every sleeve — BTC>200d => full size, else
// CASH (XSec's macro_gate, RipRider's regime-exit). A single 200d cross flips
// the WHOLE book on/off; every family gets the identical gate regardless of what
// it actually needs; and there is no graded response between "risk-on" and "flat".
//
// AFTER: breadth (share of the universe with positive trailing momentum) maps to
// a CONTINUOUS exposure multiplier in [0,1] via the R2 bands, hysteresis-smoothed
// so it does not oscillate at a band boundary, with a hard defensive override
// (severe data / liquidity / drawdown alarm => 0%). The multiplier is then made
// FAMILY-SPECIFIC (R2 table): XSec reads breadth+dispersion; RipRider keeps a
// STRONG regime requirement (goes flat below a firm breadth floor); UpJump permits
// a WEAKER macro but reduces size.
//
// NO 200DMA anywhere (standing crypto rule). Breadth + dispersion are the inputs.
// Header-only, no deps, cheaply unit-testable.
// ============================================================================
#include <algorithm>
#include <cmath>

namespace chimera {

enum class Family { XSEC, RIPRIDER, UPJUMP, EDGE, OTHER };

inline const char* family_str(Family f) {
    switch (f) {
        case Family::XSEC:     return "XSEC";
        case Family::RIPRIDER: return "RIPRIDER";
        case Family::UPJUMP:   return "UPJUMP";
        case Family::EDGE:     return "EDGE";
        case Family::OTHER:    return "OTHER";
    }
    return "?";
}

class RegimeExposure {
public:
    // hysteresis = the breadth dead-band the current level holds through, so a
    // small wiggle across a band boundary does NOT move exposure (no oscillation).
    void configure(double hysteresis = 0.05) { hysteresis_ = hysteresis; }

    // ── R2 continuous breadth->exposure curve ────────────────────────────────
    // Bands (R2):  <25% -> 0-10% | 25-45% -> 20-35% | 45-65% -> 50-70% | >65% -> 80-95%.
    // Rendered as a CONTINUOUS, monotonic piecewise-linear curve through the band
    // centres so exposure rises smoothly with breadth instead of stepping.
    static double raw_curve(double breadth) {
        breadth = std::clamp(breadth, 0.0, 1.0);
        // anchor points: (breadth, exposure) — band centres, endpoints pinned.
        static const double bx[] = {0.00, 0.125, 0.350, 0.550, 0.825, 1.00};
        static const double by[] = {0.00, 0.050, 0.275, 0.600, 0.875, 0.95};
        const int N = 6;
        if (breadth <= bx[0])   return by[0];
        if (breadth >= bx[N-1]) return by[N-1];
        for (int i = 1; i < N; ++i) {
            if (breadth <= bx[i]) {
                double t = (breadth - bx[i-1]) / (bx[i] - bx[i-1]);
                return by[i-1] + t * (by[i] - by[i-1]);
            }
        }
        return by[N-1];
    }

    // Global exposure multiplier from breadth, with hysteresis + hard override.
    // severe_alarm (bad data / illiquidity / DD alarm) forces 0% immediately.
    double update(double breadth, bool severe_alarm) {
        if (severe_alarm) { out_ = 0.0; held_breadth_ = breadth; init_ = true; return 0.0; }
        if (!init_ || std::fabs(breadth - held_breadth_) >= hysteresis_) {
            held_breadth_ = breadth;   // move the anchor only on a real breadth move
            init_ = true;
        }
        out_ = raw_curve(held_breadth_);
        return out_;
    }

    double exposure() const { return out_; }
    double held_breadth() const { return held_breadth_; }

    // ── FAMILY-SPECIFIC gate (R2 table) ──────────────────────────────────────
    // Applies the family's own regime requirement on top of the global breadth
    // exposure. dispersion = cross-sectional spread of momentum (XSec needs it;
    // a low-dispersion tape means the cross-section has no edge to harvest).
    //   XSEC     : breadth * dispersion-scaled (needs both breadth AND dispersion).
    //   RIPRIDER : STRONG regime requirement — flat below a firm breadth floor,
    //              full curve above it (keeps the tail; no half-measures).
    //   UPJUMP   : permits a WEAKER macro (fires at lower breadth) but REDUCES
    //              size (0.6x) so a weak-macro entry rides small.
    //   EDGE/OTHER: the plain continuous curve.
    double family_exposure(Family fam, double breadth, double dispersion,
                           bool severe_alarm) {
        double g = update(breadth, severe_alarm);   // global, hysteresis-smoothed
        if (severe_alarm) return 0.0;
        switch (fam) {
            case Family::XSEC: {
                // needs cross-sectional dispersion to have something to rank.
                double d = std::clamp(dispersion / xsec_disp_ref_, 0.0, 1.0);
                return g * (0.5 + 0.5 * d);          // 0.5x..1.0x of curve on dispersion
            }
            case Family::RIPRIDER:
                // strong requirement: nothing below the floor, full curve above.
                return (held_breadth_ >= rip_breadth_floor_) ? g : 0.0;
            case Family::UPJUMP:
                // weaker macro tolerated, smaller size.
                return g * upjump_size_ + upjump_floor_bonus(breadth);
            case Family::EDGE:
            case Family::OTHER:
            default:
                return g;
        }
    }

    // tunables (research starting points; NOT signal logic)
    double rip_breadth_floor_ = 0.45;   // RipRider stays flat below this breadth
    double upjump_size_       = 0.60;   // UpJump reduced size under weak macro
    double xsec_disp_ref_     = 0.50;   // dispersion that fully satisfies XSec

private:
    // UpJump keeps a small floor of exposure even at low breadth (weaker macro
    // permitted) but capped so it can never exceed its reduced size envelope.
    double upjump_floor_bonus(double breadth) const {
        if (breadth >= 0.25) return 0.0;
        return 0.05 * (breadth / 0.25);   // up to +5% at the 25% edge
    }
    double hysteresis_   = 0.05;
    double held_breadth_ = 0.0;
    double out_          = 0.0;
    bool   init_         = false;
};

} // namespace chimera
