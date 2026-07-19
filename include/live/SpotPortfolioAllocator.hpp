#pragma once
// ============================================================================
// SpotPortfolioAllocator — items 15 + 16 (Phase-3 portfolio unification, 2026-07-11).
//
// "Strategies produce TARGETS, not orders."  BEFORE, every sleeve sized off the
// same max_position_usd and fired a raw order straight at the gateway: XSec, the
// Mimic parents, RipRider and the EdgeEngines could each independently target the
// SAME coin, so the book took 3-4x the intended exposure to one name, momentum
// sleeves that are really ONE factor were double-counted as diversification, and
// nothing reconciled the desired target against what the exchange actually holds.
//
// AFTER, each production sleeve DECLARES a desired (symbol, target_usd, factor,
// family) with the allocator instead of ordering directly. The allocator, per
// symbol:
//   1. MERGES overlapping per-symbol targets across all strategies,           [15]
//   2. applies the FAMILY regime exposure + global drawdown scale,            [18,19]
//   3. caps AGGREGATE momentum-factor exposure (XSec/TSMOM/Mimic/RipRider are
//      ONE factor — not independent diversification),                          [16]
//   4. applies the portfolio vol-target / cluster / crypto-beta risk scale,   [17]
//   5. applies ONE symbol-level cap AFTER the merge,                          [15]
//   6. computes the DELTA vs the ExchangeLedger actual + pending,             [15]
//   7. emits the netted order delta to gateway.submit.                        [15]
//
// TRACK-ONLY (default, SHADOW): plan() COMPUTES + LOGS the full merged / capped /
// netted vector so the entire layer is exercised and tested, but does NOT emit —
// the existing per-sleeve shadow books and the 32-cell Mimic threshold GRID keep
// their own records untouched (the grid cells never register a target, so they are
// preserved by construction). ENFORCE (go-live flag) actually emits the netted
// deltas and suppresses the raw per-sleeve orders.
//
// It NEVER edits a validated sleeve's signal/exit logic — only the target->order
// step between them. Header-only; the emit sink is a std::function so it is unit-
// testable with no gateway.
//
// Phase-8A Stage-2 (2026-07-11) — HARD-CAP ENFORCEMENT (staged progression, NOT
// full sizing authority). A THIRD mode sits between TRACK-ONLY (off) and the
// full plan()-emit path (full): HARDCAP. In hardcap the engines still PROPOSE
// their own quantity; the allocator does NOT set the final size (that stays
// Stage-3). It only acts as a SAFETY BACKSTOP — govern_entry() can REDUCE (to the
// remaining headroom) or REJECT (no headroom) a proposed BUY that would breach a
// hard cap: the per-symbol cap, the aggregate momentum-factor cap, and drawdown
// scaling (the DD governor's exposure_scale multiplies both caps, so entries
// shrink in a drawdown and are rejected on HALT). Cash is deliberately NOT
// enforced here in shadow — it is gated on portfolio_cash_usd>0 at the caller
// (go-live only). CRITICAL invariant for promotion (no erroneous rejections): an
// IN-LIMIT order returns byte-identical to what the sleeve proposed, so hardcap
// is indistinguishable from track-only for every order that does not genuinely
// breach a cap; only a true breach reduces/rejects. Exits are never governed.
// ============================================================================
#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <cstdio>
#include <cstring>
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

// Enforcement stage (Phase-8A). OFF = TRACK-ONLY (compute+log, never touch an
// order). HARDCAP = Stage-2 safety backstop (govern_entry reduces/rejects only a
// genuine cap breach; in-limit orders pass unchanged). FULL = Stage-3+ plan()
// emits the netted deltas and the raw per-sleeve orders defer to it.
enum class EnforceMode { OFF, HARDCAP, FULL };
inline const char* enforce_mode_str(EnforceMode m) {
    switch (m) { case EnforceMode::HARDCAP: return "HARDCAP";
                 case EnforceMode::FULL:    return "FULL";
                 default:                   return "OFF"; }
}

// The verdict govern_entry() returns for a proposed BUY entry (Phase-8A Stage-2).
// approved=false => REJECT (submit nothing). reduced=true => qty/usd were cut to
// the cap headroom. Neither set => PASS unchanged (byte-identical to track-only).
struct CapDecision {
    bool        approved     = true;
    bool        reduced      = false;
    double      approved_usd = 0.0;   // notional the allocator permits
    double      approved_qty = 0.0;   // base qty at ref_px
    double      proposed_usd = 0.0;
    double      proposed_qty = 0.0;
    const char* reason       = "pass";
};

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
    // (Back-compat: bool maps OFF/FULL. Phase-8A adds set_enforce_mode() for the
    //  three-way OFF | HARDCAP | FULL.)
    void configure(bool enforce, double symbol_cap_usd,
                   double momentum_cap_usd, double target_portfolio_vol,
                   double max_cluster_frac, double max_crypto_beta) {
        emode_ = enforce ? EnforceMode::FULL : EnforceMode::OFF;
        symbol_cap_usd_ = symbol_cap_usd;
        momentum_cap_usd_ = momentum_cap_usd;
        target_vol_ = target_portfolio_vol;
        max_cluster_frac_ = max_cluster_frac;
        max_crypto_beta_ = max_crypto_beta;
    }
    // Phase-8A: set the exact enforcement stage. FULL routes plan()'s emit path;
    // HARDCAP arms govern_entry() (Stage-2 safety caps); OFF = track-only.
    void set_enforce_mode(EnforceMode m) { emode_ = m; }
    EnforceMode enforce_mode() const { return emode_; }
    // FULL-emit path (plan()) keys on this; HARDCAP does NOT emit from plan().
    bool enforce() const { return emode_ == EnforceMode::FULL; }

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
                enforce() ? "" : "-TRACK", sym.c_str(), d.merged_usd, d.capped_usd,
                d.held_usd, d.is_buy ? "BUY" : "SELL", d.usd, d.qty,
                mom_scale, dd_scale, family_str(sym_family[sym]));

            // 5. emit ONLY in FULL; TRACK-ONLY / HARDCAP leave the shadow books +
            //    grid experiment completely undisturbed (HARDCAP acts at the
            //    sleeve's own submit via govern_entry, not from plan()).
            if (enforce() && emit && d.usd > 0.0 && px > 0.0)
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

    // ── Phase-8A Stage-2 HARD-CAP governor ────────────────────────────────────
    // Evaluate a PROPOSED buy entry (a sleeve's own quantity) against the hard
    // SAFETY caps. Returns a CapDecision: PASS (unchanged), REDUCE (to the exact
    // remaining headroom), or REJECT (no headroom). NEVER sets final sizing — an
    // in-limit order is returned byte-identical (approved, not reduced), which is
    // the no-erroneous-rejection invariant. Only bites in HARDCAP mode; in OFF /
    // FULL it is an immediate passthrough. Exits must NOT be passed here.
    //
    //   * symbol cap   — (held+pending for this symbol) + order <= symbol_cap * dd
    //   * momentum cap — (held+pending across all momentum symbols) + order
    //                    <= momentum_cap * dd   (momentum entries only)
    //   * drawdown     — dd = DrawdownGovernor::exposure_scale() (1.0 normal,
    //                    <1 in drawdown => caps tighten, 0 on HALT => reject)
    // Cash is intentionally omitted (go-live only; gated on portfolio_cash_usd>0
    // at the caller) so shadow never enforces cash.
    CapDecision govern_entry(const std::string& symbol, double qty, double ref_px,
                             Factor factor, const ExchangeLedger* ledger) const {
        CapDecision d;
        d.proposed_qty = qty; d.proposed_usd = qty * ref_px;
        d.approved_qty = qty; d.approved_usd = d.proposed_usd;
        if (emode_ != EnforceMode::HARDCAP)   return d;   // OFF / FULL: inert
        if (qty <= 0.0 || ref_px <= 0.0)      return d;   // invalids: let the gateway reject

        const double dd = dd_overlay_ ? dd_.exposure_scale() : 1.0;
        const double proposed = d.proposed_usd;
        double allowed = proposed;                        // running headroom allowance

        // symbol hard cap (× dd)
        if (symbol_cap_usd_ > 0.0) {
            double held = 0.0;
            if (ledger)
                held = ledger->position_value(symbol, ref_px) + ledger->pending_buy_value(symbol);
            double headroom = symbol_cap_usd_ * dd - held;
            if (headroom < allowed) { allowed = headroom; d.reason = "symbol-cap"; }
        }
        // aggregate momentum-factor hard cap (× dd) — momentum entries only
        if (factor == Factor::MOMENTUM && momentum_cap_usd_ > 0.0) {
            double mom_held = momentum_exposure_held(ledger);
            double headroom = momentum_cap_usd_ * dd - mom_held;
            if (headroom < allowed) { allowed = headroom; d.reason = "momentum-cap"; }
        }
        // drawdown HALT forces reject even if both caps are disabled (0.0).
        if (dd <= 0.0) { allowed = 0.0; d.reason = "drawdown-halt"; }

        if (allowed >= proposed - 1e-9) return d;          // IN-LIMIT: unchanged (pass)
        if (allowed <= 1e-9) {                             // no headroom: REJECT
            d.approved = false; d.approved_usd = 0.0; d.approved_qty = 0.0;
            if (std::strcmp(d.reason, "pass") == 0) d.reason = "cap-reject";
            return d;
        }
        d.reduced = true;                                  // partial headroom: REDUCE
        d.approved_usd = allowed;
        d.approved_qty = allowed / ref_px;
        return d;
    }

    // Held+pending exposure (exchange-truth) summed over every symbol a MOMENTUM
    // sleeve currently targets — the base the aggregate momentum cap measures the
    // incremental order against. Uses ref_px to value the other symbols.
    double momentum_exposure_held(const ExchangeLedger* ledger) const {
        if (!ledger) return 0.0;
        std::set<std::string> mom;
        for (auto& kv : targets_)
            if (kv.second.factor == Factor::MOMENTUM) mom.insert(kv.second.symbol);
        double tot = 0.0;
        for (auto& s : mom) {
            double px = ref_px ? ref_px(s) : 0.0;
            if (px > 0.0) tot += ledger->position_value(s, px) + ledger->pending_buy_value(s);
        }
        return tot;
    }

private:
    struct Key { std::string sid, sym;
        bool operator<(const Key& o) const {
            return sid != o.sid ? sid < o.sid : sym < o.sym; } };

    EnforceMode emode_ = EnforceMode::OFF;   // Phase-8A: OFF | HARDCAP | FULL
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
