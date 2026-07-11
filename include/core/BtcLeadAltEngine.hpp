// ============================================================================
//  BtcLeadAltEngine.hpp — Phase-6b family (BTC-lead ALT confirmation).
//  Long-only spot, NO shorts, NO 200DMA (regime = breadth in the base).
//
//  THESIS: BTC leads the crypto complex. When BTC is advancing (positive over
//  `btc_lb` days — a MOMENTUM read on BTC, NOT a 200-day MA), an ALT that then
//  CONFIRMS with its own breakout + positive short momentum is a higher-quality
//  long than an alt breaking out while BTC is flat/soft. BTC is used as a LEAD
//  filter only; every trade is a spot-long ALT (BTC itself is not traded here).
//  Screened hard vs the pick-edge control — a BTC-up filter is close to breadth
//  timing, so the burden is to show ALT SELECTION beyond "BTC is up".
//
//  ENTRY (close i, eligible, s != BTC, uptrend emaFast>emaSlow):
//    * BTC leads: BTC close(i) > BTC close(i-btc_lb) * (1+btc_min).
//    * alt confirms: close breaks its own `alt_range_lb`-bar high.
//    * alt short momentum positive (ret over `confirm_short` > 0), bullish close.
//  Ranking: strongest 30d return first.
//  EXIT (base): trend break (close<emaSlow) / ATR-trail / time-failure.
// ============================================================================
#pragma once
#include "core/LongOnlyDailyBase.hpp"

namespace chimera {

struct BtcLeadAltConfig {
    int    ema_fast       = 20;
    int    ema_slow       = 50;
    int    btc_lb         = 20;    // BTC lead-momentum lookback
    double btc_min        = 0.0;   // BTC must be up at least this over btc_lb
    int    alt_range_lb   = 20;    // alt breakout lookback
    int    confirm_short  = 10;    // alt short-momentum window
    double atr_mult       = 3.0;
    int    max_hold       = 40;
    double min_progress   = 0.05;
    std::string btc_sym   = "BTC";
    int    max_positions  = 8;
    double per_name_cap   = 0.20;
    double breadth_thresh = 0.40;
    double cost_bps       = 15.0;
};

class BtcLeadAltEngine : public LongOnlyDailyBase {
public:
    explicit BtcLeadAltEngine(BtcLeadAltConfig c = {})
        : LongOnlyDailyBase(make_base(c)), bl_(c) {}
protected:
    bool entry_signal(const std::string& s, size_t i) const override {
        if (s == bl_.btc_sym) return false;                            // trade alts, not the lead
        if ((int)i < bl_.btc_lb + 2) return false;
        double btcN = closeAt(bl_.btc_sym, i), btcP = closeAt(bl_.btc_sym, i - bl_.btc_lb);
        if (std::isnan(btcN)||std::isnan(btcP)||btcP<=0) return false;
        if (!(btcN > btcP*(1.0+bl_.btc_min))) return false;            // BTC leads up
        double ef = emaF(s,i), es = emaS(s,i), cl = closeAt(s,i), clPrev = closeAt(s,i-1);
        if (std::isnan(ef)||std::isnan(es)||std::isnan(cl)||std::isnan(clPrev)) return false;
        if (!(ef > es)) return false;                                  // alt uptrend
        double altHigh = highestClose(s, i, bl_.alt_range_lb);
        if (std::isnan(altHigh) || !(cl > altHigh)) return false;      // alt confirms (breakout)
        double as = ret(s, i, bl_.confirm_short);
        if (std::isnan(as) || as <= 0) return false;                   // alt short momentum
        return cl > clPrev;                                            // bullish
    }
    bool exit_signal(const std::string& s, size_t i, const Position& p) const override {
        double cl = closeAt(s,i), es = emaS(s,i), a = atr(s,i);
        if (std::isnan(cl)) return false;
        if (!std::isnan(es) && cl < es) return true;
        double ref = std::max(p.entry_price, p.highest_close);
        if (!std::isnan(a) && a > 0 && cl < ref - bl_.atr_mult*a) return true;
        if (p.bars_held >= bl_.max_hold && cl < p.entry_price*(1.0+bl_.min_progress)) return true;
        return false;
    }
    double entry_score(const std::string& s, size_t i) const override { return ret(s, i, 30); }
private:
    BtcLeadAltConfig bl_;
    static LODConfig make_base(const BtcLeadAltConfig& c) {
        LODConfig b; b.ema_fast_n=c.ema_fast; b.ema_slow_n=c.ema_slow;
        b.max_positions=c.max_positions; b.per_name_cap=c.per_name_cap;
        b.breadth_thresh=c.breadth_thresh; b.cost_bps=c.cost_bps; b.inverse_vol=true;
        return b;
    }
};

} // namespace chimera
