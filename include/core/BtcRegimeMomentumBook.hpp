// ============================================================================
// BtcRegimeMomentumBook — LIVE directional BTC regime-momentum book
// ----------------------------------------------------------------------------
// Certified: S-2026-07-20af. Cert artifacts (Crypto repo):
//   backtest/BTC_TRENDCORE_GATEFIX_2026-07-20.md   (verdict + honest null)
//   backtest/btc_trendcore_2026-07-20/gatefix_results.txt   (raw T1..T4)
//   backtest/btc_trendcore_gatefix_bt.py  (T3 ensemble driver, run_cell import)
//   backtest/btc_trendcore_cert_bt.py     (run_cell mechanics: gap-honest fills)
//   backtest/slowtrend_daily_bt.py        (TSMOM30 daily mechanics)
//
// This is a 50/50 TWO-SLEEVE, LONG-ONLY BTC spot book (NOT a mimic/companion —
// it is directional, so the BE-floor-on-open companion mandate does not apply;
// its protection is the certified structural stop + trail + momentum-flip exit):
//   sleeve A  TRENDCORE  — daily-bar Donchian breakout LB20/EMA200/EX20/CH3.0,
//                          NO-ADD. Signals on COMPLETED daily bars; execution on
//                          COMPLETED 1h bars (buy-stop max(open,trigger); gap-
//                          honest stop min(open,stop); chase guard 0.75*ATR;
//                          structural stop clamped 1.75-3.0*ATR; protection floor
//                          at MFE>=2R (66bp); chandelier+donchian monotonic trail;
//                          time+regime exits; loss cooldown / profit re-arm).
//   sleeve B  TSMOM30    — 30-day total-return momentum: close>close[-30] => long,
//                          else flat. Daily signal, NEXT-daily-open fills, 30bp RT.
//
// Certified figures (costs inside fills; 28bp measured, batteries used >=30bp):
//   orig 2021-01..2026-07 pooled  n=112 +11144bp PF1.70 conc 11.6%
//   ext  2017-08..2026-07 pooled  n=171 +44878bp PF2.63 conc 18.7%
//   (TSMOM30 dominates the pooled net; TRENDCORE is the minority sleeve.)
//
// HONEST NULL FRAMING (from the cert, must not be lost): the EDGE is
// REGIME + TRAIL + MOMENTUM (+ BTC bull beta) — NOT breakout entry timing. The
// random-entry regime null puts the TRENDCORE breakout trigger at the 59th
// percentile on full history (statistically indistinguishable from a random
// entry inside the same regime under identical management). The breakout is
// treated as ONE arbitrary-but-harmless entry trigger, not a certified timing
// edge. Concentration passes ONLY as the ensemble (single-sleeve TRENDCORE and
// the folds gate fail on extended history); the two sleeves together certify.
//
// ADVERSE-PROTECTION (backtested verdict, S-2026-07-20af): TRENDCORE rides with a
//   STRUCTURAL ATR stop (clamped 1.75-3.0*ATR) from open + a 2R protection floor
//   (66bp) + a monotonic chandelier/donchian trail + time/regime exits — a cold
//   loss-cut is NOT added (a slow-trend book's edge is the ride; the cert's stop
//   IS the protection). TSMOM30's protection is the 30d momentum-flip to flat.
//   Both are certified net-positive after cost, both WF halves, both regimes.
// PROFIT-LOCK: the certified chandelier+donchian monotonic trail + 2R floor ARE
//   the giveback lock (TRENDCORE); TSMOM30 flips flat on momentum loss. No book
//   rides profit back to BE unlocked (profit-lock mandate satisfied by the cert).
// ============================================================================
#pragma once
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <unordered_map>

// The live-routing path uses chimera::OrderIntent / OrderResult / Factor. The
// standalone parity harness (backtest/btc_regime_book_parity_bt.cpp) defines
// BTC_BOOK_STANDALONE and supplies field-identical stubs, so it can drive the
// EXACT engine logic without pulling the live stack (curl/websocket) into the TU.
#ifdef BTC_BOOK_STANDALONE
namespace chimera {
struct OrderIntent { std::string symbol; bool is_buy = true; double qty = 0.0; double ref_px = 0.0;
                     bool is_exit = false; const char* source = "?"; };
struct OrderResult { bool ok = false; bool shadow = false; double executed_qty = 0.0; double avg_price = 0.0; std::string error; };
enum class Factor { MOMENTUM, MEANREV, OTHER };
}
#else
#include "live/ExecutionGateway.hpp"        // OrderIntent / OrderResult
#include "live/SpotPortfolioAllocator.hpp"  // Factor
#endif

namespace chimera {

class BtcRegimeMomentumBook {
public:
    // ── certified constants (do not tune without a fresh honest cert) ────────
    static constexpr int    TC_LB        = 20;      // Donchian breakout lookback (daily)
    static constexpr int    TC_EMA_N     = 200;     // regime EMA (daily)
    static constexpr int    TC_EXIT_LB   = 20;      // trailing donchian exit lookback
    static constexpr double TC_CH_ATR    = 3.0;     // chandelier ATR multiple
    static constexpr double TC_COST_RT   = 0.0028;  // 28bp measured RT (cert primary tier)
    static constexpr double FLOOR_BP     = 0.0066;  // 2R protection floor = 2*28bp+10bp
    static constexpr double CHASE_ATR    = 0.75;    // gap-beyond-trigger cancel
    static constexpr double STOP_MIN_ATR = 1.75;
    static constexpr double STOP_MAX_ATR = 3.00;
    static constexpr int    TS_K         = 30;      // TSMOM total-return lookback (days)
    static constexpr double TS_COST_RT   = 0.0030;  // 30bp RT (slowtrend battery)
    // di < max(LB,EMA_N,20)+1  ->  need di>=201, i.e. >=202 completed daily bars
    static constexpr int    MIN_COMPLETED_DAILY = TC_EMA_N + 2;

    enum Sleeve { TRENDCORE = 0, TSMOM30 = 1 };
    struct TradeRec {
        Sleeve sleeve; int64_t entry_ts; int64_t exit_ts;
        double net_frac;      // per-notional net return (costs inside fills)
        double entry_px; double exit_px; const char* reason;
    };

    // ── config / wiring ──────────────────────────────────────────────────────
    bool        live_enabled = false;                 // route REAL orders (LIVE mode only)
    double      order_usd    = 0.0;                   // per-sleeve clip notional (pilot $/order)
    std::string symbol       = "btcusdt";
    std::string persist_path = "data/btc_regime_book.json";
    std::string trades_csv   = "data/btc_book_trades.csv";
    std::function<OrderResult(const OrderIntent&, Factor)> submit;   // governed_submit
    std::function<double(const std::string&)> free_base;             // base(UPPER)->free qty; null in shadow
    std::function<void(const TradeRec&)> on_trade;                   // accounting/parity sink
    // desk PnL fold (S-2026-07-20af): fired on every REAL live close so the desk
    // ledger/_clivetot includes this book. Args: (strat, coin, entry, exit, qty, usd, reason).
    // null in shadow/parity. usd is the book's already-real-fill delta — desk must not re-derive.
    std::function<void(const char*, const std::string&, double, double, double, double, const char*)> on_live_close;

    // ─────────────────────────────────────────────────────────────────────────
    //  DAILY HISTORY  (completed bars only; index i = day daily_[i].day)
    // ─────────────────────────────────────────────────────────────────────────
    struct DBar { int64_t day; double o, h, l, c; };
    std::vector<DBar>    daily_;
    std::vector<double>  ema_, atr_;
    std::deque<double>   tr20_;   double tr20_sum_ = 0.0;
    std::unordered_map<int64_t,int> day_index_;

    void push_daily_(const DBar& b) {
        int i = (int)daily_.size();
        daily_.push_back(b);
        day_index_[b.day] = i;
        // EMA200 (same recursion as btc_trendcore_cert_bt.precompute)
        const double k = 2.0 / (TC_EMA_N + 1.0);
        ema_.push_back(i == 0 ? b.c : b.c * k + ema_[i-1] * (1.0 - k));
        // ATR20: tr = max(h-l,|h-pc|,|l-pc|); pc = prev close (or this open at i=0)
        double pc = (i > 0) ? daily_[i-1].c : b.o;
        double tr = std::max(b.h - b.l, std::max(std::fabs(b.h - pc), std::fabs(b.l - pc)));
        tr20_.push_back(tr); tr20_sum_ += tr;
        double a;
        if (i < 20) a = tr20_sum_ / (double)tr20_.size();
        else { tr20_sum_ -= tr20_.front(); tr20_.pop_front(); a = tr20_sum_ / 20.0; }
        atr_.push_back(a);
    }
    double hh_(int i, int n) const {                 // highest high of daily_[i-n+1 .. i]
        int lo = i - n + 1; if (lo < 0) lo = 0; double m = daily_[lo].h;
        for (int j = lo + 1; j <= i; ++j) if (daily_[j].h > m) m = daily_[j].h; return m;
    }
    double ll_(int i, int n) const {
        int lo = i - n + 1; if (lo < 0) lo = 0; double m = daily_[lo].l;
        for (int j = lo + 1; j <= i; ++j) if (daily_[j].l < m) m = daily_[j].l; return m;
    }

    // ── TSMOM30 daily sleeve state ───────────────────────────────────────────
    bool    ts_long_     = false;
    double  ts_entry_px_ = 0.0;
    int64_t ts_entry_ts_ = 0;
    int     ts_pending_   = -1;   // -1 none, 0 want-flat, 1 want-long
    double  ts_live_qty_  = 0.0;
    double  ts_live_px_   = 0.0;

    // ── TRENDCORE sleeve state ───────────────────────────────────────────────
    bool    tc_active_ = false;
    double  tc_e_ = 0, tc_stop_ = 0, tc_R_ = 0, tc_mfe_ = 0, tc_peak_ = 0, tc_floor_ = 0;
    int64_t tc_ts_ = 0;
    int     tc_di0_ = -1, tc_last_trail_di_ = -1;
    bool    tc_protected_ = false, tc_exit_next_ = false;
    int64_t tc_cooldown_until_day_ = -1;
    bool    tc_have_block_ = false;  double tc_block_high_ = 0.0;
    bool    tc_have_last_trig_ = false; double tc_last_trigger_ = 0.0;
    double  tc_live_qty_ = 0.0, tc_live_px_ = 0.0;

    // ── live 1h aggregation from ticks (live only; parity feeds on_h1_bar) ────
    bool    agg_have_ = false; int64_t agg_hour_ = 0, agg_day_ = 0;
    double  agg_o_ = 0, agg_h_ = 0, agg_l_ = 0, agg_c_ = 0;
    // live current-day accumulator (built from completed 1h bars)
    bool    cur_have_ = false; int64_t cur_day_ = 0; double cur_o_ = 0, cur_h_ = 0, cur_l_ = 0, cur_c_ = 0;
    double  realized_usd_ = 0.0; int n_trades_ = 0;

    // =========================================================================
    //  SEED (historical warmup) — completed daily bars from REST 1d klines.
    //  Advances ema/atr/donchian AND the TSMOM want-state, but takes NO position
    //  and emits NO trade (live starts flat; positions come only from persist).
    // =========================================================================
    void seed_daily(int64_t day, double o, double h, double l, double c) {
        push_daily_({day, o, h, l, c});
    }
    void finalize_seed() {
        // TSMOM has no historical position to carry (live starts flat unless persist
        // restored a long). The first live day-rollover recomputes want_{D-1} from the
        // seeded closes and enters at that day's open if want!=state — no pending
        // variable needed (want != long-state IS the change condition).
    }
    bool warm() const { return (int)daily_.size() >= MIN_COMPLETED_DAILY; }
    bool regime_on() const {
        int i = (int)daily_.size() - 1;
        if (i < 20) return false;
        return daily_[i].c > ema_[i] && ema_[i] > ema_[i - 20];
    }
    int ts_want() const {
        int i = (int)daily_.size() - 1;
        if (i < TS_K) return 0;
        return daily_[i].c > daily_[i - TS_K].c ? 1 : 0;
    }

    // =========================================================================
    //  LIVE tick → 1h aggregation. On each completed 1h bar drive on_h1_bar().
    // =========================================================================
    void on_tick(double px, int64_t now_ms) {
        if (px <= 0.0) return;
        int64_t ts = now_ms / 1000;
        int64_t hour = ts / 3600;
        if (!agg_have_) { agg_have_ = true; agg_hour_ = hour; agg_o_ = agg_h_ = agg_l_ = agg_c_ = px; return; }
        if (hour != agg_hour_) {
            // finalize the completed 1h bar at its bar-start ts
            int64_t bar_ts = agg_hour_ * 3600;
            double o = agg_o_, h = agg_h_, l = agg_l_, c = agg_c_;
            agg_hour_ = hour; agg_o_ = agg_h_ = agg_l_ = agg_c_ = px;
            on_h1_bar(bar_ts, o, h, l, c);
            return;
        }
        agg_c_ = px; if (px > agg_h_) agg_h_ = px; if (px < agg_l_) agg_l_ = px;
    }

    // =========================================================================
    //  CORE — one completed 1h bar. FAITHFUL port of run_cell (NO-ADD path) +
    //  the TSMOM30 daily driver. Parity harness calls this directly per 1h bar.
    // =========================================================================
    void on_h1_bar(int64_t ts, double o, double h, double l, double c) {
        int64_t d = ts / 86400;
        // ── day rollover: finalize the previous day, run the TSMOM daily step ──
        if (cur_have_ && d != cur_day_) {
            finalize_current_day_(o, ts);   // pushes cur_day_ bar; TSMOM exec+signal at THIS open
        }
        if (!cur_have_ || d != cur_day_) {
            cur_have_ = true; cur_day_ = d; cur_o_ = o; cur_h_ = h; cur_l_ = l; cur_c_ = c;
        } else {
            cur_c_ = c; if (h > cur_h_) cur_h_ = h; if (l < cur_l_) cur_l_ = l;
        }

        // ── TRENDCORE: needs the LAST COMPLETED daily bar (di = day d-1) ──────
        auto it = day_index_.find(d - 1);
        if (it == day_index_.end()) return;
        int di = it->second;
        if (di < MIN_COMPLETED_DAILY - 1) return;   // di < max(LB,EMA_N,20)+1
        const double A = atr_[di];
        const bool regime = (daily_[di].c > ema_[di] && ema_[di] > ema_[di - 20]);
        const double trigger = hh_(di, TC_LB) + 0.10 * A;
        const double half = TC_COST_RT / 2.0;

        if (!tc_active_) {
            bool armed = regime;
            if (d <= tc_cooldown_until_day_) {
                if (tc_have_last_trig_ && trigger >= tc_last_trigger_ + 0.5 * A) { /* armed stays */ }
                else armed = false;
            }
            if (tc_have_block_) {
                if (hh_(di, TC_LB) > tc_block_high_) tc_have_block_ = false;
                else armed = false;
            }
            if (armed && h >= trigger) {
                double fill = std::max(o, trigger);
                if (fill > trigger + CHASE_ATR * A) {
                    // chase guard: cancelled this bar
                } else {
                    double e = fill * (1.0 + half);
                    double stop_raw = ll_(di, 10) - 0.10 * A;
                    double dist = e - stop_raw;
                    if (dist < STOP_MIN_ATR * A) dist = STOP_MIN_ATR * A;
                    if (dist > STOP_MAX_ATR * A) dist = STOP_MAX_ATR * A;
                    tc_active_ = true; tc_e_ = e; tc_ts_ = ts; tc_stop_ = e - dist; tc_R_ = dist;
                    tc_mfe_ = e; tc_peak_ = c; tc_protected_ = false; tc_exit_next_ = false;
                    tc_di0_ = di; tc_last_trail_di_ = -1;
                    tc_last_trigger_ = trigger; tc_have_last_trig_ = true;
                    tc_open_live_(fill);
                }
            }
            return;
        }

        // ── in position ──────────────────────────────────────────────────────
        if (tc_exit_next_) {
            double x = o * (1.0 - half);
            tc_close_(x, ts, "EXIT_NEXT", d);
            return;
        }
        if (h > tc_mfe_) tc_mfe_ = h;
        if (c > tc_peak_) tc_peak_ = c;
        double mfe_R = (tc_mfe_ - tc_e_) / tc_R_;
        if (!tc_protected_ && mfe_R >= 2.0) {
            tc_protected_ = true;
            tc_floor_ = tc_e_ * (1.0 + FLOOR_BP);
            if (tc_floor_ > tc_stop_) tc_stop_ = tc_floor_;
        }
        if (tc_protected_ && di > tc_last_trail_di_) {
            tc_last_trail_di_ = di;
            double hi_close = daily_[tc_di0_].c;
            for (int j = tc_di0_ + 1; j <= di; ++j) if (daily_[j].c > hi_close) hi_close = daily_[j].c;
            double chand = hi_close - TC_CH_ATR * A;
            double donch = ll_(di, TC_EXIT_LB) - 0.10 * A;
            double s = tc_stop_;
            if (tc_floor_ > s) s = tc_floor_;
            if (chand > s) s = chand;
            if (donch > s) s = donch;
            tc_stop_ = s;
        }
        if (l <= tc_stop_) {
            double x = std::min(o, tc_stop_) * (1.0 - half);
            tc_close_(x, ts, "STOP", d);
            return;
        }
        // daily-close exits (evaluate on the LAST 1h bar of the day; fill next open)
        if ((ts + 3600) / 86400 != d) {
            int64_t days_in = d - (tc_ts_ / 86400);
            // dtoday close/ema: this is the in-progress day; at the last 1h bar its
            // close == c and ema_today == c*k + ema[di]*(1-k) — EXACTLY the value
            // btc_trendcore_cert_bt precomputes for daily[dtoday] (whose close IS c).
            const double k = 2.0 / (TC_EMA_N + 1.0);
            double ema_today = c * k + ema_[di] * (1.0 - k);
            if (days_in >= 15 && mfe_R < 1.0 && c <= tc_e_ * (1.0 + TC_COST_RT)) tc_exit_next_ = true;
            if (c < ema_today) tc_exit_next_ = true;
        }
    }

    // parity only: close any open sleeve at the final close (matches the python
    // harnesses' end-of-stream flush: run_cell closes at h1[-1].c; tsmom30 closes
    // an open long at bars[-1].c).
    void finalize_stream(int64_t last_ts, double last_close) {
        if (tc_active_) {
            double x = last_close * (1.0 - TC_COST_RT / 2.0);
            tc_close_(x, last_ts, "EOD_FINAL", last_ts / 86400);
        }
        if (ts_long_) {
            double net = last_close / ts_entry_px_ - 1.0 - TS_COST_RT;
            emit_(TradeRec{TSMOM30, ts_entry_ts_, last_ts, net, ts_entry_px_, last_close, "TSMOM_EOD"});
            ts_long_ = false;
        }
    }

private:
    // Finalize the day that just ended (cur_*) and run the TSMOM30 daily step.
    // new_day_open / new_day_open_ts = the OPEN of the day now beginning. TSMOM's
    // signal is momentum on the JUST-COMPLETED day's close (want_{i}); it executes
    // NEXT-open = at new_day_open (python: pending set at bar i executes at bar i+1
    // open). Computing want at finalize and filling at the incoming open collapses
    // the one-bar pending lag correctly (want != long-state IS the change condition).
    void finalize_current_day_(double new_day_open, int64_t new_day_open_ts) {
        push_daily_({cur_day_, cur_o_, cur_h_, cur_l_, cur_c_});
        int i = (int)daily_.size() - 1;
        int want = (i < TS_K) ? 0 : (daily_[i].c > daily_[i - TS_K].c ? 1 : 0);
        if (want != (ts_long_ ? 1 : 0)) {
            if (want == 1) {   // enter long at the incoming day's open
                ts_long_ = true; ts_entry_px_ = new_day_open; ts_entry_ts_ = new_day_open_ts;
                ts_open_live_(new_day_open);
            } else {           // momentum flip -> flat at the incoming day's open
                double net = new_day_open / ts_entry_px_ - 1.0 - TS_COST_RT;
                emit_(TradeRec{TSMOM30, ts_entry_ts_, new_day_open_ts, net, ts_entry_px_, new_day_open, "TSMOM_FLIP"});
                ts_long_ = false;
                ts_close_live_(new_day_open);
            }
        }
    }

    void tc_close_(double x, int64_t ts, const char* reason, int64_t d) {
        double net = x / tc_e_ - 1.0;
        emit_(TradeRec{TRENDCORE, tc_ts_, ts, net, tc_e_, x, reason});
        if (x < tc_e_) tc_cooldown_until_day_ = d + 10;
        else { tc_have_block_ = true; tc_block_high_ = tc_peak_; }
        tc_close_live_(x, reason);
        tc_active_ = false;
    }

    void emit_(const TradeRec& t) {
        ++n_trades_;
        if (on_trade) on_trade(t);
    }

    // ── live order routing (no-ops in parity/shadow: live_enabled=false) ──────
    void tc_open_live_(double px) {
        if (!live_enabled || !submit || order_usd <= 0.0 || px <= 0.0 || tc_live_qty_ > 0.0) return;
        auto r = submit({ up_(symbol), true, order_usd / px, px, /*is_exit*/false, "BTC-REGIME-TC" },
                        Factor::MOMENTUM);
        if (r.ok && !r.shadow && r.executed_qty > 0.0) {
            tc_live_qty_ = r.executed_qty; tc_live_px_ = r.avg_price > 0.0 ? r.avg_price : px;
            std::printf("[BTC-REGIME-BOOK] TRENDCORE BUY qty=%.8f @%.2f (~$%.2f)\n",
                        tc_live_qty_, tc_live_px_, tc_live_qty_ * tc_live_px_);
            std::fflush(stdout); persist_();
        } else if (!r.ok) {
            std::printf("[BTC-REGIME-BOOK] TRENDCORE BUY skipped: %s\n", r.error.empty() ? "rejected" : r.error.c_str());
            std::fflush(stdout);
        }
    }
    void tc_close_live_(double px, const char* reason = "CLOSE") {
        if (!live_enabled || !submit || tc_live_qty_ <= 0.0) return;
        double q = tc_live_qty_;
        if (free_base) { std::string b = base_(symbol); if (!b.empty()) { double f = free_base(b); if (f >= 0.0) q = std::min(q, f * (1.0 - 1e-4)); } }
        auto r = submit({ up_(symbol), false, q, px, /*is_exit*/true, "BTC-REGIME-TC" }, Factor::MOMENTUM);
        double xp = r.ok && r.avg_price > 0.0 ? r.avg_price : px;
        book_live_(TRENDCORE, tc_live_px_, xp, q, reason);
        tc_live_qty_ = 0.0; tc_live_px_ = 0.0; persist_();
    }
    void ts_open_live_(double px) {
        if (!live_enabled || !submit || order_usd <= 0.0 || px <= 0.0 || ts_live_qty_ > 0.0) return;
        auto r = submit({ up_(symbol), true, order_usd / px, px, /*is_exit*/false, "BTC-REGIME-TS" },
                        Factor::MOMENTUM);
        if (r.ok && !r.shadow && r.executed_qty > 0.0) {
            ts_live_qty_ = r.executed_qty; ts_live_px_ = r.avg_price > 0.0 ? r.avg_price : px;
            std::printf("[BTC-REGIME-BOOK] TSMOM30 BUY qty=%.8f @%.2f (~$%.2f)\n",
                        ts_live_qty_, ts_live_px_, ts_live_qty_ * ts_live_px_);
            std::fflush(stdout); persist_();
        } else if (!r.ok) {
            std::printf("[BTC-REGIME-BOOK] TSMOM30 BUY skipped: %s\n", r.error.empty() ? "rejected" : r.error.c_str());
            std::fflush(stdout);
        }
    }
    void ts_close_live_(double px) {
        if (!live_enabled || !submit || ts_live_qty_ <= 0.0) return;
        double q = ts_live_qty_;
        if (free_base) { std::string b = base_(symbol); if (!b.empty()) { double f = free_base(b); if (f >= 0.0) q = std::min(q, f * (1.0 - 1e-4)); } }
        auto r = submit({ up_(symbol), false, q, px, /*is_exit*/true, "BTC-REGIME-TS" }, Factor::MOMENTUM);
        double xp = r.ok && r.avg_price > 0.0 ? r.avg_price : px;
        book_live_(TSMOM30, ts_live_px_, xp, q, "TSMOM_FLIP");
        ts_live_qty_ = 0.0; ts_live_px_ = 0.0; persist_();
    }
    void book_live_(Sleeve s, double entry, double exit_px, double qty, const char* reason) {
        double usd = (exit_px - entry) * qty; realized_usd_ += usd;
        long long now_s = (long long)std::time(nullptr);
        std::ofstream f(trades_csv, std::ios::app);
        if (f.is_open()) {
            f << now_s * 1000LL << "," << (s == TRENDCORE ? "TRENDCORE" : "TSMOM30") << ","
              << symbol << "," << qty << "," << entry << "," << exit_px << "," << usd << "," << reason << "\n";
        }
        std::printf("[BTC-REGIME-BOOK] %s SELL qty=%.8f %.2f->%.2f realized $%.4f (%s) — total $%.2f\n",
                    s == TRENDCORE ? "TRENDCORE" : "TSMOM30", qty, entry, exit_px, usd, reason, realized_usd_);
        std::fflush(stdout);
        // desk PnL fold: real fill delta into the desk ledger/_clivetot (null in shadow/parity)
        if (on_live_close) on_live_close(s == TRENDCORE ? "BTC-REGIME-TC" : "BTC-REGIME-TS",
                                         base_(symbol), entry, exit_px, qty, usd, reason);
    }
    static std::string up_(const std::string& s) { std::string u = s; for (auto& c : u) c = (char)std::toupper((unsigned char)c); return u; }
    static std::string base_(const std::string& sym_lower) {
        auto p = sym_lower.rfind("usdt"); if (p == std::string::npos || p == 0) return {};
        return up_(sym_lower.substr(0, p));
    }

public:
    // ── persistence (restart-safe) ───────────────────────────────────────────
    void persist_() const {
        std::ostringstream js; js << std::fixed;
        js << "{\"tc\":{\"active\":" << (tc_active_ ? "true" : "false")
           << std::setprecision(2) << ",\"e\":" << tc_e_ << ",\"ts\":" << tc_ts_
           << ",\"stop\":" << tc_stop_ << ",\"R\":" << tc_R_ << ",\"mfe\":" << tc_mfe_
           << ",\"peak\":" << tc_peak_ << ",\"floor\":" << tc_floor_
           << ",\"protected\":" << (tc_protected_ ? "true" : "false")
           << ",\"exit_next\":" << (tc_exit_next_ ? "true" : "false")
           << ",\"di0\":" << tc_di0_ << ",\"last_trail_di\":" << tc_last_trail_di_
           << ",\"cooldown\":" << tc_cooldown_until_day_
           << ",\"have_block\":" << (tc_have_block_ ? "true" : "false") << ",\"block_high\":" << tc_block_high_
           << ",\"have_last_trig\":" << (tc_have_last_trig_ ? "true" : "false") << ",\"last_trig\":" << tc_last_trigger_
           << std::setprecision(8) << ",\"live_qty\":" << tc_live_qty_ << std::setprecision(2) << ",\"live_px\":" << tc_live_px_ << "},"
           << "\"ts\":{\"long\":" << (ts_long_ ? "true" : "false")
           << ",\"entry_px\":" << ts_entry_px_ << ",\"entry_ts\":" << ts_entry_ts_ << ",\"pending\":" << ts_pending_
           << std::setprecision(8) << ",\"live_qty\":" << ts_live_qty_ << std::setprecision(2) << ",\"live_px\":" << ts_live_px_ << "},"
           << "\"realized_usd\":" << realized_usd_ << "}\n";
        std::string tmp = persist_path + ".tmp";
        std::ofstream f(tmp); if (!f.is_open()) return; f << js.str(); f.close();
        std::rename(tmp.c_str(), persist_path.c_str());
    }
    void load() {
        std::ifstream f(persist_path); if (!f.is_open()) return;
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        // tc and ts are separate JSON objects; live_qty/live_px keys repeat, so parse
        // each block independently (split at the "ts": object key).
        auto tcp = s.find("\"tc\":");
        auto tsp = s.find("\"ts\":", tcp == std::string::npos ? 0 : tcp + 5);
        auto sub = [&](size_t a, size_t b) { return (a == std::string::npos) ? std::string() : s.substr(a, (b == std::string::npos ? s.size() : b) - a); };
        std::string tcs = sub(tcp, tsp), tss = sub(tsp, std::string::npos);
        auto gnum = [](const std::string& blk, const char* key, double def) -> double {
            auto p = blk.find(std::string("\"") + key + "\":"); if (p == std::string::npos) return def;
            p += std::strlen(key) + 3; return atof(blk.c_str() + p);
        };
        auto gbl = [](const std::string& blk, const char* key) -> bool {
            auto p = blk.find(std::string("\"") + key + "\":"); if (p == std::string::npos) return false;
            p += std::strlen(key) + 3; return blk.compare(p, 4, "true") == 0;
        };
        tc_active_ = gbl(tcs, "active"); tc_e_ = gnum(tcs, "e", 0); tc_ts_ = (int64_t)gnum(tcs, "ts", 0);
        tc_stop_ = gnum(tcs, "stop", 0); tc_R_ = gnum(tcs, "R", 0); tc_mfe_ = gnum(tcs, "mfe", 0);
        tc_peak_ = gnum(tcs, "peak", 0); tc_floor_ = gnum(tcs, "floor", 0);
        tc_protected_ = gbl(tcs, "protected"); tc_exit_next_ = gbl(tcs, "exit_next");
        tc_di0_ = (int)gnum(tcs, "di0", -1); tc_last_trail_di_ = (int)gnum(tcs, "last_trail_di", -1);
        tc_cooldown_until_day_ = (int64_t)gnum(tcs, "cooldown", -1);
        tc_have_block_ = gbl(tcs, "have_block"); tc_block_high_ = gnum(tcs, "block_high", 0);
        tc_have_last_trig_ = gbl(tcs, "have_last_trig"); tc_last_trigger_ = gnum(tcs, "last_trig", 0);
        tc_live_qty_ = gnum(tcs, "live_qty", 0); tc_live_px_ = gnum(tcs, "live_px", 0);
        ts_long_ = gbl(tss, "long"); ts_entry_px_ = gnum(tss, "entry_px", 0);
        ts_entry_ts_ = (int64_t)gnum(tss, "entry_ts", 0); ts_pending_ = (int)gnum(tss, "pending", -1);
        ts_live_qty_ = gnum(tss, "live_qty", 0); ts_live_px_ = gnum(tss, "live_px", 0);
        realized_usd_ = gnum(tss, "realized_usd", 0);   // realized_usd sits after ts block; search whole
        { auto p = s.find("\"realized_usd\":"); if (p != std::string::npos) realized_usd_ = atof(s.c_str() + p + 15); }
        std::printf("[BTC-REGIME-BOOK] restored state: TRENDCORE %s%s | TSMOM30 %s | realized $%.2f\n",
                    tc_active_ ? "IN-POSITION" : "flat",
                    tc_live_qty_ > 0.0 ? " (live coins held)" : "",
                    ts_long_ ? "LONG" : "flat", realized_usd_);
        std::fflush(stdout);
    }

    // ── boot summary line (proves the book loaded with certified params) ──────
    std::string boot_summary() const {
        char b[512];
        std::snprintf(b, sizeof(b),
            "TRENDCORE LB%d/EMA%d/EX%d/CH%.1f NO-ADD (28bp) + TSMOM30 K%d (30bp) 50/50 | "
            "daily=%d bars warm=%s regime=%s | TC=%s TSMOM=%s | clip=$%.2f",
            TC_LB, TC_EMA_N, TC_EXIT_LB, TC_CH_ATR, TS_K,
            (int)daily_.size(), warm() ? "YES" : "NO", regime_on() ? "BULL" : "off",
            tc_active_ ? "in-pos" : "flat",
            ts_long_ ? "long" : (ts_want() == 1 ? "want-long(buys next open)" : "flat"),
            order_usd);
        return std::string(b);
    }

    int    live_qty_positions() const { return (tc_live_qty_ > 0.0 ? 1 : 0) + (ts_live_qty_ > 0.0 ? 1 : 0); }
    double realized_usd() const { return realized_usd_; }

    // ── KILL flatten: market-sell any live sleeve holding (mirrors the desk kill) ──
    int flatten_all(const char* why) {
        if (!live_enabled || !submit) return 0;
        int sold = 0;
        if (tc_live_qty_ > 0.0) { tc_close_live_(tc_live_px_ > 0.0 ? tc_live_px_ : tc_e_, why); ++sold; tc_active_ = false; }
        if (ts_live_qty_ > 0.0) { ts_close_live_(ts_live_px_ > 0.0 ? ts_live_px_ : ts_entry_px_); ++sold; ts_long_ = false; }
        persist_();
        return sold;
    }
};

} // namespace chimera
