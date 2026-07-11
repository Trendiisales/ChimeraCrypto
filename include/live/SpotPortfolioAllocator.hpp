#pragma once
// ============================================================================
// SpotPortfolioAllocator — items 15 + 16 (Phase-3 portfolio unification, 2026-07-11).
//
// "Strategies produce TARGETS, not orders."  BEFORE, every sleeve sized off the
// same max_position_usd and fired a raw order straight at the gateway: XSec, the
// UpJump parents, RipRider and the EdgeEngines could each independently target the
// SAME coin, so the book took 3-4x the intended exposure to one name, momentum
// sleeves that are really ONE factor were double-counted as diversification, and
// nothing reconciled the desired target against what the exchange actually holds.
//
// AFTER, each production sleeve DECLARES a desired (symbol, target_usd, factor,
// family) with the allocator instead of ordering directly. The allocator, per
// symbol:
//   1. MERGES overlapping per-symbol targets across all strategies,           [15]
//   2. applies the FAMILY regime exposure + global drawdown scale,            [18,19]
//   3. caps AGGREGATE momentum-factor exposure (XSec/TSMOM/UpJump/RipRider are
//      ONE factor — not independent diversification),                          [16]
//   4. applies the portfolio vol-target / cluster / crypto-beta risk scale,   [17]
//   5. applies ONE symbol-level cap AFTER the merge,                          [15]
//   6. computes the DELTA vs the ExchangeLedger actual + pending,             [15]
//   7. emits the netted order delta to gateway.submit.                        [15]
//
// TRACK-ONLY (default, SHADOW): plan() COMPUTES + LOGS the full merged / capped /
// netted vector so the entire layer is exercised and tested, but does NOT emit —
// the existing per-sleeve shadow books and the 32-cell UpJump threshold GRID keep
// their own records untouched (the grid cells never register a target, so they are
// preserved by construction). ENFORCE (go-live flag) actually emits the netted
// deltas and suppresses the raw per-sleeve orders.
//
// It NEVER edits a validated sleeve's signal/exit logic — only the target->order
// step between them. Header-only; the emit sink is a std::function so it is unit-
// testable with no gateway.
// ============================================================================
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "live/RegimeExposure.hpp"
#include "live/DrawdownGovernor.hpp"
#include "live/PortfolioRisk.hpp"
#include "live/ExchangeLedger.hpp"

namespace chimera {

enum class Factor { MOMENTUM, MEANREV, OTHER };
inline const char* factor_str(Factor f) {
    switch (f) { case Factor::MOMENTUM: return "MOMENTUM";
                 case Factor::MEANREV:  return "MEANREV";
                 default:               return "OTHER"; }
}

// A strategy's DESIRED exposure to a symbol (a target, not an order).
struct StrategyTarget {
    std::string strategy_id;
    std::string symbol;         // upper exchange symbol, e.g. "SOLUSDT"
    double      target_usd = 0.0;
    Factor      factor  = Factor::OTHER;
    Family      family  = Family::OTHER;
};

// The netted order the allocator would place for a symbol after all merging/caps.
struct AllocDelta {
    std::string symbol;
    bool        is_buy = true;
    double      usd    = 0.0;   // notional of the delta
    double      qty    = 0.0;   // base qty at ref_px
    double      merged_usd = 0.0;   // pre-cap merged target
    double      capped_usd = 0.0;   // post-cap desired target
    double      held_usd   = 0.0;   // ledger actual + pending
    const char* note   = "";
};

class SpotPortfolioAllocator {
public:
    // enforce=false => TRACK-ONLY (compute + log, never emit). SHADOW default.
    void configure(bool enforce, double symbol_cap_usd,
                   double momentum_cap_usd, double target_portfolio_vol,
                   double max_cluster_frac, double max_crypto_beta) {
        enforce_ = enforce;
        symbol_cap_usd_ = symbol_cap_usd;
        momentum_cap_usd_ = momentum_cap_usd;
        target_vol_ = target_portfolio_vol;
        max_cluster_frac_ = max_cluster_frac;
        max_crypto_beta_ = max_crypto_beta;
    }
    bool enforce() const { return enforce_; }

    // Overlay toggles — the base MERGE/CAP/NET (item 15) always runs; the exposure
    // overlays can be enabled independently. Default: all overlays ON. Turning them
    // off yields the pure item-15 netting (family mult = dd scale = risk scale = 1).
    void set_regime_overlay(bool v)   { regime_overlay_ = v; }
    void set_drawdown_overlay(bool v) { dd_overlay_ = v; }
    void set_momentum_overlay(bool v) { momentum_overlay_ = v; }
    void set_risk_overlay(bool v)     { risk_overlay_ = v; }

    RegimeExposure&  regime()   { return regime_; }
    DrawdownGovernor& drawdown(){ return dd_; }
    PortfolioRisk&   risk()     { return risk_; }

    // The gateway sink (ENFORCE only). Signature mirrors the OrderIntent fields the
    // caller needs. In TRACK-ONLY this is never invoked.
    std::function<void(const AllocDelta&, Factor, Family)> emit;

    // Reference-price + cluster providers so the allocator can value positions and
    // group symbols for the cluster cap.
    std::function<double(const std::string&)> ref_px;      // symbol -> last px (0 if unknown)
    std::function<int(const std::string&)>    cluster_of;  // symbol -> cluster id
    std::string benchmark = "BTCUSDT";                      // crypto-beta reference

    // Regime inputs (updated by the caller each cycle).
    void set_regime_inputs(double breadth, double dispersion, bool severe_alarm) {
        breadth_ = breadth; dispersion_ = dispersion; severe_ = severe_alarm;
    }

    // ── strategies DECLARE targets (replace-on-write per strategy+symbol) ─────
    void set_target(const std::string& strategy_id, const std::string& symbol,
                    double target_usd, Factor factor, Family family) {
        Key k{strategy_id, symbol};
        if (target_usd <= 0.0) { targets_.erase(k); return; }
        targets_[k] = StrategyTarget{strategy_id, symbol, target_usd, factor, family};
    }
    void clear_target(const std::string& strategy_id, const std::string& symbol) {
        targets_.erase(Key{strategy_id, symbol});
    }
    size_t num_targets() const { return targets_.size(); }

    // ── the plan: merge -> family regime -> momentum-factor cap -> risk scale ->
    // symbol cap -> net vs ledger -> emit (ENFORCE) / log (TRACK-ONLY). ──────────
    std::vector<AllocDelta> plan(const ExchangeLedger* ledger) {
        // 1. family regime + drawdown scale applied to EACH target, then merge by
        //    symbol. Track per-symbol dominant factor/family for downstream steps.
        double dd_scale = dd_overlay_ ? dd_.exposure_scale() : 1.0;
        std::map<std::string,double> merged;                 // symbol -> merged usd
        std::map<std::string,Factor> sym_factor;
        std::map<std::string,Family> sym_family;
        double momentum_total = 0.0;
        for (auto& kv : targets_) {
            const StrategyTarget& t = kv.second;
            double fam_mult = regime_overlay_
                ? regime_.family_exposure(t.family, breadth_, dispersion_, severe_) : 1.0;
            double eff = t.target_usd * fam_mult * dd_scale;
            merged[t.symbol] += eff;
            // record the dominant (largest-contribution) factor/family per symbol.
            if (eff >= per_sym_max_[t.symbol]) { per_sym_max_[t.symbol] = eff;
                sym_factor[t.symbol] = t.factor; sym_family[t.symbol] = t.family; }
            if (t.factor == Factor::MOMENTUM) momentum_total += eff;
        }
        per_sym_max_.clear();

        // 2. MOMENTUM-FACTOR aggregate cap (item 16): if the summed momentum
        //    exposure exceeds the factor cap, scale EVERY momentum symbol down
        //    proportionally (one factor, not independent diversification).
        double mom_scale = 1.0;
        if (momentum_overlay_ && momentum_cap_usd_ > 0.0 && momentum_total > momentum_cap_usd_)
            mom_scale = momentum_cap_usd_ / momentum_total;
        if (mom_scale < 1.0)
            for (auto& kv : merged)
                if (sym_factor[kv.first] == Factor::MOMENTUM) kv.second *= mom_scale;

        // 3. PORTFOLIO RISK scale (item 17): vol-target x cluster-cap x beta-cap on
        //    the merged weight vector. Computed on the ACTUAL rolling covariance;
        //    1.0 while the covariance is still warming.
        std::map<std::string,double> risk_adj = risk_overlay_ ? apply_risk(merged) : merged;

        // 4. ONE symbol-level cap AFTER merge, then net vs the ledger.
        std::vector<AllocDelta> out;
        for (auto& kv : risk_adj) {
            const std::string& sym = kv.first;
            double capped = kv.second;
            if (symbol_cap_usd_ > 0.0) capped = std::min(capped, symbol_cap_usd_);

            double px = ref_px ? ref_px(sym) : 0.0;
            double held = 0.0;
            if (ledger && px > 0.0)
                held = ledger->position_value(sym, px) + ledger->pending_buy_value(sym);

            double delta_usd = capped - held;
            AllocDelta d;
            d.symbol = sym; d.merged_usd = merged[sym]; d.capped_usd = capped;
            d.held_usd = held; d.is_buy = delta_usd >= 0.0;
            d.usd = std::fabs(delta_usd);
            d.qty = (px > 0.0) ? d.usd / px : 0.0;
            out.push_back(d);

            std::fprintf(stderr,
                "[ALLOC%s] %s merged=$%.2f cap=$%.2f held=$%.2f -> %s $%.2f (qty=%.8f) "
                "[mom_scale=%.3f dd=%.2f regime=%s]\n",
                enforce_ ? "" : "-TRACK", sym.c_str(), d.merged_usd, d.capped_usd,
                d.held_usd, d.is_buy ? "BUY" : "SELL", d.usd, d.qty,
                mom_scale, dd_scale, family_str(sym_family[sym]));

            // 5. emit ONLY in ENFORCE; TRACK-ONLY leaves the shadow books + grid
            //    experiment completely undisturbed.
            if (enforce_ && emit && d.usd > 0.0 && px > 0.0)
                emit(d, sym_factor[sym], sym_family[sym]);
        }
        return out;
    }

    // Risk scale exposed for testing / logging.
    std::map<std::string,double> apply_risk(std::map<std::string,double> w) const {
        if (w.empty()) return w;
        // cluster cap first (reshapes weights), then vol-target + beta scalars.
        if (cluster_of && max_cluster_frac_ > 0.0) {
            std::map<std::string,int> cm;
            for (auto& kv : w) cm[kv.first] = cluster_of(kv.first);
            w = risk_.cap_clusters(w, cm, max_cluster_frac_);
        }
        double s = 1.0;
        if (target_vol_ > 0.0)      s *= risk_.vol_target_scale(w, target_vol_);
        if (max_crypto_beta_ > 0.0) s *= risk_.crypto_beta_scale(w, benchmark, max_crypto_beta_);
        if (s < 1.0) for (auto& kv : w) kv.second *= s;
        return w;
    }

private:
    struct Key { std::string sid, sym;
        bool operator<(const Key& o) const {
            return sid != o.sid ? sid < o.sid : sym < o.sym; } };

    bool   enforce_ = false;
    double symbol_cap_usd_   = 0.0;
    double momentum_cap_usd_ = 0.0;
    double target_vol_       = 0.0;
    double max_cluster_frac_ = 0.0;
    double max_crypto_beta_  = 0.0;
    double breadth_ = 1.0, dispersion_ = 1.0; bool severe_ = false;
    bool   regime_overlay_ = true, dd_overlay_ = true,
           momentum_overlay_ = true, risk_overlay_ = true;

    std::map<Key, StrategyTarget> targets_;
    mutable std::map<std::string,double> per_sym_max_;
    RegimeExposure   regime_;
    DrawdownGovernor dd_;
    PortfolioRisk    risk_;
};

} // namespace chimera
