// ============================================================================
//  RelStrengthAccelEngine.hpp — Phase-6b family (relative-strength ACCELERATION).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: momentum-of-momentum. Beyond "is trending up", prefer coins whose
//  own return is ACCELERATING (the recent leg outruns the prior leg) — a 2nd-
//  derivative signal, and cross-sectionally the strongest such names are picked
//  by the base's entry_score ranking. This is the Phase-6b candidate MOST likely
//  to carry genuine SELECTION edge (it ranks names), so it is screened hardest
//  against the exposure-matched pick-edge control.
//
//  ENTRY (close i, eligible, uptrend emaFast>emaSlow):
//    * positive absolute momentum: ret over `mom_lb` > 0.
//    * acceleration: ret over the last `acc_short` bars > ret over the prior
//      `acc_short` bars, AND the recent leg itself is positive.
//    * bullish close (close>prev close).
//  Ranking (entry_score): strongest `mom_lb` return first (cross-sectional pick).
//  EXIT (base): trend break (close<emaSlow) / ATR-trail / time-failure.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct RelStrengthAccelConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    mom_lb         = 30;    // absolute-momentum lookback (also the rank key)
    int    acc_short      = 10;    // acceleration half-window
    double atr_mult       = 3.0;
    int    max_hold       = 40;
    double min_progress   = 0.05;
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class RelStrengthAccelEngine : public LongOnlyDailyBase {
public:
    explicit RelStrengthAccelEngine(RelStrengthAccelConfig c = {})
        : LongOnlyDailyBase(make_base(c)), rs_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if ((int)i < rs_.mom_lb + rs_.acc_short + 2) return false;
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(clPrev)) return false;
        if (!(ef > es)) return false;                                   // uptrend
        double m = ret(s, i, rs_.mom_lb);
        if (std::isnan(m) || m <= 0) return false;                      // positive momentum
        double legNow = ret(s, i, rs_.acc_short);
        double legPrior = ret(s, i - rs_.acc_short, rs_.acc_short);
        if (std::isnan(legNow) || std::isnan(legPrior)) return false;
        if (!(legNow > legPrior && legNow > 0)) return false;           // acceleration
        return cl > clPrev;                                             // bullish
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - rs_.atr_mult*a) return true;
        if (p.bars_held >= rs_.max_hold && cl < p.entry_price*(1.0+rs_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override { return ret(s, i, rs_.mom_lb); }
private:
    RelStrengthAccelConfig rs_;
    static LODConfig make_base(const RelStrengthAccelConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
