// ============================================================================
//  BreakoutRetestEngine.hpp — Phase-6b family (breakout-RETEST).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: a range high breaks, price RETESTS the broken level (pulls back to
//  old resistance = new support) and HOLDS — enter on the hold, a lower-risk
//  entry than chasing the initial break. Distinct from CompressionBreakout
//  (buys the expansion candle) — this waits for the pullback-to-breakout + hold.
//
//  ENTRY (close i, eligible, uptrend emaFast>emaSlow):
//    * established resistance `res` = highest close over the window ending
//      `pullback_lb` bars ago.
//    * breakout occurred: highest close over the last `pullback_lb` bars > res.
//    * retest: within the last `retest_lb` bars a LOW came back to within
//      `band` of res (pullback to the broken level).
//    * hold today: close > res AND bullish (close>open) AND close>prev close.
//  EXIT (base): trend break (close<emaSlow) / ATR-trail / time-failure.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct BreakoutRetestConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    range_lb       = 20;    // resistance lookback
    int    pullback_lb    = 8;     // window in which the breakout must be recent
    int    retest_lb      = 6;     // window in which the retest must occur
    double band           = 0.03;  // retest proximity to the broken level
    double atr_mult       = 3.0;
    int    max_hold       = 40;
    double min_progress   = 0.05;
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class BreakoutRetestEngine : public LongOnlyDailyBase {
public:
    explicit BreakoutRetestEngine(BreakoutRetestConfig c = {})
        : LongOnlyDailyBase(make_base(c)), br_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if ((int)i < br_.range_lb + br_.pullback_lb + 2) return false;
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i),
               op = openAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(op)||std::isnan(clPrev)) return false;
        if (!(ef > es)) return false;                                   // uptrend
        double res = highestClose(s, i - br_.pullback_lb, br_.range_lb);
        if (std::isnan(res) || res <= 0) return false;
        double recentHigh = highestClose(s, i, br_.pullback_lb);
        if (std::isnan(recentHigh) || recentHigh <= res) return false;  // breakout happened
        bool retested = false;                                          // pullback to the level
        for (int j = (int)i - br_.retest_lb; j < (int)i; ++j) { if (j < 0) continue;
            double lo = lowAt(s,j); if (!std::isnan(lo) && lo <= res*(1.0+br_.band)) { retested = true; break; } }
        if (!retested) return false;
        return (cl > res && cl > op && cl > clPrev);                    // hold today
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - br_.atr_mult*a) return true;
        if (p.bars_held >= br_.max_hold && cl < p.entry_price*(1.0+br_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override { return ret(s, i, 90); }
private:
    BreakoutRetestConfig br_;
    static LODConfig make_base(const BreakoutRetestConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
