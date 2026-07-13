// ═══════════════════════════════════════════════════════════════════════════
// CryptoCostLedger — measured per-symbol effective execution cost (13j §2.11)
// ═══════════════════════════════════════════════════════════════════════════
// First of the campaign architecture components (operator directive
// SESSION_HANDOFF_2026-07-13j §2.11: CostLedger + OpportunityGate +
// CampaignManager). Centralizes what was previously scattered
// round_trip_bp=20.0 / fee=0.001 literals:
//
//   net_rt_bp(sym)       — the RT cost charged against a book's net PnL.
//                          Defaults to the validated-cell basis (20bp Binance
//                          spot taker RT) so shadow books stay comparable to
//                          the backtests that gated them.
//   effective_rt_bp(sym) — the GATING cost: known RT + measured (or default)
//                          slip + uncertainty reserve. This is what the
//                          OpportunityGate compares against a cell's validated
//                          stress bound (13j §2.7: "Gate uses measured costs").
//   record_measured_slip — live measurement hook (per-fill slip in bp). In
//                          SHADOW there are no real fills, so the default slip
//                          stands until the executor feeds real measurements.
//
// Research basis: campaign cells validated at RT 20bp with full re-sims at
// 30bp and 40bp (backtest/CAMPAIGN_LEVERS_2026-07-13.md in the Crypto repo);
// funding-equation reserve terms slip=3bp, reserve=2bp.
#pragma once

#include <string>
#include <map>
#include <sstream>
#include <iomanip>

namespace chimera {

class CryptoCostLedger {
public:
    struct SymCost {
        double known_rt_bp = 20.0;   // Binance spot taker RT (0.10%/side)
        double slip_bp     = 3.0;    // default until measured
        double reserve_bp  = 2.0;    // uncertainty reserve (13j funding equation)
        int    n_measured  = 0;      // real-fill slip samples folded in
        double slip_sum    = 0.0;    // running sum of measured slip bp
    };

    void configure(const std::string& sym, double known_rt_bp,
                   double slip_bp, double reserve_bp) {
        auto& c = costs_[sym];
        c.known_rt_bp = known_rt_bp; c.slip_bp = slip_bp; c.reserve_bp = reserve_bp;
    }

    // RT cost charged against net PnL — the validated-cell basis.
    double net_rt_bp(const std::string& sym) const {
        auto it = costs_.find(sym);
        return it != costs_.end() ? it->second.known_rt_bp : dflt_.known_rt_bp;
    }

    // Gating cost: known + measured-or-default slip + reserve.
    double effective_rt_bp(const std::string& sym) const {
        const SymCost& c = get_(sym);
        const double slip = c.n_measured > 0 ? c.slip_sum / c.n_measured : c.slip_bp;
        return c.known_rt_bp + slip + c.reserve_bp;
    }

    // Live measurement hook: per-fill realized slip in bp. Shadow: unused
    // (no real fills); the executor calls this once campaigns route orders.
    void record_measured_slip(const std::string& sym, double slip_bp) {
        auto& c = costs_[sym];
        c.slip_sum += slip_bp; c.n_measured++;
    }

    std::string state_json() const {
        std::ostringstream js; js << std::fixed << std::setprecision(2) << "[";
        bool first = true;
        for (const auto& kv : costs_) {
            if (!first) js << ","; first = false;
            js << "{\"sym\":\"" << kv.first << "\",\"eff_rt_bp\":" << effective_rt_bp(kv.first)
               << ",\"net_rt_bp\":" << kv.second.known_rt_bp
               << ",\"n_measured\":" << kv.second.n_measured << "}";
        }
        js << "]";
        return js.str();
    }

private:
    const SymCost& get_(const std::string& sym) const {
        auto it = costs_.find(sym);
        return it != costs_.end() ? it->second : dflt_;
    }
    std::map<std::string, SymCost> costs_;
    SymCost dflt_;
};

} // namespace chimera
