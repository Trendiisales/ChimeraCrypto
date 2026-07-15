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
        // Phase-2 (§2) fee decomposition + depth-adjusted slip reserves. These
        // feed safe_cost_bps(); default so effective_rt_bp() is unchanged.
        double buy_fee_bp   = 10.0;  // taker/side; maker entry can lower this
        double sell_fee_bp  = 10.0;  // taker/side (exit is aggressive per §8)
        double entry_slip99_bp = 0.0;// p99 buy-slip at campaign Q (depth model)
        double exit_slip99_bp  = 0.0;// p99 sell-slip at campaign Q (depth model)
        double spread_bp    = 2.0;   // spread/market-impact allowance (§2)
        double latency_bp   = 5.0;   // latency allowance (§2)
        double dust_bp      = 1.0;   // rounding/dust allowance (§2)
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

    // ── Phase-2 (§2) depth-adjusted safe cost ────────────────────────────────
    // Feed the p99 entry/exit slippage measured by DepthLiquidationModel at the
    // campaign's actual quantity into the ledger. (Depth model lives separately
    // so the ledger stays data-source-agnostic.)
    void set_depth_slip(const std::string& sym, double entry_slip99_bp,
                        double exit_slip99_bp) {
        auto& c = costs_[sym];
        c.entry_slip99_bp = entry_slip99_bp; c.exit_slip99_bp = exit_slip99_bp;
    }
    void set_fees(const std::string& sym, double buy_fee_bp, double sell_fee_bp) {
        auto& c = costs_[sym]; c.buy_fee_bp = buy_fee_bp; c.sell_fee_bp = sell_fee_bp;
    }

    // The spec §2 authoritative safe cost: NOT the flat 20bp. Sums the fee
    // decomposition + p99 depth-adjusted entry & exit slip + spread/latency/dust
    // reserves. This is what the CORE break-even / campaign floor arithmetic uses
    // (CoreNetLiquidationPnL clears break-even only when > this).
    double safe_cost_bps(const std::string& sym) const {
        const SymCost& c = get_(sym);
        return c.buy_fee_bp + c.sell_fee_bp
             + c.entry_slip99_bp + c.exit_slip99_bp
             + c.spread_bp + c.latency_bp + c.dust_bp;
    }

    // Pure combiner form for callers that carry slip locally (e.g. a backtest
    // walking the depth book per trade) without mutating ledger state.
    double safe_cost_bps(const std::string& sym, double entry_slip_bp,
                         double exit_slip_bp) const {
        const SymCost& c = get_(sym);
        return c.buy_fee_bp + c.sell_fee_bp + entry_slip_bp + exit_slip_bp
             + c.spread_bp + c.latency_bp + c.dust_bp;
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
