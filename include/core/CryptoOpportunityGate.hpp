// ═══════════════════════════════════════════════════════════════════════════
// CryptoOpportunityGate — cost-aware campaign entry gate (13j §2.11)
// ═══════════════════════════════════════════════════════════════════════════
// Second campaign architecture component. v1 is a COST-VIABILITY gate, not a
// forecast: a campaign cell may only enter while the CostLedger's effective RT
// stays at-or-below the cell's VALIDATED stress bound (every wired cell passed
// full re-sims at 30bp and 40bp — CAMPAIGN_LEVERS_2026-07-13.md). If measured
// live costs ever exceed what the backtest survived, the gate blocks entries
// rather than trading an unvalidated cost regime.
//
// DELIBERATELY NOT in v1: a remaining-move forecast hurdle. The obvious static
// proxy (cell median win vs 3x cost) would veto TRX-W8 (medWin 44bp), a cell
// that PASSED the full validation stack — a strawman gate that second-guesses
// the backtest is worse than no gate (feedback-verify-kill-replicates-
// mechanism). A real remaining-move forecast is the documented v2 upgrade;
// the check() signature already carries the slot for it.
#pragma once

namespace chimera {

class CryptoOpportunityGate {
public:
    struct Decision { bool allow; const char* reason; };

    // effective_rt_bp        — CostLedger gating cost for the symbol, now.
    // max_validated_rt_bp    — the highest RT the cell's backtest re-sim passed
    //                          (40bp for all 4 wired campaign cells).
    // retired / rank_out     — book latches: no new entries, ever.
    // expected_remaining_bp  — v2 forecast slot; <=0 means "no forecast" and
    //                          the hurdle is skipped (v1 behaviour).
    Decision check(double effective_rt_bp, double max_validated_rt_bp,
                   bool retired, bool rank_out,
                   double expected_remaining_bp = 0.0,
                   double hurdle_mult = 3.0) const {
        if (retired)  return {false, "RETIRED"};
        if (rank_out) return {false, "RANK_OUT"};
        if (effective_rt_bp > max_validated_rt_bp)
            return {false, "COST_ABOVE_VALIDATED"};
        if (expected_remaining_bp > 0.0 &&
            expected_remaining_bp < hurdle_mult * effective_rt_bp)
            return {false, "REMAINING_BELOW_HURDLE"};
        return {true, "OK"};
    }
};

} // namespace chimera
