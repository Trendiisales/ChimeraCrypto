// S55: GridEngine — maker-native passive market-making for spot-long.
// Multi-lot ladder: buy a lot on each g% drop (post-only/maker), sell each lot on
// a g% rise. Earns the spacing per oscillation; long-only -> accumulates dips,
// distributes rips. Validated (sim): ~+10%/yr on BTC/SOL, uncorrelated to trend.
// ADVERSE-PROTECTION (verdict, S-2026-07-05): PRIMARY = macro gate (no new lots while
//   BTC<200d-MA) + max_lots cap. SECONDARY = crash_dd_stop circuit-breaker: a >cap
//   equity-peak drawdown latches halted_ -> STOPS opening new lots (caps a fast flash-DD
//   the slow 200d-MA gate misses). Existing inventory is deliberately NOT force-flattened:
//   validated finding (dip-flatten sold the bottom, hurt SOL/ETH) -> holding + macro gate
//   is net-better. Backtested: engine REJECTED for allocation (100% bull-beta, SOL -100%
//   wipe, fails WF) -> ships SHADOW only; this guard closes the "crash_dd_stop was a DEAD
//   field / on_tick never enforced it" bug (S-2026-07-05f handoff B2).
// Separate from EdgeEngine (single-position); GridEngine tracks total_bp() for the dashboard.
#pragma once
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cmath>

namespace chimera {

class GridEngine {
public:
    struct Config {
        std::string symbol, tag;
        double grid_pct      = 0.02;    // spacing (validated ~2%)
        int    max_lots      = 12;      // inventory cap
        double maker_fee     = 0.0005;  // round-trip maker (~5bp)
        double crash_dd_stop = 0.25;    // halt+flatten if equity DD > 25%
        double runner_trail  = 0.05;    // S55: first +g lot becomes a RUNNER, trails this
                                        // % from peak instead of selling at +g (rides the
                                        // sustained move). 0 = plain grid. Validated: +2-4%/90d.
        bool   shadow        = true;
    };
    explicit GridEngine(const Config& c) : cfg_(c) {}

    const Config& cfg() const { return cfg_; }
    int    fills()   const { return fills_; }
    int    open_lots() const { return (int)lots_.size() + (has_runner_ ? 1 : 0); }
    bool   has_runner() const { return has_runner_; }
    double total_bp() const { return realized_bp_; }   // realized only (closed cycles)
    bool   halted()  const { return halted_; }

    // px = latest price. macro_ok = caller's "not in a confirmed downtrend" signal
    // (e.g. BTC>200d-MA). on_buy/on_sell fire paper order intents for the mirror.
    void on_tick(double px, long long ts_ms,
                 bool macro_ok,
                 void(*on_order)(const std::string&,const std::string&,bool,double,long long)=nullptr) {
        if (px <= 0) return;
        last_px_ = px;
        double eq = equity_(px);
        if (eq > peak_eq_) peak_eq_ = eq;
        if (ref_px_ <= 0) ref_px_ = px;

        // S-2026-07-05 crash circuit-breaker (was a DEAD field: on_tick tracked eq/peak_eq_
        // but NEVER enforced crash_dd_stop). A >cap equity-peak drawdown LATCHES halted_,
        // which gates the BUY block below -> no new lots into a crash. Existing inventory is
        // NOT force-flattened (validated: dip-flatten sells the bottom, hurt SOL/ETH); it
        // exits on bounces via the normal sell/runner path. See ADVERSE-PROTECTION header.
        if (peak_eq_ > 0.0 && (peak_eq_ - eq) / peak_eq_ > cfg_.crash_dd_stop) halted_ = true;

        // S55 RUNNER: trail the runner lot (rides the sustained move), exit on pullback.
        if (has_runner_) {
            if (px > run_peak_) run_peak_ = px;
            if (px <= run_peak_ * (1.0 - cfg_.runner_trail)) {
                realized_bp_ += (px / run_entry_ - 1.0 - cfg_.maker_fee) * 1e4 / cfg_.max_lots;
                has_runner_ = false; fills_++;
                if (on_order) on_order(cfg_.tag, cfg_.symbol, false, px, ts_ms);
            }
        }

        // Protection is the MACRO GATE (caller passes macro_ok = BTC>200d-MA): in a
        // SUSTAINED bear we stop BUYING (no new lots) but HOLD existing inventory —
        // flattening on a dip just sells the bottom (validated: hurt SOL/ETH). The
        // inventory recovers on the bounce; max_lots caps exposure. The bear is sat
        // out at the macro level, not by panic-selling intra-trend.
        // BUY on g-drop (maker), if slot free (incl runner) + macro_ok
        if (!halted_ && macro_ok && (int)(lots_.size() + (has_runner_ ? 1 : 0)) < cfg_.max_lots
                     && px <= ref_px_ * (1.0 - cfg_.grid_pct)) {
            lots_.push_back(px); ref_px_ = px; fills_++;
            if (on_order) on_order(cfg_.tag, cfg_.symbol, true, px, ts_ms);
        }
        // SELL lots that rose g above entry (maker), lowest-entry first. With the
        // hybrid, the FIRST +g lot (when no runner) becomes the runner instead.
        std::sort(lots_.begin(), lots_.end());
        for (size_t k = 0; k < lots_.size();) {
            if (px >= lots_[k] * (1.0 + cfg_.grid_pct)) {
                if (cfg_.runner_trail > 0.0 && !has_runner_) {
                    has_runner_ = true; run_entry_ = lots_[k]; run_peak_ = px;
                } else {
                    realized_bp_ += (px / lots_[k] - 1.0 - cfg_.maker_fee) * 1e4 / cfg_.max_lots;
                    if (on_order) on_order(cfg_.tag, cfg_.symbol, false, px, ts_ms);
                }
                ref_px_ = px; fills_++;
                lots_.erase(lots_.begin() + k);
            } else k++;
        }
    }

    // equity in "1.0 + return" terms (per full max_lots capital)
    double equity_(double px) const {
        double e = 1.0;
        for (double lp : lots_) e += (px / lp - 1.0) / cfg_.max_lots;
        if (has_runner_) e += (px / run_entry_ - 1.0) / cfg_.max_lots;
        return e + realized_bp_ / 1e4;
    }

private:
    Config cfg_;
    std::vector<double> lots_;
    double ref_px_ = 0, last_px_ = 0, peak_eq_ = 1.0, realized_bp_ = 0;
    int fills_ = 0;
    bool halted_ = false;
    bool has_runner_ = false;
    double run_entry_ = 0, run_peak_ = 0;
};

} // namespace chimera
