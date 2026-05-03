// ============================================================================
// SwingEngine — H4/D1 Multi-Strategy Spot Swing Engine  (v8 — accounting)
//
// CHANGE LOG (v8 — bookkeeping fix after v7 hit PF 1.02 / +279 bp but the
// trade-log was systematically under-counting wins):
//   * Each position now stores partial_exit_px (the price at which the
//     50% partial-exit fired). Previously this price was logged but not
//     persisted on the position, so the trade-log's pnl_pct only captured
//     entry -> final-exit, missing the partial leg entirely.
//   * Added _compute_trade_pnl_pct() helper. Returns the trade's true
//     economic % return on deployed capital, computed as:
//       partial_qty x (partial_exit_px - entry_px)
//         + remaining_qty x (exit_px - blended_entry)
//       all divided by qty_full * entry_px.
//     Pyramid-add positions are handled correctly via blended_entry, which
//     is already the weighted-avg cost basis of the remaining position.
//   * total_pnl_pct_ now updated once per trade (at _close) using the true
//     pnl_pct, instead of the previous broken split that added partial-half
//     in _manage and the full final-leg pct in _close (which double-counted
//     the post-partial portion).
//   * trade_log entries now record true pnl_pct, so backtest summary
//     reflects real economic outcomes including partial profits.
//   * No strategy logic changes — entries, stops, trail, vol filter, symbol
//     whitelist all preserved from v7.
//
// CHANGE LOG (v7 — single change after v6 backtest landed at PF 0.98 / WR
// 55.3% / -468 bp, i.e. right at the breakeven threshold pre-fees):
//   * Added a volatility regime filter on Donchian entries. Compute the
//     average true range over the most recent 5 H4 bars (current_atr_5) and
//     over the prior 20 H4 bars (baseline_atr_20). Skip the entry when
//     current_atr_5 > 1.5 x baseline_atr_20. Theory: Donchian breakouts in
//     normal vol tend to extend (start of a move); breakouts during a vol
//     spike are usually exhaustion blowoffs (end of a move).
//   * No other changes from v6 — entries, stops, trail, partial, pyramid,
//     symbol whitelist (BTC/ETH/XRP) all preserved.
//
// CHANGE LOG (v6 — ablation after v5 backtest revealed two clear signals):
//   * DON_REVERSE was destroying value: 49 trades at PF 0.03 (-7193 bp).
//     Reverse-Donchian on 10-bar lookback triggered on normal pullbacks
//     inside intact trends — 86% of reverse-exits were losers because price
//     would have recovered. The call site in on_tick is removed; the
//     _check_donchian_reverse helper stays in the file for future use.
//   * Without DON_REVERSE, SL_HIT alone produced PF 1.00 over 173 trades —
//     i.e. the 1.8x ATR stop change from v5 was correct, the reverse exit
//     was the problem.
//   * Tradable list further restricted: BTC, ETH, XRP only. Dropped SOL and
//     BNB — together they accounted for ~96% of losses (-6937 bp). SOL is
//     too volatile for a 20-bar Donchian on H4 (false breakouts dominate).
//
// CHANGE LOG (v5 — exit-side fixes after v4 backtest showed PF=0.76 with
// 57.7% WR but losses 1.79x bigger than wins, i.e. entries are roughly right
// but stops bleed too much on false breakouts):
//   * S1_ATR_SL_MULT: 2.5 -> 1.8. Donchian breakouts fire when ATR is
//     elevated (end of momentum bursts), so a 2.5x ATR stop produced losses
//     averaging ~5% per failed entry. 1.8x cuts that by ~28% before WR
//     trade-off. Net effect on expectancy: positive even at lower WR.
//   * Added reverse-Donchian (Turtle "S2 exit") at DON_EXIT_BARS=10. While a
//     long position is open and an H4 bar closes below the lowest close of
//     the prior 10 H4 bars, exit immediately at that close. Mirror for
//     shorts. This catches losing trades earlier than the ATR stop in cases
//     where the trend has clearly reversed, without giving back the full
//     1.8x ATR every time. Recorded as why="DON_REVERSE".
//   * Entries unchanged from v4 — BTC/ETH at 63%/62% WR show the breakout
//     signal itself is sound; only the exit side needs work.
//
// CHANGE LOG (v4 — strategy pivot after v3 backtest showed PF=0.67 on 219
// trades, i.e. EMA-pullback has no edge in 2025-2026 crypto):
//   * S1 entry rewritten as a 20-bar H4 Donchian breakout in confirmed D1
//     trend regime. We now BUY strength (close at 20-bar high in D1 bull) and
//     SELL weakness (close at 20-bar low in D1 bear) instead of fading
//     pullbacks. Trend-following captures the fat tail that drives crypto
//     returns; pullback systems lose to it.
//   * Tradable symbol whitelist: BTC, ETH, SOL, BNB, XRP only. AVAX, LINK,
//     DOGE dropped — they were the bottom three in the v3 backtest with PF
//     0.51 / 0.52 / 0.59 and accounted for ~66% of total losses. The engine
//     still seeds and tracks indicators for all 8 (so the GUI shows them) but
//     does not enter positions on the dropped three.
//   * S2 / S3 / S4 disabled (calls commented out in on_tick). The code is
//     left in place for later reactivation once Donchian S1's edge is
//     verified in isolation.
//   * S1 enum value kept as S1_PULLBACK to preserve GUI compatibility (the
//     dashboard shows "SWING-S1" regardless of underlying logic).
//   * Trail / partial-exit / pyramid logic in _manage unchanged — sound
//     for trend-following too.
//
// CHANGE LOG (v3 — calibration after backtest showed only 3 trades in 8 mo):
//   * S1: RSI gate is now a simple cross through 50 (prev<50 AND current>50)
//     instead of v2's "prev<=45 AND current>=55" (which required a 10+ point
//     single-bar swing — empirically almost never triggers).
//   * S1: dropped redundant "D1 close on right side of EMA21" gate (already
//     covered by EMA21 vs EMA50 separation check).
//   * S1: dropped redundant "MACD not deeply negative" gate (already covered
//     by the rising-histogram check).
//   * S1: D1 EMA21/EMA50 separation 1.0% -> 0.6% (1.0% locked out healthy
//     swing trends; 0.6% is tighter than v1's 0.5% but actually achievable).
//   * S1: SL 2.5xATR -> 2.0xATR. v2 was over-correcting v1's 1.5x.
//   * S1: trail arm 3xATR MFE -> 2xATR MFE. v2's 3x meant winners that
//     reached +1.3xATR (like the BTC trade in the backtest) timed out before
//     the trail ever armed.
//   * S2: RSI window widened 40-55 -> 30-60. Drop "MACD must be positive";
//     only require "MACD rising".
//   * S3: SL 2.0xATR -> 1.8xATR.
//   * S4: compression ratio 0.45 -> 0.60 (0.45 is essentially never reached
//     on H4 in crypto; 0.60 lets compression actually trigger).
//
// Kept from v2:
//   * Entries fire ONLY on H4 bar close (this fixed v1's 17-trades-in-82s).
//   * Structural exit on D1 regime flip.
//   * 12h same-symbol cooldown, 24h flip cooldown.
//
// CHANGE LOG (v2):
//   * Entries now fire ONLY on H4 bar close. Previously _try_s1..s4 ran on
//     every tick, which caused the engine to fire as soon as a tick brushed a
//     pullback / retest zone mid-bar. That produced fast micro-fills that hit
//     ATR-tight stops on noise. Bar-close gating is the single biggest fix.
//   * Tighter trend filter: D1 EMA21/EMA50 must be ≥1.0% apart (was 0.5%).
//   * Wider stops: S1 2.5×ATR (was 1.5×), S2 2.0×ATR (was 1.2×),
//                  S3 2.0×ATR (was 1.2×).
//   * True RSI cross confirmation: tracks the previous closed-bar RSI so the
//     long entry requires "prev RSI was ≤45 AND current RSI ≥55" (and the
//     mirrored window for shorts). The earlier "RSI is between 42 and 65"
//     proxy let entries fire while RSI was actually FALLING through 50.
//   * Stricter momentum: MACD histogram must be both ≥ prior and rising.
//   * Structural exit: if D1 regime flips against an open S1/S3 position, the
//     position closes immediately rather than waiting 5–7 days to time out.
//   * Cooldowns extended (6h→12h same-symbol, 12h→24h flip).
//   * Public TradeLog + set_max_trade_log_size + get_trade_log accessors so
//     the chimera_backtest harness can pull every closed trade.
//
// Public API unchanged from v1 — main.cpp / gui_server.py do not need changes.
// ============================================================================
#pragma once

#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <string>
#include <chrono>
#include <algorithm>

#include "core/SymbolIndex.hpp"
#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "version_generated.hpp"
#include <curl/curl.h>

#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

namespace chimera {

struct OHLCBar {
    double open  = 0.0;
    double high  = 0.0;
    double low   = 0.0;
    double close = 0.0;
    double volume = 0.0;
    int64_t bar_ms = 0;
};

struct BarBuilder {
    static constexpr int HISTORY = 32;

    OHLCBar  current;
    OHLCBar  closed[HISTORY];
    int      head     = 0;
    int      count    = 0;
    int64_t  bar_boundary_ms = 0;
    int64_t  period_ms;

    explicit BarBuilder(int64_t period_ms_) : period_ms(period_ms_) {}

    bool on_tick(double price, int64_t now_ms, double volume_increment = 0.0) {
        const int64_t boundary = (now_ms / period_ms) * period_ms;

        if (bar_boundary_ms == 0) {
            bar_boundary_ms = boundary;
            current = {price, price, price, price, 0.0, boundary};
            return false;
        }

        if (boundary != bar_boundary_ms) {
            closed[head] = current;
            head         = (head + 1) % HISTORY;
            if (count < HISTORY) ++count;

            current         = {price, price, price, price, volume_increment, boundary};
            bar_boundary_ms = boundary;
            return true;
        }

        if (price > current.high) current.high = price;
        if (price < current.low)  current.low  = price;
        current.close  = price;
        current.volume += volume_increment;
        return false;
    }

    const OHLCBar* get(int offset) const {
        if (offset < 0 || offset >= count) return nullptr;
        int idx = (head - 1 - offset + HISTORY) % HISTORY;
        return &closed[idx];
    }

    bool ready(int min_bars = 1) const { return count >= min_bars; }
};

struct SwingIndicators {
    double ema9   = 0.0;
    double ema21  = 0.0;
    double ema50  = 0.0;
    double atr14  = 0.0;
    double rsi14       = 50.0;
    double prev_rsi14  = 50.0;
    double avg_gain    = 0.0;
    double avg_loss    = 0.0;
    double ema12   = 0.0;
    double ema26   = 0.0;
    double macd    = 0.0;
    double signal  = 0.0;
    double hist    = 0.0;
    double prev_hist = 0.0;
    double obv     = 0.0;
    double obv_ema = 0.0;
    int  bar_count = 0;
    bool ready     = false;

    void update(double h, double l, double close, double prev_close, double volume) {
        const double alpha9  = 2.0 / 10.0;
        const double alpha21 = 2.0 / 22.0;
        const double alpha50 = 2.0 / 51.0;
        const double alpha12 = 2.0 / 13.0;
        const double alpha26 = 2.0 / 27.0;
        const double alpha9s = 2.0 / 10.0;
        const double alpha14 = 1.0 / 14.0;

        prev_rsi14 = rsi14;

        if (bar_count == 0) {
            ema9 = ema21 = ema50 = ema12 = ema26 = close;
            avg_gain = avg_loss = 0.0;
            obv = obv_ema = 0.0;
        } else {
            ema9  += alpha9  * (close - ema9);
            ema21 += alpha21 * (close - ema21);
            ema50 += alpha50 * (close - ema50);

            double tr = h - l;
            if (prev_close > 0.0) {
                double tr2 = std::fabs(h - prev_close);
                double tr3 = std::fabs(l - prev_close);
                if (tr2 > tr) tr = tr2;
                if (tr3 > tr) tr = tr3;
            }
            if (atr14 == 0.0) atr14 = tr;
            else               atr14 += alpha14 * (tr - atr14);

            if (prev_close > 0.0) {
                double chg = close - prev_close;
                double gain = chg > 0.0 ? chg : 0.0;
                double loss = chg < 0.0 ? -chg : 0.0;
                if (bar_count == 1) {
                    avg_gain = gain;
                    avg_loss = loss;
                } else {
                    avg_gain = avg_gain + alpha14 * (gain - avg_gain);
                    avg_loss = avg_loss + alpha14 * (loss - avg_loss);
                }
                if (avg_loss < 1e-10) rsi14 = 100.0;
                else {
                    double rs = avg_gain / avg_loss;
                    rsi14 = 100.0 - (100.0 / (1.0 + rs));
                }
            }

            ema12 += alpha12 * (close - ema12);
            ema26 += alpha26 * (close - ema26);
            prev_hist = hist;
            macd = ema12 - ema26;
            if (bar_count == 1) signal = macd;
            else                signal += alpha9s * (macd - signal);
            hist = macd - signal;

            if (prev_close > 0.0) {
                if (close > prev_close)      obv += volume;
                else if (close < prev_close) obv -= volume;
            }
            obv_ema += alpha14 * (obv - obv_ema);
        }

        ++bar_count;
        if (bar_count >= 50) ready = true;
    }
};

enum class SwingStrategy : uint8_t {
    NONE = 0,
    S1_PULLBACK   = 1,
    S2_DIVERGENCE = 2,
    S3_BREAKOUT   = 3,
    S4_BRACKET    = 4
};

struct SwingPosition {
    bool           active        = false;
    bool           is_long       = true;
    SwingStrategy  strategy      = SwingStrategy::NONE;
    double         entry_px      = 0.0;
    double         sl_px         = 0.0;
    double         tp_px         = 0.0;
    double         trail_sl      = 0.0;
    double         mfe           = 0.0;
    bool           trail_armed   = false;
    int            trail_stage   = 0;
    bool           partial_done  = false;
    double         qty           = 0.0;
    double         qty_full      = 0.0;
    int64_t        entry_ms      = 0;
    int64_t        max_hold_ms   = 0;
    char           symbol[16]    = {};
    int            trade_id      = 0;
    int            entry_d1_dir  = 0;
    bool           pyramid_done  = false;
    double         pyramid_qty   = 0.0;
    double         blended_entry = 0.0;
    // v8: track partial-exit price so trade pnl_pct includes the partial leg
    double         partial_exit_px = 0.0;
};

class SwingEngine {
public:
    static constexpr int64_t H4_MS  = 14400000LL;
    static constexpr int64_t D1_MS  = 86400000LL;

    static constexpr double MIN_QTY_USD = 50.0;
    static constexpr double MAX_QTY_USD = 500.0;

    // v4 Donchian breakout (S1) + v5 reverse-Donchian exit + v7 vol filter
    static constexpr int    DON_BREAKOUT_BARS = 20;     // H4 lookback for breakout extreme (entry)
    static constexpr int    DON_EXIT_BARS     = 10;     // v5: H4 lookback for reverse-Donchian exit (disabled in v6)
    static constexpr int    VOL_FILTER_FAST   = 5;      // v7: short-window ATR
    static constexpr int    VOL_FILTER_SLOW   = 20;     // v7: baseline ATR
    static constexpr double VOL_FILTER_RATIO  = 1.5;    // v7: skip entry if fast/slow > this
    static constexpr double S1_EMA_SEP_PCT   = 0.006;   // D1 EMA21/EMA50 trend strength gate
    static constexpr double S1_PULLBACK_ATR  = 0.6;     // (legacy v3, unused in Donchian)
    static constexpr double S1_RSI_LONG_LO   = 50.0;    // (legacy v3, unused in Donchian)
    static constexpr double S1_RSI_LONG_HI   = 50.0;    // (legacy v3, unused in Donchian)
    static constexpr double S1_RSI_SHORT_HI  = 50.0;    // (legacy v3, unused in Donchian)
    static constexpr double S1_RSI_SHORT_LO  = 50.0;    // (legacy v3, unused in Donchian)
    static constexpr double S1_ATR_SL_MULT   = 1.8;     // v4:2.5 -> v5:1.8 (cut avg loss ~28%)
    static constexpr double S1_TRAIL_ARM_ATR = 2.0;     // trail arms at 2xATR favourable move
    static constexpr double S1_TRAIL_ATR     = 1.5;
    static constexpr double S1_TRAIL_STAGE2_ATR = 5.0;
    static constexpr double S1_TRAIL_DIST2   = 1.0;
    static constexpr double S1_TRAIL_STAGE3_ATR = 8.0;
    static constexpr double S1_TRAIL_DIST3   = 0.5;
    static constexpr int64_t S1_MAX_HOLD_MS  = 604800000LL;

    static constexpr double S2_RSI_OVERSOLD  = 30.0;    // v2:40 -> v3:30 (widened)
    static constexpr double S2_RSI_CEILING   = 60.0;    // v3 (was hardcoded 55 in body)
    static constexpr int    S2_DIV_LOOKBACK  = 5;
    static constexpr double S2_ATR_SL_MULT   = 1.8;     // v2:2.0 -> v3:1.8
    static constexpr double S2_R_MULT        = 3.0;
    static constexpr int64_t S2_MAX_HOLD_MS  = 432000000LL;

    static constexpr int     S3_RESIST_BARS  = 20;
    static constexpr double  S3_RETEST_ATR   = 0.4;
    static constexpr double  S3_ATR_SL_MULT  = 1.8;     // v2:2.0 -> v3:1.8
    static constexpr int64_t S3_MAX_HOLD_MS  = 432000000LL;

    static constexpr int     S4_COMPRESS_BARS  = 5;
    static constexpr int     S4_BASELINE_BARS  = 20;
    static constexpr double  S4_COMPRESS_RATIO = 0.60;  // v2:0.45 -> v3:0.60
    static constexpr double  S4_OBV_CONFIRM    = 0.0;
    static constexpr double  S4_ATR_SL_MULT    = 1.0;
    static constexpr int64_t S4_MAX_HOLD_MS    = 432000000LL;
    static constexpr int64_t S4_FLIP_EXEMPT_MS = 0LL;

    static constexpr int64_t COOLDOWN_MS      = 43200000LL;
    static constexpr int64_t FLIP_COOLDOWN_MS = 86400000LL;

    bool shadow_mode = true;

    void set_executor(SpotExecutor* ex) { executor_ = ex; }

    void seed_from_history() {
        printf("[SWING-SEED] Fetching H4 + D1 history for %d symbols...\n", MAX_SYMBOLS);
        fflush(stdout);
        for (int id = 0; id < MAX_SYMBOLS; ++id) {
            _seed_symbol(id, "4h",  h4_builders_[id], h4_ind_[id]);
            _seed_symbol(id, "1d",  d1_builders_[id], d1_ind_[id]);
        }
        printf("[SWING-SEED] Done. Ready symbols (H4/D1):\n");
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            printf("  %s: H4=%s D1=%s\n",
                   sym_short(i),
                   h4_ind_[i].ready ? "READY" : "warming",
                   d1_ind_[i].ready ? "READY" : "warming");
        }
        fflush(stdout);
    }

    void on_tick(int id, const MarketTick& tick, int64_t now_ms) {
        if (id < 0 || id >= MAX_SYMBOLS) return;

        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (price <= 0.0) return;
        prices_[id] = price;
        now_ms_last_[id] = now_ms;

        double vol_inc = tick.trade_qty;

        const bool h4_closed = h4_builders_[id].on_tick(price, now_ms, vol_inc);
        const bool d1_closed = d1_builders_[id].on_tick(price, now_ms, vol_inc);

        if (h4_closed && h4_builders_[id].count >= 2) {
            const OHLCBar* b   = h4_builders_[id].get(0);
            const OHLCBar* bprev = h4_builders_[id].get(1);
            if (b && bprev) {
                h4_ind_[id].update(b->high, b->low, b->close, bprev->close, b->volume);
            }
        }
        if (d1_closed && d1_builders_[id].count >= 2) {
            const OHLCBar* b   = d1_builders_[id].get(0);
            const OHLCBar* bprev = d1_builders_[id].get(1);
            if (b && bprev) {
                d1_ind_[id].update(b->high, b->low, b->close, bprev->close, b->volume);
            }
        }

        auto& pos = positions_[id];
        if (pos.active) {
            // v6: DON_REVERSE call removed — the 10-bar reverse exit fired
            // on normal in-trend pullbacks (86% of reverse-exits were
            // losers in v5 backtest) and destroyed -7193 bp of value.
            // The _check_donchian_reverse helper is preserved below for
            // possible future re-enable with a different lookback.
            _manage(id, price, now_ms);
            return;
        }

        if (!h4_closed) return;
        if (!h4_ind_[id].ready) return;
        if (!d1_ind_[id].ready) return;
        if (now_ms < cooldown_until_ms_[id]) return;

        const OHLCBar* b0 = h4_builders_[id].get(0);
        if (!b0) return;
        const double signal_px = b0->close;

        if (_try_s1(id, signal_px, now_ms)) return;
        // v4: S2/S3/S4 disabled while we evaluate Donchian S1 in isolation.
        // Re-enable individually after S1 backtest confirms positive edge.
        // if (_try_s2(id, signal_px, now_ms)) return;
        // if (_try_s3(id, signal_px, now_ms)) return;
        // if (_try_s4(id, signal_px, now_ms)) return;
    }

    void kill_all() {
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            auto& pos = positions_[i];
            if (!pos.active) continue;
            printf("[SWING-KILL] %s %s entry=%.4f\n",
                   sym_short(i), pos.is_long ? "LONG" : "SHORT", pos.entry_px);
            fflush(stdout);
            if (executor_) executor_->execute(pos.symbol, !pos.is_long, pos.qty, prices_[i]);
            // v8: compute true economic pnl including any partial leg
            const double kill_pnl = _compute_trade_pnl_pct(pos, prices_[i]);
            total_pnl_pct_ += kill_pnl;
            if (kill_pnl > 0) ++wins_;
            _record_exit(i, prices_[i], now_ms_last_[i], "KILL", kill_pnl);
            pos = SwingPosition{};
        }
    }

    void update_price(int id, double price) {
        if (id >= 0 && id < MAX_SYMBOLS) prices_[id] = price;
    }

    int total_trades()    const { return trade_counter_; }
    double total_pnl_pct() const { return total_pnl_pct_; }

    int _count_active() const {
        int n = 0;
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            if (positions_[i].active) ++n;
        return n;
    }

    std::string state_json() const {
        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{";

        const double session_pnl_bp = total_pnl_pct_ * 100.0;
        js << "\"pnl\":"           << session_pnl_bp  << ",";
        js << "\"realized_pnl\":"  << session_pnl_bp  << ",";
        js << "\"total_trades\":"  << trade_counter_  << ",";
        js << "\"open_positions\":" << _count_active() << ",";
        js << "\"build_ver\":\""   << BUILD_VERSION   << "\",";
        js << "\"latency_p95\":1.0,";
        js << "\"shadow\":true,";

        static constexpr const char* FULL[MAX_SYMBOLS] = {
            "btcusdt","ethusdt","solusdt","bnbusdt","avaxusdt","linkusdt","xrpusdt","dogeusdt"
        };

        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            js << "\"" << FULL[i] << "_price\":" << prices_[i] << ",";
        }

        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            const auto& pos  = positions_[i];
            const auto& h4   = h4_ind_[i];
            const auto& d1   = d1_ind_[i];
            const double px  = prices_[i];

            const char* regime = "DEAD";
            if (h4.ready && d1.ready) {
                regime = (d1.ema21 > d1.ema50) ? "BUILDUP" : "GRIND";
            }

            const double ema_sep_pct = d1.ema50 > 0
                ? std::fabs(d1.ema21 - d1.ema50) / d1.ema50 * 100.0 : 0.0;
            const double vol_ratio   = ema_sep_pct / 0.5;

            const double disp_bp = (px > 0 && h4.ema21 > 0)
                ? (px - h4.ema21) / h4.ema21 * 10000.0 : 0.0;

            const double readiness = std::min(1.0, (double)h4.bar_count / 50.0);

            const double pnl_now = pos.active
                ? (pos.is_long ? (px - pos.entry_px) / pos.entry_px * 10000.0
                               : (pos.entry_px - px) / pos.entry_px * 10000.0)
                : 0.0;

            js << "\"" << FULL[i] << "\":{"
               << "\"regime_state\":\"" << regime << "\","
               << "\"vol_ratio\":"       << vol_ratio  << ","
               << "\"displacement_bp\":" << disp_bp    << ","
               << "\"dynamic_cap_R\":"   << (h4.atr14 > 0 ? 2.0 : 0.0) << ","
               << "\"micro_active\":"    << (pos.active ? "true" : "false") << ","
               << "\"liq_active\":"      << (pos.active ? "true" : "false") << ","
               << "\"liq_move_bp\":"     << pnl_now    << ","
               << "\"liq_mfe_bp\":"      << (pos.active ? pos.mfe / (px > 0 ? px : 1.0) * 10000.0 : 0.0) << ","
               << "\"liq_notional\":0.0,"
               << "\"bracket_active\":false,"
               << "\"bracket_state\":\"IDLE\","
               << "\"bracket_range_pct\":0.0,"
               << "\"bracket_total_pnl_bp\":0.0,"
               << "\"bracket_trail_armed\":false,"
               << "\"bracket_trail_floor\":0.0,"
               << "\"bracket_move_bp\":0.0,"
               << "\"bracket_mfe_bp\":0.0,"
               << "\"basis_active\":false,"
               << "\"basis_move_bp\":0.0,"
               << "\"basis_mfe_bp\":0.0,"
               << "\"basis_trail_armed\":false,"
               << "\"basis_trail_floor\":0.0,"
               << "\"fundwin_active\":false,"
               << "\"fundwin_move_bp\":0.0,"
               << "\"fundwin_mfe_bp\":0.0,"
               << "\"fundwin_rate_bp\":0.0,"
               << "\"fundwin_secs_to_next\":0,"
               << "\"perp_funding_rate\":0.0,"
               << "\"perp_basis_bp\":0.0,"
               << "\"btc_move_bp\":0.0,"
               << "\"mm_imbal_ema\":0.5,"
               << "\"vwap_deviation_bp\":0.0,"
               << "\"vwap_ready\":false,"
               << "\"structural_total_pnl_bp\":0.0,"
               << "\"convex_total_pnl_bp\":0.0,"
               << "\"compression_total_pnl_bp\":0.0,"
               << "\"obi_total_pnl_bp\":0.0,"
               << "\"afe_total_pnl_bp\":0.0,"
               << "\"pce_total_pnl_bp\":0.0,"
               << "\"liq_readiness\":"        << readiness << ","
               << "\"structural_readiness\":" << readiness << ","
               << "\"convex_readiness\":"     << readiness << ","
               << "\"compression_readiness\":" << readiness << ","
               << "\"vol_ratio_raw\":"        << vol_ratio  << ","
               << "\"h4_rsi\":"               << h4.rsi14   << ","
               << "\"h4_ema21\":"             << h4.ema21   << ","
               << "\"d1_ema21\":"             << d1.ema21   << ","
               << "\"d1_ema50\":"             << d1.ema50   << ","
               << "\"strategy\":"             << (int)pos.strategy << ","
               << "\"day_high\":"             << prices_[i] * 1.01 << ","
               << "\"day_low\":"              << prices_[i] * 0.99
               << "},";
        }

        js << "\"trade_log\":[";
        bool first = true;
        for (const auto& t : trade_log_) {
            if (!first) js << ",";
            first = false;
            js << "{\"time\":\"" << t.time << "\","
               << "\"s\":\""    << t.sym  << "\","
               << "\"e\":\"SWING-S" << (int)t.strategy << "\","
               << "\"p\":"      << (t.pnl_pct * 100.0) << ","
               << "\"en\":"     << t.entry << ","
               << "\"ex\":"     << t.exit  << ","
               << "\"mfe\":"    << (t.mfe / (t.entry > 0 ? t.entry : 1.0) * 10000.0) << ","
               << "\"mae\":0.0,"
               << "\"why\":\"" << t.why << "\"}";
        }
        js << "],";

        js << "\"boost_leadlag\":1.0,\"boost_ll_eth_sol\":1.0,";
        js << "\"boost_expand\":1.0,\"boost_vwap\":1.0,";
        js << "\"boost_liq\":1.0,\"boost_ngas\":1.0,";
        js << "\"boost_fund\":1.0,\"boost_sweep\":1.0,\"boost_volshock\":1.0,\"boost_ofi\":1.0,";
        js << "\"layer_adapt\":{},";
        js << "\"rejections\":{},";
        js << "\"session_stats\":{}";
        js << "}";
        return js.str();
    }

    struct TradeLog {
        std::string sym, side, time, why;
        SwingStrategy strategy = SwingStrategy::NONE;
        double entry=0, exit=0, pnl_pct=0, mfe=0;
        int64_t exit_ms = 0;
    };

    void set_max_trade_log_size(int n) { max_trade_log_size_ = n; }
    const std::vector<TradeLog>& get_trade_log() const { return trade_log_; }

private:

    double         prices_[MAX_SYMBOLS]          = {};
    int64_t        now_ms_last_[MAX_SYMBOLS]     = {};
    BarBuilder     h4_builders_[MAX_SYMBOLS]     { BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS), BarBuilder(H4_MS) };
    BarBuilder     d1_builders_[MAX_SYMBOLS]     { BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS), BarBuilder(D1_MS) };
    SwingIndicators h4_ind_[MAX_SYMBOLS];
    SwingIndicators d1_ind_[MAX_SYMBOLS];
    SwingPosition   positions_[MAX_SYMBOLS];
    int64_t         cooldown_until_ms_[MAX_SYMBOLS] = {};
    int             last_exit_dir_[MAX_SYMBOLS]      = {};
    int64_t         last_exit_ms_[MAX_SYMBOLS]       = {};
    SpotExecutor*   executor_     = nullptr;
    int             trade_counter_ = 0;
    double          total_pnl_pct_ = 0.0;
    int             wins_          = 0;

    std::vector<TradeLog> trade_log_;
    int                   max_trade_log_size_ = 100;

    // ── v6: Tradable-symbol whitelist (refined from v4) ────────────────────
    // Indices match SymbolIndex.hpp: BTC=0, ETH=1, SOL=2, BNB=3, AVAX=4,
    // LINK=5, XRP=6, DOGE=7. v4 dropped AVAX/LINK/DOGE; v6 also drops SOL
    // and BNB after they accounted for ~96% of v5 losses (PF 0.59 each).
    // Active set: BTC, ETH, XRP.
    static constexpr bool _is_tradable(int id) {
        return id == 0 || id == 1 || id == 6;
    }

    // ── v8: True economic pnl_pct for a trade ──────────────────────────────
    // Returns the trade's % return on the original deployed capital
    // (qty_full × entry_px), accounting correctly for:
    //   - Partial exit at partial_exit_px (50% of qty_full closed early)
    //   - Pyramid additions (folded into blended_entry, so the remaining-leg
    //     formula handles them)
    //   - Final close at exit_px of whatever qty remains active
    // For trades with no partial / no pyramid this reduces to the simple
    // (exit - entry) / entry expression, so behaviour for those is unchanged.
    double _compute_trade_pnl_pct(const SwingPosition& pos, double exit_px) const {
        if (pos.qty_full <= 0.0 || pos.entry_px <= 0.0) return 0.0;
        const double original_notional = pos.qty_full * pos.entry_px;
        if (original_notional <= 0.0) return 0.0;

        double pnl_dollars = 0.0;

        // Partial-exit leg (if it fired)
        if (pos.partial_done && pos.partial_exit_px > 0.0) {
            const double partial_qty = pos.qty_full * 0.5;
            const double per_unit = pos.is_long
                ? (pos.partial_exit_px - pos.entry_px)
                : (pos.entry_px - pos.partial_exit_px);
            pnl_dollars += partial_qty * per_unit;
        }

        // Remaining-leg close at exit_px (qty includes pyramid additions if any;
        // blended_entry is the weighted-avg cost basis for that remaining qty).
        const double per_unit_close = pos.is_long
            ? (exit_px - pos.blended_entry)
            : (pos.blended_entry - exit_px);
        pnl_dollars += pos.qty * per_unit_close;

        return (pnl_dollars / original_notional) * 100.0;
    }

    // ── v5: Reverse-Donchian exit check ────────────────────────────────────
    // Called on H4 close while a Donchian (S1) position is active.
    // For longs: returns true if the most recent closed H4 bar's close is
    // below the lowest close of the prior DON_EXIT_BARS bars.
    // For shorts: returns true if it's above the highest close of those
    // prior DON_EXIT_BARS bars.
    bool _check_donchian_reverse(int id, bool is_long) const {
        const auto& bb = h4_builders_[id];
        if (bb.count < DON_EXIT_BARS + 1) return false;
        const OHLCBar* b0 = bb.get(0);
        if (!b0) return false;
        double opp = is_long ? 1e18 : 0.0;
        for (int k = 1; k <= DON_EXIT_BARS; ++k) {
            const OHLCBar* bk = bb.get(k);
            if (!bk) break;
            if (is_long) { if (bk->close < opp) opp = bk->close; }
            else         { if (bk->close > opp) opp = bk->close; }
        }
        if (opp <= 0.0 || opp >= 1e17) return false;
        return is_long ? (b0->close < opp) : (b0->close > opp);
    }

    // ── Strategy S1: Donchian Breakout (v4) ─────────────────────────────────
    // Buy when H4 close exceeds the highest H4 close of the prior 20 bars in
    // a confirmed D1 bull regime. Mirror for shorts in D1 bear regime. The
    // entry rides the actual driver of crypto returns (long-term trend), not
    // the v3 "pullback in trend" approach which was net-negative empirically.
    bool _try_s1(int id, double price, int64_t now_ms) {
        if (!_is_tradable(id)) return false;

        const auto& d1  = d1_ind_[id];
        const auto& h4  = h4_ind_[id];
        const auto& bb  = h4_builders_[id];

        if (!d1.ready || !h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (bb.count < DON_BREAKOUT_BARS + 1) return false;

        // v7: Volatility regime filter. Compute average true range over the
        // most-recent VOL_FILTER_FAST H4 bars (current vol) and the prior
        // VOL_FILTER_SLOW bars (baseline vol). Skip the entry if current vol
        // is more than VOL_FILTER_RATIO x baseline — these are vol-spike
        // exhaustion breakouts, empirically the worst-performing subset.
        if (bb.count < VOL_FILTER_SLOW + 1) return false;
        double fast_atr = 0.0;
        int    fast_n   = 0;
        for (int k = 0; k < VOL_FILTER_FAST; ++k) {
            const OHLCBar* b  = bb.get(k);
            const OHLCBar* bp = bb.get(k + 1);
            if (!b || !bp) break;
            double tr = b->high - b->low;
            const double t2 = std::fabs(b->high - bp->close);
            const double t3 = std::fabs(b->low  - bp->close);
            if (t2 > tr) tr = t2;
            if (t3 > tr) tr = t3;
            fast_atr += tr;
            ++fast_n;
        }
        double slow_atr = 0.0;
        int    slow_n   = 0;
        for (int k = 0; k < VOL_FILTER_SLOW; ++k) {
            const OHLCBar* b  = bb.get(k);
            const OHLCBar* bp = bb.get(k + 1);
            if (!b || !bp) break;
            double tr = b->high - b->low;
            const double t2 = std::fabs(b->high - bp->close);
            const double t3 = std::fabs(b->low  - bp->close);
            if (t2 > tr) tr = t2;
            if (t3 > tr) tr = t3;
            slow_atr += tr;
            ++slow_n;
        }
        if (fast_n < VOL_FILTER_FAST || slow_n < VOL_FILTER_SLOW) return false;
        fast_atr /= fast_n;
        slow_atr /= slow_n;
        if (slow_atr <= 0.0) return false;
        if (fast_atr > VOL_FILTER_RATIO * slow_atr) return false;  // skip vol-spike entries

        // D1 trend regime gate
        const double d1_sep = std::fabs(d1.ema21 - d1.ema50) / d1.ema50;
        if (d1_sep < S1_EMA_SEP_PCT) return false;
        const bool d1_bull = d1.ema21 > d1.ema50;
        const bool d1_bear = d1.ema21 < d1.ema50;

        // Flip cooldown — don't reverse our last exit's direction within FLIP_COOLDOWN_MS
        if (last_exit_dir_[id] != 0 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) {
            if (d1_bull && last_exit_dir_[id] == -1) return false;
            if (d1_bear && last_exit_dir_[id] == +1) return false;
        }

        // Most recent closed H4 bar (the breakout candidate) is bar 0;
        // the "prior" range we compare against is bars 1..DON_BREAKOUT_BARS.
        const OHLCBar* b0 = bb.get(0);
        if (!b0) return false;
        const double current_close = b0->close;

        double don_high = 0.0;
        double don_low  = 1e18;
        for (int k = 1; k <= DON_BREAKOUT_BARS; ++k) {
            const OHLCBar* bk = bb.get(k);
            if (!bk) break;
            if (bk->close > don_high) don_high = bk->close;
            if (bk->close < don_low)  don_low  = bk->close;
        }
        if (don_high <= 0.0 || don_low >= 1e17) return false;

        if (d1_bull && current_close > don_high) {
            _open_position(id, true, current_close, now_ms,
                           SwingStrategy::S1_PULLBACK,   // enum kept for GUI compat
                           S1_ATR_SL_MULT, 0.0, S1_MAX_HOLD_MS, +1);
            return true;
        }

        if (d1_bear && current_close < don_low) {
            _open_position(id, false, current_close, now_ms,
                           SwingStrategy::S1_PULLBACK,
                           S1_ATR_SL_MULT, 0.0, S1_MAX_HOLD_MS, -1);
            return true;
        }

        return false;
    }

    bool _try_s2(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& bb  = h4_builders_[id];

        if (!h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (bb.count < S2_DIV_LOOKBACK + 1) return false;

        const OHLCBar* b0 = bb.get(0);
        if (!b0) return false;

        double prior_low = b0->low;
        for (int k = 1; k <= S2_DIV_LOOKBACK; ++k) {
            const OHLCBar* bk = bb.get(k);
            if (!bk) break;
            if (bk->low < prior_low) prior_low = bk->low;
        }

        const bool new_price_low  = (b0->low < prior_low);
        const bool rsi_not_at_low = (h4.rsi14 > S2_RSI_OVERSOLD);
        const bool rsi_in_zone    = (h4.rsi14 < S2_RSI_CEILING);

        if (!new_price_low || !rsi_not_at_low || !rsi_in_zone) return false;
        // MACD histogram must be rising (don't require strictly positive — just turning)
        if (h4.hist <= h4.prev_hist) return false;
        if (last_exit_dir_[id] == -1 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) return false;

        const double sl_dist = S2_ATR_SL_MULT * h4.atr14;
        const double sl      = price - sl_dist;
        const double tp      = price + S2_R_MULT * sl_dist;

        _open_position_raw(id, true, price, sl, tp, now_ms, SwingStrategy::S2_DIVERGENCE,
                           S2_MAX_HOLD_MS, 0);
        return true;
    }

    bool _try_s3(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& d1_bb = d1_builders_[id];

        if (!h4.ready || !d1_ind_[id].ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (d1_bb.count < S3_RESIST_BARS) return false;

        {
            const auto& d1_s3 = d1_ind_[id];
            if (d1_s3.ema21 <= d1_s3.ema50) return false;
            const double d1_sep_s3 = std::fabs(d1_s3.ema21 - d1_s3.ema50) / d1_s3.ema50;
            if (d1_sep_s3 < S1_EMA_SEP_PCT) return false;
        }

        double resistance = 0.0;
        for (int k = 0; k < S3_RESIST_BARS; ++k) {
            const OHLCBar* b = d1_bb.get(k);
            if (!b) break;
            if (b->close > resistance) resistance = b->close;
        }
        if (resistance <= 0.0) return false;

        bool broke_out = false;
        for (int k = 0; k < 4; ++k) {
            const OHLCBar* b = h4_builders_[id].get(k);
            if (!b) break;
            if (b->close > resistance && b->volume > 0) {
                broke_out = true;
                break;
            }
        }
        if (!broke_out) return false;

        if (h4_ind_[id].obv < h4_ind_[id].obv_ema) return false;
        if (h4_ind_[id].hist <= 0.0) return false;

        const double retest_lo = resistance - S3_RETEST_ATR * h4.atr14;
        const double retest_hi = resistance + S3_RETEST_ATR * h4.atr14;
        if (price < retest_lo || price > retest_hi) return false;
        if (price < resistance * 0.998) return false;
        if (last_exit_dir_[id] == -1 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) return false;

        const OHLCBar* b_break = nullptr;
        for (int k = 0; k < 8; ++k) {
            const OHLCBar* b = h4_builders_[id].get(k);
            if (b && b->close > resistance) { b_break = b; break; }
        }
        const double breakout_range = b_break ? (b_break->close - resistance) : h4.atr14;
        const double sl = price - S3_ATR_SL_MULT * h4.atr14;
        const double tp = price + 2.0 * std::max(breakout_range, h4.atr14);

        _open_position_raw(id, true, price, sl, tp, now_ms, SwingStrategy::S3_BREAKOUT,
                           S3_MAX_HOLD_MS, +1);
        return true;
    }

    bool _try_s4(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& bb  = h4_builders_[id];

        if (!h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (bb.count < S4_BASELINE_BARS) return false;

        double baseline_atr = 0.0;
        int baseline_count  = 0;
        for (int k = 0; k < S4_BASELINE_BARS; ++k) {
            const OHLCBar* b  = bb.get(k);
            const OHLCBar* bp = bb.get(k + 1);
            if (!b || !bp) break;
            double tr = b->high - b->low;
            double t2 = std::fabs(b->high - bp->close);
            double t3 = std::fabs(b->low  - bp->close);
            if (t2 > tr) tr = t2;
            if (t3 > tr) tr = t3;
            baseline_atr += tr;
            ++baseline_count;
        }
        if (baseline_count < S4_BASELINE_BARS / 2) return false;
        baseline_atr /= baseline_count;
        if (baseline_atr <= 0.0) return false;

        double current_atr = 0.0;
        int    cur_count   = 0;
        for (int k = 0; k < S4_COMPRESS_BARS; ++k) {
            const OHLCBar* b  = bb.get(k);
            const OHLCBar* bp = bb.get(k + 1);
            if (!b || !bp) break;
            double tr = b->high - b->low;
            double t2 = std::fabs(b->high - bp->close);
            double t3 = std::fabs(b->low  - bp->close);
            if (t2 > tr) tr = t2;
            if (t3 > tr) tr = t3;
            current_atr += tr;
            ++cur_count;
        }
        if (cur_count < S4_COMPRESS_BARS - 1) return false;
        current_atr /= cur_count;

        if (current_atr >= S4_COMPRESS_RATIO * baseline_atr) return false;

        double bracket_hi = 0.0;
        double bracket_lo = 1e18;
        for (int k = 0; k < S4_COMPRESS_BARS; ++k) {
            const OHLCBar* b = bb.get(k);
            if (!b) break;
            if (b->high > bracket_hi) bracket_hi = b->high;
            if (b->low  < bracket_lo) bracket_lo = b->low;
        }
        if (bracket_hi <= bracket_lo) return false;
        const double bracket_width = bracket_hi - bracket_lo;

        const OHLCBar* b0 = bb.get(0);
        if (!b0) return false;

        const bool broke_up   = (b0->close > bracket_hi);
        const bool broke_down = (b0->close < bracket_lo);
        if (!broke_up && !broke_down) return false;

        const bool obv_confirms_up   = (h4.obv >= h4.obv_ema);
        const bool obv_confirms_down = (h4.obv <= h4.obv_ema);
        if (broke_up   && !obv_confirms_up)   return false;
        if (broke_down && !obv_confirms_down) return false;

        const int64_t S4_SAME_COOLDOWN = 7200000LL;
        if (last_exit_dir_[id] != 0 && now_ms - last_exit_ms_[id] < S4_SAME_COOLDOWN) {
            const bool same_dir = (broke_up && last_exit_dir_[id] == 1) ||
                                  (broke_down && last_exit_dir_[id] == -1);
            if (same_dir) return false;
        }

        const bool is_long = broke_up;

        const double sl = is_long  ? bracket_lo : bracket_hi;
        const double tp = is_long  ? (price + 2.0 * bracket_width)
                                   : (price - 2.0 * bracket_width);

        printf("[SWING-S4] %s %s BRACKET compression=%.3f/%.3f hi=%.4f lo=%.4f width=%.4f\n",
               sym_short(id), is_long ? "LONG" : "SHORT",
               current_atr, baseline_atr, bracket_hi, bracket_lo, bracket_width);
        fflush(stdout);

        _open_position_raw(id, is_long, price, sl, tp, now_ms,
                           SwingStrategy::S4_BRACKET, S4_MAX_HOLD_MS, 0);
        return true;
    }

    void _open_position(int id, bool is_long, double price, int64_t now_ms,
                        SwingStrategy strat, double sl_atr_mult,
                        double fixed_tp, int64_t max_hold_ms, int d1_dir) {
        const auto& h4 = h4_ind_[id];
        const double sl = is_long ? (price - sl_atr_mult * h4.atr14)
                                  : (price + sl_atr_mult * h4.atr14);
        _open_position_raw(id, is_long, price, sl, fixed_tp, now_ms, strat, max_hold_ms, d1_dir);
    }

    void _open_position_raw(int id, bool is_long, double entry, double sl, double tp,
                             int64_t now_ms, SwingStrategy strat, int64_t max_hold_ms,
                             int d1_dir) {
        auto& existing = positions_[id];
        if (existing.active && existing.is_long != is_long) {
            printf("[SWING-CONFLICT] %s closing %s %s S%d to enter %s S%d\n",
                   sym_short(id),
                   existing.is_long ? "LONG" : "SHORT", existing.symbol,
                   (int)existing.strategy,
                   is_long ? "LONG" : "SHORT",
                   (int)strat);
            fflush(stdout);
            _close(id, entry, now_ms, "CONFLICT_FLIP");
        }
        if (existing.active) return;

        double qty = MAX_QTY_USD / entry;
        qty = std::floor(qty * 100000.0) / 100000.0;
        if (qty * entry < MIN_QTY_USD) return;

        auto& pos          = positions_[id];
        pos.active         = true;
        pos.is_long        = is_long;
        pos.strategy       = strat;
        pos.entry_px       = entry;
        pos.sl_px          = sl;
        pos.tp_px          = tp;
        pos.trail_sl       = sl;
        pos.mfe            = 0.0;
        pos.trail_armed    = false;
        pos.trail_stage    = 0;
        pos.partial_done   = false;
        pos.qty            = qty;
        pos.qty_full       = qty;
        pos.pyramid_done   = false;
        pos.pyramid_qty    = 0.0;
        pos.blended_entry  = entry;
        pos.entry_ms       = now_ms;
        pos.max_hold_ms    = max_hold_ms;
        pos.entry_d1_dir   = d1_dir;
        pos.trade_id       = ++trade_counter_;
        strncpy(pos.symbol, sym_full(id), 15);
        pos.symbol[15] = '\0';

        now_ms_last_[id] = now_ms;
        cooldown_until_ms_[id] = now_ms + COOLDOWN_MS;

        const char* pfx = shadow_mode ? "[SWING-SHADOW]" : "[SWING]";
        printf("%s %s %s S%d entry=%.4f sl=%.4f tp=%.4f atr4h=%.4f rsi=%.1f qty=%.5f\n",
               pfx, sym_short(id), is_long ? "LONG" : "SHORT", (int)strat,
               entry, sl, tp, h4_ind_[id].atr14, h4_ind_[id].rsi14, qty);
        fflush(stdout);

        if (executor_) executor_->execute(pos.symbol, is_long, qty, entry);
    }

    void _manage(int id, double price, int64_t now_ms) {
        auto& pos      = positions_[id];
        const auto& h4 = h4_ind_[id];
        const auto& d1 = d1_ind_[id];
        now_ms_last_[id] = now_ms;

        const double move = pos.is_long ? (price - pos.entry_px)
                                        : (pos.entry_px - price);
        if (move > pos.mfe) pos.mfe = move;

        if (pos.tp_px > 0.0) {
            const bool tp_hit = pos.is_long ? (price >= pos.tp_px)
                                            : (price <= pos.tp_px);
            if (tp_hit) { _close(id, pos.tp_px, now_ms, "TP_HIT"); return; }
        }

        if (pos.entry_d1_dir != 0 && d1.ema50 > 0.0) {
            const double d1_sep = std::fabs(d1.ema21 - d1.ema50) / d1.ema50;
            if (d1_sep >= S1_EMA_SEP_PCT * 0.5) {
                const bool now_bull = (d1.ema21 > d1.ema50);
                const bool flipped  =
                    (pos.entry_d1_dir == +1 && !now_bull) ||
                    (pos.entry_d1_dir == -1 &&  now_bull);
                if (flipped) {
                    _close(id, price, now_ms, "REGIME_FLIP");
                    return;
                }
            }
        }

        if (h4.atr14 > 0.0) {
            if (pos.strategy == SwingStrategy::S1_PULLBACK) {
                if (pos.trail_stage == 0 && move >= S1_TRAIL_ARM_ATR * h4.atr14) {
                    pos.trail_stage  = 1;
                    pos.trail_armed  = true;
                    const double new_sl = pos.is_long
                        ? (pos.entry_px + pos.mfe - S1_TRAIL_ATR * h4.atr14)
                        : (pos.entry_px - pos.mfe + S1_TRAIL_ATR * h4.atr14);
                    if (pos.is_long ? (new_sl > pos.trail_sl) : (new_sl < pos.trail_sl))
                        pos.trail_sl = new_sl;
                    printf("[SWING-TRAIL-STAGE1] %s %s mfe=%.4f trail_sl=%.4f\n",
                           sym_short(id), pos.is_long ? "LONG" : "SHORT",
                           pos.mfe, pos.trail_sl);
                    fflush(stdout);
                    if (!pos.partial_done && pos.qty_full > 0.0) {
                        const double partial_qty = pos.qty_full * 0.5;
                        const double partial_pnl_pct = pos.is_long
                            ? ((price - pos.entry_px) / pos.entry_px * 100.0)
                            : ((pos.entry_px - price) / pos.entry_px * 100.0);
                        printf("[SWING-PARTIAL] %s %s 50pct exit @ %.4f pnl=%.3f%% qty=%.5f\n",
                               sym_short(id), pos.is_long ? "LONG" : "SHORT",
                               price, partial_pnl_pct, partial_qty);
                        fflush(stdout);
                        if (executor_) executor_->execute(pos.symbol, !pos.is_long, partial_qty, price);
                        pos.qty             = pos.qty_full - partial_qty;
                        pos.partial_done    = true;
                        pos.partial_exit_px = price;   // v8: persist for true pnl_pct calc
                        // v8: total_pnl_pct_ no longer updated here. The full
                        // trade pnl (partial leg + remaining leg) is added once
                        // at _close via _compute_trade_pnl_pct, avoiding the
                        // earlier double-count on the post-partial portion.
                    }
                }
                if (pos.trail_stage == 1 && move >= S1_TRAIL_STAGE2_ATR * h4.atr14) {
                    pos.trail_stage = 2;
                    const double new_sl = pos.is_long
                        ? (pos.entry_px + pos.mfe - S1_TRAIL_DIST2 * h4.atr14)
                        : (pos.entry_px - pos.mfe + S1_TRAIL_DIST2 * h4.atr14);
                    if (pos.is_long ? (new_sl > pos.trail_sl) : (new_sl < pos.trail_sl))
                        pos.trail_sl = new_sl;
                    printf("[SWING-TRAIL-STAGE2] %s %s mfe=%.4f trail_sl=%.4f\n",
                           sym_short(id), pos.is_long ? "LONG" : "SHORT",
                           pos.mfe, pos.trail_sl);
                    fflush(stdout);
                    if (!pos.pyramid_done) {
                        const double add_qty_raw = pos.qty_full * 0.25;
                        const double max_total = MAX_QTY_USD / price;
                        const double current_total = pos.qty + add_qty_raw;
                        const double add_qty = (current_total > max_total)
                            ? std::max(0.0, max_total - pos.qty)
                            : add_qty_raw;
                        if (add_qty * price >= MIN_QTY_USD) {
                            const double new_total = pos.qty + add_qty;
                            pos.blended_entry = (pos.blended_entry * pos.qty
                                                + price * add_qty) / new_total;
                            pos.qty          += add_qty;
                            pos.pyramid_qty   = add_qty;
                            pos.pyramid_done  = true;
                            printf("[SWING-PYRAMID] %s %s +25pct @ %.4f qty=%.5f"
                                   " blended=%.4f total_qty=%.5f\n",
                                   sym_short(id), pos.is_long ? "LONG" : "SHORT",
                                   price, add_qty, pos.blended_entry, pos.qty);
                            fflush(stdout);
                            if (executor_) executor_->execute(
                                pos.symbol, pos.is_long, add_qty, price);
                        }
                    }
                }
                if (pos.trail_stage == 2 && move >= S1_TRAIL_STAGE3_ATR * h4.atr14) {
                    pos.trail_stage = 3;
                    const double new_sl = pos.is_long
                        ? (pos.entry_px + pos.mfe - S1_TRAIL_DIST3 * h4.atr14)
                        : (pos.entry_px - pos.mfe + S1_TRAIL_DIST3 * h4.atr14);
                    if (pos.is_long ? (new_sl > pos.trail_sl) : (new_sl < pos.trail_sl))
                        pos.trail_sl = new_sl;
                    printf("[SWING-TRAIL-STAGE3] %s %s mfe=%.4f trail_sl=%.4f (LOCKED)\n",
                           sym_short(id), pos.is_long ? "LONG" : "SHORT",
                           pos.mfe, pos.trail_sl);
                    fflush(stdout);
                }
                if (pos.trail_armed) {
                    const double dist = (pos.trail_stage >= 3) ? S1_TRAIL_DIST3 * h4.atr14
                                      : (pos.trail_stage == 2) ? S1_TRAIL_DIST2 * h4.atr14
                                                                : S1_TRAIL_ATR   * h4.atr14;
                    const double new_sl = pos.is_long
                        ? (pos.entry_px + pos.mfe - dist)
                        : (pos.entry_px - pos.mfe + dist);
                    if (pos.is_long ? (new_sl > pos.trail_sl) : (new_sl < pos.trail_sl))
                        pos.trail_sl = new_sl;
                }
            } else {
                const double arm_mult   = 2.0;
                const double trail_dist = 1.2 * h4.atr14;
                if (!pos.trail_armed && move >= arm_mult * h4.atr14) {
                    pos.trail_armed = true;
                    printf("[SWING-TRAIL-ARM] %s %s S%d mfe=%.4f\n",
                           sym_short(id), pos.is_long ? "LONG" : "SHORT",
                           (int)pos.strategy, move);
                    fflush(stdout);
                }
                if (pos.trail_armed) {
                    const double new_sl = pos.is_long
                        ? (pos.entry_px + pos.mfe - trail_dist)
                        : (pos.entry_px - pos.mfe + trail_dist);
                    if (pos.is_long  && new_sl > pos.trail_sl) pos.trail_sl = new_sl;
                    if (!pos.is_long && new_sl < pos.trail_sl) pos.trail_sl = new_sl;
                }
            }
        }

        const bool sl_hit = pos.is_long ? (price <= pos.trail_sl)
                                        : (price >= pos.trail_sl);
        const bool timeout = (now_ms - pos.entry_ms >= pos.max_hold_ms);

        if (sl_hit)  { _close(id, pos.trail_sl, now_ms, "SL_HIT");  return; }
        if (timeout) { _close(id, price,         now_ms, "TIMEOUT"); return; }
    }

    void _close(int id, double exit_px, int64_t now_ms, const char* why) {
        auto& pos = positions_[id];

        // v8: true economic pnl_pct (includes partial leg + remaining-leg)
        const double pnl_pct = _compute_trade_pnl_pct(pos, exit_px);

        total_pnl_pct_ += pnl_pct;
        if (pnl_pct > 0) ++wins_;

        const char* pfx = shadow_mode ? "[SWING-SHADOW]" : "[SWING]";
        printf("%s %s CLOSE %s S%d entry=%.4f exit=%.4f pnl=%.3f%% mfe=%.4f why=%s\n",
               pfx, sym_short(id), pos.is_long ? "LONG" : "SHORT",
               (int)pos.strategy, pos.entry_px, exit_px, pnl_pct, pos.mfe, why);
        fflush(stdout);

        _record_exit(id, exit_px, now_ms, why, pnl_pct);

        last_exit_dir_[id] = pos.is_long ? 1 : -1;
        last_exit_ms_[id]  = now_ms;

        if (executor_) executor_->execute(pos.symbol, !pos.is_long, pos.qty, exit_px);

        pos = SwingPosition{};
    }

    void _record_exit(int id, double exit_px, int64_t now_ms, const char* why,
                      double pnl_pct) {
        const auto& pos = positions_[id];
        if (!pos.active && pos.entry_px == 0.0) return;

        TradeLog tl;
        tl.sym      = sym_short(id);
        tl.side     = pos.is_long ? "LONG" : "SHORT";
        tl.entry    = pos.entry_px;
        tl.exit     = exit_px;
        tl.pnl_pct  = pnl_pct;   // v8: caller passes the true economic pnl_pct
        tl.mfe      = pos.mfe;
        tl.strategy = pos.strategy;
        tl.why      = why;

        time_t t = (time_t)(now_ms / 1000);
        struct tm ti{};
        gmtime_r(&t, &ti);
        char tbuf[20];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
        tl.time = tbuf;

        tl.exit_ms = now_ms;
        trade_log_.push_back(tl);
        if (max_trade_log_size_ > 0 && (int)trade_log_.size() > max_trade_log_size_)
            trade_log_.erase(trade_log_.begin());
    }

    static size_t _curl_write(void* ptr, size_t size, size_t nmemb, std::string* out) {
        out->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    void _seed_symbol(int id, const char* interval,
                      BarBuilder& bb, SwingIndicators& ind) {
        const char* sym_up = sym_full(id);
        char sym_upper[16] = {};
        for (int i = 0; sym_up[i] && i < 15; ++i)
            sym_upper[i] = (char)toupper((unsigned char)sym_up[i]);

        std::string url = std::string("https://api.binance.com/api/v3/klines?symbol=")
                        + sym_upper + "&interval=" + interval + "&limit=200";

        CURL* curl = curl_easy_init();
        if (!curl) {
            printf("[SWING-SEED] curl_easy_init failed %s %s\n", sym_upper, interval);
            return;
        }

        std::string body;
        curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,       15L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            printf("[SWING-SEED] curl failed %s %s: %s\n",
                   sym_upper, interval, curl_easy_strerror(res));
            return;
        }

        int    bars_loaded  = 0;
        double prev_close   = 0.0;
        double prev_volume  = 0.0;
        size_t pos          = 0;

        while (pos < body.size()) {
            size_t arr_start = body.find('[', pos);
            if (arr_start == std::string::npos) break;
            size_t arr_end   = body.find(']', arr_start);
            if (arr_end   == std::string::npos) break;

            std::string arr = body.substr(arr_start + 1, arr_end - arr_start - 1);
            pos = arr_end + 1;

            std::vector<std::string> fields;
            size_t p2 = 0;
            while (p2 < arr.size()) {
                size_t comma = arr.find(',', p2);
                if (comma == std::string::npos) comma = arr.size();
                std::string field = arr.substr(p2, comma - p2);
                size_t s1 = field.find_first_not_of(" \"");
                size_t s2 = field.find_last_not_of(" \"");
                if (s1 != std::string::npos) field = field.substr(s1, s2 - s1 + 1);
                fields.push_back(field);
                p2 = comma + 1;
            }
            if (fields.size() < 6) continue;

            double open=0, high=0, low=0, close=0, volume=0;
            try {
                open   = std::stod(fields[1]);
                high   = std::stod(fields[2]);
                low    = std::stod(fields[3]);
                close  = std::stod(fields[4]);
                volume = std::stod(fields[5]);
            } catch (...) { continue; }

            if (high <= 0.0 || low <= 0.0 || close <= 0.0) continue;

            ind.update(high, low, close, prev_close, volume);

            OHLCBar bar{open, high, low, close, volume, 0};
            bb.closed[bb.head] = bar;
            bb.head = (bb.head + 1) % BarBuilder::HISTORY;
            if (bb.count < BarBuilder::HISTORY) ++bb.count;

            prev_close  = close;
            prev_volume = volume;
            ++bars_loaded;
        }

        prices_[id] = prev_close;
        printf("[SWING-SEED] %s %s: %d bars | ema21=%.4f ema50=%.4f atr=%.4f rsi=%.1f ready=%s\n",
               sym_upper, interval, bars_loaded,
               ind.ema21, ind.ema50, ind.atr14, ind.rsi14,
               ind.ready ? "YES" : "NO");
        fflush(stdout);
    }
};

} // namespace chimera
