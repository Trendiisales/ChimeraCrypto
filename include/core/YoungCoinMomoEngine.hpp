// ============================================================================
//  YoungCoinMomoEngine.hpp — Phase-6b family (young-liquid-coin MOMENTUM).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: freshly-listed but already-LIQUID coins trend harder (immature price
//  discovery, no overhead supply). A small point-in-time sleeve: trade momentum
//  ONLY in names that listed within the last `young_max` days (and still clear
//  the base's $-volume liquidity floor). Youth is measured point-in-time from
//  history length up to i — no look-ahead, no survivor list.
//
//  ENTRY (close i, eligible, uptrend emaFast>emaSlow):
//    * young: eligible now BUT was NOT eligible `young_max` bars ago (i.e. it
//      lacked enough history then → it listed inside the window). Early-history
//      bars (i < young_max) count as young.
//    * momentum: ret over `mom_lb` > 0, bullish close.
//  Ranking: strongest `mom_lb` return first.
//  EXIT (base): trend break / ATR-trail / time-failure.
//
//  NOTE: on an established-coin universe this sleeve is naturally most active
//  early in each coin's life and near-inert later — reported honestly, not tuned
//  to look busy.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct YoungCoinMomoConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    young_max      = 240;   // "young" = listed within this many bars
    int    mom_lb         = 20;
    double atr_mult       = 3.0;
    int    max_hold       = 40;
    double min_progress   = 0.05;
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class YoungCoinMomoEngine : public LongOnlyDailyBase {
public:
    explicit YoungCoinMomoEngine(YoungCoinMomoConfig c = {})
        : LongOnlyDailyBase(make_base(c)), yc_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if ((int)i < yc_.mom_lb + 2) return false;
        // youth (point-in-time): not eligible young_max bars ago => listed recently
        bool young = ((int)i < yc_.young_max) ? true
                        : !eligible(s, (size_t)((int)i - yc_.young_max));
        if (!young) return false;
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(clPrev)) return false;
        if (!(ef > es)) return false;
        double m = ret(s, i, yc_.mom_lb);
        if (std::isnan(m) || m <= 0) return false;
        return cl > clPrev;
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - yc_.atr_mult*a) return true;
        if (p.bars_held >= yc_.max_hold && cl < p.entry_price*(1.0+yc_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override { return ret(s, i, yc_.mom_lb); }
private:
    YoungCoinMomoConfig yc_;
    static LODConfig make_base(const YoungCoinMomoConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
