// ============================================================================
//  CompressionBreakoutDailyEngine.hpp  —  Phase-6 family 2 (compression
//  breakout).  Long-only spot, NO shorts, NO 200DMA (regime = breadth in base).
//
//  THESIS: volatility mean-reverts.  When Bollinger bandwidth sits in a LOW
//  percentile of its own recent history (a coiled range), the next decisive
//  range expansion has an edge.  Enter on a CLOSE that breaks the upper band /
//  range high WITH rising spot volume (participation confirms it's real).
//  Distinct from trend-pullback (buys a dip) and UpJump (buys a single-bar
//  impulse): this waits for a coil, then buys the expansion.
//
//  ENTRY (close i, eligible):
//    * compression: bandwidth percentile over `pct_lb` bars <= `pct_thresh`
//    * breakout:    close > upper Bollinger  AND  close > highestClose(range_lb)
//    * volume:      today's volume > vol_mult * avg volume (rising participation)
//  EXIT (base, first true):
//    * failed breakout: close back below the Bollinger MID (range re-absorbed)
//    * ATR runner:      close < highest_close − atr_mult*ATR (trailing)
//    * time-failure:    held >= max_hold AND not up >= min_progress from entry
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct CompressionBreakoutConfig {
    int    bb_n           = 20;
    double bb_k           = 2.0;
    int    pct_lb         = 120;    // bandwidth-percentile lookback
    double pct_thresh     = 0.25;   // "compressed" = bandwidth in the bottom 25%
    int    range_lb       = 20;     // breakout must exceed this many-bar high
    double vol_mult       = 1.2;    // volume confirmation
    double atr_mult       = 3.0;
    int    max_hold       = 40;
    double min_progress   = 0.05;
    // portfolio / regime (base)
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class CompressionBreakoutDailyEngine : public LongOnlyDailyBase {
public:
    explicit CompressionBreakoutDailyEngine(CompressionBreakoutConfig c = {})
        : LongOnlyDailyBase(make_base(c)), cb_(c) {}

protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        double cl = closeAt(s,i), up = bbUp(s,i);
        if (std::isnan(cl) || std::isnan(up)) return false;
        double pct = bandwidthPct(s, i, cb_.pct_lb);
        if (std::isnan(pct) || pct > cb_.pct_thresh) return false;      // must be compressed
        double rangeHigh = highestClose(s, i, cb_.range_lb);
        if (std::isnan(rangeHigh)) return false;
        if (!(cl > up && cl > rangeHigh)) return false;                 // breakout
        double vol = volAt(s,i), va = volAvg(s,i);
        if (va > 0.0 && !std::isnan(vol) && vol < cb_.vol_mult * va) return false; // volume confirm
        return true;
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), mid = bbMid(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(mid) && cl < mid) return true;                  // failed breakout
        if (!std::isnan(a) && a > 0 && cl < p.highest_close - cb_.atr_mult*a) return true; // ATR runner
        if (p.bars_held >= cb_.max_hold && cl < p.entry_price*(1.0+cb_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override {
        // prefer the tightest coils (lowest bandwidth percentile = strongest setup)
        double pct = bandwidthPct(s, i, cb_.pct_lb);
        return std::isnan(pct) ? -1.0 : (1.0 - pct);
    }
private:
    CompressionBreakoutConfig cb_;
    static LODConfig make_base(const CompressionBreakoutConfig& c) {
        LODConfig b; b.bb_n=c.bb_n; b.bb_k=c.bb_k;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
