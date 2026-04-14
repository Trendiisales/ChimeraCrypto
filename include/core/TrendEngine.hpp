#pragma once
#include <vector>
#include <sstream>
#include <iomanip>
// ============================================================================
// TrendEngine — H1 EMA crossover trend following for BTC/ETH/SOL
//
// Strategy:
//   - Builds H1 OHLC bars from tick stream
//   - EMA9 > EMA50 = LONG bias, EMA9 < EMA50 = SHORT bias
//   - Entry: price crosses above EMA9 (LONG) or below EMA9 (SHORT)
//             with EMA separation >= MIN_EMA_SEP_PCT (trend is real)
//   - SL: 1.5x ATR14 from entry
//   - TP: trail, arm at 2x ATR, trail at 1x ATR behind peak
//   - Session: 07:00-22:00 UTC only
//   - Max 1 position per symbol, shadow mode
//   - Cooldown: 30min between entries, 4h between direction flips
// ============================================================================

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <string>
#include <chrono>

#include "core/SymbolIndex.hpp"
#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "version_generated.hpp"
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

namespace chimera {

// ── Per-symbol H1 bar state ──────────────────────────────────────────────────
struct H1Bar {
    double open  = 0.0;
    double high  = 0.0;
    double low   = 0.0;
    double close = 0.0;
    int64_t bar_ms = 0;  // epoch ms of bar open
};

// ── Per-symbol indicator state ───────────────────────────────────────────────
struct TrendIndicators {
    double ema9     = 0.0;
    double ema50    = 0.0;
    double atr14    = 0.0;
    int    bar_count = 0;
    bool   ready    = false;

    // EMA update: alpha = 2/(N+1)
    void update_ema(double price) {
        if (bar_count == 0) {
            ema9 = ema50 = price;
        } else {
            ema9  = ema9  + (2.0/10.0) * (price - ema9);
            ema50 = ema50 + (2.0/51.0) * (price - ema50);
        }
        if (bar_count >= 50) ready = true;
        ++bar_count;
    }

    // ATR update (Wilder smoothing, alpha=1/14)
    void update_atr(double h, double l, double prev_close) {
        double tr = h - l;
        if (prev_close > 0.0) {
            double tr2 = std::fabs(h - prev_close);
            double tr3 = std::fabs(l - prev_close);
            if (tr2 > tr) tr = tr2;
            if (tr3 > tr) tr = tr3;
        }
        if (atr14 == 0.0) atr14 = tr;
        else atr14 = atr14 + (1.0/14.0) * (tr - atr14);
    }
};

// ── Per-symbol open position ──────────────────────────────────────────────────
struct TrendPosition {
    bool   active    = false;
    bool   is_long   = false;
    double entry_px  = 0.0;
    double sl_px     = 0.0;
    double trail_sl  = 0.0;
    double mfe       = 0.0;
    bool   trail_armed = false;
    double qty       = 0.0;
    int64_t entry_ms = 0;
    char   symbol[16] = {};
    int    trade_id  = 0;
};

// ── Per-symbol bar builder ────────────────────────────────────────────────────
struct BarBuilder {
    H1Bar  current;
    H1Bar  prev;
    bool   has_prev = false;
    int64_t bar_boundary_ms = 0;

    // Returns true when a bar closes
    bool on_tick(double price, int64_t now_ms) {
        const int64_t H1_MS = 3600000LL;
        const int64_t boundary = (now_ms / H1_MS) * H1_MS;

        if (bar_boundary_ms == 0) {
            // First tick — initialise
            bar_boundary_ms = boundary;
            current = {price, price, price, price, boundary};
            return false;
        }

        if (boundary != bar_boundary_ms) {
            // Bar closed
            prev     = current;
            has_prev = true;
            current  = {price, price, price, price, boundary};
            bar_boundary_ms = boundary;
            return true;
        }

        // Update current bar
        if (price > current.high) current.high = price;
        if (price < current.low)  current.low  = price;
        current.close = price;
        return false;
    }
};

// ── TrendEngine ───────────────────────────────────────────────────────────────
class TrendEngine {
public:
    // Config
    static constexpr double MIN_EMA_SEP_PCT  = 0.003;  // 0.3% min EMA9 vs EMA50 separation
    static constexpr double ATR_SL_MULT      = 1.5;    // SL = 1.5 * ATR
    static constexpr double ATR_TRAIL_ARM    = 2.0;    // arm trail at 2x ATR profit
    static constexpr double ATR_TRAIL_DIST   = 1.0;    // trail at 1x ATR behind peak
    static constexpr double MIN_QTY_USD      = 50.0;   // minimum position size USD
    static constexpr double MAX_QTY_USD      = 500.0;  // maximum position size USD
    static constexpr int    SESSION_START_UTC = 7;     // 07:00 UTC
    static constexpr int    SESSION_END_UTC   = 22;    // 22:00 UTC
    static constexpr int64_t COOLDOWN_MS     = 1800000LL;   // 30min between entries
    static constexpr int64_t FLIP_COOLDOWN_MS = 14400000LL; // 4h direction flip cooldown

    bool shadow_mode = true;  // NEVER set false without explicit authorization

    void set_executor(SpotExecutor* ex) { executor_ = ex; }

    void update_price(int id, double price) {
        if (id >= 0 && id < MAX_SYMBOLS) prices_[id] = price;
    }

    void kill_all() {
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            auto& pos = positions_[i];
            if (!pos.active) continue;
            printf("[TREND-KILL] %s %s entry=%.4f\n",
                   sym_short(i), pos.is_long ? "LONG" : "SHORT", pos.entry_px);
            fflush(stdout);
            if (executor_) executor_->execute(pos.symbol, !pos.is_long, pos.qty, prices_[i]);
            last_exit_dir_[i] = pos.is_long ? 1 : -1;
            last_exit_ms_[i]  = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            pos = TrendPosition{};
        }
    }

    // Called every tick from main.cpp feed callback
    void on_tick(int id, const MarketTick& tick, int64_t now_ms) {
        if (id < 0 || id >= MAX_SYMBOLS) return;

        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (price <= 0.0) return;
        prices_[id] = price;

        auto& bb  = builders_[id];
        auto& ind = indicators_[id];
        auto& pos = positions_[id];

        // Build H1 bar
        bool bar_closed = bb.on_tick(price, now_ms);
        if (bar_closed && bb.has_prev) {
            const H1Bar& b = bb.prev;
            ind.update_ema(b.close);
            ind.update_atr(b.high, b.low,
                           bb.has_prev ? bb.prev.close : 0.0);
        }

        // Manage open position
        if (pos.active) {
            _manage(id, price, now_ms);
            return;
        }

        // Gate checks
        if (!ind.ready)                     return;
        if (ind.atr14 <= 0.0)               return;
        if (!_session_ok(now_ms))           return;
        if (now_ms < cooldown_until_ms_[id]) return;

        // EMA separation gate
        const double ema_sep = std::fabs(ind.ema9 - ind.ema50) / ind.ema50;
        if (ema_sep < MIN_EMA_SEP_PCT)      return;

        const bool ema_long  = (ind.ema9 > ind.ema50);
        const bool ema_short = (ind.ema9 < ind.ema50);

        // Price crossover confirmation
        const bool price_above_ema9 = (price > ind.ema9);
        const bool price_below_ema9 = (price < ind.ema9);

        bool enter_long  = ema_long  && price_above_ema9;
        bool enter_short = ema_short && price_below_ema9;

        // Direction flip cooldown
        if (last_exit_dir_[id] != 0) {
            if (now_ms - last_exit_ms_[id] < FLIP_COOLDOWN_MS) {
                if (enter_long  && last_exit_dir_[id] == -1) enter_long  = false;
                if (enter_short && last_exit_dir_[id] == +1) enter_short = false;
            }
        }

        if (!enter_long && !enter_short) return;

        const bool is_long = enter_long;

        // Size: fixed USD / price → qty
        double qty = MAX_QTY_USD / price;
        // Round to 5 decimal places (Binance lot size)
        qty = std::floor(qty * 100000.0) / 100000.0;
        if (qty * price < MIN_QTY_USD) return;

        // Open position
        pos.active    = true;
        pos.is_long   = is_long;
        pos.entry_px  = price;
        pos.sl_px     = is_long ? (price - ATR_SL_MULT * ind.atr14)
                                : (price + ATR_SL_MULT * ind.atr14);
        pos.trail_sl  = pos.sl_px;
        pos.mfe       = 0.0;
        pos.trail_armed = false;
        pos.qty       = qty;
        pos.entry_ms  = now_ms;
        pos.trade_id  = ++trade_counter_;
        strncpy(pos.symbol, sym_full(id), 15);
        pos.symbol[15] = '\0';

        const char* pfx = shadow_mode ? "[TREND-SHADOW]" : "[TREND]";
        printf("%s %s %s entry=%.4f sl=%.4f atr=%.4f ema9=%.4f ema50=%.4f sep=%.3f%% qty=%.5f\n",
               pfx, sym_short(id), is_long ? "LONG" : "SHORT",
               price, pos.sl_px, ind.atr14, ind.ema9, ind.ema50,
               ema_sep * 100.0, qty);
        fflush(stdout);

        cooldown_until_ms_[id] = now_ms + COOLDOWN_MS;

        // Execute (shadow: logged only)
        if (executor_) {
            executor_->execute(pos.symbol, is_long, qty, price);
        }
    }

    // JSON state for GUI
    std::string state_json() const {
        std::ostringstream js;
        js << std::fixed << std::setprecision(6);
        js << "{";
        js << "\"trades\":" << trade_counter_ << ",";
        js << "\"shadow\":" << (shadow_mode ? "true" : "false") << ",";
        js << "\"total_pnl_pct\":" << total_pnl_pct_ << ",";
        const double wr = trade_counter_ > 0 ? (double)wins_ / trade_counter_ : 0.0;
        js << "\"win_rate\":" << wr << ",";
        js << "\"build\":\"" << BUILD_VERSION << "\",";
        // Prices
        js << "\"prices\":{";
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            js << "\"" << sym_short(i) << "\":" << prices_[i];
        }
        js << "},";
        // Positions
        js << "\"positions\":[";
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            const auto& pos = positions_[i];
            const auto& ind = indicators_[i];
            js << "{\"sym\":\"" << sym_short(i) << "\","
               << "\"active\":" << (pos.active?"true":"false") << ","
               << "\"side\":\"" << (pos.active?(pos.is_long?"LONG":"SHORT"):"FLAT") << "\","
               << "\"entry\":" << pos.entry_px << ","
               << "\"sl\":" << pos.sl_px << ","
               << "\"mfe\":" << pos.mfe << ","
               << "\"trail_armed\":" << (pos.trail_armed?"true":"false") << ","
               << "\"qty\":" << pos.qty << ","
               << "\"ema9\":" << ind.ema9 << ","
               << "\"ema50\":" << ind.ema50 << ","
               << "\"atr\":" << ind.atr14 << ","
               << "\"bars\":" << ind.bar_count << ","
               << "\"ready\":" << (ind.ready?"true":"false") << ","
               << "\"pnl_pct\":0.0}";
        }
        js << "],";
        // Trade log
        js << "\"trade_log\":[";
        bool first = true;
        for (const auto& t : trade_log_) {
            if (!first) js << ",";
            first = false;
            js << "{\"time\":\"" << t.time << "\","
               << "\"sym\":\"" << t.sym << "\","
               << "\"side\":\"" << t.side << "\","
               << "\"entry\":" << t.entry << ","
               << "\"exit\":" << t.exit << ","
               << "\"pnl_pct\":" << t.pnl_pct << ","
               << "\"mfe\":" << t.mfe << ","
               << "\"why\":\"" << t.why << "\","
               << "\"bars_held\":" << t.bars_held << "}";
        }
        js << "]}";
        return js.str();
    }

    int total_trades()    const { return trade_counter_; }
    double total_pnl_pct() const { return total_pnl_pct_; }

private:
    // Price cache (latest tick price per symbol)
    double          prices_[MAX_SYMBOLS] = {};

    // Trade log (last 100 trades)
    struct TradeLog {
        std::string sym, side, time, why;
        double entry=0, exit=0, pnl_pct=0, mfe=0;
        int bars_held=0;
    };
    std::vector<TradeLog> trade_log_;
    int wins_ = 0;

    BarBuilder      builders_[MAX_SYMBOLS];
    TrendIndicators indicators_[MAX_SYMBOLS];
    TrendPosition   positions_[MAX_SYMBOLS];
    int64_t         cooldown_until_ms_[MAX_SYMBOLS] = {};
    int             last_exit_dir_[MAX_SYMBOLS]     = {};
    int64_t         last_exit_ms_[MAX_SYMBOLS]      = {};
    SpotExecutor*   executor_    = nullptr;
    int             trade_counter_ = 0;
    double          total_pnl_pct_ = 0.0;

    static bool _session_ok(int64_t now_ms) {
        time_t t = (time_t)(now_ms / 1000);
        struct tm ti{};
        gmtime_r(&t, &ti);
        const int h = ti.tm_hour;
        return (h >= SESSION_START_UTC && h < SESSION_END_UTC);
    }

    void _manage(int id, double price, int64_t now_ms) {
        auto& pos = positions_[id];
        const auto& ind = indicators_[id];

        const double move = pos.is_long ? (price - pos.entry_px)
                                        : (pos.entry_px - price);
        if (move > pos.mfe) pos.mfe = move;

        // Arm trail
        if (!pos.trail_armed && ind.atr14 > 0.0 && move >= ATR_TRAIL_ARM * ind.atr14) {
            pos.trail_armed = true;
            printf("[TREND-TRAIL-ARM] %s %s mfe=%.4f atr=%.4f\n",
                   sym_short(id), pos.is_long ? "LONG" : "SHORT", move, ind.atr14);
            fflush(stdout);
        }

        // Update trail SL
        if (pos.trail_armed && ind.atr14 > 0.0) {
            const double trail_dist = ATR_TRAIL_DIST * ind.atr14;
            const double new_sl = pos.is_long ? (pos.entry_px + pos.mfe - trail_dist)
                                              : (pos.entry_px - pos.mfe + trail_dist);
            if (pos.is_long  && new_sl > pos.trail_sl) pos.trail_sl = new_sl;
            if (!pos.is_long && new_sl < pos.trail_sl) pos.trail_sl = new_sl;
        }

        // SL hit?
        const bool sl_hit = pos.is_long ? (price <= pos.trail_sl)
                                        : (price >= pos.trail_sl);
        // Max hold: 48h
        const bool timeout = (now_ms - pos.entry_ms >= 172800000LL);

        if (!sl_hit && !timeout) return;

        // Close position
        const double exit_px = sl_hit ? pos.trail_sl : price;
        const double pnl_pct = pos.is_long ? ((exit_px - pos.entry_px) / pos.entry_px * 100.0)
                                           : ((pos.entry_px - exit_px) / pos.entry_px * 100.0);
        total_pnl_pct_ += pnl_pct;
        if (pnl_pct > 0) ++wins_;
        // Record to trade log
        {
            TradeLog tl;
            tl.sym  = sym_short(id);
            tl.side = pos.is_long ? "LONG" : "SHORT";
            tl.entry = pos.entry_px;
            tl.exit  = exit_px;
            tl.pnl_pct = pnl_pct;
            tl.mfe = pos.mfe;
            tl.why = sl_hit ? "SL_HIT" : "TIMEOUT";
            tl.bars_held = builders_[id].has_prev ? builders_[id].prev.bar_ms > 0 ? 1 : 0 : 0;
            // UTC time string
            time_t t = (time_t)(now_ms/1000);
            struct tm ti{}; gmtime_r(&t, &ti);
            char tbuf[20];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d",
                     ti.tm_hour, ti.tm_min, ti.tm_sec);
            tl.time = tbuf;
            trade_log_.push_back(tl);
            if (trade_log_.size() > 100) trade_log_.erase(trade_log_.begin());
        }

        const char* pfx = shadow_mode ? "[TREND-SHADOW]" : "[TREND]";
        printf("%s %s CLOSE %s entry=%.4f exit=%.4f pnl=%.3f%% mfe=%.4f why=%s\n",
               pfx, sym_short(id), pos.is_long ? "LONG" : "SHORT",
               pos.entry_px, exit_px, pnl_pct, pos.mfe,
               sl_hit ? "SL_HIT" : "TIMEOUT");
        fflush(stdout);

        last_exit_dir_[id] = pos.is_long ? 1 : -1;
        last_exit_ms_[id]  = now_ms;

        if (executor_) {
            // Close: reverse side
            executor_->execute(pos.symbol, !pos.is_long, pos.qty, exit_px);
        }

        pos = TrendPosition{};  // reset
    }
};

} // namespace chimera
