#pragma once
// ============================================================================
// DrawdownGovernor — item 18 (Phase-3 portfolio unification, 2026-07-11).
//
// BEFORE: a single HARD daily-loss kill (g_daily_kill / g_emergency_halt) — the
// book runs at 100% until a threshold is breached, then slams to 0. There is no
// graded de-risking as a drawdown deepens, and "resume" is an unguarded clock
// expiry — the book can re-arm at full size straight back into the same regime
// that just drew it down.
//
// AFTER: a GRADUATED response — exposure scales 100 -> 75 -> 50 -> 25 -> HALT as
// the drawdown from the equity peak deepens; and once HALTED a RECOVERY STATE is
// required to resume (healthy data + reconciled ledger + reduced risk + operator
// ack) — an equity tick-up alone can NOT auto-resume a halted book.
//
// This is a portfolio-level exposure MULTIPLIER, applied on top of the regime
// exposure — it never edits a validated sleeve's signal/exit logic.
// Header-only, no deps, cheaply unit-testable.
// ============================================================================
#include <algorithm>
#include <cmath>

namespace chimera {

class DrawdownGovernor {
public:
    // Drawdown step thresholds (fraction of peak) -> exposure scale.
    // dd<warn1 ->1.00 | warn1..warn2 ->0.75 | warn2..warn3 ->0.50 |
    // warn3..halt ->0.25 | >=halt -> 0 (HALT, latched).
    void configure(double warn1 = 0.05, double warn2 = 0.10,
                   double warn3 = 0.15, double halt = 0.20) {
        warn1_ = warn1; warn2_ = warn2; warn3_ = warn3; halt_dd_ = halt;
    }

    // Feed the latest portfolio equity. Returns the exposure scale in [0,1].
    // Latches HALT at/above the halt drawdown until try_resume() succeeds.
    double update_equity(double equity) {
        if (equity > peak_) peak_ = equity;
        last_equity_ = equity;
        double dd = drawdown();
        if (dd >= halt_dd_) { halted_ = true; }
        if (halted_) { scale_ = 0.0; return 0.0; }
        if      (dd < warn1_) scale_ = 1.00;
        else if (dd < warn2_) scale_ = 0.75;
        else if (dd < warn3_) scale_ = 0.50;
        else                  scale_ = 0.25;
        return scale_;
    }

    double drawdown() const {
        if (peak_ <= 0.0) return 0.0;
        double dd = (peak_ - last_equity_) / peak_;
        return dd < 0.0 ? 0.0 : dd;
    }
    double exposure_scale() const { return halted_ ? 0.0 : scale_; }
    bool   halted() const { return halted_; }
    double peak()   const { return peak_; }

    // ── Recovery state (required to leave HALT) ──────────────────────────────
    void set_data_healthy(bool v)      { data_healthy_ = v; }
    void set_ledger_reconciled(bool v) { ledger_reconciled_ = v; }
    void set_risk_reduced(bool v)      { risk_reduced_ = v; }
    void set_operator_ack(bool v)      { operator_ack_ = v; }

    bool recovery_ready() const {
        return data_healthy_ && ledger_reconciled_ && risk_reduced_ && operator_ack_;
    }

    // Attempt to leave HALT. Succeeds ONLY when every recovery condition holds.
    // On success un-halts and re-baselines the peak to current equity so the book
    // resumes at a reduced-risk 25% step (dd measured from the new, lower peak),
    // not straight back to 100%. Returns true iff resumed.
    bool try_resume() {
        if (!halted_) return true;
        if (!recovery_ready()) return false;
        halted_ = false;
        peak_ = last_equity_;   // fresh baseline — no snap-back to full size
        scale_ = 0.25;          // resume small; must earn its way back up
        operator_ack_ = false;  // ack is single-use
        return true;
    }

private:
    double warn1_ = 0.05, warn2_ = 0.10, warn3_ = 0.15, halt_dd_ = 0.20;
    double peak_ = 0.0, last_equity_ = 0.0, scale_ = 1.0;
    bool   halted_ = false;
    bool   data_healthy_ = false, ledger_reconciled_ = false,
           risk_reduced_ = false, operator_ack_ = false;
};

} // namespace chimera
