// ============================================================================
//  CapitulationRecoveryEngine.hpp — Phase-6b family (capitulation-RECOVERY).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: after a deep capitulation flush (coin down >= cap_dd from its recent
//  high) the first genuine RECLAIM off the low — bullish close, RSI turning up,
//  price back off the lows — snaps back hard. The BREADTH regime gate keeps this
//  from buying knives in a true bear (a long-only book sits bears out anyway).
//  Distinct from bull-regime MR (a shallow RSI dip): this needs a DEEP drawdown.
//
//  ENTRY (close i, eligible):
//    * capitulation: (1 − recentLow / high(dd_lb)) >= cap_dd, recentLow over
//      the last `rebound_lb` bars.
//    * recovery: close >= recentLow * (1+rebound), bullish (close>prev),
//      RSI rising (rsi(i)>rsi(i-1)).
//  Ranking: strongest 5d recovery first.
//  EXIT (own, NOT the base trend-break — entries sit below emaSlow by design):
//    * mean target: close >= emaFast (reverted) — take profit.
//    * hard stop:   close < entry*(1−stop).
//    * time stop:   held >= max_hold.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct CapitulationRecoveryConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    rsi_n          = 14;
    int    dd_lb          = 60;    // high lookback for the drawdown measure
    int    rebound_lb     = 10;    // window holding the capitulation low
    double cap_dd         = 0.35;  // required drawdown depth from the high
    double rebound        = 0.03;  // close must be this far off the recent low
    double stop           = 0.12;  // hard stop below entry
    int    max_hold       = 20;
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class CapitulationRecoveryEngine : public LongOnlyDailyBase {
public:
    explicit CapitulationRecoveryEngine(CapitulationRecoveryConfig c = {})
        : LongOnlyDailyBase(make_base(c)), cr_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if ((int)i < cr_.dd_lb + 2) return false;
        double cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        double r = rsi(s,i), rPrev = rsi(s,i-1);
        if (std::isnan(cl)||std::isnan(clPrev)||std::isnan(r)||std::isnan(rPrev)) return false;
        double hi = highestClose(s, i, cr_.dd_lb);
        if (std::isnan(hi) || hi <= 0) return false;
        double recentLow = 1e18;                                       // lowest low over rebound_lb
        for (int j=(int)i-cr_.rebound_lb; j<=(int)i; ++j){ if(j<0) continue;
            double lo=lowAt(s,j); if(!std::isnan(lo)) recentLow=std::min(recentLow,lo); }
        if (recentLow > 1e17) return false;
        double dd = 1.0 - recentLow/hi;
        if (dd < cr_.cap_dd) return false;                             // deep capitulation
        if (!(cl >= recentLow*(1.0+cr_.rebound))) return false;        // off the low
        if (!(cl > clPrev && r > rPrev)) return false;                 // reclaim + RSI turn
        return true;
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), ef = emaF(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(ef) && cl >= ef) return true;                  // mean target
        if (cl < p.entry_price*(1.0 - cr_.stop)) return true;          // hard stop
        if (p.bars_held >= cr_.max_hold) return true;                  // time stop
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override {
        double r5 = ret(s, i, 5); return std::isnan(r5) ? -1.0 : r5;
    }
private:
    CapitulationRecoveryConfig cr_;
    static LODConfig make_base(const CapitulationRecoveryConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow; b.rsi_n=c.rsi_n;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
