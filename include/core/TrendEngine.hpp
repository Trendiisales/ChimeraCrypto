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
#include <curl/curl.h>
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

    // ── Seed indicators from Binance H1 klines history ───────────────────────
    // Fetches last 100 H1 candles per symbol via public REST API (no auth).
    // Called once at startup so engine is ready immediately, not after 50h.
    void seed_from_history() {
        printf("[TREND-SEED] Fetching H1 history for %d symbols...\n", MAX_SYMBOLS);
        fflush(stdout);
        for (int id = 0; id < MAX_SYMBOLS; ++id) {
            _seed_symbol(id);
        }
        printf("[TREND-SEED] Done. Ready symbols: ");
        int ready_count = 0;
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            if (indicators_[i].ready) {
                printf("%s ", sym_short(i));
                ++ready_count;
            }
        }
        printf("(%d/%d)\n", ready_count, MAX_SYMBOLS);
        fflush(stdout);
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
        // Output JSON matching the original Chimera GUI schema exactly.
        // Microstructure fields (liq, bracket, basis etc) are zeroed —
        // TrendEngine replaces them with H1 EMA crossover data.
        std::ostringstream js;
        js << std::fixed << std::setprecision(4);
        js << "{";

        // Top-level stats (old GUI reads these directly)
        const double session_pnl_bp = total_pnl_pct_ * 100.0;  // convert % to bp approximation
        js << "\"pnl\":" << session_pnl_bp << ",";
        js << "\"realized_pnl\":" << session_pnl_bp << ",";
        js << "\"total_trades\":" << trade_counter_ << ",";
        js << "\"open_positions\":" << _count_active() << ",";
        js << "\"build_ver\":\"" << BUILD_VERSION << "\",";
        js << "\"latency_p95\":1.0,";
        js << "\"shadow\":true,";

        // Per-symbol prices (old GUI reads data['btcusdt_price'])
        static constexpr const char* FULL[MAX_SYMBOLS] = {
            "btcusdt","ethusdt","solusdt","bnbusdt","avaxusdt","linkusdt","xrpusdt","dogeusdt"
        };
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            js << "\"" << FULL[i] << "_price\":" << prices_[i] << ",";
        }

        // Per-symbol objects (old GUI reads data['btcusdt'].regime_state etc)
        for (int i = 0; i < MAX_SYMBOLS; ++i) {
            const auto& pos = positions_[i];
            const auto& ind = indicators_[i];
            const double price = prices_[i];

            // Map TrendEngine state to old GUI fields
            // regime_state: BUILDUP=bullish EMA alignment, GRIND=bearish, DEAD=not ready
            const char* regime = "DEAD";
            if (ind.ready) {
                regime = (ind.ema9 > ind.ema50) ? "BUILDUP" : "GRIND";
            }

            // vol_ratio: use EMA separation as proxy (0=flat, 2=strong trend)
            const double ema_sep_pct = ind.ema50 > 0
                ? std::fabs(ind.ema9 - ind.ema50) / ind.ema50 * 100.0
                : 0.0;
            const double vol_ratio = ema_sep_pct / 0.3;  // 0.3% sep = ratio 1.0

            // displacement_bp: distance of price from EMA9 in bp
            const double disp_bp = (price > 0 && ind.ema9 > 0)
                ? (price - ind.ema9) / ind.ema9 * 10000.0
                : 0.0;

            // dynamic_cap_R: use ATR as size proxy
            const double cap_r = ind.atr14 > 0 ? 2.0 : 0.0;

            // micro_active: true when position is open (drives card glow)
            const bool micro_active = pos.active;

            // readiness: bar warmup progress 0-1
            const double readiness = std::min(1.0, (double)ind.bar_count / 50.0);

            js << "\"" << FULL[i] << "\":{"
               << "\"regime_state\":\"" << regime << "\","
               << "\"vol_ratio\":" << vol_ratio << ","
               << "\"displacement_bp\":" << disp_bp << ","
               << "\"dynamic_cap_R\":" << cap_r << ","
               << "\"micro_active\":" << (micro_active ? "true" : "false") << ","
               // Position data mapped into liq/bracket fields so GUI shows it
               << "\"liq_active\":" << (micro_active ? "true" : "false") << ","
               << "\"liq_move_bp\":" << (pos.active ? (pos.mfe / (prices_[i] > 0 ? prices_[i] : 1.0) * 10000.0) : 0.0) << ","
               << "\"liq_mfe_bp\":" << (pos.active ? (pos.mfe / (prices_[i] > 0 ? prices_[i] : 1.0) * 10000.0) : 0.0) << ","
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
               // Readiness fields for the warmup bar
               << "\"liq_readiness\":" << readiness << ","
               << "\"structural_readiness\":" << readiness << ","
               << "\"convex_readiness\":" << readiness << ","
               << "\"compression_readiness\":" << readiness << ","
               << "\"vol_ratio_raw\":" << vol_ratio << ","
               << "\"day_high\":" << prices_[i] * 1.01 << ","
               << "\"day_low\":" << prices_[i] * 0.99
               << "},";
        }

        // Trade log (old GUI reads data.trade_log array)
        js << "\"trade_log\":[";
        bool first = true;
        for (const auto& t : trade_log_) {
            if (!first) js << ",";
            first = false;
            // Convert pnl_pct to bp for old GUI display
            const double pnl_bp = t.pnl_pct * 100.0;
            js << "{\"time\":\"" << t.time << "\","
               << "\"s\":\"" << t.sym << "\","
               << "\"e\":\"TREND\","
               << "\"p\":" << pnl_bp << ","
               << "\"en\":" << t.entry << ","
               << "\"ex\":" << t.exit << ","
               << "\"mfe\":" << (t.mfe / (t.entry > 0 ? t.entry : 1.0) * 10000.0) << ","
               << "\"mae\":0.0,"
               << "\"why\":\"" << t.why << "\"}";
        }
        js << "],";

        // Boost/layer stats (old GUI may read these)
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

    int _count_active() const {
        int n = 0;
        for (int i = 0; i < MAX_SYMBOLS; ++i)
            if (positions_[i].active) ++n;
        return n;
    }

    int total_trades()    const { return trade_counter_; }
    double total_pnl_pct() const { return total_pnl_pct_; }

private:

    // ── CURL write callback ───────────────────────────────────────────────────
    static size_t _curl_write(void* ptr, size_t size, size_t nmemb, std::string* out) {
        out->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    // ── Seed one symbol from Binance H1 klines ───────────────────────────────
    // Klines response: [[open_time, open, high, low, close, volume, ...], ...]
    // We process oldest→newest to build EMA/ATR correctly.
    void _seed_symbol(int id) {
        const char* sym_up = sym_full(id);
        // Convert to uppercase for REST API
        char sym_upper[16] = {};
        for (int i = 0; sym_up[i] && i < 15; ++i)
            sym_upper[i] = (char)toupper((unsigned char)sym_up[i]);

        std::string url = std::string("https://api.binance.com/api/v3/klines?symbol=")
                        + sym_upper + "&interval=1h&limit=100";

        CURL* curl = curl_easy_init();
        if (!curl) { printf("[TREND-SEED] curl_easy_init failed for %s\n", sym_upper); return; }

        std::string body;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            printf("[TREND-SEED] curl failed for %s: %s\n", sym_upper, curl_easy_strerror(res));
            return;
        }

        // Parse JSON array of arrays manually — no JSON lib dependency
        // Each element: [open_time_ms, "open", "high", "low", "close", "volume", ...]
        // We need: high (idx 2), low (idx 3), close (idx 4)
        auto& ind = indicators_[id];
        auto& bb  = builders_[id];

        int bars_loaded = 0;
        double prev_close = 0.0;
        size_t pos = 0;

        while (pos < body.size()) {
            // Find next '[' that starts an inner array
            size_t arr_start = body.find('[', pos);
            if (arr_start == std::string::npos) break;
            size_t arr_end = body.find(']', arr_start);
            if (arr_end == std::string::npos) break;

            std::string arr = body.substr(arr_start + 1, arr_end - arr_start - 1);
            pos = arr_end + 1;

            // Split by comma, extract fields 1(open),2(high),3(low),4(close)
            std::vector<std::string> fields;
            size_t p2 = 0;
            while (p2 < arr.size()) {
                size_t comma = arr.find(',', p2);
                if (comma == std::string::npos) comma = arr.size();
                std::string field = arr.substr(p2, comma - p2);
                // Strip quotes and whitespace
                size_t s1 = field.find_first_not_of(" \"");
                size_t s2 = field.find_last_not_of(" \"");
                if (s1 != std::string::npos) field = field.substr(s1, s2 - s1 + 1);
                fields.push_back(field);
                p2 = comma + 1;
            }
            if (fields.size() < 5) continue;

            // field[0]=open_time, [1]=open, [2]=high, [3]=low, [4]=close
            double high  = 0.0, low = 0.0, close = 0.0;
            try {
                high  = std::stod(fields[2]);
                low   = std::stod(fields[3]);
                close = std::stod(fields[4]);
            } catch (...) { continue; }

            if (high <= 0.0 || low <= 0.0 || close <= 0.0) continue;

            // Feed into indicators
            ind.update_ema(close);
            ind.update_atr(high, low, prev_close);
            prev_close = close;

            // Seed bar builder with last known close
            bb.current.close = close;
            bb.prev.close    = close;
            bb.has_prev      = true;

            ++bars_loaded;
        }

        prices_[id] = prev_close;  // seed price cache with last close
        printf("[TREND-SEED] %s: %d bars loaded | ema9=%.4f ema50=%.4f atr=%.4f ready=%s\n",
               sym_upper, bars_loaded,
               ind.ema9, ind.ema50, ind.atr14,
               ind.ready ? "YES" : "NO");
        fflush(stdout);
    }

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
