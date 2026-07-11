// ============================================================================
//  BreadthThrustEngine.hpp — Phase-6b family (breadth-THRUST).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS (Zweig-style): when participation SURGES — breadth thrusts from a low
//  reading up through a high one over a few days — the complex has flipped risk-
//  on and the strongest names run. Enter the leaders on the thrust.
//
//  HONEST CAVEAT: this family IS a breadth-timing signal by construction, and
//  the pick-edge control is ITSELF breadth-gated — so this is the family most
//  expected to FAIL the control (its edge is exactly the timing the control
//  already has). Built + screened for completeness/transparency, not because a
//  pass is expected.
//
//  ENTRY (close i, eligible, uptrend emaFast>emaSlow):
//    * thrust: breadth(i) − breadth(i−thrust_lb) >= thrust_delta AND
//              breadth(i) >= min_breadth_now.
//    * positive 20d momentum, bullish close. Leaders picked by 30d return.
//  EXIT (base): trend break / ATR-trail / time-failure.
//
//  breadth() is O(N·history); it is memoised per day-index so entry_signal stays
//  cheap when called across the universe on the same bar.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct BreadthThrustConfig {
    int    ema_fast        = 20;
    int    ema_slow        = 50;
    int    thrust_lb       = 10;    // days over which breadth must surge
    double thrust_delta    = 0.25;  // required breadth increase
    double min_breadth_now = 0.55;  // breadth must now be elevated
    int    mom_lb          = 20;
    double atr_mult        = 3.0;
    int    max_hold        = 40;
    double min_progress    = 0.05;
    int    max_positions   = 8;
    double per_name_cap    = 0.20;
    double breadth_thresh  = 0.40;
    double cost_bps        = 15.0;
};

class BreadthThrustEngine : public LongOnlyDailyBase {
public:
    explicit BreadthThrustEngine(BreadthThrustConfig c = {})
        : LongOnlyDailyBase(make_base(c)), bt_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if ((int)i < bt_.thrust_lb + bt_.mom_lb + 2) return false;
        // memoised global thrust check (breadth is O(N·history))
        if (cache_i_ != (int64_t)i) {
            double bn = breadth(i);
            double bp = ((int)i >= bt_.thrust_lb) ? breadth(i - bt_.thrust_lb) : 0.0;
            thrust_ok_ = (bn - bp) >= bt_.thrust_delta && bn >= bt_.min_breadth_now;
            cache_i_ = (int64_t)i;
        }
        if (!thrust_ok_) return false;
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(clPrev)) return false;
        if (!(ef > es)) return false;
        double m = ret(s, i, bt_.mom_lb);
        if (std::isnan(m) || m <= 0) return false;
        return cl > clPrev;
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - bt_.atr_mult*a) return true;
        if (p.bars_held >= bt_.max_hold && cl < p.entry_price*(1.0+bt_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override { return ret(s, i, 30); }
private:
    BreadthThrustConfig bt_;
    mutable int64_t cache_i_ = -1;
    mutable bool    thrust_ok_ = false;
    static LODConfig make_base(const BreadthThrustConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
