// ============================================================================
//  TrendPullbackReclaimEngine.hpp  —  Phase-6 family 1 (trend-pullback/reclaim).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: in an established daily uptrend, price pulls back to a RISING fast EMA
//  (the trend's dynamic support), then RECLAIMS it with a bullish close — a
//  lower-risk continuation entry than chasing the breakout.  Distinct from
//  Mimic (buys the impulse) and TSMOM/XSec (rank/hold): this buys the DIP inside
//  an uptrend, on the reclaim.
//
//  ENTRY (all at close i, coin eligible):
//    * uptrend structure:  emaFast > emaSlow  AND  emaFast rising (vs 5 bars ago)
//    * pulled back:  within the last `pullback_lookback` bars the LOW touched or
//                    dipped below emaFast (price came back to dynamic support)
//    * reclaim:  close > emaFast  AND  bullish close (close > open)  AND
//                yesterday's close <= emaFast (i.e. the reclaim happens TODAY)
//  EXIT (base drives, first true):
//    * trend break:   close < emaSlow
//    * ATR-failure / trailing stop:  close < max(entry, highest_close) − atr_mult*ATR
//    * time-failure:  held >= max_hold bars AND not up >= min_progress from entry
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct TrendPullbackConfig {
    int    ema_fast        = 20;
    int    ema_slow        = 50;
    int    rising_lb       = 5;     // emaFast must exceed its value this many bars ago
    int    pullback_lb     = 6;     // window in which a pullback-to-EMA must have occurred
    double atr_mult        = 3.0;   // trailing/ATR-failure stop distance
    int    max_hold        = 40;    // time-failure horizon
    double min_progress    = 0.05;  // must be +5% from entry to escape the time-stop
    // portfolio / regime (base)
    int    max_positions   = 8;
    double per_name_cap    = 0.20;
    double breadth_thresh  = 0.40;
    double cost_bps        = 15.0;
};

class TrendPullbackReclaimEngine : public LongOnlyDailyBase {
public:
    explicit TrendPullbackReclaimEngine(TrendPullbackConfig c = {})
        : LongOnlyDailyBase(make_base(c)), tp_(c) {}

protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if (i < 6) return false;
        double ef = emaF(s,i), es = emaS(s,i), efPrev = emaF(s, i-tp_.rising_lb);
        double cl = closeAt(s,i), op = openAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(efPrev)||std::isnan(cl)||std::isnan(op)||std::isnan(clPrev))
            return false;
        // uptrend structure
        if (!(ef > es && ef > efPrev)) return false;
        // reclaim TODAY: was at/under EMA yesterday, close back above today, bullish
        if (!(clPrev <= emaF(s,i-1) && cl > ef && cl > op)) return false;
        // a genuine pullback occurred in the window (low touched/broke the fast EMA)
        bool pulled = false;
        for (int j = (int)i - tp_.pullback_lb; j < (int)i; ++j) {
            if (j < 0) continue; double lo = lowAt(s,j), efj = emaF(s,j);
            if (!std::isnan(lo) && !std::isnan(efj) && lo <= efj) { pulled = true; break; }
        }
        return pulled;
    }

    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;                    // trend break
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - tp_.atr_mult * a) return true; // ATR/trail stop
        if (p.bars_held >= tp_.max_hold && cl < p.entry_price * (1.0 + tp_.min_progress))
            return true;                                                // time-failure
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override {
        return ret(s, i, 90);   // prefer the strongest longer-trend names
    }

private:
    TrendPullbackConfig tp_;
    static LODConfig make_base(const TrendPullbackConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
