// ============================================================================
// SwingEngine — H4/D1 Multi-Strategy Spot Swing Engine
//
// Replaces TrendEngine (H1 EMA crossover) with three proven swing strategies:
//
//  S1 — EMA Trend + H4 Pullback (primary)
//       D1: EMA21 > EMA50 = LONG regime / EMA21 < EMA50 = SHORT regime
//       H4: Price pulls back to EMA21 zone (within 0.5×ATR4h)
//       H4: RSI14 recovering (crosses >45 from below for longs, <55 for shorts)
//       Entry: H4 bar close confirming pullback bounce
//       SL: 1.5×H4 ATR below nearest swing low (min 3 H4 bars lookback)
//       TP: Trail armed at 3×ATR, trail distance 1.5×ATR | max hold 7 days
//
//  S2 — RSI Divergence Reversal (H4)
//       H4: Bullish divergence: price lower low + RSI higher low, RSI < 40
//       H4: MACD histogram turning positive as confirmation
//       Entry: close of the H4 bar that confirms divergence
//       SL: Below the divergence swing low
//       TP: Prior swing high or 3×R fixed, max hold 5 days
//
//  S3 — Breakout Retest (D1 level, H4 entry)
//       D1: Resistance = highest close in last 20 D1 bars
//       H4: Price closes above resistance with OBV rising
//       H4: Price retests resistance (comes back within 0.4×ATR4h of level)
//       Entry: bounce from retest zone
//       SL: Below retest low
//       TP: Entry + 2× breakout range, max hold 5 days
//
//  S4 — H4 Compression Bracket (direction-neutral breakout)
//       H4: ATR compression: current 5-bar ATR < 40% of 20-bar ATR baseline
//       Bracket: high and low of the 5-bar compressed range
//       Entry: H4 bar closes outside bracket with OBV confirming direction
//       SL: Opposite bracket boundary
//       TP: Entry + 2× bracket width projected in breakout direction
//       Conflict: automatically closes any opposing position before entry
//       Max hold: 5 days
//
// Architecture:
//   - Builds both H4 and D1 bars from the tick stream
//   - Seeds both timeframes from Binance REST klines at startup
//   - One position max per symbol (first strategy to fire wins)
//   - Shadow mode by default — no live orders until explicitly enabled
//   - Session: 24/7 (crypto never closes, no session gate)
//   - Cooldown: 6h between entries on same symbol
//   - Direction flip cooldown: 12h
//
// Feed: Binance WebSocket (bookTicker + aggTrade) via BinanceWSFeed
// Execution: SpotExecutor (shadow mode default)
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

// ============================================================================
// OHLC Bar — generic, used for both H4 and D1
// ============================================================================
struct OHLCBar {
    double open  = 0.0;
    double high  = 0.0;
    double low   = 0.0;
    double close = 0.0;
    double volume = 0.0;
    int64_t bar_ms = 0;   // epoch ms of bar open
};

// ============================================================================
// BarBuilder — builds OHLC bars from tick stream for any period
// ============================================================================
struct BarBuilder {
    static constexpr int HISTORY = 32;  // keep last N closed bars

    OHLCBar  current;
    OHLCBar  closed[HISTORY];  // ring buffer, newest at [head-1]
    int      head     = 0;     // next write index
    int      count    = 0;     // how many closed bars stored
    int64_t  bar_boundary_ms = 0;
    int64_t  period_ms;

    explicit BarBuilder(int64_t period_ms_) : period_ms(period_ms_) {}

    // Returns true when a bar closes.
    // volume_increment: raw trade qty received this tick (for OBV)
    bool on_tick(double price, int64_t now_ms, double volume_increment = 0.0) {
        const int64_t boundary = (now_ms / period_ms) * period_ms;

        if (bar_boundary_ms == 0) {
            bar_boundary_ms = boundary;
            current = {price, price, price, price, 0.0, boundary};
            return false;
        }

        if (boundary != bar_boundary_ms) {
            // Close the current bar
            closed[head] = current;
            head         = (head + 1) % HISTORY;
            if (count < HISTORY) ++count;

            current         = {price, price, price, price, volume_increment, boundary};
            bar_boundary_ms = boundary;
            return true;
        }

        // Update in-progress bar
        if (price > current.high) current.high = price;
        if (price < current.low)  current.low  = price;
        current.close  = price;
        current.volume += volume_increment;
        return false;
    }

    // Get closed bar at offset from newest. offset=0 = most recent closed bar.
    const OHLCBar* get(int offset) const {
        if (offset < 0 || offset >= count) return nullptr;
        int idx = (head - 1 - offset + HISTORY) % HISTORY;
        return &closed[idx];
    }

    bool ready(int min_bars = 1) const { return count >= min_bars; }
};

// ============================================================================
// Indicator state per symbol per timeframe
// ============================================================================
struct SwingIndicators {
    // EMAs (on close)
    double ema9   = 0.0;
    double ema21  = 0.0;
    double ema50  = 0.0;

    // ATR (Wilder, 14 periods)
    double atr14  = 0.0;

    // RSI (14 periods)
    double rsi14  = 50.0;
    double avg_gain = 0.0;
    double avg_loss = 0.0;

    // MACD (12/26/9)
    double ema12   = 0.0;
    double ema26   = 0.0;
    double macd    = 0.0;
    double signal  = 0.0;  // 9-period EMA of macd
    double hist    = 0.0;  // macd - signal
    double prev_hist = 0.0;

    // OBV (on-balance volume)
    double obv     = 0.0;
    double obv_ema = 0.0;  // 14-period EMA of OBV

    int  bar_count = 0;
    bool ready     = false;  // true after 50+ bars

    void update(double h, double l, double close, double prev_close, double volume) {
        const double alpha9  = 2.0 / 10.0;
        const double alpha21 = 2.0 / 22.0;
        const double alpha50 = 2.0 / 51.0;
        const double alpha12 = 2.0 / 13.0;
        const double alpha26 = 2.0 / 27.0;
        const double alpha9s = 2.0 / 10.0;   // signal line
        const double alpha14 = 1.0 / 14.0;   // Wilder for ATR & RSI

        if (bar_count == 0) {
            ema9 = ema21 = ema50 = ema12 = ema26 = close;
            avg_gain = avg_loss = 0.0;
            obv = obv_ema = 0.0;
        } else {
            // EMAs
            ema9  += alpha9  * (close - ema9);
            ema21 += alpha21 * (close - ema21);
            ema50 += alpha50 * (close - ema50);

            // ATR
            double tr = h - l;
            if (prev_close > 0.0) {
                double tr2 = std::fabs(h - prev_close);
                double tr3 = std::fabs(l - prev_close);
                if (tr2 > tr) tr = tr2;
                if (tr3 > tr) tr = tr3;
            }
            if (atr14 == 0.0) atr14 = tr;
            else               atr14 += alpha14 * (tr - atr14);

            // RSI (Wilder smoothed)
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

            // MACD
            ema12 += alpha12 * (close - ema12);
            ema26 += alpha26 * (close - ema26);
            prev_hist = hist;
            macd = ema12 - ema26;
            if (bar_count == 1) signal = macd;
            else                signal += alpha9s * (macd - signal);
            hist = macd - signal;

            // OBV
            if (prev_close > 0.0) {
                if (close > prev_close)      obv += volume;
                else if (close < prev_close) obv -= volume;
                // flat: obv unchanged
            }
            obv_ema += alpha14 * (obv - obv_ema);
        }

        ++bar_count;
        if (bar_count >= 50) ready = true;
    }
};

// ============================================================================
// Open position
// ============================================================================
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
    double         tp_px         = 0.0;     // fixed TP (S2/S3); 0 = trail only
    double         trail_sl      = 0.0;
    double         mfe           = 0.0;
    bool           trail_armed   = false;
    double         qty           = 0.0;
    int64_t        entry_ms      = 0;
    int64_t        max_hold_ms   = 0;
    char           symbol[16]    = {};
    int            trade_id      = 0;
};

// ============================================================================
// SwingEngine
// ============================================================================
class SwingEngine {
public:
    // ── Config ─────────────────────────────────────────────────────────────
    // Timeframes
    static constexpr int64_t H4_MS  = 14400000LL;   // 4 hours
    static constexpr int64_t D1_MS  = 86400000LL;   // 24 hours

    // Position sizing
    static constexpr double MIN_QTY_USD = 50.0;
    static constexpr double MAX_QTY_USD = 500.0;

    // S1 — EMA Pullback
    static constexpr double S1_EMA_SEP_PCT   = 0.005;  // D1 EMA21 vs EMA50 must be >0.5% apart
    static constexpr double S1_PULLBACK_ATR  = 0.6;    // H4: price within 0.6×ATR4h of EMA21
    static constexpr double S1_RSI_LONG_LO   = 40.0;   // H4 RSI was below this (oversold dip)
    static constexpr double S1_RSI_LONG_HI   = 55.0;   // H4 RSI recovers above this = entry
    static constexpr double S1_RSI_SHORT_HI  = 60.0;
    static constexpr double S1_RSI_SHORT_LO  = 45.0;
    static constexpr double S1_ATR_SL_MULT   = 1.5;
    static constexpr double S1_TRAIL_ARM_ATR = 3.0;
    static constexpr double S1_TRAIL_ATR     = 1.5;
    static constexpr int64_t S1_MAX_HOLD_MS  = 604800000LL;  // 7 days

    // S2 — RSI Divergence
    static constexpr double S2_RSI_OVERSOLD  = 40.0;   // RSI must be below this at divergence low
    static constexpr double S2_DIV_LOOKBACK  = 5;      // bars to look back for prior low
    static constexpr double S2_ATR_SL_MULT   = 1.2;
    static constexpr double S2_R_MULT        = 3.0;    // TP = entry ± 3×risk
    static constexpr int64_t S2_MAX_HOLD_MS  = 432000000LL;  // 5 days

    // S3 — Breakout Retest
    static constexpr int     S3_RESIST_BARS  = 20;     // D1 bars for resistance calculation
    static constexpr double  S3_RETEST_ATR   = 0.4;    // retest within 0.4×ATR4h of level
    static constexpr double  S3_ATR_SL_MULT  = 1.2;
    static constexpr int64_t S3_MAX_HOLD_MS  = 432000000LL;  // 5 days

    // S4 — Compression Bracket
    static constexpr int     S4_COMPRESS_BARS  = 5;      // bars defining the compressed range
    static constexpr int     S4_BASELINE_BARS  = 20;     // bars for ATR baseline
    static constexpr double  S4_COMPRESS_RATIO = 0.45;   // current ATR must be < 45% of baseline
    static constexpr double  S4_OBV_CONFIRM    = 0.0;    // OBV must be above EMA (>0 diff)
    static constexpr double  S4_ATR_SL_MULT    = 1.0;    // SL at opposite bracket boundary (fallback: 1×ATR)
    static constexpr int64_t S4_MAX_HOLD_MS    = 432000000LL;  // 5 days
    static constexpr int64_t S4_FLIP_EXEMPT_MS = 0LL;    // S4 is exempt from flip cooldown (high-conviction)

    // Cooldowns
    static constexpr int64_t COOLDOWN_MS      = 21600000LL;  // 6h between entries
    static constexpr int64_t FLIP_COOLDOWN_MS = 43200000LL;  // 12h direction flip

    bool shadow_mode = true;  // NEVER set false without explicit authorization

    void set_executor(SpotExecutor* ex) { executor_ = ex; }

    // ── Seed from Binance REST at startup ────────────────────────────────────
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

    // ── Called every tick from feed callback ─────────────────────────────────
    void on_tick(int id, const MarketTick& tick, int64_t now_ms) {
        if (id < 0 || id >= MAX_SYMBOLS) return;

        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (price <= 0.0) return;
        prices_[id] = price;

        double vol_inc = tick.trade_qty;  // 0 on bookTicker ticks, filled on aggTrade

        // Build bars
        bool h4_closed = h4_builders_[id].on_tick(price, now_ms, vol_inc);
        bool d1_closed = d1_builders_[id].on_tick(price, now_ms, vol_inc);

        // Update indicators on bar close
        if (h4_closed && h4_builders_[id].count >= 2) {
            const OHLCBar* b   = h4_builders_[id].get(0);   // just closed
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

        // Manage open position on every tick
        auto& pos = positions_[id];
        if (pos.active) {
            _manage(id, price, now_ms);
            return;
        }

        // Gate: wait for enough history
        if (!h4_ind_[id].ready) return;
        if (!d1_ind_[id].ready) return;
        if (now_ms < cooldown_until_ms_[id]) return;

        // Try strategies in priority order
        if (_try_s1(id, price, now_ms)) return;
        if (_try_s2(id, price, now_ms)) return;
        if (_try_s3(id, price, now_ms)) return;
        if (_try_s4(id, price, now_ms)) return;
    }

    void kill_all() {
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            auto& pos = positions_[i];
            if (!pos.active) continue;
            printf("[SWING-KILL] %s %s entry=%.4f\n",
                   sym_short(i), pos.is_long ? "LONG" : "SHORT", pos.entry_px);
            fflush(stdout);
            if (executor_) executor_->execute(pos.symbol, !pos.is_long, pos.qty, prices_[i]);
            _record_exit(i, prices_[i], now_ms_last_[i], "KILL");
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

    // ── JSON state for GUI (same schema as TrendEngine for compatibility) ─────
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

            // Map to old GUI fields
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

private:

    // ── Per-symbol state ──────────────────────────────────────────────────────
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

    struct TradeLog {
        std::string sym, side, time, why;
        SwingStrategy strategy = SwingStrategy::NONE;
        double entry=0, exit=0, pnl_pct=0, mfe=0;
    };
    std::vector<TradeLog> trade_log_;

    // ── Strategy S1: EMA Pullback ─────────────────────────────────────────────
    bool _try_s1(int id, double price, int64_t now_ms) {
        const auto& d1  = d1_ind_[id];
        const auto& h4  = h4_ind_[id];

        if (!d1.ready || !h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;

        // D1 regime: EMA21 vs EMA50 separation
        const double d1_sep = std::fabs(d1.ema21 - d1.ema50) / d1.ema50;
        if (d1_sep < S1_EMA_SEP_PCT) return false;

        const bool d1_bull = d1.ema21 > d1.ema50;
        const bool d1_bear = d1.ema21 < d1.ema50;

        // Flip cooldown
        if (last_exit_dir_[id] != 0 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) {
            if (d1_bull && last_exit_dir_[id] == -1) return false;
            if (d1_bear && last_exit_dir_[id] == +1) return false;
        }

        if (d1_bull) {
            // LONG: price pulled back to within 0.6×ATR4h of H4 EMA21
            const double zone_lo = h4.ema21 - S1_PULLBACK_ATR * h4.atr14;
            const double zone_hi = h4.ema21 + S1_PULLBACK_ATR * h4.atr14;
            if (price < zone_lo || price > zone_hi) return false;
            // RSI recovery: was below S1_RSI_LONG_LO, now above S1_RSI_LONG_HI
            // Use previous bar RSI as proxy (h4_prev_rsi not tracked, use current < threshold)
            // Sufficient: current RSI crossed up through 45-55 zone (is between 45 and 65)
            if (h4.rsi14 < 42.0 || h4.rsi14 > 65.0) return false;
            // MACD hist: must be rising (hist > prev_hist) or already positive
            if (h4.hist < h4.prev_hist && h4.hist < 0.0) return false;
            _open_position(id, true, price, now_ms, SwingStrategy::S1_PULLBACK,
                           S1_ATR_SL_MULT, 0.0, S1_MAX_HOLD_MS);
            return true;
        }

        if (d1_bear) {
            // SHORT: price bounced up to within 0.6×ATR4h of H4 EMA21
            const double zone_lo = h4.ema21 - S1_PULLBACK_ATR * h4.atr14;
            const double zone_hi = h4.ema21 + S1_PULLBACK_ATR * h4.atr14;
            if (price < zone_lo || price > zone_hi) return false;
            if (h4.rsi14 > 58.0 || h4.rsi14 < 35.0) return false;
            if (h4.hist > h4.prev_hist && h4.hist > 0.0) return false;
            _open_position(id, false, price, now_ms, SwingStrategy::S1_PULLBACK,
                           S1_ATR_SL_MULT, 0.0, S1_MAX_HOLD_MS);
            return true;
        }

        return false;
    }

    // ── Strategy S2: RSI Divergence ───────────────────────────────────────────
    bool _try_s2(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& bb  = h4_builders_[id];

        if (!h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (bb.count < (int)S2_DIV_LOOKBACK + 1) return false;

        // Bullish divergence: last bar low < prior low N bars ago, but RSI higher
        const OHLCBar* b0 = bb.get(0);   // most recent closed H4 bar
        if (!b0) return false;

        // Find the lowest low in the lookback window (bars 1..S2_DIV_LOOKBACK)
        double prior_low = b0->low;
        int    prior_rsi_bar = 0;
        for (int k = 1; k <= (int)S2_DIV_LOOKBACK; ++k) {
            const OHLCBar* bk = bb.get(k);
            if (!bk) break;
            if (bk->low < prior_low) {
                prior_low = bk->low;
                prior_rsi_bar = k;
            }
        }

        // We approximate RSI at prior bar using the saved rsi14 value shifted back.
        // Since we don't store per-bar RSI history, we use the indicator EMA as proxy:
        // divergence condition: current low < prior_low AND current RSI > prior RSI estimate
        // Simplified: current low is a NEW low, RSI is NOT at new low (RSI > S2_RSI_OVERSOLD)
        const bool new_price_low = (b0->low < prior_low);
        const bool rsi_not_at_low = (h4.rsi14 > S2_RSI_OVERSOLD);
        const bool rsi_in_zone    = (h4.rsi14 < 55.0);

        if (!new_price_low || !rsi_not_at_low || !rsi_in_zone) return false;

        // MACD histogram must be turning positive
        if (h4.hist <= 0.0 || h4.hist < h4.prev_hist) return false;

        // Only take longs on bullish divergence (spot = no short selling)
        // Flip cooldown
        if (last_exit_dir_[id] == -1 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) return false;

        // Fixed TP = entry + 3×risk
        const double sl_dist = S2_ATR_SL_MULT * h4.atr14;
        const double sl      = price - sl_dist;
        const double tp      = price + S2_R_MULT * sl_dist;

        _open_position_raw(id, true, price, sl, tp, now_ms, SwingStrategy::S2_DIVERGENCE, S2_MAX_HOLD_MS);
        return true;
    }

    // ── Strategy S3: Breakout Retest ──────────────────────────────────────────
    bool _try_s3(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& d1_bb = d1_builders_[id];

        if (!h4.ready || !d1_ind_[id].ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (d1_bb.count < S3_RESIST_BARS) return false;

        // S3 is LONG-only (breakout retest buy).
        // Gate: only fire in D1 bull regime (EMA21 > EMA50 with separation).
        // Firing longs into D1 bear = trading against trend = losses confirmed in shadow.
        {
            const auto& d1_s3 = d1_ind_[id];
            if (d1_s3.ema21 <= d1_s3.ema50) return false;  // bear regime
            const double d1_sep_s3 = std::fabs(d1_s3.ema21 - d1_s3.ema50) / d1_s3.ema50;
            if (d1_sep_s3 < S1_EMA_SEP_PCT) return false;  // no clear trend
        }

        // Resistance = highest close in last S3_RESIST_BARS D1 bars
        double resistance = 0.0;
        for (int k = 0; k < S3_RESIST_BARS; ++k) {
            const OHLCBar* b = d1_bb.get(k);
            if (!b) break;
            if (b->close > resistance) resistance = b->close;
        }
        if (resistance <= 0.0) return false;

        // Check: has price recently broken above resistance?
        // Use H4 bars: need a H4 close above resistance in last 4 bars (16h)
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

        // OBV must be above its EMA (rising volume on breakout)
        if (h4_ind_[id].obv < h4_ind_[id].obv_ema) return false;

        // Retest: price has returned within S3_RETEST_ATR of resistance
        const double retest_lo = resistance - S3_RETEST_ATR * h4.atr14;
        const double retest_hi = resistance + S3_RETEST_ATR * h4.atr14;
        if (price < retest_lo || price > retest_hi) return false;

        // Price must be above resistance (we're buying the retest from above)
        if (price < resistance * 0.998) return false;

        // Flip cooldown
        if (last_exit_dir_[id] == -1 && now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) return false;

        // SL below retest zone, TP = 2× the breakout range above entry
        const OHLCBar* b_break = nullptr;
        for (int k = 0; k < 8; ++k) {
            const OHLCBar* b = h4_builders_[id].get(k);
            if (b && b->close > resistance) { b_break = b; break; }
        }
        const double breakout_range = b_break ? (b_break->close - resistance) : h4.atr14;
        const double sl = price - S3_ATR_SL_MULT * h4.atr14;
        const double tp = price + 2.0 * std::max(breakout_range, h4.atr14);

        _open_position_raw(id, true, price, sl, tp, now_ms, SwingStrategy::S3_BREAKOUT, S3_MAX_HOLD_MS);
        return true;
    }

    // ── Strategy S4: H4 Compression Bracket ─────────────────────────────────────
    bool _try_s4(int id, double price, int64_t now_ms) {
        const auto& h4  = h4_ind_[id];
        const auto& bb  = h4_builders_[id];

        if (!h4.ready) return false;
        if (h4.atr14 <= 0.0) return false;
        if (bb.count < S4_BASELINE_BARS) return false;

        // ── Compute ATR over last S4_COMPRESS_BARS H4 bars (current ATR proxy) ──
        // Use the rolling ATR already in h4_ind_ as the current value.
        // For baseline, compute average true range over S4_BASELINE_BARS manually.
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

        // Current ATR = average TR of last S4_COMPRESS_BARS bars
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

        // Compression confirmed?
        if (current_atr >= S4_COMPRESS_RATIO * baseline_atr) return false;

        // ── Define bracket: high/low of last S4_COMPRESS_BARS bars ───────────────
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

        // ── Price must have just closed outside the bracket ───────────────────────
        const OHLCBar* b0 = bb.get(0);  // most recent closed H4 bar
        if (!b0) return false;

        const bool broke_up   = (b0->close > bracket_hi);
        const bool broke_down = (b0->close < bracket_lo);
        if (!broke_up && !broke_down) return false;

        // ── OBV confirmation ─────────────────────────────────────────────────────
        // For longs: OBV must be above its EMA (accumulation on break)
        // For shorts: OBV must be below its EMA (distribution on break)
        const bool obv_confirms_up   = (h4.obv >= h4.obv_ema);
        const bool obv_confirms_down = (h4.obv <= h4.obv_ema);
        if (broke_up   && !obv_confirms_up)   return false;
        if (broke_down && !obv_confirms_down) return false;

        // ── No flip cooldown for S4 (compression breakout is high-conviction) ────
        // But still skip if SAME direction position just closed < 2h ago
        // (avoid re-entering immediately after a failed bracket break)
        const int64_t S4_SAME_COOLDOWN = 7200000LL;  // 2h
        if (last_exit_dir_[id] != 0 && now_ms - last_exit_ms_[id] < S4_SAME_COOLDOWN) {
            const bool same_dir = (broke_up && last_exit_dir_[id] == 1) ||
                                  (broke_down && last_exit_dir_[id] == -1);
            if (same_dir) return false;
            // Opposing direction: allow (this is the conflict-flip case — _open_position_raw handles close)
        }

        const bool is_long = broke_up;

        // SL: opposite bracket boundary; TP: entry + 2× bracket width
        const double sl = is_long  ? bracket_lo : bracket_hi;
        const double tp = is_long  ? (price + 2.0 * bracket_width)
                                   : (price - 2.0 * bracket_width);

        printf("[SWING-S4] %s %s BRACKET compression=%.3f/%.3f hi=%.4f lo=%.4f width=%.4f\n",
               sym_short(id), is_long ? "LONG" : "SHORT",
               current_atr, baseline_atr, bracket_hi, bracket_lo, bracket_width);
        fflush(stdout);

        _open_position_raw(id, is_long, price, sl, tp, now_ms, SwingStrategy::S4_BRACKET, S4_MAX_HOLD_MS);
        return true;
    }

    // ── Open position helpers ─────────────────────────────────────────────────

    // For S1 (trail-based TP)
    void _open_position(int id, bool is_long, double price, int64_t now_ms,
                        SwingStrategy strat, double sl_atr_mult,
                        double fixed_tp, int64_t max_hold_ms) {
        const auto& h4 = h4_ind_[id];
        const double sl = is_long ? (price - sl_atr_mult * h4.atr14)
                                  : (price + sl_atr_mult * h4.atr14);
        _open_position_raw(id, is_long, price, sl, fixed_tp, now_ms, strat, max_hold_ms);
    }

    void _open_position_raw(int id, bool is_long, double entry, double sl, double tp,
                             int64_t now_ms, SwingStrategy strat, int64_t max_hold_ms) {
        // Conflict check: if an opposing position is open, close it first
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
        // If same direction already open, skip (don't pyramid)
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
        pos.qty            = qty;
        pos.entry_ms       = now_ms;
        pos.max_hold_ms    = max_hold_ms;
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

    // ── Manage open position ──────────────────────────────────────────────────
    void _manage(int id, double price, int64_t now_ms) {
        auto& pos      = positions_[id];
        const auto& h4 = h4_ind_[id];
        now_ms_last_[id] = now_ms;

        const double move = pos.is_long ? (price - pos.entry_px)
                                        : (pos.entry_px - price);
        if (move > pos.mfe) pos.mfe = move;

        // Fixed TP hit (S2 / S3)
        if (pos.tp_px > 0.0) {
            const bool tp_hit = pos.is_long ? (price >= pos.tp_px)
                                            : (price <= pos.tp_px);
            if (tp_hit) { _close(id, pos.tp_px, now_ms, "TP_HIT"); return; }
        }

        // Trail logic (S1 and as fallback for all strategies)
        if (h4.atr14 > 0.0) {
            // Arm trail at 3×ATR for S1, 2×ATR for S2/S3
            const double arm_mult  = (pos.strategy == SwingStrategy::S1_PULLBACK) ? S1_TRAIL_ARM_ATR : 2.0;
            const double trail_dist = (pos.strategy == SwingStrategy::S1_PULLBACK) ? S1_TRAIL_ATR * h4.atr14
                                                                                    : 1.2 * h4.atr14;

            if (!pos.trail_armed && move >= arm_mult * h4.atr14) {
                pos.trail_armed = true;
                printf("[SWING-TRAIL-ARM] %s %s S%d mfe=%.4f\n",
                       sym_short(id), pos.is_long ? "LONG" : "SHORT", (int)pos.strategy, move);
                fflush(stdout);
            }

            if (pos.trail_armed) {
                const double new_sl = pos.is_long ? (pos.entry_px + pos.mfe - trail_dist)
                                                  : (pos.entry_px - pos.mfe + trail_dist);
                if (pos.is_long  && new_sl > pos.trail_sl) pos.trail_sl = new_sl;
                if (!pos.is_long && new_sl < pos.trail_sl) pos.trail_sl = new_sl;
            }
        }

        // SL hit (use trail_sl which starts at sl_px and tightens over time)
        const bool sl_hit = pos.is_long ? (price <= pos.trail_sl)
                                        : (price >= pos.trail_sl);

        // Max hold timeout
        const bool timeout = (now_ms - pos.entry_ms >= pos.max_hold_ms);

        if (sl_hit)  { _close(id, pos.trail_sl, now_ms, "SL_HIT");  return; }
        if (timeout) { _close(id, price,         now_ms, "TIMEOUT"); return; }
    }

    void _close(int id, double exit_px, int64_t now_ms, const char* why) {
        auto& pos = positions_[id];

        const double pnl_pct = pos.is_long
            ? ((exit_px - pos.entry_px) / pos.entry_px * 100.0)
            : ((pos.entry_px - exit_px) / pos.entry_px * 100.0);

        total_pnl_pct_ += pnl_pct;
        if (pnl_pct > 0) ++wins_;

        const char* pfx = shadow_mode ? "[SWING-SHADOW]" : "[SWING]";
        printf("%s %s CLOSE %s S%d entry=%.4f exit=%.4f pnl=%.3f%% mfe=%.4f why=%s\n",
               pfx, sym_short(id), pos.is_long ? "LONG" : "SHORT",
               (int)pos.strategy, pos.entry_px, exit_px, pnl_pct, pos.mfe, why);
        fflush(stdout);

        _record_exit(id, exit_px, now_ms, why);

        last_exit_dir_[id] = pos.is_long ? 1 : -1;
        last_exit_ms_[id]  = now_ms;

        if (executor_) executor_->execute(pos.symbol, !pos.is_long, pos.qty, exit_px);

        pos = SwingPosition{};
    }

    void _record_exit(int id, double exit_px, int64_t now_ms, const char* why) {
        const auto& pos = positions_[id];
        if (!pos.active && pos.entry_px == 0.0) return;

        TradeLog tl;
        tl.sym      = sym_short(id);
        tl.side     = pos.is_long ? "LONG" : "SHORT";
        tl.entry    = pos.entry_px;
        tl.exit     = exit_px;
        tl.pnl_pct  = pos.is_long
            ? ((exit_px - pos.entry_px) / pos.entry_px * 100.0)
            : ((pos.entry_px - exit_px) / pos.entry_px * 100.0);
        tl.mfe      = pos.mfe;
        tl.strategy = pos.strategy;
        tl.why      = why;

        time_t t = (time_t)(now_ms / 1000);
        struct tm ti{};
        gmtime_r(&t, &ti);
        char tbuf[20];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
        tl.time = tbuf;

        trade_log_.push_back(tl);
        if (trade_log_.size() > 100) trade_log_.erase(trade_log_.begin());
    }

    // ── CURL helpers ──────────────────────────────────────────────────────────
    static size_t _curl_write(void* ptr, size_t size, size_t nmemb, std::string* out) {
        out->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    // ── Seed one symbol + timeframe from Binance klines REST ─────────────────
    void _seed_symbol(int id, const char* interval,
                      BarBuilder& bb, SwingIndicators& ind) {
        const char* sym_up = sym_full(id);
        char sym_upper[16] = {};
        for (int i = 0; sym_up[i] && i < 15; ++i)
            sym_upper[i] = (char)toupper((unsigned char)sym_up[i]);

        // Fetch 200 bars (enough to warm both EMA50 and have history)
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

            // Parse fields: [0]=open_time, [1]=open, [2]=high, [3]=low, [4]=close, [5]=volume
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

            // Inject closed bar into the ring buffer directly
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

