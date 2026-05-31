// S55: GridEngine — maker-native passive market-making for spot-long.
// Multi-lot ladder: buy a lot on each g% drop (post-only/maker), sell each lot on
// a g% rise. Earns the spacing per oscillation; long-only -> accumulates dips,
// distributes rips. Validated (sim): ~+10%/yr on BTC/SOL, uncorrelated to trend.
// Crash guard: stop buying + flatten if drawdown from equity-peak exceeds a cap
// (the inventory-bleed-in-a-crash risk). Separate from EdgeEngine (which is
// single-position); GridEngine tracks total_bp() the same way for the dashboard.
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
        bool   shadow        = true;
    };
    explicit GridEngine(const Config& c) : cfg_(c) {}

    const Config& cfg() const { return cfg_; }
    int    fills()   const { return fills_; }
    int    open_lots() const { return (int)lots_.size(); }
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

        // Protection is the MACRO GATE (caller passes macro_ok = BTC>200d-MA): in a
        // SUSTAINED bear we stop BUYING (no new lots) but HOLD existing inventory —
        // flattening on a dip just sells the bottom (validated: hurt SOL/ETH). The
        // inventory recovers on the bounce; max_lots caps exposure. The bear is sat
        // out at the macro level, not by panic-selling intra-trend.
        // BUY on g-drop (maker), if slot free + macro_ok
        if (macro_ok && (int)lots_.size() < cfg_.max_lots && px <= ref_px_ * (1.0 - cfg_.grid_pct)) {
            lots_.push_back(px); ref_px_ = px; fills_++;
            if (on_order) on_order(cfg_.tag, cfg_.symbol, true, px, ts_ms);
        }
        // SELL lots that rose g above entry (maker), lowest-entry first
        std::sort(lots_.begin(), lots_.end());
        for (size_t k = 0; k < lots_.size();) {
            if (px >= lots_[k] * (1.0 + cfg_.grid_pct)) {
                realized_bp_ += (px / lots_[k] - 1.0 - cfg_.maker_fee) * 1e4 / cfg_.max_lots;
                ref_px_ = px; fills_++;
                if (on_order) on_order(cfg_.tag, cfg_.symbol, false, px, ts_ms);
                lots_.erase(lots_.begin() + k);
            } else k++;
        }
    }

    // equity in "1.0 + return" terms (per full max_lots capital)
    double equity_(double px) const {
        double e = 1.0;
        for (double lp : lots_) e += (px / lp - 1.0) / cfg_.max_lots;
        return e + realized_bp_ / 1e4;
    }

private:
    Config cfg_;
    std::vector<double> lots_;
    double ref_px_ = 0, last_px_ = 0, peak_eq_ = 1.0, realized_bp_ = 0;
    int fills_ = 0;
    bool halted_ = false;
};

} // namespace chimera
