// ============================================================================
//  BullRegimeMeanReversionEngine.hpp  —  Phase-6 family 3 (bull-regime mean-
//  reversion).  Long-only spot, NO shorts, NO 200DMA (regime = breadth in base).
//
//  THESIS: BUY DIPS, but ONLY in an uptrend.  In a higher-TF bull structure a
//  short-term oversold flush (RSI < os_thresh) that then RECLAIMS tends to snap
//  back to the mean.  The uptrend filter is what keeps a long-only MR book from
//  catching a falling knife in a bear (which it must sit out anyway).  Distinct
//  from the two momentum-continuation families: this is COUNTER-trend on the
//  short horizon, pro-trend on the long horizon — low correlation by design.
//
//  ENTRY (close i, eligible):
//    * bull structure:  emaFast > emaSlow (per-coin higher-TF trend up; NO 200DMA)
//    * was oversold:     RSI dipped below os_thresh within the last `os_lb` bars
//    * reclaim:          RSI back above os_thresh today AND close > yesterday close
//                        (the snap-back has begun)
//  EXIT (base, first true):
//    * mean target:   close >= emaFast (reverted to the mean) — take profit
//    * trend fail:    close < emaSlow (higher-TF trend broke) — bail
//    * time stop:     held >= max_hold (the snap-back didn't come)
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct BullMeanRevConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    rsi_n          = 14;
    double os_thresh      = 35.0;   // oversold line
    int    os_lb          = 5;      // must have been oversold within this window
    int    max_hold       = 15;     // MR is fast — short leash
    // portfolio / regime (base)
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class BullRegimeMeanReversionEngine : public LongOnlyDailyBase {
public:
    explicit BullRegimeMeanReversionEngine(BullMeanRevConfig c = {})
        : LongOnlyDailyBase(make_base(c)), mr_(c) {}

protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if (i < 2) return false;
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        double r = rsi(s,i), rPrev = rsi(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(clPrev)||std::isnan(r)||std::isnan(rPrev))
            return false;
        if (!(ef > es)) return false;                                   // bull structure
        // reclaim today: RSI crossed back up through the oversold line
        if (!(rPrev < mr_.os_thresh && r >= mr_.os_thresh && cl > clPrev)) return false;
        // ensure it was genuinely oversold in the window (not a marginal wiggle)
        bool os = false;
        for (int j=(int)i-mr_.os_lb;j<(int)i;++j){ if(j<0) continue; double rj=rsi(s,j);
            if(!std::isnan(rj) && rj < mr_.os_thresh){ os=true; break; } }
        return os;
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), ef = emaF(s,i), es = emaS(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(ef) && cl >= ef) return true;                   // mean target hit
        if (!std::isnan(es) && cl < es) return true;                    // trend fail
        if (p.bars_held >= mr_.max_hold) return true;                   // time stop
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override {
        // deepest oversold snap-backs first (lowest recent RSI)
        double lo = 100.0; for (int j=(int)i-mr_.os_lb;j<(int)i;++j){ if(j<0)continue;
            double rj=rsi(s,j); if(!std::isnan(rj)) lo=std::min(lo,rj); }
        return 100.0 - lo;
    }
private:
    BullMeanRevConfig mr_;
    static LODConfig make_base(const BullMeanRevConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow; b.rsi_n=c.rsi_n;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
