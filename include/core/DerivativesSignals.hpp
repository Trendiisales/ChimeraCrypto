#pragma once
// ============================================================================
// DerivativesSignals — Phase-7 review layer (item 29: DERIVATIVES-DATA-AS-SIGNAL).
//
// Derivatives + microstructure data used ONLY as a QUALITY annotation / SIZE
// modifier on the EXISTING spot-long entries (UpJump / XSec / RipRider). The
// data is NEVER an instrument — every executed trade stays SPOT-LONG. This
// module changes entry QUALITY, not the strategies, and per the Phase-7
// backtest verdict it is wired OBSERVATION-ONLY (it does NOT veto or resize any
// live order — it stamps each entry with the derivative context + resolves the
// forward outcome, so a real forward dataset accrues to re-judge Phase 7 once
// there is more than the 1-year derivatives history the backtest had).
//
// *** ADDITIVE ONLY / NO 200DMA. *** This module never touches the signal-price
// shadow book, the 32-cell UpJump grid, the allocator, or any execution path.
// It is a measurement layer. Long-only spot; derivatives data is a SIGNAL only.
//
// DATA SUPPORTED (backtested on data/{funding,klines_spot,klines_perp}, 8 sym,
// 2025-05-10..2026-05-10):
//   • funding_pct  — trailing-90d percentile of the 8h funding rate (crowded-long
//                    proxy: high positive funding = leveraged longs paying shorts).
//   • cvd_div      — spot-vs-perp CVD divergence over a trailing window: normalized
//                    (2*taker_buy_base - volume)/volume for spot MINUS perp. >0 =
//                    spot-led (healthy); <0 = perp-led (leverage-driven).
//   • basis_pct    — (perp_close - spot_close)/spot_close (crowding proxy).
// DEFERRED — NO HISTORICAL DATA (would fabricate): open-interest change (no OI
//   file; Binance REST OI hist is ~30d only), real order-book liquidity cost /
//   expected impact (DepthManager is live-only, no historical depth snapshots),
//   liquidation clusters (LiquidationWSFeed live-only), stablecoin/exchange
//   flows, event risk (unlocks/delistings). Build those only when data exists.
//
// Header-only, dependency-free, unit-tested standalone.
// ============================================================================
#include <string>
#include <map>
#include <deque>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace chimera {

struct DerivParams {
    int    funding_lookback_days = 90;   // trailing window for the funding percentile
    int    cvd_window_bars       = 24;   // trailing H1 bars for the CVD divergence
    // ---- SIZE-MODIFIER thresholds (documented, but INERT in observation-only
    //      wiring — the Phase-7 backtest REJECTED all three as robust filters:
    //      none rank entry forward-quality monotonically on the 1yr window). ----
    double funding_extreme_pct   = 0.80; // funding percentile above which longs are "crowded"
    double basis_extreme         = 0.0;  // basis above which longs are "crowded" (set per-sym live)
};

// Per-entry derivative context stamped at signal time.
struct DerivSignal {
    bool   ready       = false;
    double funding_rate = 0.0;
    double funding_pct  = 0.5;   // 0..1 trailing-90d percentile
    double cvd_div      = 0.0;   // spot_cvd_norm - perp_cvd_norm (>0 spot-led)
    bool   spot_led     = true;  // cvd_div >= 0
    double basis_pct    = 0.0;   // (perp-spot)/spot
    // Backtest-derived verdict (see phase7_derivsignals_bt.cpp / ChimeraReviewPhase7):
    // a SIZE MULTIPLIER a promoted filter WOULD apply. Kept for the forward record;
    // in observation-only wiring the caller does NOT apply it to any live order.
    double size_mult(const DerivParams& p) const {
        double m = 1.0;
        if (funding_pct >= p.funding_extreme_pct) m *= 0.75; // crowded-long haircut
        if (!spot_led)                            m *= 0.75; // perp-led haircut
        return m;
    }
};

// One rolling book across all symbols. Fed live from the spot feed (close+vol+
// taker_buy_base per H1), the perp feed (same), and the funding stream; or, in
// the backtest, replayed from the committed CSVs. eval() is a pure read.
class DerivativesSignalBook {
public:
    explicit DerivativesSignalBook(DerivParams p = {}) : p_(p) {}

    void on_spot_h1(const std::string& sym, int64_t hour_ms, double close,
                    double volume, double taker_buy_base) {
        auto& s = book_[sym];
        s.spot_close = close;
        double cvd_norm = volume > 0 ? (2.0*taker_buy_base - volume) / volume : 0.0;
        push(s.spot_cvd, cvd_norm, p_.cvd_window_bars);
        (void)hour_ms;
    }
    void on_perp_h1(const std::string& sym, int64_t hour_ms, double close,
                    double volume, double taker_buy_base) {
        auto& s = book_[sym];
        s.perp_close = close;
        double cvd_norm = volume > 0 ? (2.0*taker_buy_base - volume) / volume : 0.0;
        push(s.perp_cvd, cvd_norm, p_.cvd_window_bars);
        (void)hour_ms;
    }
    void on_funding(const std::string& sym, int64_t funding_ms, double rate) {
        auto& s = book_[sym];
        s.funding.push_back({funding_ms, rate});
        // keep at most lookback+buffer, drop the oldest beyond the window
        int64_t cut = funding_ms - (int64_t)p_.funding_lookback_days*86400000LL;
        while (!s.funding.empty() && s.funding.front().first < cut) s.funding.pop_front();
    }

    DerivSignal eval(const std::string& sym) const {
        DerivSignal d;
        auto it = book_.find(sym);
        if (it == book_.end()) return d;
        const auto& s = it->second;
        if (s.funding.empty() || s.spot_cvd.empty() || s.perp_cvd.empty()) return d;
        d.ready = true;
        d.funding_rate = s.funding.back().second;
        // trailing-90d percentile of the current rate
        int le = 0; for (auto& f : s.funding) if (f.second <= d.funding_rate) ++le;
        d.funding_pct = (double)le / (double)s.funding.size();
        // spot-vs-perp CVD divergence (mean of the normalized per-bar CVD)
        double sc = mean(s.spot_cvd), pc = mean(s.perp_cvd);
        d.cvd_div  = sc - pc;
        d.spot_led = d.cvd_div >= 0.0;
        // basis
        d.basis_pct = s.spot_close > 0 ? (s.perp_close - s.spot_close)/s.spot_close : 0.0;
        return d;
    }

    const DerivParams& params() const { return p_; }

private:
    struct Sym {
        std::deque<double> spot_cvd, perp_cvd;
        std::deque<std::pair<int64_t,double>> funding;
        double spot_close = 0.0, perp_close = 0.0;
    };
    static void push(std::deque<double>& q, double v, int cap) {
        q.push_back(v); while ((int)q.size() > cap) q.pop_front();
    }
    static double mean(const std::deque<double>& q) {
        if (q.empty()) return 0.0; double s=0; for (double x:q) s+=x; return s/q.size();
    }
    DerivParams p_;
    std::map<std::string, Sym> book_;
};

} // namespace chimera
