// ============================================================================
// Chimera — Tier-2 long-only longer-timeframe edges (rewrite 2026-05-16)
//
// After full OOS backtest + multi-symbol parameter sweep (2026-05-16/17):
//   - 5 D1 engines (Sessions 13-14)
//   - 3 H12 engines (Session 14)
//   - 8 H6 engines (Session 15 — strongest timeframe discovered)
//   - 7 H4 engines (Session 14)
//   - 7 H2 engines (Session 17+20 — new timeframe discovery)
//   - 7 H3 engines (Session 17+20 — new timeframe, no native Binance candles)
//   - 3 H1 engines (Session 15 — XRP/SOL/LINK only, rest eaten by costs)
//   - 12 counter-trend engines (Session 19 — RSI_REVERT + BOLLINGER dip-buy)
//   - 9 new-symbol TSMOM engines (Session 20 — NEAR/SUI/APT/ARB)
//   - 31 counter-trend on new symbols (Session 21a — RSI_REVERT + BOLLINGER on NEAR/SUI/APT/ARB)
//   - 100 exotic TF + extended sweep (Session 21b — H8/H16/D2/D3 TSMOM + H6/H8/H12 CT + DONCHIAN)
//   - 39 new + 18 re-optimized engines (Session 22 — RSI/BOLL H8/H16 + DONCHIAN H8/H16/D2/D3)
//   - Removed: SOL-RSI-H6 (overfit exposed by extended data)
//   - 15 new engines (Session 24 — DONCHIAN H2/H3/H4/H6/H12 gap-fill + TSMOM H12 fill)
//   - Total: 245 engines, all shadow mode, all long-only spot
//
// SESSION 15 DISCOVERY:
//   H6 timeframe was never tested before Session 15.
//   Optimizer found H6 to be the strongest timeframe overall:
//     - XRP-H6: 98.9% of combos profitable, PF=2.68
//     - BTC-H6: 73.3%, PF=2.59
//     - All 8 symbols pass deploy criteria on H6
//   H1 yielded 3 deploy-grade engines (XRP PF=1.66, SOL PF=1.40, LINK PF=1.32)
//
//   instance              symbol     strat   tf    OOS PF  OOS Shrp  Nbr%  Trades
//   ──────────────────────────────────────────────────────────────────────────────
//   D1 ENGINES (5):
//   btc_tsmom_d1          BTCUSDT    TSMOM   D1    1.92    1.67      85%     24
//   eth_tsmom_d1          ETHUSDT    TSMOM   D1    3.15    3.17      91%     26
//   sol_tsmom_d1          SOLUSDT    TSMOM   D1    2.25    2.41      89%     15
//   link_tsmom_d1         LINKUSDT   TSMOM   D1    2.18    1.92     100%     23
//   bnb_tsmom_d1          BNBUSDT    TSMOM   D1    3.16    2.91      90%     32
//
//   H12 ENGINES (3):
//   btc_tsmom_h12         BTCUSDT    TSMOM   H12   3.63    3.40      96%     31
//   doge_tsmom_h12        DOGEUSDT   TSMOM   H12   2.78    3.66     100%     82
//   avax_tsmom_h12        AVAXUSDT   TSMOM   H12   2.61    2.98      87%     76
//
//   H6 ENGINES (8) — SESSION 15:
//   xrp_tsmom_h6          XRPUSDT    TSMOM   H6    2.68    4.41     100%    120
//   btc_tsmom_h6          BTCUSDT    TSMOM   H6    2.59    5.16     100%    169
//   eth_tsmom_h6          ETHUSDT    TSMOM   H6    2.07    3.70     100%    151
//   sol_tsmom_h6          SOLUSDT    TSMOM   H6    2.07    3.25     100%    127
//   bnb_tsmom_h6          BNBUSDT    TSMOM   H6    2.07    2.76     100%     95
//   link_tsmom_h6         LINKUSDT   TSMOM   H6    2.07    3.13     100%     81
//   doge_tsmom_h6         DOGEUSDT   TSMOM   H6    1.72    2.24      77%     91
//   avax_tsmom_h6         AVAXUSDT   TSMOM   H6    1.37    1.82      67%    207
//
//   H4 ENGINES (7):
//   xrp_tsmom_h4          XRPUSDT    TSMOM   H4    2.43    5.80     100%    267
//   bnb_tsmom_h4          BNBUSDT    TSMOM   H4    1.91    3.79     100%    291
//   link_tsmom_h4         LINKUSDT   TSMOM   H4    1.91    4.07      95%    205
//   sol_tsmom_h4          SOLUSDT    TSMOM   H4    1.89    3.82     100%    208
//   btc_tsmom_h4          BTCUSDT    TSMOM   H4    1.82    3.54     100%    167
//   eth_tsmom_h4          ETHUSDT    TSMOM   H4    1.76    3.26     100%    196
//   avax_tsmom_h4         AVAXUSDT   TSMOM   H4    1.47    2.17      83%    231
//
//   H2 ENGINES (5) — NEW SESSION 17:
//   btc_tsmom_h2          BTCUSDT    TSMOM   H2    1.99    4.98     100%    281
//   eth_tsmom_h2          ETHUSDT    TSMOM   H2    1.50    3.02     100%    359
//   sol_tsmom_h2          SOLUSDT    TSMOM   H2    1.78    4.17     100%    340
//   xrp_tsmom_h2          XRPUSDT    TSMOM   H2    2.00    4.70     100%    320
//   link_tsmom_h2         LINKUSDT   TSMOM   H2    1.69    3.76     100%    357
//   doge_tsmom_h2         DOGEUSDT   TSMOM   H2    1.21    1.72      93%    387
//   bnb_tsmom_h2          BNBUSDT    TSMOM   H2    1.19    1.12      87%    436
//
//   H3 ENGINES (7) — SESSION 17+20 (no native Binance candles — cold-start):
//   btc_tsmom_h3          BTCUSDT    TSMOM   H3    1.96    3.52     100%    156
//   eth_tsmom_h3          ETHUSDT    TSMOM   H3    1.74    3.65      98%    278
//   sol_tsmom_h3          SOLUSDT    TSMOM   H3    1.92    4.15      93%    259
//   xrp_tsmom_h3          XRPUSDT    TSMOM   H3    2.19    4.70     100%    243
//   link_tsmom_h3         LINKUSDT   TSMOM   H3    1.94    4.19     100%    254
//   bnb_tsmom_h3          BNBUSDT    TSMOM   H3    1.55    2.74      97%    349
//   doge_tsmom_h3         DOGEUSDT   TSMOM   H3    1.25    1.48      87%    309
//
//   H1 ENGINES (3) — SESSION 15:
//   xrp_tsmom_h1          XRPUSDT    TSMOM   H1    1.66    3.73     100%    327
//   sol_tsmom_h1          SOLUSDT    TSMOM   H1    1.40    3.31     100%    527
//   link_tsmom_h1         LINKUSDT   TSMOM   H1    1.32    3.08      95%    798
//
//   COUNTER-TREND ENGINES (12) — SESSION 19 (fire in bear/ranging markets):
//   TIER 1 (7):
//   eth_rsi30_h3          ETHUSDT    RSI_REV H3    2.41    2.18      92%     87
//   eth_rsi30_h4          ETHUSDT    RSI_REV H4    2.13    1.95      88%     62
//   doge_rsi30_h3         DOGEUSDT   RSI_REV H3    1.98    1.82      85%     94
//   avax_rsi25_h2         AVAXUSDT   RSI_REV H2    2.27    2.05      90%    143
//   doge_rsi25_h2         DOGEUSDT   RSI_REV H2    1.85    1.71      83%    118
//   btc_rsi35_h3          BTCUSDT    RSI_REV H3    1.92    1.78      87%     76
//   bnb_boll25_h3         BNBUSDT    BOLLING H3    2.08    1.93      86%     68
//   TIER 2 (5):
//   eth_boll25_h3         ETHUSDT    BOLLING H3    1.74    1.62      81%     55
//   btc_rsi25_h2          BTCUSDT    RSI_REV H2    1.68    1.55      79%    102
//   link_boll30_h1        LINKUSDT   BOLLING H1    1.59    1.48      77%    134
//   xrp_rsi30_h6          XRPUSDT    RSI_REV H6    1.82    1.69      84%     48
//   xrp_rsi30_h2          XRPUSDT    RSI_REV H2    1.71    1.58      80%    112
//
// DISABLED (no OOS edge — configs preserved below for reference):
//   eth_bb_h6        ETHUSDT    bollinger    H6     0.72   -0.59
//   sol_donch_h6     SOLUSDT    donchian     H6     0.83   -0.57
//   xrp_donch_h1     XRPUSDT    donchian     H1     0.82   -1.19
//   link_rsi_h6      LINKUSDT   rsi_revert   H6     1.17    0.14  (4 trades)
//   btc_overnight_h1 BTCUSDT    overnight    H1     0.31   -4.91
//   btc_weekday_d1   BTCUSDT    weekday      D1     0.44   -1.86
//   doge_tsmom_h4    DOGEUSDT   TSMOM        H4     1.28    1.78  (Nbr=49%)
//
// All instances spot-LONG-only, shadow_mode = true by default. Promote to live
// only after 4 weeks of paper trades match backtest WR/PF within +/- 10%.
//
// COLD-START SEEDING: After constructing the engines and before starting the
// live tick feed, we fetch the most recent N OHLC bars from Binance REST
// (/api/v3/klines, public endpoint) and pre-populate each engine's closed-bar
// deque. H1 = "1h", H2 = "2h", H4 = "4h", H6 = "6h", H12 = "12h", D1 = "1d".
// H3 has NO native Binance interval — those engines cold-start from websocket
// feed only (warm-up ~2-5 days via internal bar synthesis).
//
// HTTP GUI :8080
//   GET  /api/state2  -> JSON with build, spot_prices, engines[]
//   GET  /api/trades  -> JSON array of all trade records
//   POST /api/kill    -> kill_all on every engine (flatten + halt)
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <mutex>
#include <vector>
#include <fstream>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "live/BinanceWSFeed.hpp"
#include "live/BinanceREST.hpp"
#include "live/SpotExecutor.hpp"
#include "core/EdgeEngine.hpp"
#include "core/SymbolIndex.hpp"
#include "core/market_data/MultiSymbolFundingFilter.hpp"
#include "core/LiquidationCascadeDetector.hpp"
#include "live/LiquidationWSFeed.hpp"

#include "version_generated.hpp"
#include "execution/ExchangeLatencyEngine.hpp"

// Required by BinanceWSFeed.cpp (extern declaration)
chimera::ExchangeLatencyEngine g_exchange_latency;
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

// ── Single-instance lock ─────────────────────────────────────────────────────
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) { std::fprintf(stderr, "[FATAL] Cannot open lock file\n"); std::exit(1); }
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        char buf[32] = {};
        if (::read(g_lock_fd, buf, sizeof(buf)-1) < 0) { /* ignore */ }
        ::close(g_lock_fd);
        std::fprintf(stderr, "[FATAL] Chimera already running (PID %s)\n", buf); std::exit(1);
    }
    if (::ftruncate(g_lock_fd, 0) != 0) { /* ignore */ }
    char pidbuf[32]; int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    if (::write(g_lock_fd, pidbuf, len) < 0) { /* ignore */ }
    ::fsync(g_lock_fd);
    std::printf("[STARTUP] Instance lock acquired PID=%d\n", (int)::getpid());
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN); ::close(g_lock_fd); ::unlink(PID_LOCK_FILE); g_lock_fd = -1;
    }
}

// ── Engine instances (shared between feed callback and HTTP server) ─────────
struct EngineSlot {
    int             symbol_id;          // index into SymbolIndex
    chimera::EdgeEngine* engine = nullptr;
    // Fields cached from Config for seeding (EdgeEngine has no public accessor)
    std::string     symbol_str;         // e.g. "btcusdt"
    int64_t         tf_secs  = 0;       // timeframe in seconds
    std::string     tag;                // e.g. "BTC-TSMOM-D1"
    // Backtest metadata for GUI display (not used by engine logic)
    double  oos_pf    = 0.0;
    double  oos_sharpe = 0.0;
    int     oos_nbr   = 0;
    int     oos_trades = 0;
    int     session   = 0;           // session that discovered this engine
};

static std::vector<EngineSlot>  g_slots;
static std::mutex               g_engine_mtx;
static int64_t                  g_startup_ts_ms = 0;   // epoch ms at startup

// Last-seen mid per symbol — used by /api/kill flatten paths and the
// /api/state2 "spot_prices" field that the GUI renders as a live price strip.
static std::atomic<uint64_t>    g_last_spot_px_bits[chimera::MAX_SYMBOLS]{};

static double load_dbl_atomic(const std::atomic<uint64_t>& a) {
    uint64_t bits = a.load(std::memory_order_relaxed);
    double v; __builtin_memcpy(&v, &bits, 8); return v;
}
static void store_dbl_atomic(std::atomic<uint64_t>& a, double v) {
    uint64_t bits; __builtin_memcpy(&bits, &v, 8);
    a.store(bits, std::memory_order_relaxed);
}

static std::string read_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string buf(sz, '\0');
    size_t got = fread(&buf[0], 1, sz, f);
    if (got != (size_t)sz) buf.resize(got);
    fclose(f);
    return buf;
}

static std::string gui_root;

// ── Session 30: Multi-symbol funding filter ─────────────────────────────────
static chimera::MultiSymbolFundingFilter g_funding_filter;
static int64_t g_last_funding_fetch_ms = 0;
static constexpr int64_t FUNDING_FETCH_INTERVAL_MS = 8 * 3600 * 1000LL; // every 8h

// ── Session 30: Liquidation cascade detector ────────────────────────────────
static chimera::LiquidationCascadeDetector g_liq_detector;
static chimera::LiquidationWSFeed g_liq_feed;

// ── Trade journal — persist to JSON, serve via /api/trades ───────────────────
static constexpr const char* TRADES_FILE = "data/trades.json";
static std::vector<chimera::EdgeEngine::TradeRecord> g_trade_log;
static std::mutex g_trades_mtx;

// Format a TradeRecord as a JSON object string
static std::string trade_to_json(const chimera::EdgeEngine::TradeRecord& t) {
    std::ostringstream js;
    js << std::fixed;
    js << "{";
    js << "\"tag\":\"" << t.tag << "\",";
    js << "\"symbol\":\"" << t.symbol << "\",";
    js << "\"strategy\":\"" << t.strategy << "\",";
    js << "\"reason\":\"" << t.reason << "\",";
    js << "\"entry_ts\":" << t.entry_ts_ms << ",";
    js << "\"exit_ts\":" << t.exit_ts_ms << ",";
    js << std::setprecision(6);
    js << "\"entry_px\":" << t.entry_px << ",";
    js << "\"exit_px\":" << t.exit_px << ",";
    js << "\"sl_px\":" << t.sl_px << ",";
    js << std::setprecision(2);
    js << "\"gross_bp\":" << t.gross_bp << ",";
    js << "\"net_bp\":" << t.net_bp << ",";
    js << "\"mfe_bp\":" << t.mfe_bp << ",";
    js << "\"trade_num\":" << t.trade_num << ",";
    js << "\"shadow\":" << (t.shadow ? "true" : "false");
    js << "}";
    return js.str();
}

// Append one trade to the JSON file on disk (newline-delimited JSON for
// simplicity — each line is a valid JSON object, easy to parse and append).
static void persist_trade(const chimera::EdgeEngine::TradeRecord& t) {
    FILE* f = fopen(TRADES_FILE, "a");
    if (!f) {
        std::fprintf(stderr, "[JOURNAL] Failed to open %s for append\n", TRADES_FILE);
        return;
    }
    std::string line = trade_to_json(t) + "\n";
    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
    std::printf("[JOURNAL] Trade #%d persisted: %s %s %+.1fbp\n",
        t.trade_num, t.tag.c_str(), t.reason.c_str(), t.net_bp);
    std::fflush(stdout);
}

// Load trade history from disk on startup (newline-delimited JSON)
static void load_trade_history() {
    FILE* f = fopen(TRADES_FILE, "r");
    if (!f) {
        std::printf("[JOURNAL] No trade history file — starting fresh\n");
        std::fflush(stdout);
        return;
    }
    char line[2048];
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        // Minimal parse — extract key fields from the JSON line
        chimera::EdgeEngine::TradeRecord rec;
        std::string s(line);

        auto extract_str = [&](const char* key) -> std::string {
            auto pos = s.find(std::string("\"") + key + "\":\"");
            if (pos == std::string::npos) return "";
            pos += strlen(key) + 4;
            auto end = s.find('"', pos);
            return (end != std::string::npos) ? s.substr(pos, end - pos) : "";
        };
        auto extract_num = [&](const char* key) -> double {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0.0;
            pos += strlen(key) + 3;
            return std::stod(s.substr(pos));
        };
        auto extract_int64 = [&](const char* key) -> int64_t {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0;
            pos += strlen(key) + 3;
            return std::stoll(s.substr(pos));
        };
        auto extract_bool = [&](const char* key) -> bool {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return false;
            pos += strlen(key) + 3;
            return s.substr(pos, 4) == "true";
        };

        rec.tag         = extract_str("tag");
        rec.symbol      = extract_str("symbol");
        rec.strategy    = extract_str("strategy");
        rec.reason      = extract_str("reason");
        rec.entry_ts_ms = extract_int64("entry_ts");
        rec.exit_ts_ms  = extract_int64("exit_ts");
        rec.entry_px    = extract_num("entry_px");
        rec.exit_px     = extract_num("exit_px");
        rec.sl_px       = extract_num("sl_px");
        rec.gross_bp    = extract_num("gross_bp");
        rec.net_bp      = extract_num("net_bp");
        rec.mfe_bp      = extract_num("mfe_bp");
        rec.trade_num   = (int)extract_num("trade_num");
        rec.shadow      = extract_bool("shadow");

        if (!rec.tag.empty()) {
            std::lock_guard<std::mutex> lk(g_trades_mtx);
            g_trade_log.push_back(rec);
            count++;
        }
    }
    fclose(f);
    std::printf("[JOURNAL] Loaded %d historical trades from %s\n", count, TRADES_FILE);
    std::fflush(stdout);
}

// Build JSON array of all trades for /api/trades
static std::string build_trades_json() {
    std::lock_guard<std::mutex> lk(g_trades_mtx);
    std::ostringstream js;
    js << "[";
    for (size_t i = 0; i < g_trade_log.size(); ++i) {
        if (i > 0) js << ",";
        js << trade_to_json(g_trade_log[i]);
    }
    js << "]";
    return js.str();
}

// Trade callback — called by EdgeEngine on every exit
static void on_trade_callback(const chimera::EdgeEngine::TradeRecord& rec) {
    {
        std::lock_guard<std::mutex> lk(g_trades_mtx);
        g_trade_log.push_back(rec);
    }
    persist_trade(rec);
}

// ── Bar journal — persist completed bars for warm-start + audit trail ────────
// Each engine gets its own file: data/bars/{TAG}.ndjson
// This is the critical fix: bars are now saved to disk so H3 engines (which
// have no REST seed source) don't lose days of accumulated data on restart,
// and ALL engines have a full audit trail of every bar + signal evaluation.

static std::string bar_file_for_tag(const std::string& tag) {
    return "data/bars/" + tag + ".ndjson";
}

static std::string bar_to_json(const chimera::EdgeEngine::BarRecord& b) {
    std::ostringstream js;
    js << std::fixed;
    js << "{";
    js << "\"tag\":\"" << b.tag << "\",";
    js << "\"open_ts_ms\":" << b.open_ts_ms << ",";
    js << "\"tf_secs\":" << b.tf_secs << ",";
    js << std::setprecision(8);
    js << "\"o\":" << b.o << ",";
    js << "\"h\":" << b.h << ",";
    js << "\"l\":" << b.l << ",";
    js << "\"c\":" << b.c << ",";
    js << std::setprecision(6);
    js << "\"atr\":" << b.atr << ",";
    js << std::setprecision(4);
    js << "\"momentum_pct\":" << b.momentum_pct << ",";
    js << "\"signal_ready\":" << (b.signal_ready ? "true" : "false") << ",";
    js << "\"signal_fired\":" << (b.signal_fired ? "true" : "false") << ",";
    js << "\"in_position\":" << (b.in_position ? "true" : "false") << ",";
    js << "\"bars_in_buffer\":" << b.bars_in_buffer;
    js << "}";
    return js.str();
}

static void persist_bar(const chimera::EdgeEngine::BarRecord& b) {
    std::string path = bar_file_for_tag(b.tag);
    FILE* f = fopen(path.c_str(), "a");
    if (!f) {
        std::fprintf(stderr, "[BAR_LOG] Failed to open %s for append\n", path.c_str());
        return;
    }
    std::string line = bar_to_json(b) + "\n";
    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
}

// Bar callback — called by EdgeEngine on every bar close
static void on_bar_callback(const chimera::EdgeEngine::BarRecord& rec) {
    persist_bar(rec);
}

// Load saved bars for warm-start. Returns a vector of SeedBar sorted by open_ts_ms.
// Reads from data/bars/{tag}.ndjson, keeping only the most recent max_bars bars.
static std::vector<chimera::EdgeEngine::SeedBar> load_saved_bars(const std::string& tag, int max_bars = 64) {
    std::vector<chimera::EdgeEngine::SeedBar> result;
    std::string path = bar_file_for_tag(tag);
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return result;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        std::string s(line);
        // Minimal JSON parse — extract o, h, l, c, open_ts_ms
        auto extract_num = [&](const char* key) -> double {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0.0;
            pos += strlen(key) + 3;
            return std::stod(s.substr(pos));
        };
        auto extract_int64 = [&](const char* key) -> int64_t {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0;
            pos += strlen(key) + 3;
            return std::stoll(s.substr(pos));
        };

        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = extract_int64("open_ts_ms");
        sb.o = extract_num("o");
        sb.h = extract_num("h");
        sb.l = extract_num("l");
        sb.c = extract_num("c");
        if (sb.open_ts_ms > 0 && sb.o > 0.0 && sb.c > 0.0) {
            result.push_back(sb);
        }
    }
    fclose(f);

    // Keep only the most recent max_bars (already in chronological order)
    if ((int)result.size() > max_bars) {
        result.erase(result.begin(), result.begin() + ((int)result.size() - max_bars));
    }

    std::printf("[WARM_START][%s] Loaded %d saved bars from %s\n",
        tag.c_str(), (int)result.size(), path.c_str());
    std::fflush(stdout);
    return result;
}

// Build the structured JSON for /api/state2.
// Extends engine state_json with backtest metadata from EngineSlot.
static std::string build_state_json() {
    std::ostringstream js;
    js << "{\"build\":\"" << BUILD_VERSION << "\",";
    js << "\"startup_ts\":" << g_startup_ts_ms << ",";
    js << "\"engine_count\":" << g_slots.size() << ",";

    // ── spot_prices ─────────────────────────────────────────────────────────
    js << "\"spot_prices\":{";
    js << std::fixed << std::setprecision(8);
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
        if (i > 0) js << ",";
        double px = load_dbl_atomic(g_last_spot_px_bits[i]);
        js << "\"" << chimera::sym_full(i) << "\":" << px;
    }
    js << "},";

    // ── engines — merge state_json() with slot metadata ─────────────────────
    js << "\"engines\":[";
    for (size_t i = 0; i < g_slots.size(); ++i) {
        if (i > 0) js << ",";
        if (g_slots[i].engine) {
            std::string ej = g_slots[i].engine->state_json();
            // Insert backtest metadata before the closing brace
            std::string meta;
            {
                std::ostringstream m;
                m << std::fixed << std::setprecision(2);
                m << ",\"oos_pf\":" << g_slots[i].oos_pf;
                m << ",\"oos_sharpe\":" << g_slots[i].oos_sharpe;
                m << ",\"oos_nbr\":" << g_slots[i].oos_nbr;
                m << ",\"oos_trades\":" << g_slots[i].oos_trades;
                m << ",\"session\":" << g_slots[i].session;
                meta = m.str();
            }
            // Insert before final '}'
            if (!ej.empty() && ej.back() == '}') {
                ej.pop_back();
                ej += meta + "}";
            }
            js << ej;
        } else {
            js << "null";
        }
    }
    js << "]}";
    return js.str();
}

static void http_server_thread(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
    listen(server_fd, 16);
    std::printf("[HTTP] GUI on port %d | root: %s\n", port, gui_root.c_str());
    std::fflush(stdout);

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) break;
        struct timeval tv{2, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char req[512] = {};
        if (read(client, req, sizeof(req)-1) < 0) { /* ignore */ }

        std::string body;
        const char* ct = "application/json";
        int status = 200;

        if (strstr(req, "GET /api/bars/")) {
            // /api/bars/{TAG} — return last 100 saved bars for an engine
            const char* tag_start = strstr(req, "/api/bars/") + 10;
            char tag_buf[128] = {};
            int ti = 0;
            while (tag_start[ti] && tag_start[ti] != ' ' && tag_start[ti] != '?' && ti < 127) {
                tag_buf[ti] = tag_start[ti]; ti++;
            }
            std::string tag_str(tag_buf, ti);
            std::string bar_path = bar_file_for_tag(tag_str);
            // Read last 100 lines from the file
            std::vector<std::string> lines;
            FILE* bf = fopen(bar_path.c_str(), "r");
            if (bf) {
                char bline[1024];
                while (fgets(bline, sizeof(bline), bf)) {
                    lines.push_back(std::string(bline));
                    if (lines.size() > 100) lines.erase(lines.begin());
                }
                fclose(bf);
            }
            std::ostringstream bjs;
            bjs << "[";
            for (size_t bi = 0; bi < lines.size(); ++bi) {
                if (bi > 0) bjs << ",";
                // Each line is already a JSON object, just strip newline
                std::string& l = lines[bi];
                while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
                bjs << l;
            }
            bjs << "]";
            body = bjs.str();
        } else if (strstr(req, "GET /api/positions")) {
            // /api/positions — return only engines currently holding a position
            // with full details: tag, symbol, strategy, timeframe, entry price,
            // stop-loss, trailing stop, unrealised P&L, bars held, MFE
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            std::ostringstream pjs;
            pjs << std::fixed << std::setprecision(6);
            pjs << "{\"positions\":[";
            int count = 0;
            for (size_t i = 0; i < g_slots.size(); ++i) {
                if (!g_slots[i].engine || !g_slots[i].engine->in_position()) continue;
                if (count > 0) pjs << ",";
                // Get spot price for unrealised P&L
                double spot = load_dbl_atomic(g_last_spot_px_bits[g_slots[i].symbol_id]);
                double entry = 0.0;
                // Parse entry_px from state_json (engine has no public accessor)
                std::string ej = g_slots[i].engine->state_json();
                // Extract entry_px
                auto extract = [&](const char* key) -> double {
                    auto pos = ej.find(std::string("\"") + key + "\":");
                    if (pos == std::string::npos) return 0.0;
                    pos += strlen(key) + 3;
                    return std::stod(ej.substr(pos));
                };
                entry = extract("entry_px");
                double sl = extract("sl_px");
                double trail_stop = extract("trail_stop_px");
                double mfe_bp = extract("mfe_bp");
                int bars_held = (int)extract("bars_held");
                bool trail_armed = (ej.find("\"trail_armed\":true") != std::string::npos);
                double unreal_bp = (spot > 0.0 && entry > 0.0)
                    ? (spot / entry - 1.0) * 1e4 : 0.0;

                pjs << "{";
                pjs << "\"tag\":\"" << g_slots[i].tag << "\",";
                pjs << "\"symbol\":\"" << g_slots[i].symbol_str << "\",";
                pjs << "\"tf_secs\":" << g_slots[i].tf_secs << ",";
                pjs << "\"tf_human\":\"";
                if (g_slots[i].tf_secs >= 86400) pjs << (g_slots[i].tf_secs/86400) << "D";
                else pjs << (g_slots[i].tf_secs/3600) << "H";
                pjs << "\",";
                pjs << "\"entry_px\":" << entry << ",";
                pjs << "\"spot_px\":" << spot << ",";
                pjs << "\"sl_px\":" << sl << ",";
                pjs << "\"trail_stop_px\":" << trail_stop << ",";
                pjs << "\"trail_armed\":" << (trail_armed ? "true" : "false") << ",";
                pjs << std::setprecision(2);
                pjs << "\"unreal_bp\":" << unreal_bp << ",";
                pjs << "\"mfe_bp\":" << mfe_bp << ",";
                pjs << "\"bars_held\":" << bars_held << ",";
                pjs << std::setprecision(2);
                pjs << "\"oos_pf\":" << g_slots[i].oos_pf << ",";
                pjs << "\"oos_sharpe\":" << g_slots[i].oos_sharpe << ",";
                pjs << "\"session\":" << g_slots[i].session;
                pjs << std::setprecision(6);
                pjs << "}";
                count++;
            }
            pjs << "],\"count\":" << count << "}";
            body = pjs.str();
        } else if (strstr(req, "GET /api/trades")) {
            body = build_trades_json();
        } else if (strstr(req, "POST /api/kill")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            for (auto& s : g_slots) {
                if (!s.engine) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                s.engine->kill_all(px, now_ms);
            }
            body = "{\"ok\":true}";
        } else if (strstr(req, "GET /api/state2") || strstr(req, "GET /api/state")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            body = build_state_json();
        } else if (strstr(req, "GET /app.js")) {
            ct = "application/javascript";
            body = read_file(gui_root + "/app.js");
            if (body.empty()) { status = 404; body = "not found"; }
        } else if (strstr(req, "GET /style.css")) {
            ct = "text/css";
            body = read_file(gui_root + "/style.css");
            if (body.empty()) { status = 404; body = "not found"; }
        } else if (strstr(req, "GET /favicon.svg")) {
            ct = "image/svg+xml";
            body = read_file(gui_root + "/favicon.svg");
            if (body.empty()) { status = 404; body = "not found"; }
        } else {
            ct = "text/html";
            body = read_file(gui_root + "/index.html");
            if (body.empty()) { status = 404; body = "index.html not found"; }
        }

        std::ostringstream resp;
        resp << "HTTP/1.1 " << status << " OK\r\n"
             << "Content-Type: " << ct << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Cache-Control: no-cache\r\n"
             << "Connection: close\r\n\r\n"
             << body;
        auto s = resp.str();
        if (write(client, s.c_str(), s.size()) < 0) { /* ignore */ }
        close(client);
    }
    close(server_fd);
}

// ── Signal handler ────────────────────────────────────────────────────────────
static std::atomic<bool> g_running{true};
void signal_handler(int) { g_running = false; }

// ── LiveRuntimeConfig — reads config/live_config.json so the runtime is
// actually driven by that file instead of the hard-coded credentials path.
// ───────────────────────────────────────────────────────────────────────
struct LiveRuntimeConfig {
    std::string credentials_file = "config/binance_credentials.json";
    bool        shadow_mode      = true;
    double      max_position_usd = 10000.0;
    double      min_edge_bps     = 10.0;
};

static std::string lrc_extract_string(const std::string& s, const char* key) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return "";
    pos = s.find('"', pos + needle.size());
    if (pos == std::string::npos) return "";
    auto end = s.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return s.substr(pos + 1, end - pos - 1);
}

static double lrc_extract_double(const std::string& s, const char* key, double fallback) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return fallback;
    pos += needle.size();
    try { return std::stod(s.substr(pos)); } catch (...) { return fallback; }
}

static bool lrc_extract_bool(const std::string& s, const char* key, bool fallback) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return fallback;
    pos += needle.size();
    auto vstart = s.find_first_not_of(" \t\r\n", pos);
    if (vstart == std::string::npos) return fallback;
    if (s.compare(vstart, 4, "true")  == 0) return true;
    if (s.compare(vstart, 5, "false") == 0) return false;
    return fallback;
}

static LiveRuntimeConfig load_live_runtime_config(const std::string& path = "config/live_config.json") {
    LiveRuntimeConfig cfg;
    std::vector<std::string> candidates = {
        path,
        "../config/live_config.json",
        "../../config/live_config.json",
    };
    std::ifstream f;
    std::string opened;
    for (const auto& p : candidates) {
        f.open(p);
        if (f.is_open()) { opened = p; break; }
    }
    if (!f.is_open()) {
        std::fprintf(stderr, "[STARTUP] live_config.json not found — using defaults\n");
        return cfg;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    std::string creds = lrc_extract_string(content, "credentials_file");
    if (!creds.empty()) cfg.credentials_file = creds;
    cfg.shadow_mode      = lrc_extract_bool(content, "shadow_mode", true);
    cfg.max_position_usd = lrc_extract_double(content, "max_position_usd", 10000.0);
    cfg.min_edge_bps     = lrc_extract_double(content, "min_edge_bps", 10.0);
    std::printf("[STARTUP] live_config loaded from %s: creds=%s shadow=%d max_pos=%.2f min_edge=%.2fbp\n",
                opened.c_str(), cfg.credentials_file.c_str(),
                cfg.shadow_mode ? 1 : 0, cfg.max_position_usd, cfg.min_edge_bps);
    std::fflush(stdout);
    return cfg;
}

// ── Map EdgeEngine tf_secs -> Binance kline interval string ────────────────
// Returns empty string if the timeframe doesn't map to a Binance interval.
static const char* tf_to_binance_interval(int64_t tf_secs) {
    switch (tf_secs) {
        case 60:     return "1m";
        case 180:    return "3m";
        case 300:    return "5m";
        case 900:    return "15m";
        case 1800:   return "30m";
        case 3600:   return "1h";
        case 7200:   return "2h";
        case 14400:  return "4h";
        case 21600:  return "6h";
        case 28800:  return "8h";
        case 43200:  return "12h";
        case 86400:  return "1d";
        case 259200: return "3d";
        case 604800: return "1w";
        default:     return "";
    }
}

// ── Seed one engine from REST klines (called once per engine at startup) ───
static void seed_engine_from_history(chimera::BinanceREST& rest,
                                     chimera::EdgeEngine& engine,
                                     const std::string& symbol,
                                     int64_t tf_secs,
                                     const std::string& tag,
                                     int limit = 64)
{
    const char* interval = tf_to_binance_interval(tf_secs);
    if (!interval || !*interval) {
        std::fprintf(stderr, "[SEED][%s] no Binance interval for tf_secs=%lld — skip\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    auto klines = rest.fetch_klines(symbol, interval, limit);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED][%s] fetch_klines returned 0 bars (symbol=%s interval=%s) — "
                              "engine will cold-start from live ticks\n",
                     tag.c_str(), symbol.c_str(), interval);
        return;
    }

    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(klines.size());
    for (const auto& k : klines) {
        chimera::EdgeEngine::SeedBar b{};
        b.open_ts_ms = k.open_ts_ms;
        b.o = k.o; b.h = k.h; b.l = k.l; b.c = k.c;
        seed.push_back(b);
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED][%s] symbol=%s interval=%s fetched=%d kept=%d\n",
                tag.c_str(), symbol.c_str(), interval, (int)klines.size(), kept);
    std::fflush(stdout);
}

// ── Seed one engine from D1 klines aggregated to target timeframe ─────────
// For engines whose bar period is >= 1 day but has no native Binance interval
// (D2 = 172800s), we fetch D1 klines and aggregate them. This gives far more
// history than H1 aggregation: 1000 D1 bars / 2 = 500 D2 bars (vs 20 from H1).
// Shadow tuning 2026-05-17: fixes 11 under-seeded D2 engines.
// ───────────────────────────────────────────────────────────────────────────
static void seed_engine_from_d1_aggregation(chimera::BinanceREST& rest,
                                            chimera::EdgeEngine& engine,
                                            const std::string& symbol,
                                            int64_t tf_secs,
                                            const std::string& tag,
                                            int target_bars = 64)
{
    int d1_per_target = (int)(tf_secs / 86400);
    if (d1_per_target < 2) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] tf_secs=%lld too small for D1 aggregation\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    int d1_needed = d1_per_target * target_bars;
    if (d1_needed > 1000) d1_needed = 1000;  // Binance hard limit

    auto klines = rest.fetch_klines(symbol, "1d", d1_needed);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] fetch_klines(1d) returned 0 — falling through\n",
                     tag.c_str());
        return;
    }

    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(target_bars);

    chimera::EdgeEngine::SeedBar cur{};
    int64_t cur_bar_id = -1;

    for (const auto& k : klines) {
        int64_t bar_id = (k.open_ts_ms / 1000) / tf_secs;

        if (bar_id != cur_bar_id) {
            if (cur_bar_id >= 0 && cur.o > 0.0) {
                seed.push_back(cur);
            }
            cur_bar_id     = bar_id;
            cur.open_ts_ms = bar_id * tf_secs * 1000;
            cur.o = k.o;
            cur.h = k.h;
            cur.l = k.l;
            cur.c = k.c;
        } else {
            if (k.h > cur.h) cur.h = k.h;
            if (k.l < cur.l) cur.l = k.l;
            cur.c = k.c;
        }
    }

    if (cur_bar_id >= 0 && cur.o > 0.0) {
        seed.push_back(cur);
    }

    if (seed.empty()) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] aggregation produced 0 bars\n",
                     tag.c_str());
        return;
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED-D1AGG][%s] symbol=%s d1_fetched=%d -> aggregated=%d bars (tf=%llds) kept=%d\n",
                tag.c_str(), symbol.c_str(), (int)klines.size(),
                (int)seed.size(), (long long)tf_secs, kept);
    std::fflush(stdout);
}

// ── Seed one engine from H1 klines aggregated to target timeframe ─────────
// For engines whose timeframe has no native Binance interval (H3, H16),
// we fetch H1 klines (which Binance DOES provide) and aggregate them into
// the target timeframe. This eliminates cold-start entirely.
//
// E.g. for H3 (10800s): fetch 192 H1 bars → aggregate into 64 H3 bars.
// For H16 (57600s): fetch 1000 H1 bars → aggregate into 62 H16 bars.
// For D2 (172800s): fetch 1000 H1 bars → aggregate into 20 D2 bars.
// ───────────────────────────────────────────────────────────────────────────
static void seed_engine_from_h1_aggregation(chimera::BinanceREST& rest,
                                            chimera::EdgeEngine& engine,
                                            const std::string& symbol,
                                            int64_t tf_secs,
                                            const std::string& tag,
                                            int target_bars = 64)
{
    // How many H1 bars do we need to produce target_bars of the target TF?
    int h1_per_target = (int)(tf_secs / 3600);
    if (h1_per_target < 2) {
        std::fprintf(stderr, "[SEED-AGG][%s] tf_secs=%lld too small for H1 aggregation\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    int h1_needed = h1_per_target * target_bars;
    if (h1_needed > 1000) h1_needed = 1000;  // Binance hard limit

    auto klines = rest.fetch_klines(symbol, "1h", h1_needed);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED-AGG][%s] fetch_klines(1h) returned 0 — cold-start from ticks\n",
                     tag.c_str());
        return;
    }

    // Aggregate H1 bars into target timeframe bars.
    // Bar boundaries are aligned to epoch: bar_id = (open_ts_ms / 1000) / tf_secs
    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(target_bars);

    chimera::EdgeEngine::SeedBar cur{};
    int64_t cur_bar_id = -1;

    for (const auto& k : klines) {
        int64_t bar_id = (k.open_ts_ms / 1000) / tf_secs;

        if (bar_id != cur_bar_id) {
            // Save the previous completed bar (if any)
            if (cur_bar_id >= 0 && cur.o > 0.0) {
                seed.push_back(cur);
            }
            // Start a new bar
            cur_bar_id     = bar_id;
            cur.open_ts_ms = bar_id * tf_secs * 1000;
            cur.o = k.o;
            cur.h = k.h;
            cur.l = k.l;
            cur.c = k.c;
        } else {
            // Extend the current bar
            if (k.h > cur.h) cur.h = k.h;
            if (k.l < cur.l) cur.l = k.l;
            cur.c = k.c;  // last close wins
        }
    }

    // Push the final bar (may be partially complete — that's OK).
    // The engine's on_tick() will extend or roll it forward correctly.
    // Including it is critical for D2 engines where 1000 H1 bars only
    // produces ~20 complete bars and lookback=20 needs 21 in the buffer.
    if (cur_bar_id >= 0 && cur.o > 0.0) {
        seed.push_back(cur);
    }

    if (seed.empty()) {
        std::fprintf(stderr, "[SEED-AGG][%s] aggregation produced 0 bars\n",
                     tag.c_str());
        return;
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED-AGG][%s] symbol=%s h1_fetched=%d -> aggregated=%d bars (tf=%llds) kept=%d\n",
                tag.c_str(), symbol.c_str(), (int)klines.size(),
                (int)seed.size(), (long long)tf_secs, kept);
    std::fflush(stdout);
}

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    g_startup_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::printf("[STARTUP] Chimera — Tier-2 Edge Engines (285 active) | build=%s\n", BUILD_VERSION);
    std::fflush(stdout);

    acquire_instance_lock();
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Runtime config — drives credentials path + shadow_mode + position sizing.
    LiveRuntimeConfig runtime_cfg = load_live_runtime_config();

    // Executor — engines mirror entry/exit intents into this via on_order_intent.
    chimera::SpotExecutor executor;
    bool exec_ok = executor.init(runtime_cfg.credentials_file);
    if (!exec_ok) {
        std::fprintf(stderr, "[STARTUP] WARNING: executor init failed — order intents will drop\n");
    }

    // wire_engine — single helper applied to every engine. Sets shadow_mode
    // from runtime config, on_trade for dashboard history, on_bar for
    // warm-start persistence, and on_order_intent to mirror entries/exits
    // into SpotExecutor (shadow mode -> signed-but-not-posted log).
    auto wire_engine = [&](chimera::EdgeEngine& engine) {
        engine.shadow_mode = runtime_cfg.shadow_mode;
        engine.set_on_trade(on_trade_callback);
        engine.set_on_bar(on_bar_callback);
        engine.set_on_order_intent(
            [&](const chimera::EdgeEngine::OrderIntentRecord& intent) {
                if (!exec_ok || !executor.is_ready()) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] executor not ready tag=%s symbol=%s side=%s px=%.8f\n",
                        intent.tag.c_str(), intent.symbol.c_str(),
                        intent.is_buy ? "BUY" : "SELL", intent.ref_px);
                    return;
                }
                if (intent.ref_px <= 0.0 || runtime_cfg.max_position_usd <= 0.0) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] invalid sizing tag=%s px=%.8f max_pos=%.2f\n",
                        intent.tag.c_str(), intent.ref_px, runtime_cfg.max_position_usd);
                    return;
                }
                double qty = runtime_cfg.max_position_usd / intent.ref_px;
                std::printf("[ORDER-INTENT] tag=%s symbol=%s side=%s qty=%.8f px=%.4f\n",
                    intent.tag.c_str(), intent.symbol.c_str(),
                    intent.is_buy ? "BUY" : "SELL", qty, intent.ref_px);
                std::fflush(stdout);
                auto result = executor.execute(intent.symbol, intent.is_buy, qty, intent.ref_px);
                if (!result.ok) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] execute failed tag=%s symbol=%s err=%s\n",
                        intent.tag.c_str(), intent.symbol.c_str(), result.error.c_str());
                }
            });
        // ── Smart Pyramid callback: only fires after BE-lock armed + profit >= pyramid_arm_atr ─
        // EdgeEngine guarantees base trail_stop floors at entry+round_trip_bp before any add.
        // Pyramid add size = base_size * size_mult (default 50%). Shared trail stop covers all legs.
        engine.set_on_pyramid(
            [&](const std::string& tag, double price, double size_mult, int add_num) {
                if (!exec_ok || !executor.is_ready()) {
                    std::fprintf(stderr,
                        "[PYRAMID] executor not ready tag=%s add=%d px=%.8f\n",
                        tag.c_str(), add_num, price);
                    return;
                }
                if (price <= 0.0 || runtime_cfg.max_position_usd <= 0.0 || size_mult <= 0.0) {
                    std::fprintf(stderr,
                        "[PYRAMID] invalid sizing tag=%s px=%.8f size_mult=%.2f\n",
                        tag.c_str(), price, size_mult);
                    return;
                }
                double add_usd = runtime_cfg.max_position_usd * size_mult;
                double qty = add_usd / price;
                std::printf("[PYRAMID-INTENT] tag=%s add=%d size_mult=%.0f%% add_usd=%.2f qty=%.8f px=%.4f\n",
                    tag.c_str(), add_num, size_mult * 100.0, add_usd, qty, price);
                std::fflush(stdout);
                // Pyramid uses engine's symbol context; resolve via tag prefix is non-trivial,
                // so use engine.symbol field captured at config time.
                auto result = executor.execute(engine.cfg().symbol, true, qty, price);
                if (!result.ok) {
                    std::fprintf(stderr,
                        "[PYRAMID] execute failed tag=%s err=%s\n",
                        tag.c_str(), result.error.c_str());
                }
            });
    };

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION A: D1 ENGINES (Sessions 13-14) ──────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE A1: BTC-TSMOM-D1 — PF=1.92, Sharpe=1.67, Nbr=85%
    chimera::EdgeEngine::Config btc_d1_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d1(btc_d1_cfg);
    wire_engine(btc_tsmom_d1);

    // ENGINE A2: ETH-TSMOM-D1 — PF=3.15, Sharpe=3.17, Nbr=91%
    chimera::EdgeEngine::Config eth_d1_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d1(eth_d1_cfg);
    wire_engine(eth_tsmom_d1);

    // ENGINE A3: SOL-TSMOM-D1 — PF=2.25, Sharpe=2.41, Nbr=89%
    chimera::EdgeEngine::Config sol_d1_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sol_tsmom_d1(sol_d1_cfg);
    wire_engine(sol_tsmom_d1);

    // ENGINE A4: LINK-TSMOM-D1 — PF=2.18, Sharpe=1.92, Nbr=100%
    chimera::EdgeEngine::Config link_d1_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d1(link_d1_cfg);
    wire_engine(link_tsmom_d1);

    // ENGINE A5: BNB-TSMOM-D1 — PF=3.16, Sharpe=2.91, Nbr=90%
    chimera::EdgeEngine::Config bnb_d1_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d1(bnb_d1_cfg);
    wire_engine(bnb_tsmom_d1);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION B: H12 ENGINES (Session 14) ─────────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE B1: BTC-TSMOM-H12 — PF=3.63, Sharpe=3.40, 31 trades, Nbr=96%
    chimera::EdgeEngine::Config btc_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h12(btc_h12_cfg);
    wire_engine(btc_tsmom_h12);

    // ENGINE B2: DOGE-TSMOM-H12 — PF=2.78, Sharpe=3.66, 82 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h12(doge_h12_cfg);
    wire_engine(doge_tsmom_h12);

    // ENGINE B3: AVAX-TSMOM-H12 — PF=2.61, Sharpe=2.98, 76 trades, Nbr=87%
    chimera::EdgeEngine::Config avax_h12_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h12(avax_h12_cfg);
    wire_engine(avax_tsmom_h12);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION C: H6 ENGINES (Session 15 — strongest TF discovered) ────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE C1: XRP-TSMOM-H6 — PF=2.68, Sharpe=4.41, 120 trades, Nbr=100%
    // 98.9% of ALL parameter combos profitable — most robust edge in the system
    chimera::EdgeEngine::Config xrp_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h6(xrp_h6_cfg);
    wire_engine(xrp_tsmom_h6);

    // ENGINE C2: BTC-TSMOM-H6 — PF=2.59, Sharpe=5.16, 169 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h6(btc_h6_cfg);
    wire_engine(btc_tsmom_h6);

    // ENGINE C3: ETH-TSMOM-H6 — PF=2.07, Sharpe=3.70, 151 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h6(eth_h6_cfg);
    wire_engine(eth_tsmom_h6);

    // ENGINE C4: SOL-TSMOM-H6 — PF=2.07, Sharpe=3.25, 127 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h6(sol_h6_cfg);
    wire_engine(sol_tsmom_h6);

    // ENGINE C5: BNB-TSMOM-H6 — PF=2.07, Sharpe=2.76, 95 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_h6_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h6(bnb_h6_cfg);
    wire_engine(bnb_tsmom_h6);

    // ENGINE C6: LINK-TSMOM-H6 — PF=2.07, Sharpe=3.13, 81 trades, Nbr=100%
    chimera::EdgeEngine::Config link_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h6(link_h6_cfg);
    wire_engine(link_tsmom_h6);

    // ENGINE C7: DOGE-TSMOM-H6 — PF=1.72, Sharpe=2.24, 91 trades, Nbr=77%
    chimera::EdgeEngine::Config doge_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h6(doge_h6_cfg);
    wire_engine(doge_tsmom_h6);

    // ENGINE C8: AVAX-TSMOM-H6 — PF=1.37, Sharpe=1.82, 207 trades, Nbr=67%
    chimera::EdgeEngine::Config avax_h6_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h6(avax_h6_cfg);
    wire_engine(avax_tsmom_h6);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION D: H4 ENGINES (Session 14) ──────────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE D1: XRP-TSMOM-H4 — PF=2.43, Sharpe=5.80, 267 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h4(xrp_h4_cfg);
    wire_engine(xrp_tsmom_h4);

    // ENGINE D2: BNB-TSMOM-H4 — PF=1.91, Sharpe=3.79, 291 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_h4_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h4(bnb_h4_cfg);
    wire_engine(bnb_tsmom_h4);

    // ENGINE D3: LINK-TSMOM-H4 — PF=1.91, Sharpe=4.07, 205 trades, Nbr=95%
    chimera::EdgeEngine::Config link_h4_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h4(link_h4_cfg);
    wire_engine(link_tsmom_h4);

    // ENGINE D4: SOL-TSMOM-H4 — PF=1.89, Sharpe=3.82, 208 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h4(sol_h4_cfg);
    wire_engine(sol_tsmom_h4);

    // ENGINE D5: BTC-TSMOM-H4 — PF=1.82, Sharpe=3.54, 167 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h4_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 25,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h4(btc_h4_cfg);
    wire_engine(btc_tsmom_h4);

    // ENGINE D6: ETH-TSMOM-H4 — PF=1.76, Sharpe=3.26, 196 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h4(eth_h4_cfg);
    wire_engine(eth_tsmom_h4);

    // ENGINE D7: AVAX-TSMOM-H4 — PF=1.47, Sharpe=2.17, 231 trades, Nbr=83%
    chimera::EdgeEngine::Config avax_h4_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h4(avax_h4_cfg);
    wire_engine(avax_tsmom_h4);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION E: H1 ENGINES (Session 15 — XRP/SOL/LINK only) ──────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE E1: XRP-TSMOM-H1 — PF=1.66, Sharpe=3.73, 327 trades, Nbr=100%
    // 69.3% of combos profitable — genuine edge even at H1 cost drag
    chimera::EdgeEngine::Config xrp_h1_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 3600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h1(xrp_h1_cfg);
    wire_engine(xrp_tsmom_h1);

    // ENGINE E2: SOL-TSMOM-H1 — PF=1.40, Sharpe=3.31, 527 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h1_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 3600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h1(sol_h1_cfg);
    wire_engine(sol_tsmom_h1);

    // ENGINE E3: LINK-TSMOM-H1 — PF=1.32, Sharpe=3.08, 798 trades, Nbr=95%
    chimera::EdgeEngine::Config link_h1_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 3600,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h1(link_h1_cfg);
    wire_engine(link_tsmom_h1);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION F: H2 ENGINES (Session 17 — new timeframe discovery) ────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE F1: BTC-TSMOM-H2 — PF=1.99, Sharpe=4.98, 281 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h2(btc_h2_cfg);
    wire_engine(btc_tsmom_h2);

    // ENGINE F2: ETH-TSMOM-H2 — PF=1.50, Sharpe=3.02, 359 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_h2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h2(eth_h2_cfg);
    wire_engine(eth_tsmom_h2);

    // ENGINE F3: SOL-TSMOM-H2 — PF=1.78, Sharpe=4.17, 340 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h2(sol_h2_cfg);
    wire_engine(sol_tsmom_h2);

    // ENGINE F4: XRP-TSMOM-H2 — PF=2.00, Sharpe=4.70, 320 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_h2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h2(xrp_h2_cfg);
    wire_engine(xrp_tsmom_h2);

    // ENGINE F5: LINK-TSMOM-H2 — PF=1.69, Sharpe=3.76, 357 trades, Nbr=100%
    chimera::EdgeEngine::Config link_h2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h2(link_h2_cfg);
    wire_engine(link_tsmom_h2);

    // ENGINE F6: DOGE-TSMOM-H2 — PF=1.21, Sharpe=1.72, 387 trades, Nbr=93%
    chimera::EdgeEngine::Config doge_h2_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h2(doge_h2_cfg);
    wire_engine(doge_tsmom_h2);

    // ENGINE F7: BNB-TSMOM-H2 — PF=1.19, Sharpe=1.12, 436 trades, Nbr=87%
    chimera::EdgeEngine::Config bnb_h2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h2(bnb_h2_cfg);
    wire_engine(bnb_tsmom_h2);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION G: H3 ENGINES (Session 17 — no native Binance candles) ──
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE G1: BTC-TSMOM-H3 — PF=1.96, Sharpe=3.52, 156 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h3(btc_h3_cfg);
    wire_engine(btc_tsmom_h3);

    // ENGINE G2: ETH-TSMOM-H3 — PF=1.74, Sharpe=3.65, 278 trades, Nbr=98%
    chimera::EdgeEngine::Config eth_h3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h3(eth_h3_cfg);
    wire_engine(eth_tsmom_h3);

    // ENGINE G3: SOL-TSMOM-H3 — PF=1.92, Sharpe=4.15, 259 trades, Nbr=93%
    chimera::EdgeEngine::Config sol_h3_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h3(sol_h3_cfg);
    wire_engine(sol_tsmom_h3);

    // ENGINE G4: XRP-TSMOM-H3 — PF=2.19, Sharpe=4.70, 243 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_h3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h3(xrp_h3_cfg);
    wire_engine(xrp_tsmom_h3);

    // ENGINE G5: LINK-TSMOM-H3 — PF=1.94, Sharpe=4.19, 254 trades, Nbr=100%
    chimera::EdgeEngine::Config link_h3_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h3(link_h3_cfg);
    wire_engine(link_tsmom_h3);

    // ENGINE G6: BNB-TSMOM-H3 — PF=1.55, Sharpe=2.74, 349 trades, Nbr=97%
    chimera::EdgeEngine::Config bnb_h3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h3(bnb_h3_cfg);
    wire_engine(bnb_tsmom_h3);

    // ENGINE G7: DOGE-TSMOM-H3 — PF=1.25, Sharpe=1.48, 309 trades, Nbr=87%
    chimera::EdgeEngine::Config doge_h3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h3(doge_h3_cfg);
    wire_engine(doge_tsmom_h3);

    // ══════════════════════════════════════════════════════════════════════
    // BEAR-MARKET COUNTER-TREND ENGINES — Session 19 (2026-05-16)
    //
    // These fire when TSMOM sits flat (bearish/ranging markets):
    //   - RSI_REVERT: buy oversold bounce (long-only spot dip-buy)
    //   - BOLLINGER K=2.5/3.0: buy extreme lower-band pierce (long-only spot)
    //
    // All spot-LONG-only. Shadow mode. Validated via optimizer_v2 with
    // neighbourhood stability scoring across 446K backtests.
    // ══════════════════════════════════════════════════════════════════════

    // ── TIER 1 — STRONG edges (Nbr >= 60%, PF > 1.3, >= 10 trades) ─────

    // ENGINE R1: ETH-RSI30-H3 — PF=7.96, Sharpe=3.62, 23 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_rsi30_h3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI30-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi30_h3(eth_rsi30_h3_cfg);
    wire_engine(eth_rsi30_h3);

    // ENGINE R2: ETH-RSI30-H4 — PF=10.12, Sharpe=4.00, 12 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_rsi30_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI30-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi30_h4(eth_rsi30_h4_cfg);
    wire_engine(eth_rsi30_h4);

    // ENGINE R3: DOGE-RSI30-H3 — PF=99.9, Sharpe=4.28, 11 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_rsi30_h3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI30-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi30_h3(doge_rsi30_h3_cfg);
    wire_engine(doge_rsi30_h3);

    // ENGINE R4: AVAX-RSI25-H2 — PF=4.45, Sharpe=1.80, 18 trades, Nbr=100%
    chimera::EdgeEngine::Config avax_rsi25_h2_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-RSI25-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 25.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_rsi25_h2(avax_rsi25_h2_cfg);
    wire_engine(avax_rsi25_h2);

    // ENGINE R5: DOGE-RSI25-H2 — PF=3.46, Sharpe=2.51, 23 trades, Nbr=65%
    chimera::EdgeEngine::Config doge_rsi25_h2_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI25-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 25.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi25_h2(doge_rsi25_h2_cfg);
    wire_engine(doge_rsi25_h2);

    // ENGINE R6: BTC-RSI35-H3 — PF=2.17, Sharpe=2.00, 41 trades, Nbr=74%
    chimera::EdgeEngine::Config btc_rsi35_h3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI35-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 35.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi35_h3(btc_rsi35_h3_cfg);
    wire_engine(btc_rsi35_h3);

    // ENGINE B1: BNB-BOLL25-H3 — PF=7.46, Sharpe=2.81, 10 trades, Nbr=66%
    chimera::EdgeEngine::Config bnb_boll25_h3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL25-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 15,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.5,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_boll25_h3(bnb_boll25_h3_cfg);
    wire_engine(bnb_boll25_h3);

    // ── TIER 2 — GOOD edges (Nbr >= 40%, PF > 1.15) ────────────────────

    // ENGINE R7: ETH-BOLL25-H3 — PF=3.00, Sharpe=1.27, 10 trades, Nbr=87%
    chimera::EdgeEngine::Config eth_boll25_h3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL25-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.5,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll25_h3(eth_boll25_h3_cfg);
    wire_engine(eth_boll25_h3);

    // ENGINE R8: BTC-RSI25-H2 — PF=1.56, Sharpe=0.90, 21 trades, Nbr=49%
    chimera::EdgeEngine::Config btc_rsi25_h2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI25-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 25.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi25_h2(btc_rsi25_h2_cfg);
    wire_engine(btc_rsi25_h2);

    // ENGINE R9: LINK-BOLL30-H1 — PF=6.59, Sharpe=1.95, 11 trades, Nbr=50%
    chimera::EdgeEngine::Config link_boll30_h1_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL30-H1",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 3600,
        .lookback       = 30,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 3.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll30_h1(link_boll30_h1_cfg);
    wire_engine(link_boll30_h1);

    // ENGINE R10: XRP-RSI30-H6 — PF=102, Sharpe=3.32, 13 trades, Nbr=40%
    chimera::EdgeEngine::Config xrp_rsi30_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI30-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi30_h6(xrp_rsi30_h6_cfg);
    wire_engine(xrp_rsi30_h6);

    // ENGINE R11: XRP-RSI30-H2 — PF=1.30, Sharpe=0.92, 56 trades (high freq)
    chimera::EdgeEngine::Config xrp_rsi30_h2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI30-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi30_h2(xrp_rsi30_h2_cfg);
    wire_engine(xrp_rsi30_h2);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION H: NEW SYMBOL ENGINES — Session 20 (SUI/APT/NEAR/ARB) ───
    // ══════════════════════════════════════════════════════════════════════
    // Discovered by running optimizer_general on freshly downloaded H1 klines.
    // NEAR is the strongest single symbol ever tested — passes D1 through H3.
    // All use cost=22bp (higher spread on newer/smaller-cap alts).

    // ENGINE H1: NEAR-TSMOM-D1 — PF=2.79, Sharpe=2.61, 46 trades, Nbr=100%
    chimera::EdgeEngine::Config near_d1_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine near_tsmom_d1(near_d1_cfg);
    wire_engine(near_tsmom_d1);

    // ENGINE H2: NEAR-TSMOM-H12 — PF=1.92, Sharpe=3.03, 126 trades, Nbr=95%
    chimera::EdgeEngine::Config near_h12_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h12(near_h12_cfg);
    wire_engine(near_tsmom_h12);

    // ENGINE H3: NEAR-TSMOM-H6 — PF=1.85, Sharpe=3.62, 257 trades, Nbr=100%
    chimera::EdgeEngine::Config near_h6_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h6(near_h6_cfg);
    wire_engine(near_tsmom_h6);

    // ENGINE H4: NEAR-TSMOM-H4 — PF=2.17, Sharpe=3.59, 209 trades, Nbr=100%
    chimera::EdgeEngine::Config near_h4_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h4(near_h4_cfg);
    wire_engine(near_tsmom_h4);

    // ENGINE H5: NEAR-TSMOM-H3 — PF=1.75, Sharpe=3.65, 351 trades, Nbr=87%
    chimera::EdgeEngine::Config near_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h3(near_h3_cfg);
    wire_engine(near_tsmom_h3);

    // ENGINE H6: SUI-TSMOM-H6 — PF=1.80, Sharpe=3.22, 129 trades, Nbr=100%
    chimera::EdgeEngine::Config sui_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h6(sui_h6_cfg);
    wire_engine(sui_tsmom_h6);

    // ENGINE H7: SUI-TSMOM-H4 — PF=1.44, Sharpe=2.11, 169 trades, Nbr=88%
    chimera::EdgeEngine::Config sui_h4_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h4(sui_h4_cfg);
    wire_engine(sui_tsmom_h4);

    // ENGINE H8: APT-TSMOM-H6 — PF=1.82, Sharpe=3.32, 149 trades, Nbr=92%
    chimera::EdgeEngine::Config apt_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_tsmom_h6(apt_h6_cfg);
    wire_engine(apt_tsmom_h6);

    // ENGINE H9: ARB-TSMOM-H6 — PF=1.48, Sharpe=2.31, 131 trades, Nbr=80%
    chimera::EdgeEngine::Config arb_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h6(arb_h6_cfg);
    wire_engine(arb_tsmom_h6);

    // ── SECTION I: COUNTER-TREND ON NEW SYMBOLS — Session 21 (RSI_REVERT + BOLLINGER) ───
//
// 31 new engines: 17 RSI_REVERT + 14 BOLLINGER on NEAR/SUI/APT/ARB
// These symbols show exceptional mean-reversion properties.
//

    // ENGINE S21-1: NEAR-RSI-H6 — PF=3.47, Sharpe=1.64, 11 trades, Nbr=88%
    chimera::EdgeEngine::Config near_rsi_h6_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h6(near_rsi_h6_cfg);
    wire_engine(near_rsi_h6);

    // ENGINE S21-2: NEAR-RSI-H4 — PF=3.24, Sharpe=1.74, 13 trades, Nbr=62%
    chimera::EdgeEngine::Config near_rsi_h4_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h4(near_rsi_h4_cfg);
    wire_engine(near_rsi_h4);

    // ENGINE S21-3: NEAR-RSI-H3 — PF=2.39, Sharpe=1.78, 26 trades, Nbr=47%
    chimera::EdgeEngine::Config near_rsi_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h3(near_rsi_h3_cfg);
    wire_engine(near_rsi_h3);

    // ENGINE S21-4: NEAR-RSI-H2 — PF=2.24, Sharpe=1.08, 14 trades, Nbr=42%
    chimera::EdgeEngine::Config near_rsi_h2_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h2(near_rsi_h2_cfg);
    wire_engine(near_rsi_h2);

    // ENGINE S21-5: SUI-RSI-H3 — PF=5.87, Sharpe=1.89, 11 trades, Nbr=49%
    chimera::EdgeEngine::Config sui_rsi_h3_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h3(sui_rsi_h3_cfg);
    wire_engine(sui_rsi_h3);

    // ENGINE S21-6: SUI-RSI-H2 — PF=2.05, Sharpe=1.16, 13 trades, Nbr=49%
    chimera::EdgeEngine::Config sui_rsi_h2_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h2(sui_rsi_h2_cfg);
    wire_engine(sui_rsi_h2);

    // ENGINE S21-7: SUI-RSI-H4 — PF=1.62, Sharpe=0.86, 17 trades, Nbr=42%
    chimera::EdgeEngine::Config sui_rsi_h4_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h4(sui_rsi_h4_cfg);
    wire_engine(sui_rsi_h4);

    // ENGINE S21-8: APT-RSI-H2 — PF=3.31, Sharpe=1.42, 18 trades, Nbr=99%
    chimera::EdgeEngine::Config apt_rsi_h2_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h2(apt_rsi_h2_cfg);
    wire_engine(apt_rsi_h2);

    // ENGINE S21-9: APT-RSI-H1 — PF=1.81, Sharpe=0.73, 39 trades, Nbr=83%
    chimera::EdgeEngine::Config apt_rsi_h1_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 3600,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h1(apt_rsi_h1_cfg);
    wire_engine(apt_rsi_h1);

    // ENGINE S21-10: APT-RSI-H4 — PF=2.53, Sharpe=1.32, 10 trades, Nbr=83%
    chimera::EdgeEngine::Config apt_rsi_h4_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h4(apt_rsi_h4_cfg);
    wire_engine(apt_rsi_h4);

    // ENGINE S21-11: APT-RSI-H6 — PF=2.27, Sharpe=1.49, 17 trades, Nbr=55%
    chimera::EdgeEngine::Config apt_rsi_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h6(apt_rsi_h6_cfg);
    wire_engine(apt_rsi_h6);

    // ENGINE S21-12: APT-RSI-H3 — PF=2.90, Sharpe=1.61, 12 trades, Nbr=46%
    chimera::EdgeEngine::Config apt_rsi_h3_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h3(apt_rsi_h3_cfg);
    wire_engine(apt_rsi_h3);

    // ENGINE S21-13: ARB-RSI-H2 — PF=4.80, Sharpe=2.66, 13 trades, Nbr=100%
    chimera::EdgeEngine::Config arb_rsi_h2_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h2(arb_rsi_h2_cfg);
    wire_engine(arb_rsi_h2);

    // ENGINE S21-14: ARB-RSI-H4 — PF=2.46, Sharpe=1.54, 14 trades, Nbr=84%
    chimera::EdgeEngine::Config arb_rsi_h4_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h4(arb_rsi_h4_cfg);
    wire_engine(arb_rsi_h4);

    // ENGINE S21-15: ARB-RSI-H1 — PF=1.85, Sharpe=1.35, 34 trades, Nbr=69%
    chimera::EdgeEngine::Config arb_rsi_h1_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 3600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h1(arb_rsi_h1_cfg);
    wire_engine(arb_rsi_h1);

    // ENGINE S21-16: ARB-RSI-H6 — PF=4.89, Sharpe=2.40, 12 trades, Nbr=66%
    chimera::EdgeEngine::Config arb_rsi_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h6(arb_rsi_h6_cfg);
    wire_engine(arb_rsi_h6);

    // ENGINE S21-17: ARB-RSI-H3 — PF=3.71, Sharpe=2.22, 11 trades, Nbr=66%
    chimera::EdgeEngine::Config arb_rsi_h3_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 35,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h3(arb_rsi_h3_cfg);
    wire_engine(arb_rsi_h3);

    // ENGINE S21-18: NEAR-BOLL-H3 — PF=1.80, Sharpe=1.66, 36 trades, Nbr=100%
    chimera::EdgeEngine::Config near_boll_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h3(near_boll_h3_cfg);
    wire_engine(near_boll_h3);

    // ENGINE S21-19: NEAR-BOLL-H6 — PF=3.70, Sharpe=1.32, 14 trades, Nbr=100%
    chimera::EdgeEngine::Config near_boll_h6_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h6(near_boll_h6_cfg);
    wire_engine(near_boll_h6);

    // ENGINE S21-20: NEAR-BOLL-H2 — PF=2.20, Sharpe=2.24, 52 trades, Nbr=95%
    chimera::EdgeEngine::Config near_boll_h2_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h2(near_boll_h2_cfg);
    wire_engine(near_boll_h2);

    // ENGINE S21-21: NEAR-BOLL-H4 — PF=4.16, Sharpe=1.94, 24 trades, Nbr=66%
    chimera::EdgeEngine::Config near_boll_h4_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h4(near_boll_h4_cfg);
    wire_engine(near_boll_h4);

    // ENGINE S21-22: APT-BOLL-H2 — PF=4.55, Sharpe=3.05, 29 trades, Nbr=100%
    chimera::EdgeEngine::Config apt_boll_h2_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h2(apt_boll_h2_cfg);
    wire_engine(apt_boll_h2);

    // ENGINE S21-23: APT-BOLL-H4 — PF=2.70, Sharpe=2.16, 28 trades, Nbr=81%
    chimera::EdgeEngine::Config apt_boll_h4_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h4(apt_boll_h4_cfg);
    wire_engine(apt_boll_h4);

    // ENGINE S21-24: APT-BOLL-H3 — PF=1.37, Sharpe=0.95, 40 trades, Nbr=77%
    chimera::EdgeEngine::Config apt_boll_h3_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h3(apt_boll_h3_cfg);
    wire_engine(apt_boll_h3);

    // ENGINE S21-25: ARB-BOLL-H6 — PF=3.95, Sharpe=2.39, 11 trades, Nbr=71%
    chimera::EdgeEngine::Config arb_boll_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_boll_h6(arb_boll_h6_cfg);
    wire_engine(arb_boll_h6);

    // ENGINE S21-26: ARB-BOLL-H3 — PF=1.44, Sharpe=0.97, 24 trades, Nbr=64%
    chimera::EdgeEngine::Config arb_boll_h3_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_boll_h3(arb_boll_h3_cfg);
    wire_engine(arb_boll_h3);

    // ENGINE S21-27: APT-BOLL-H6 — PF=1.75, Sharpe=0.91, 14 trades, Nbr=54%
    chimera::EdgeEngine::Config apt_boll_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h6(apt_boll_h6_cfg);
    wire_engine(apt_boll_h6);

    // ENGINE S21-28: SUI-BOLL-H6 — PF=6.72, Sharpe=2.96, 14 trades, Nbr=51%
    chimera::EdgeEngine::Config sui_boll_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_boll_h6(sui_boll_h6_cfg);
    wire_engine(sui_boll_h6);

    // ENGINE S21-29: SUI-BOLL-H4 — PF=1.78, Sharpe=1.37, 29 trades, Nbr=48%
    chimera::EdgeEngine::Config sui_boll_h4_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_boll_h4(sui_boll_h4_cfg);
    wire_engine(sui_boll_h4);

    // ENGINE S21-30: ARB-BOLL-H2 — PF=1.27, Sharpe=0.78, 45 trades, Nbr=40%
    chimera::EdgeEngine::Config arb_boll_h2_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_boll_h2(arb_boll_h2_cfg);
    wire_engine(arb_boll_h2);

    // ENGINE S21-31: ARB-BOLL-H4 — PF=2.50, Sharpe=2.00, 16 trades, Nbr=40%
    chimera::EdgeEngine::Config arb_boll_h4_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_boll_h4(arb_boll_h4_cfg);
    wire_engine(arb_boll_h4);

    // ── SECTION J: EXOTIC TFs + EXTENDED COUNTER-TREND — Session 21 ───────
    //
    // 100 new engines from full sweep: H8/H16/D2/D3 TSMOM + H6/H8/H12 counter-trend + DONCHIAN
    //

    // ENGINE S21X-1: BTC-TSMOM-H8 — PF=1.99, Sharpe=2.55, 77 trades, Nbr=82%
    chimera::EdgeEngine::Config btc_tsmom_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h8(btc_tsmom_h8_cfg);
    wire_engine(btc_tsmom_h8);
    // ENGINE S21X-2: ETH-TSMOM-H8 — PF=2.90, Sharpe=5.10, 121 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_tsmom_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h8(eth_tsmom_h8_cfg);
    wire_engine(eth_tsmom_h8);
    // ENGINE S21X-3: SOL-TSMOM-H8 — PF=2.16, Sharpe=3.32, 76 trades, Nbr=70%
    chimera::EdgeEngine::Config sol_tsmom_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h8(sol_tsmom_h8_cfg);
    wire_engine(sol_tsmom_h8);
    // ENGINE S21X-4: XRP-TSMOM-H8 — PF=2.81, Sharpe=2.92, 52 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h8(xrp_tsmom_h8_cfg);
    wire_engine(xrp_tsmom_h8);
    // ENGINE S21X-5: LINK-TSMOM-H8 — PF=2.95, Sharpe=4.78, 119 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h8(link_tsmom_h8_cfg);
    wire_engine(link_tsmom_h8);
    // ENGINE S21X-6: NEAR-TSMOM-H8 — PF=2.10, Sharpe=3.79, 171 trades, Nbr=97%
    chimera::EdgeEngine::Config near_tsmom_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h8(near_tsmom_h8_cfg);
    wire_engine(near_tsmom_h8);
    // ENGINE S21X-7: BNB-TSMOM-H8 — PF=2.86, Sharpe=3.67, 138 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h8(bnb_tsmom_h8_cfg);
    wire_engine(bnb_tsmom_h8);
    // ENGINE S21X-8: DOGE-TSMOM-H8 — PF=2.02, Sharpe=2.54, 107 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_tsmom_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h8(doge_tsmom_h8_cfg);
    wire_engine(doge_tsmom_h8);
    // ENGINE S21X-9: AVAX-TSMOM-H8 — PF=1.90, Sharpe=2.33, 101 trades, Nbr=77%
    chimera::EdgeEngine::Config avax_tsmom_h8_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h8(avax_tsmom_h8_cfg);
    wire_engine(avax_tsmom_h8);
    // ENGINE S21X-10: SUI-TSMOM-H8 — PF=2.27, Sharpe=2.50, 62 trades, Nbr=81%
    chimera::EdgeEngine::Config sui_tsmom_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h8(sui_tsmom_h8_cfg);
    wire_engine(sui_tsmom_h8);
    // ENGINE S21X-11: APT-TSMOM-H8 — PF=2.54, Sharpe=3.45, 89 trades, Nbr=100%
    chimera::EdgeEngine::Config apt_tsmom_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_tsmom_h8(apt_tsmom_h8_cfg);
    wire_engine(apt_tsmom_h8);
    // ENGINE S21X-12: ARB-TSMOM-H8 — PF=2.01, Sharpe=2.84, 86 trades, Nbr=50%
    chimera::EdgeEngine::Config arb_tsmom_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h8(arb_tsmom_h8_cfg);
    wire_engine(arb_tsmom_h8);
    // ENGINE S21X-13: BTC-TSMOM-H16 — PF=5.16, Sharpe=4.01, 22 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h16(btc_tsmom_h16_cfg);
    wire_engine(btc_tsmom_h16);
    // ENGINE S21X-14: ETH-TSMOM-H16 — PF=4.39, Sharpe=2.83, 28 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_tsmom_h16_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h16(eth_tsmom_h16_cfg);
    wire_engine(eth_tsmom_h16);
    // ENGINE S21X-15: SOL-TSMOM-H16 — PF=3.47, Sharpe=3.77, 54 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_tsmom_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h16(sol_tsmom_h16_cfg);
    wire_engine(sol_tsmom_h16);
    // ENGINE S21X-16: XRP-TSMOM-H16 — PF=4.72, Sharpe=4.14, 55 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h16(xrp_tsmom_h16_cfg);
    wire_engine(xrp_tsmom_h16);
    // ENGINE S21X-17: LINK-TSMOM-H16 — PF=3.15, Sharpe=3.17, 39 trades, Nbr=97%
    chimera::EdgeEngine::Config link_tsmom_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h16(link_tsmom_h16_cfg);
    wire_engine(link_tsmom_h16);
    // ENGINE S21X-18: NEAR-TSMOM-H16 — PF=2.03, Sharpe=1.65, 29 trades, Nbr=45%
    chimera::EdgeEngine::Config near_tsmom_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h16(near_tsmom_h16_cfg);
    wire_engine(near_tsmom_h16);
    // ENGINE S21X-19: BNB-TSMOM-H16 — PF=2.76, Sharpe=2.70, 61 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h16(bnb_tsmom_h16_cfg);
    wire_engine(bnb_tsmom_h16);
    // ENGINE S21X-20: DOGE-TSMOM-H16 — PF=2.16, Sharpe=2.33, 54 trades, Nbr=92%
    chimera::EdgeEngine::Config doge_tsmom_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h16(doge_tsmom_h16_cfg);
    wire_engine(doge_tsmom_h16);
    // ENGINE S21X-21: AVAX-TSMOM-H16 — PF=2.74, Sharpe=2.40, 24 trades, Nbr=59%
    chimera::EdgeEngine::Config avax_tsmom_h16_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h16(avax_tsmom_h16_cfg);
    wire_engine(avax_tsmom_h16);
    // ENGINE S21X-22: SUI-TSMOM-H16 — PF=2.13, Sharpe=2.16, 40 trades, Nbr=85%
    chimera::EdgeEngine::Config sui_tsmom_h16_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h16(sui_tsmom_h16_cfg);
    wire_engine(sui_tsmom_h16);
    // ENGINE S21X-23: APT-TSMOM-H16 — PF=1.67, Sharpe=1.65, 49 trades, Nbr=64%
    chimera::EdgeEngine::Config apt_tsmom_h16_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_tsmom_h16(apt_tsmom_h16_cfg);
    wire_engine(apt_tsmom_h16);
    // ENGINE S21X-24: ARB-TSMOM-H16 — PF=2.33, Sharpe=2.84, 43 trades, Nbr=40%
    chimera::EdgeEngine::Config arb_tsmom_h16_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h16(arb_tsmom_h16_cfg);
    wire_engine(arb_tsmom_h16);
    // ENGINE S21X-25: BTC-TSMOM-D2 — PF=38.30, Sharpe=7.17, 10 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_d2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d2(btc_tsmom_d2_cfg);
    wire_engine(btc_tsmom_d2);
    // ENGINE S21X-26: ETH-TSMOM-D2 — PF=5.99, Sharpe=2.48, 10 trades, Nbr=88%
    chimera::EdgeEngine::Config eth_tsmom_d2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d2(eth_tsmom_d2_cfg);
    wire_engine(eth_tsmom_d2);
    // ENGINE S21X-27: SOL-TSMOM-D2 — PF=5.97, Sharpe=3.30, 14 trades, Nbr=82%
    chimera::EdgeEngine::Config sol_tsmom_d2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sol_tsmom_d2(sol_tsmom_d2_cfg);
    wire_engine(sol_tsmom_d2);
    // ENGINE S21X-28: XRP-TSMOM-D2 — PF=26.86, Sharpe=3.35, 22 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_d2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 10,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_tsmom_d2(xrp_tsmom_d2_cfg);
    wire_engine(xrp_tsmom_d2);
    // ENGINE S21X-29: LINK-TSMOM-D2 — PF=28.76, Sharpe=3.56, 14 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_d2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d2(link_tsmom_d2_cfg);
    wire_engine(link_tsmom_d2);
    // ENGINE S21X-30: NEAR-TSMOM-D2 — PF=4.09, Sharpe=2.62, 13 trades, Nbr=53%
    chimera::EdgeEngine::Config near_tsmom_d2_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine near_tsmom_d2(near_tsmom_d2_cfg);
    wire_engine(near_tsmom_d2);
    // ENGINE S21X-31: BNB-TSMOM-D2 — PF=13.09, Sharpe=2.78, 31 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_d2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 40,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d2(bnb_tsmom_d2_cfg);
    wire_engine(bnb_tsmom_d2);
    // ENGINE S21X-32: DOGE-TSMOM-D2 — PF=4.99, Sharpe=3.51, 20 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_tsmom_d2_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 40,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_tsmom_d2(doge_tsmom_d2_cfg);
    wire_engine(doge_tsmom_d2);
    // ENGINE S21X-33: AVAX-TSMOM-D2 — PF=4.48, Sharpe=3.04, 19 trades, Nbr=93%
    chimera::EdgeEngine::Config avax_tsmom_d2_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine avax_tsmom_d2(avax_tsmom_d2_cfg);
    wire_engine(avax_tsmom_d2);
    // ENGINE S21X-34: SUI-TSMOM-D2 — PF=3.84, Sharpe=2.10, 13 trades, Nbr=91%
    chimera::EdgeEngine::Config sui_tsmom_d2_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sui_tsmom_d2(sui_tsmom_d2_cfg);
    wire_engine(sui_tsmom_d2);
    // ENGINE S21X-35: ARB-TSMOM-D2 — PF=1.30, Sharpe=0.46, 13 trades, Nbr=54%
    chimera::EdgeEngine::Config arb_tsmom_d2_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine arb_tsmom_d2(arb_tsmom_d2_cfg);
    wire_engine(arb_tsmom_d2);
    // ENGINE S21X-36: BTC-TSMOM-D3 — PF=242.75, Sharpe=6.40, 15 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_d3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d3(btc_tsmom_d3_cfg);
    wire_engine(btc_tsmom_d3);
    // ENGINE S21X-37: ETH-TSMOM-D3 — PF=7.40, Sharpe=2.70, 11 trades, Nbr=89%
    chimera::EdgeEngine::Config eth_tsmom_d3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 25,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d3(eth_tsmom_d3_cfg);
    wire_engine(eth_tsmom_d3);
    // ENGINE S21X-38: SOL-TSMOM-D3 — PF=2.69, Sharpe=1.67, 15 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_tsmom_d3_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sol_tsmom_d3(sol_tsmom_d3_cfg);
    wire_engine(sol_tsmom_d3);
    // ENGINE S21X-39: XRP-TSMOM-D3 — PF=45.74, Sharpe=3.90, 11 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_d3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_tsmom_d3(xrp_tsmom_d3_cfg);
    wire_engine(xrp_tsmom_d3);
    // ENGINE S21X-40: LINK-TSMOM-D3 — PF=10.45, Sharpe=3.74, 10 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_d3_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d3(link_tsmom_d3_cfg);
    wire_engine(link_tsmom_d3);
    // ENGINE S21X-41: NEAR-TSMOM-D3 — DISABLED (FAILED re-validation: OOS PF=0.86, edge not confirmed)
    // chimera::EdgeEngine::Config near_tsmom_d3_cfg{...};
    // chimera::EdgeEngine near_tsmom_d3(near_tsmom_d3_cfg);
    // ENGINE S21X-42: BNB-TSMOM-D3 — PF=34.14, Sharpe=2.67, 13 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_d3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 30,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d3(bnb_tsmom_d3_cfg);
    wire_engine(bnb_tsmom_d3);
    // ENGINE S21X-43: DOGE-TSMOM-D3 — PF=3.72, Sharpe=2.05, 13 trades, Nbr=57%
    chimera::EdgeEngine::Config doge_tsmom_d3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_tsmom_d3(doge_tsmom_d3_cfg);
    wire_engine(doge_tsmom_d3);
    // ENGINE S21X-44: AVAX-TSMOM-D3 — PF=2.51, Sharpe=1.34, 11 trades, Nbr=58%
    chimera::EdgeEngine::Config avax_tsmom_d3_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine avax_tsmom_d3(avax_tsmom_d3_cfg);
    wire_engine(avax_tsmom_d3);
    // ENGINE S21X-45: SUI-TSMOM-D3 — PF=2.38, Sharpe=1.43, 10 trades, Nbr=56%
    chimera::EdgeEngine::Config sui_tsmom_d3_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 8,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sui_tsmom_d3(sui_tsmom_d3_cfg);
    wire_engine(sui_tsmom_d3);
    // ENGINE S21X-46: ARB-TSMOM-D3 — PF=1.50, Sharpe=0.83, 14 trades, Nbr=67%
    chimera::EdgeEngine::Config arb_tsmom_d3_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine arb_tsmom_d3(arb_tsmom_d3_cfg);
    wire_engine(arb_tsmom_d3);
    // ENGINE S21X-47: BTC-RSI-H6 — PF=3.27, Sharpe=2.09, 24 trades, Nbr=52%
    chimera::EdgeEngine::Config btc_rsi_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi_h6(btc_rsi_h6_cfg);
    wire_engine(btc_rsi_h6);
    // ENGINE S21X-48: ETH-RSI-H6 — PF=41.75, Sharpe=3.43, 12 trades, Nbr=97%
    chimera::EdgeEngine::Config eth_rsi_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h6(eth_rsi_h6_cfg);
    wire_engine(eth_rsi_h6);
    // ENGINE S21X-49: SOL-RSI-H6 — PF=3.64, Sharpe=1.83, 10 trades, Nbr=80%
    // ENGINE S21X-50: LINK-RSI-H6 — PF=8.79, Sharpe=3.20, 12 trades, Nbr=95%
    chimera::EdgeEngine::Config link_rsi_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_rsi_h6(link_rsi_h6_cfg);
    wire_engine(link_rsi_h6);
    // ENGINE S21X-51: BNB-RSI-H6 — DISABLED (FAILED re-validation: OOS PF=1.08, original 373.9 was mirage)
    // chimera::EdgeEngine::Config bnb_rsi_h6_cfg{...};
    // chimera::EdgeEngine bnb_rsi_h6(bnb_rsi_h6_cfg);
    // ENGINE S21X-52: DOGE-RSI-H6 — PF=3.72, Sharpe=1.96, 16 trades, Nbr=67%
    chimera::EdgeEngine::Config doge_rsi_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h6(doge_rsi_h6_cfg);
    wire_engine(doge_rsi_h6);
    // ENGINE S21X-53: AVAX-RSI-H6 — PF=1.77, Sharpe=1.15, 19 trades, Nbr=40%
    chimera::EdgeEngine::Config avax_rsi_h6_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_rsi_h6(avax_rsi_h6_cfg);
    wire_engine(avax_rsi_h6);
    // ENGINE S21X-54: BTC-BOLL-H6 — PF=8.04, Sharpe=3.24, 18 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_boll_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h6(btc_boll_h6_cfg);
    wire_engine(btc_boll_h6);
    // ENGINE S21X-55: ETH-BOLL-H6 — PF=4.87, Sharpe=2.38, 10 trades, Nbr=99%
    chimera::EdgeEngine::Config eth_boll_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll_h6(eth_boll_h6_cfg);
    wire_engine(eth_boll_h6);
    // ENGINE S21X-56: SOL-BOLL-H6 — PF=5.77, Sharpe=3.23, 14 trades, Nbr=84%
    chimera::EdgeEngine::Config sol_boll_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h6(sol_boll_h6_cfg);
    wire_engine(sol_boll_h6);
    // ENGINE S21X-57: XRP-BOLL-H6 — PF=2.16, Sharpe=1.15, 16 trades, Nbr=70%
    chimera::EdgeEngine::Config xrp_boll_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h6(xrp_boll_h6_cfg);
    wire_engine(xrp_boll_h6);
    // ENGINE S21X-58: LINK-BOLL-H6 — PF=99.90, Sharpe=6.18, 14 trades, Nbr=100%
    chimera::EdgeEngine::Config link_boll_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h6(link_boll_h6_cfg);
    wire_engine(link_boll_h6);
    // ENGINE S21X-59: BNB-BOLL-H6 — PF=4.06, Sharpe=2.05, 17 trades, Nbr=49%
    chimera::EdgeEngine::Config bnb_boll_h6_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_boll_h6(bnb_boll_h6_cfg);
    wire_engine(bnb_boll_h6);
    // ENGINE S21X-60: DOGE-BOLL-H6 — PF=99.90, Sharpe=4.78, 15 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_boll_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_boll_h6(doge_boll_h6_cfg);
    wire_engine(doge_boll_h6);
    // ENGINE S21X-61: AVAX-BOLL-H6 — PF=2.51, Sharpe=1.88, 20 trades, Nbr=87%
    chimera::EdgeEngine::Config avax_boll_h6_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 5,
        .hold_bars      = 12,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_boll_h6(avax_boll_h6_cfg);
    wire_engine(avax_boll_h6);
    // ENGINE S21X-62: BTC-RSI-H12 — PF=3.57, Sharpe=2.51, 12 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_rsi_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi_h12(btc_rsi_h12_cfg);
    wire_engine(btc_rsi_h12);
    // ENGINE S21X-63: ETH-RSI-H12 — PF=1.55, Sharpe=0.65, 11 trades, Nbr=44%
    chimera::EdgeEngine::Config eth_rsi_h12_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h12(eth_rsi_h12_cfg);
    wire_engine(eth_rsi_h12);
    // ENGINE S21X-64: SOL-RSI-H12 — PF=11.81, Sharpe=2.33, 10 trades, Nbr=63%
    chimera::EdgeEngine::Config sol_rsi_h12_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_rsi_h12(sol_rsi_h12_cfg);
    wire_engine(sol_rsi_h12);
    // ENGINE S21X-65: XRP-RSI-H12 — DISABLED (FAILED re-validation: OOS PF=1.09, recent PF=0.89, no edge)
    // chimera::EdgeEngine::Config xrp_rsi_h12_cfg{...};
    // chimera::EdgeEngine xrp_rsi_h12(xrp_rsi_h12_cfg);
    // ENGINE S21X-66: LINK-RSI-H12 — PF=15.83, Sharpe=5.99, 12 trades, Nbr=100%
    chimera::EdgeEngine::Config link_rsi_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_rsi_h12(link_rsi_h12_cfg);
    wire_engine(link_rsi_h12);
    // ENGINE S21X-67: DOGE-RSI-H12 — PF=2.00, Sharpe=1.03, 14 trades, Nbr=40%
    chimera::EdgeEngine::Config doge_rsi_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h12(doge_rsi_h12_cfg);
    wire_engine(doge_rsi_h12);
    // ENGINE S21X-68: AVAX-RSI-H12 — PF=1.68, Sharpe=0.65, 10 trades, Nbr=61%
    chimera::EdgeEngine::Config avax_rsi_h12_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 25,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_rsi_h12(avax_rsi_h12_cfg);
    wire_engine(avax_rsi_h12);
    // ENGINE S21X-69: BTC-BOLL-H12 — PF=6.45, Sharpe=2.64, 10 trades, Nbr=97%
    chimera::EdgeEngine::Config btc_boll_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h12(btc_boll_h12_cfg);
    wire_engine(btc_boll_h12);
    // ENGINE S21X-70: SOL-BOLL-H12 — PF=3.55, Sharpe=1.48, 10 trades, Nbr=66%
    chimera::EdgeEngine::Config sol_boll_h12_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h12(sol_boll_h12_cfg);
    wire_engine(sol_boll_h12);
    // ENGINE S21X-71: XRP-BOLL-H12 — PF=54.35, Sharpe=2.52, 10 trades, Nbr=96%
    chimera::EdgeEngine::Config xrp_boll_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h12(xrp_boll_h12_cfg);
    wire_engine(xrp_boll_h12);
    // ENGINE S21X-72: LINK-BOLL-H12 — PF=2.37, Sharpe=1.32, 10 trades, Nbr=92%
    chimera::EdgeEngine::Config link_boll_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 40,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h12(link_boll_h12_cfg);
    wire_engine(link_boll_h12);
    // ENGINE S21X-73: DOGE-BOLL-H12 — PF=5.08, Sharpe=1.89, 11 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_boll_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_boll_h12(doge_boll_h12_cfg);
    wire_engine(doge_boll_h12);
    // ENGINE S21X-74: BTC-RSI-H8 — PF=1.79, Sharpe=0.87, 16 trades, Nbr=69%
    chimera::EdgeEngine::Config btc_rsi_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi_h8(btc_rsi_h8_cfg);
    wire_engine(btc_rsi_h8);
    // ENGINE S21X-75: ETH-RSI-H8 — PF=99.90, Sharpe=3.99, 13 trades, Nbr=66%
    // ENGINE S21X-76: XRP-RSI-H8 — PF=10.46, Sharpe=2.69, 12 trades, Nbr=100%
    // ENGINE S21X-77: LINK-RSI-H8 — PF=4.82, Sharpe=2.15, 14 trades, Nbr=67%
    // ENGINE S21X-78: BNB-RSI-H8 — PF=1.95, Sharpe=1.78, 31 trades, Nbr=96%
    // ENGINE S21X-79: NEAR-RSI-H8 — PF=2.09, Sharpe=1.35, 27 trades, Nbr=81%
    // ENGINE S21X-80: APT-RSI-H8 — PF=3.03, Sharpe=1.55, 11 trades, Nbr=68%
    // ENGINE S21X-81: ARB-RSI-H8 — PF=1.56, Sharpe=0.70, 14 trades, Nbr=53%
    // ENGINE S21X-82: BTC-BOLL-H8 — PF=7.73, Sharpe=2.59, 10 trades, Nbr=70%
    // ENGINE S21X-83: ETH-BOLL-H8 — PF=2.91, Sharpe=1.93, 11 trades, Nbr=95%
    // ENGINE S21X-84: XRP-BOLL-H8 — PF=14.69, Sharpe=3.04, 16 trades, Nbr=91%
    // ENGINE S21X-85: LINK-BOLL-H8 — PF=16.22, Sharpe=3.65, 13 trades, Nbr=100%
    // ENGINE S21X-86: BNB-BOLL-H8 — PF=5.10, Sharpe=2.25, 12 trades, Nbr=45%
    // ENGINE S21X-87: NEAR-BOLL-H8 — PF=2.57, Sharpe=1.56, 14 trades, Nbr=69%
    // ENGINE S21X-88: APT-BOLL-H8 — PF=1.96, Sharpe=0.92, 11 trades, Nbr=61%
    // ENGINE S21X-89: ARB-BOLL-H8 — PF=2.53, Sharpe=1.56, 12 trades, Nbr=64%
    // ENGINE S21X-90: NEAR-DONCH-H6 — PF=1.37, Sharpe=0.80, 45 trades, Nbr=70%
    chimera::EdgeEngine::Config near_donch_h6_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h6(near_donch_h6_cfg);
    wire_engine(near_donch_h6);
    // ENGINE S21X-91: NEAR-DONCH-H8 — PF=2.99, Sharpe=2.46, 40 trades, Nbr=76%
    // ENGINE S21X-92: SUI-DONCH-H6 — PF=1.83, Sharpe=1.23, 15 trades, Nbr=46%
    chimera::EdgeEngine::Config sui_donch_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest showed only +1-2% CAGR lift (avg 0.04 adds/trade) — trail
        // tightens before profit reaches arm threshold. SUI exits too fast to extend.
    };
    chimera::EdgeEngine sui_donch_h6(sui_donch_h6_cfg);
    wire_engine(sui_donch_h6);
    // ENGINE S21X-93: SUI-DONCH-H8 — PF=5.02, Sharpe=2.18, 11 trades, Nbr=100%
    // ENGINE S21X-94: APT-DONCH-H4 — PF=2.26, Sharpe=1.91, 26 trades, Nbr=81%
    chimera::EdgeEngine::Config apt_donch_h4_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-DONCH-H4",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_donch_h4(apt_donch_h4_cfg);
    wire_engine(apt_donch_h4);
    // ENGINE S21X-95: APT-DONCH-H6 — PF=1.73, Sharpe=1.22, 23 trades, Nbr=45%
    chimera::EdgeEngine::Config apt_donch_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_donch_h6(apt_donch_h6_cfg);
    wire_engine(apt_donch_h6);
    // ENGINE S21X-96: ARB-DONCH-H6 — PF=1.28, Sharpe=0.47, 13 trades, Nbr=40%
    chimera::EdgeEngine::Config arb_donch_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_donch_h6(arb_donch_h6_cfg);
    wire_engine(arb_donch_h6);
    // ENGINE S21X-97: ARB-DONCH-H8 — PF=2.00, Sharpe=1.22, 10 trades, Nbr=51%
    // ENGINE S21X-98: BTC-RSI-D1 — PF=1.33, Sharpe=0.55, 9 trades, Nbr=55%
    chimera::EdgeEngine::Config btc_rsi_d1_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-D1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 86400,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_rsi_d1(btc_rsi_d1_cfg);
    wire_engine(btc_rsi_d1);
    // ENGINE S21X-99: DOGE-RSI-D1 — PF=1.36, Sharpe=0.60, 10 trades, Nbr=60%
    chimera::EdgeEngine::Config doge_rsi_d1_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-D1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 86400,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_rsi_d1(doge_rsi_d1_cfg);
    wire_engine(doge_rsi_d1);
    // ENGINE S21X-100: LINK-RSI-D1 — PF=1.44, Sharpe=0.43, 12 trades, Nbr=43%
    chimera::EdgeEngine::Config link_rsi_d1_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-D1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 86400,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_rsi_d1(link_rsi_d1_cfg);
    wire_engine(link_rsi_d1);

    // ══════════════════════════════════════════════════════════════════════
    // SESSION 22 — Extended-data validated engines (57 new)
    // RSI_REVERT H8, BOLLINGER H8, RSI_REVERT H16, BOLLINGER H16,
    // DONCHIAN H8/H16/D2/D3
    // ══════════════════════════════════════════════════════════════════════

    chimera::EdgeEngine::Config eth_rsi_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 5,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h8(eth_rsi_h8_cfg);
    wire_engine(eth_rsi_h8);

    chimera::EdgeEngine::Config bnb_rsi_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_rsi_h8(bnb_rsi_h8_cfg);
    wire_engine(bnb_rsi_h8);

    chimera::EdgeEngine::Config doge_rsi_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h8(doge_rsi_h8_cfg);
    wire_engine(doge_rsi_h8);

    chimera::EdgeEngine::Config xrp_rsi_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi_h8(xrp_rsi_h8_cfg);
    wire_engine(xrp_rsi_h8);

    chimera::EdgeEngine::Config apt_rsi_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h8(apt_rsi_h8_cfg);
    wire_engine(apt_rsi_h8);

    chimera::EdgeEngine::Config sol_rsi_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_rsi_h8(sol_rsi_h8_cfg);
    wire_engine(sol_rsi_h8);

    chimera::EdgeEngine::Config link_rsi_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_rsi_h8(link_rsi_h8_cfg);
    wire_engine(link_rsi_h8);

    chimera::EdgeEngine::Config arb_rsi_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h8(arb_rsi_h8_cfg);
    wire_engine(arb_rsi_h8);

    chimera::EdgeEngine::Config near_rsi_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H8",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h8(near_rsi_h8_cfg);
    wire_engine(near_rsi_h8);

    chimera::EdgeEngine::Config btc_boll_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h8(btc_boll_h8_cfg);
    wire_engine(btc_boll_h8);

    chimera::EdgeEngine::Config eth_boll_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll_h8(eth_boll_h8_cfg);
    wire_engine(eth_boll_h8);

    chimera::EdgeEngine::Config sol_boll_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h8(sol_boll_h8_cfg);
    wire_engine(sol_boll_h8);

    chimera::EdgeEngine::Config bnb_boll_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_boll_h8(bnb_boll_h8_cfg);
    wire_engine(bnb_boll_h8);

    chimera::EdgeEngine::Config avax_boll_h8_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_boll_h8(avax_boll_h8_cfg);
    wire_engine(avax_boll_h8);

    chimera::EdgeEngine::Config link_boll_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h8(link_boll_h8_cfg);
    wire_engine(link_boll_h8);

    chimera::EdgeEngine::Config xrp_boll_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h8(xrp_boll_h8_cfg);
    wire_engine(xrp_boll_h8);

    chimera::EdgeEngine::Config doge_boll_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_boll_h8(doge_boll_h8_cfg);
    wire_engine(doge_boll_h8);

    chimera::EdgeEngine::Config sui_boll_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_boll_h8(sui_boll_h8_cfg);
    wire_engine(sui_boll_h8);

    chimera::EdgeEngine::Config apt_boll_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h8(apt_boll_h8_cfg);
    wire_engine(apt_boll_h8);

    chimera::EdgeEngine::Config near_boll_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h8(near_boll_h8_cfg);
    wire_engine(near_boll_h8);

    chimera::EdgeEngine::Config arb_boll_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_boll_h8(arb_boll_h8_cfg);
    wire_engine(arb_boll_h8);

    // ETH-RSI-H16 — DISABLED (FAILED re-validation: OOS PF=0.49, recent PF=0.24, actively losing)
    // chimera::EdgeEngine::Config eth_rsi_h16_cfg{...};
    // chimera::EdgeEngine eth_rsi_h16(eth_rsi_h16_cfg);

    chimera::EdgeEngine::Config bnb_rsi_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_rsi_h16(bnb_rsi_h16_cfg);
    wire_engine(bnb_rsi_h16);

    chimera::EdgeEngine::Config xrp_rsi_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi_h16(xrp_rsi_h16_cfg);
    wire_engine(xrp_rsi_h16);

    chimera::EdgeEngine::Config link_rsi_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_rsi_h16(link_rsi_h16_cfg);
    wire_engine(link_rsi_h16);

    chimera::EdgeEngine::Config near_rsi_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h16(near_rsi_h16_cfg);
    wire_engine(near_rsi_h16);

    chimera::EdgeEngine::Config btc_rsi_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_rsi_h16(btc_rsi_h16_cfg);
    wire_engine(btc_rsi_h16);

    chimera::EdgeEngine::Config sol_rsi_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_rsi_h16(sol_rsi_h16_cfg);
    wire_engine(sol_rsi_h16);

    chimera::EdgeEngine::Config doge_rsi_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H16",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h16(doge_rsi_h16_cfg);
    wire_engine(doge_rsi_h16);

    chimera::EdgeEngine::Config link_boll_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h16(link_boll_h16_cfg);
    wire_engine(link_boll_h16);

    chimera::EdgeEngine::Config xrp_boll_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h16(xrp_boll_h16_cfg);
    wire_engine(xrp_boll_h16);

    chimera::EdgeEngine::Config btc_boll_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h16(btc_boll_h16_cfg);
    wire_engine(btc_boll_h16);

    chimera::EdgeEngine::Config near_boll_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h16(near_boll_h16_cfg);
    wire_engine(near_boll_h16);

    chimera::EdgeEngine::Config eth_boll_h16_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll_h16(eth_boll_h16_cfg);
    wire_engine(eth_boll_h16);

    chimera::EdgeEngine::Config xrp_donch_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h8(xrp_donch_h8_cfg);
    wire_engine(xrp_donch_h8);

    chimera::EdgeEngine::Config near_donch_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h8(near_donch_h8_cfg);
    wire_engine(near_donch_h8);

    chimera::EdgeEngine::Config sui_donch_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_donch_h8(sui_donch_h8_cfg);
    wire_engine(sui_donch_h8);

    chimera::EdgeEngine::Config btc_donch_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_donch_h8(btc_donch_h8_cfg);
    wire_engine(btc_donch_h8);

    chimera::EdgeEngine::Config arb_donch_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_donch_h8(arb_donch_h8_cfg);
    wire_engine(arb_donch_h8);

    chimera::EdgeEngine::Config xrp_donch_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h16(xrp_donch_h16_cfg);
    wire_engine(xrp_donch_h16);

    chimera::EdgeEngine::Config bnb_donch_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest 2021-2026 showed +3% CAGR lift (avg 0.27 adds/trade),
        // not worth the operational complexity. Trail tightens at 1.5 ATR before adds trigger.
    };
    chimera::EdgeEngine bnb_donch_h16(bnb_donch_h16_cfg);
    wire_engine(bnb_donch_h16);

    chimera::EdgeEngine::Config btc_donch_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest showed +7% CAGR (avg 0.25 adds/trade) — below 10% lift threshold.
    };
    chimera::EdgeEngine btc_donch_h16(btc_donch_h16_cfg);
    wire_engine(btc_donch_h16);

    chimera::EdgeEngine::Config link_donch_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_donch_h16(link_donch_h16_cfg);
    wire_engine(link_donch_h16);

    chimera::EdgeEngine::Config sui_donch_h16_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_donch_h16(sui_donch_h16_cfg);
    wire_engine(sui_donch_h16);

    chimera::EdgeEngine::Config near_donch_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h16(near_donch_h16_cfg);
    wire_engine(near_donch_h16);

    chimera::EdgeEngine::Config sol_donch_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_donch_h16(sol_donch_h16_cfg);
    wire_engine(sol_donch_h16);

    chimera::EdgeEngine::Config doge_donch_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 30,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_donch_h16(doge_donch_h16_cfg);
    wire_engine(doge_donch_h16);

    chimera::EdgeEngine::Config bnb_donch_d2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_donch_d2(bnb_donch_d2_cfg);
    wire_engine(bnb_donch_d2);

    chimera::EdgeEngine::Config xrp_donch_d2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_donch_d2(xrp_donch_d2_cfg);
    wire_engine(xrp_donch_d2);

    chimera::EdgeEngine::Config btc_donch_d2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_donch_d2(btc_donch_d2_cfg);
    wire_engine(btc_donch_d2);

    chimera::EdgeEngine::Config eth_donch_d2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_donch_d2(eth_donch_d2_cfg);
    wire_engine(eth_donch_d2);

    chimera::EdgeEngine::Config link_donch_d2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_donch_d2(link_donch_d2_cfg);
    wire_engine(link_donch_d2);

    chimera::EdgeEngine::Config btc_donch_d3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 10,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_donch_d3(btc_donch_d3_cfg);
    wire_engine(btc_donch_d3);

    chimera::EdgeEngine::Config eth_donch_d3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_donch_d3(eth_donch_d3_cfg);
    wire_engine(eth_donch_d3);

    chimera::EdgeEngine::Config xrp_donch_d3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_donch_d3(xrp_donch_d3_cfg);
    wire_engine(xrp_donch_d3);

    chimera::EdgeEngine::Config bnb_donch_d3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_donch_d3(bnb_donch_d3_cfg);
    wire_engine(bnb_donch_d3);

    chimera::EdgeEngine::Config doge_donch_d3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_donch_d3(doge_donch_d3_cfg);
    wire_engine(doge_donch_d3);

    // ══════════════════════════════════════════════════════════════════════
    // ── SESSION 24: DONCHIAN gap-fill (H2/H3/H4/H6/H12) + TSMOM H12 ───
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S24-1: XRP-DONCH-H2 — PF=1.68, Sharpe=1.69, 125 trades, Nbr=98%
    chimera::EdgeEngine::Config xrp_donch_h2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h2(xrp_donch_h2_cfg);
    wire_engine(xrp_donch_h2);

    // ENGINE S24-2: NEAR-DONCH-H2 — PF=1.48, Sharpe=1.55, 90 trades, Nbr=61%
    chimera::EdgeEngine::Config near_donch_h2_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h2(near_donch_h2_cfg);
    wire_engine(near_donch_h2);

    // ENGINE S24-3: XRP-DONCH-H3 — PF=1.61, Sharpe=1.48, 121 trades, Nbr=99%
    chimera::EdgeEngine::Config xrp_donch_h3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 10800,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h3(xrp_donch_h3_cfg);
    wire_engine(xrp_donch_h3);

    // ENGINE S24-4: NEAR-DONCH-H3 — PF=1.64, Sharpe=1.70, 85 trades, Nbr=88%
    chimera::EdgeEngine::Config near_donch_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h3(near_donch_h3_cfg);
    wire_engine(near_donch_h3);

    // ENGINE S24-5: XRP-DONCH-H4 — PF=1.72, Sharpe=1.38, 71 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_donch_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H4",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h4(xrp_donch_h4_cfg);
    wire_engine(xrp_donch_h4);

    // ENGINE S24-6: XRP-DONCH-H6 — PF=2.63, Sharpe=2.25, 45 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_donch_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h6(xrp_donch_h6_cfg);
    wire_engine(xrp_donch_h6);

    // ENGINE S24-7: BNB-DONCH-H6 — PF=2.08, Sharpe=1.84, 31 trades, Nbr=64%
    chimera::EdgeEngine::Config bnb_donch_h6_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_donch_h6(bnb_donch_h6_cfg);
    wire_engine(bnb_donch_h6);

    // ENGINE S24-8: XRP-DONCH-H12 — PF=3.40, Sharpe=2.09, 26 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_donch_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H12",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h12(xrp_donch_h12_cfg);
    wire_engine(xrp_donch_h12);

    // ENGINE S24-9: NEAR-DONCH-H12 — PF=2.25, Sharpe=1.43, 21 trades, Nbr=65%
    chimera::EdgeEngine::Config near_donch_h12_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H12",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h12(near_donch_h12_cfg);
    wire_engine(near_donch_h12);

    // ENGINE S24-10: ETH-TSMOM-H12 — PF=1.61, Sharpe=1.44, 100 trades, Nbr=94%
    chimera::EdgeEngine::Config eth_tsmom_h12_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h12(eth_tsmom_h12_cfg);
    wire_engine(eth_tsmom_h12);

    // ENGINE S24-11: SOL-TSMOM-H12 — PF=1.91, Sharpe=2.30, 120 trades, Nbr=86%
    chimera::EdgeEngine::Config sol_tsmom_h12_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h12(sol_tsmom_h12_cfg);
    wire_engine(sol_tsmom_h12);

    // ENGINE S24-12: BNB-TSMOM-H12 — PF=2.45, Sharpe=3.08, 96 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h12_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h12(bnb_tsmom_h12_cfg);
    wire_engine(bnb_tsmom_h12);

    // ENGINE S24-13: LINK-TSMOM-H12 — PF=1.62, Sharpe=2.00, 187 trades, Nbr=90%
    chimera::EdgeEngine::Config link_tsmom_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h12(link_tsmom_h12_cfg);
    wire_engine(link_tsmom_h12);

    // ENGINE S24-14: XRP-TSMOM-H12 — PF=1.54, Sharpe=1.52, 153 trades, Nbr=73%
    chimera::EdgeEngine::Config xrp_tsmom_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h12(xrp_tsmom_h12_cfg);
    wire_engine(xrp_tsmom_h12);

    // ENGINE S24-15: APT-TSMOM-H12 — PF=2.32, Sharpe=3.54, 81 trades, Nbr=89%
    chimera::EdgeEngine::Config apt_tsmom_h12_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 25,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_tsmom_h12(apt_tsmom_h12_cfg);
    wire_engine(apt_tsmom_h12);

    // ── Session 26 — RSI_REVERT H4 + BOLLINGER H4/H2 new edges ──────────

    // ENGINE S26-1: ETH-RSI-H4 — PF=1.82, Sharpe=1.45, 38 trades, Nbr=72%
    chimera::EdgeEngine::Config eth_rsi_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h4(eth_rsi_h4_cfg);
    wire_engine(eth_rsi_h4);

    // ENGINE S26-2: DOGE-RSI-H4 — PF=1.67, Sharpe=1.31, 42 trades, Nbr=68%
    chimera::EdgeEngine::Config doge_rsi_h4_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h4(doge_rsi_h4_cfg);
    wire_engine(doge_rsi_h4);

    // ENGINE S26-3: XRP-RSI-H4 — PF=1.74, Sharpe=1.38, 45 trades, Nbr=70%
    chimera::EdgeEngine::Config xrp_rsi_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi_h4(xrp_rsi_h4_cfg);
    wire_engine(xrp_rsi_h4);

    // ENGINE S26-4: SOL-RSI-H4 — PF=1.58, Sharpe=1.22, 35 trades, Nbr=65%
    chimera::EdgeEngine::Config sol_rsi_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_rsi_h4(sol_rsi_h4_cfg);
    wire_engine(sol_rsi_h4);

    // ENGINE S26-5: AVAX-BOLL-H4 — PF=1.71, Sharpe=1.35, 28 trades, Nbr=66%
    chimera::EdgeEngine::Config avax_boll_h4_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_boll_h4(avax_boll_h4_cfg);
    wire_engine(avax_boll_h4);

    // ENGINE S26-6: LINK-BOLL-H4 — PF=1.65, Sharpe=1.28, 31 trades, Nbr=63%
    chimera::EdgeEngine::Config link_boll_h4_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h4(link_boll_h4_cfg);
    wire_engine(link_boll_h4);

    // ENGINE S26-7: SOL-BOLL-H4 — PF=1.69, Sharpe=1.33, 26 trades, Nbr=67%
    chimera::EdgeEngine::Config sol_boll_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h4(sol_boll_h4_cfg);
    wire_engine(sol_boll_h4);

    // ENGINE S26-8: SOL-BOLL-H2 — PF=1.53, Sharpe=1.19, 52 trades, Nbr=61%
    chimera::EdgeEngine::Config sol_boll_h2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h2(sol_boll_h2_cfg);
    wire_engine(sol_boll_h2);

    // ENGINE S26-9: BTC-BOLL-H4 — PF=1.61, Sharpe=1.25, 33 trades, Nbr=64%
    chimera::EdgeEngine::Config btc_boll_h4_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h4(btc_boll_h4_cfg);
    wire_engine(btc_boll_h4);

    // ENGINE S26-10: XRP-BOLL-H4 — PF=1.56, Sharpe=1.21, 29 trades, Nbr=62%
    chimera::EdgeEngine::Config xrp_boll_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 25,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h4(xrp_boll_h4_cfg);
    wire_engine(xrp_boll_h4);

    // ENGINE S27-1: SUI-RSI-H12 — PF=1.61, Sharpe=1.12, 21 trades, Nbr=96%
    // Walk-forward: IS PF=0.94, OOS PF=1.66, Stability=100%
    chimera::EdgeEngine::Config sui_rsi_h12_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 8,
        .hold_bars      = 6,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h12(sui_rsi_h12_cfg);
    wire_engine(sui_rsi_h12);

    // ENGINE S27-2: ETH-BOLL-H12 — PF=45.90, Sharpe=3.14, 12 trades, Nbr=100%
    // Walk-forward: IS PF=0.56, OOS PF=48.01, Stability=100%
    chimera::EdgeEngine::Config eth_boll_h12_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll_h12(eth_boll_h12_cfg);
    wire_engine(eth_boll_h12);


    // ══════════════════════════════════════════════════════════════════════
    // SESSION 28 — KELTNER_REVERT + DUAL_THRUST engines (8 engines)
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S28-1: DOGE-KELTNER-H6 — PF=5.24, Sharpe=2.43, 15 trades, Nbr=95%
    // Walk-forward: IS PF=0.87, OOS PF=5.24, Stability=95%
    chimera::EdgeEngine::Config doge_keltner_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-KELTNER-H6",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .keltner_ema_len  = 40,
        .keltner_atr_mult = 3.0,
    };
    chimera::EdgeEngine doge_keltner_h6(doge_keltner_h6_cfg);
    wire_engine(doge_keltner_h6);

    // ENGINE S28-2: LINK-KELTNER-H12 — PF=6.85, Sharpe=2.21, 11 trades, Nbr=66%
    // Walk-forward: IS PF=1.07, OOS PF=6.85, Stability=66%
    chimera::EdgeEngine::Config link_keltner_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-KELTNER-H12",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .keltner_ema_len  = 30,
        .keltner_atr_mult = 2.5,
    };
    chimera::EdgeEngine link_keltner_h12(link_keltner_h12_cfg);
    wire_engine(link_keltner_h12);

    // ENGINE S28-3: DOGE-KELTNER-H8 — PF=4.38, Sharpe=2.45, 18 trades, Nbr=64%
    // Walk-forward: IS PF=0.97, OOS PF=4.38, Stability=64%
    chimera::EdgeEngine::Config doge_keltner_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-KELTNER-H8",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .keltner_ema_len  = 40,
        .keltner_atr_mult = 2.5,
    };
    chimera::EdgeEngine doge_keltner_h8(doge_keltner_h8_cfg);
    wire_engine(doge_keltner_h8);

    // ENGINE S28-4: BTC-KELTNER-H12 — PF=3.03, Sharpe=1.69, 30 trades, Nbr=58%
    // Walk-forward: IS PF=1.03, OOS PF=3.03, Stability=58%
    chimera::EdgeEngine::Config btc_keltner_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-KELTNER-H12",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .keltner_ema_len  = 30,
        .keltner_atr_mult = 1.5,
    };
    chimera::EdgeEngine btc_keltner_h12(btc_keltner_h12_cfg);
    wire_engine(btc_keltner_h12);

    // ENGINE S28-5: SUI-KELTNER-H12 — PF=4.82, Sharpe=1.99, 16 trades, Nbr=40%
    // Walk-forward: IS PF=0.96, OOS PF=4.82, Stability=40%
    chimera::EdgeEngine::Config sui_keltner_h12_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-KELTNER-H12",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 43200,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .keltner_ema_len  = 25,
        .keltner_atr_mult = 1.5,
    };
    chimera::EdgeEngine sui_keltner_h12(sui_keltner_h12_cfg);
    wire_engine(sui_keltner_h12);

    // ENGINE S28-6: APT-KELTNER-H8 — PF=3.21, Sharpe=1.40, 11 trades, Nbr=46%
    // Walk-forward: IS PF=3.60, OOS PF=3.21, Stability=46%
    chimera::EdgeEngine::Config apt_keltner_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-KELTNER-H8",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .keltner_ema_len  = 25,
        .keltner_atr_mult = 2.5,
    };
    chimera::EdgeEngine apt_keltner_h8(apt_keltner_h8_cfg);
    wire_engine(apt_keltner_h8);

    // ENGINE S28-7: SOL-DT-H12 — PF=3.08, Sharpe=2.42, 29 trades, Nbr=56%
    // Walk-forward: IS PF=1.72, OOS PF=3.01, Stability=56%
    chimera::EdgeEngine::Config sol_dt_h12_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-DT-H12",
        .kind           = chimera::StrategyKind::DUAL_THRUST,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .dt_k1          = 0.7,
        .dt_range_bars  = 5,
    };
    chimera::EdgeEngine sol_dt_h12(sol_dt_h12_cfg);
    wire_engine(sol_dt_h12);

    // ENGINE S28-8: XRP-DT-H8 — PF=1.71, Sharpe=1.38, 101 trades, Nbr=64%
    // Walk-forward: IS PF=0.90, OOS PF=1.70, Stability=64%
    chimera::EdgeEngine::Config xrp_dt_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DT-H8",
        .kind           = chimera::StrategyKind::DUAL_THRUST,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .dt_k1          = 0.3,
        .dt_range_bars  = 10,
    };
    chimera::EdgeEngine xrp_dt_h8(xrp_dt_h8_cfg);
    wire_engine(xrp_dt_h8);


    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION K: ICHIMOKU ENGINES (Session 29) ────────────────────────
    // Cloud breakout + Tenkan/Kijun cross. Trend-following, complementary
    // to TSMOM (different signal timing — waits for cloud confirmation).
    // Params tuned for crypto: Tenkan=20, Kijun=60, SenkouB=120
    // (standard 9/26/52 is too fast for crypto's noise).
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S29-1: BTC-ICHI-H6 — Ichimoku cloud breakout
    chimera::EdgeEngine::Config btc_ichi_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-ICHI-H6",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine btc_ichi_h6(btc_ichi_h6_cfg);
    wire_engine(btc_ichi_h6);

    // ENGINE S29-2: ETH-ICHI-H6
    chimera::EdgeEngine::Config eth_ichi_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-ICHI-H6",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine eth_ichi_h6(eth_ichi_h6_cfg);
    wire_engine(eth_ichi_h6);

    // ENGINE S29-3: SOL-ICHI-H6
    chimera::EdgeEngine::Config sol_ichi_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-ICHI-H6",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine sol_ichi_h6(sol_ichi_h6_cfg);
    wire_engine(sol_ichi_h6);

    // ENGINE S29-4: XRP-ICHI-H6
    chimera::EdgeEngine::Config xrp_ichi_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-ICHI-H6",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine xrp_ichi_h6(xrp_ichi_h6_cfg);
    wire_engine(xrp_ichi_h6);

    // ENGINE S29-5: LINK-ICHI-H12
    chimera::EdgeEngine::Config link_ichi_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-ICHI-H12",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine link_ichi_h12(link_ichi_h12_cfg);
    wire_engine(link_ichi_h12);

    // ENGINE S29-6: DOGE-ICHI-H12
    chimera::EdgeEngine::Config doge_ichi_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-ICHI-H12",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine doge_ichi_h12(doge_ichi_h12_cfg);
    wire_engine(doge_ichi_h12);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION L: SUPERTREND ENGINES (Session 29) ──────────────────────
    // ATR-based trailing trend indicator. Enters on flip from bearish to
    // bullish. Very popular in crypto — different signal timing than TSMOM
    // (requires actual price/ATR flip vs simple lookback momentum).
    // Params: multiplier=3.0, ATR period=10 (standard crypto config).
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S29-7: BTC-ST-H6
    chimera::EdgeEngine::Config btc_st_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-ST-H6",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine btc_st_h6(btc_st_h6_cfg);
    wire_engine(btc_st_h6);

    // ENGINE S29-8: ETH-ST-H6
    chimera::EdgeEngine::Config eth_st_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-ST-H6",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine eth_st_h6(eth_st_h6_cfg);
    wire_engine(eth_st_h6);

    // ENGINE S29-9: SOL-ST-H6
    chimera::EdgeEngine::Config sol_st_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-ST-H6",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine sol_st_h6(sol_st_h6_cfg);
    wire_engine(sol_st_h6);

    // ENGINE S29-10: XRP-ST-H4
    chimera::EdgeEngine::Config xrp_st_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-ST-H4",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 14400,
        .lookback       = 20,
        .hold_bars      = 18,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine xrp_st_h4(xrp_st_h4_cfg);
    wire_engine(xrp_st_h4);

    // ENGINE S29-11: LINK-ST-H8
    chimera::EdgeEngine::Config link_st_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-ST-H8",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 14,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine link_st_h8(link_st_h8_cfg);
    wire_engine(link_st_h8);

    // ENGINE S29-12: DOGE-ST-H8
    chimera::EdgeEngine::Config doge_st_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-ST-H8",
        .kind           = chimera::StrategyKind::SUPERTREND,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 14,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .st_multiplier  = 3.0,
        .st_atr_period  = 10,
    };
    chimera::EdgeEngine doge_st_h8(doge_st_h8_cfg);
    wire_engine(doge_st_h8);


    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION M: WILLIAMS %R ENGINES (Session 29b) ────────────────────
    // Mean-reversion using Williams %R oscillator. Different normalization
    // than RSI — uses (HH-Close)/(HH-LL) which is more responsive to
    // recent price extremes. Fires at different times than RSI_REVERT.
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S29b-1: ETH-WR-H4
    chimera::EdgeEngine::Config eth_wr_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-WR-H4",
        .kind           = chimera::StrategyKind::WILLIAMS_R,
        .tf_secs        = 14400,
        .lookback       = 14,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .willr_period   = 14,
        .willr_threshold = -80.0,
    };
    chimera::EdgeEngine eth_wr_h4(eth_wr_h4_cfg);
    wire_engine(eth_wr_h4);

    // ENGINE S29b-2: SOL-WR-H4
    chimera::EdgeEngine::Config sol_wr_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-WR-H4",
        .kind           = chimera::StrategyKind::WILLIAMS_R,
        .tf_secs        = 14400,
        .lookback       = 14,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .willr_period   = 14,
        .willr_threshold = -80.0,
    };
    chimera::EdgeEngine sol_wr_h4(sol_wr_h4_cfg);
    wire_engine(sol_wr_h4);

    // ENGINE S29b-3: DOGE-WR-H6
    chimera::EdgeEngine::Config doge_wr_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-WR-H6",
        .kind           = chimera::StrategyKind::WILLIAMS_R,
        .tf_secs        = 21600,
        .lookback       = 14,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .willr_period   = 14,
        .willr_threshold = -80.0,
    };
    chimera::EdgeEngine doge_wr_h6(doge_wr_h6_cfg);
    wire_engine(doge_wr_h6);

    // ENGINE S29b-4: XRP-WR-H6
    chimera::EdgeEngine::Config xrp_wr_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-WR-H6",
        .kind           = chimera::StrategyKind::WILLIAMS_R,
        .tf_secs        = 21600,
        .lookback       = 14,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .willr_period   = 14,
        .willr_threshold = -80.0,
    };
    chimera::EdgeEngine xrp_wr_h6(xrp_wr_h6_cfg);
    wire_engine(xrp_wr_h6);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION N: STOCHASTIC RSI ENGINES (Session 29b) ─────────────────
    // Stochastic RSI = RSI normalized within its own range. Faster than
    // raw RSI — catches reversals sooner. Ideal for timing mean-reversion
    // entries when RSI is oscillating in a narrow band (StochRSI breaks out
    // of 0/100 more readily than RSI does from 30/70).
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S29b-5: ETH-SRSI-H4
    chimera::EdgeEngine::Config eth_srsi_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-SRSI-H4",
        .kind           = chimera::StrategyKind::STOCH_RSI,
        .tf_secs        = 14400,
        .lookback       = 14,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .stochrsi_rsi_period   = 14,
        .stochrsi_stoch_period = 14,
        .stochrsi_threshold    = 20.0,
    };
    chimera::EdgeEngine eth_srsi_h4(eth_srsi_h4_cfg);
    wire_engine(eth_srsi_h4);

    // ENGINE S29b-6: SOL-SRSI-H4
    chimera::EdgeEngine::Config sol_srsi_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-SRSI-H4",
        .kind           = chimera::StrategyKind::STOCH_RSI,
        .tf_secs        = 14400,
        .lookback       = 14,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .stochrsi_rsi_period   = 14,
        .stochrsi_stoch_period = 14,
        .stochrsi_threshold    = 20.0,
    };
    chimera::EdgeEngine sol_srsi_h4(sol_srsi_h4_cfg);
    wire_engine(sol_srsi_h4);

    // ENGINE S29b-7: DOGE-SRSI-H6
    chimera::EdgeEngine::Config doge_srsi_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-SRSI-H6",
        .kind           = chimera::StrategyKind::STOCH_RSI,
        .tf_secs        = 21600,
        .lookback       = 14,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .stochrsi_rsi_period   = 14,
        .stochrsi_stoch_period = 14,
        .stochrsi_threshold    = 20.0,
    };
    chimera::EdgeEngine doge_srsi_h6(doge_srsi_h6_cfg);
    wire_engine(doge_srsi_h6);

    // ENGINE S29b-8: LINK-SRSI-H6
    chimera::EdgeEngine::Config link_srsi_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-SRSI-H6",
        .kind           = chimera::StrategyKind::STOCH_RSI,
        .tf_secs        = 21600,
        .lookback       = 14,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .stochrsi_rsi_period   = 14,
        .stochrsi_stoch_period = 14,
        .stochrsi_threshold    = 20.0,
    };
    chimera::EdgeEngine link_srsi_h6(link_srsi_h6_cfg);
    wire_engine(link_srsi_h6);


    // ══════════════════════════════════════════════════════════════════════
    // SESSION 30 — W1 Mean-Reversion Engines (Edge 6)
    // Weekly timeframe RSI/BOLL on BTC + ETH. Few trades but huge PF on
    // big drawdown reversals. tf_secs=604800 (7 days).
    // Parameters: conservative oversold levels for weekly — fewer but higher quality
    // ══════════════════════════════════════════════════════════════════════

    // BTC W1 RSI_REVERT — weekly oversold reversals
    chimera::EdgeEngine::Config btc_rsi_w1_cfg;
    btc_rsi_w1_cfg.symbol       = "btcusdt";
    btc_rsi_w1_cfg.tag          = "BTC-RSI-W1";
    btc_rsi_w1_cfg.kind         = chimera::StrategyKind::RSI_REVERT;
    btc_rsi_w1_cfg.tf_secs      = 604800;  // W1
    btc_rsi_w1_cfg.lookback     = 14;
    btc_rsi_w1_cfg.hold_bars    = 4;       // hold 4 weeks
    btc_rsi_w1_cfg.sl_atr_mult  = 3.0;    // wider stop for weekly
    btc_rsi_w1_cfg.atr_period   = 14;
    btc_rsi_w1_cfg.rsi_threshold = 25.0;   // very oversold on weekly = strong reversal
    btc_rsi_w1_cfg.trail_arm_atr = 1.5;
    btc_rsi_w1_cfg.trail_dist_atr = 0.8;
    btc_rsi_w1_cfg.max_history  = 128;     // need more bars for weekly ATR
    chimera::EdgeEngine btc_rsi_w1(btc_rsi_w1_cfg);
    wire_engine(btc_rsi_w1);

    // ETH W1 RSI_REVERT
    chimera::EdgeEngine::Config eth_rsi_w1_cfg;
    eth_rsi_w1_cfg.symbol       = "ethusdt";
    eth_rsi_w1_cfg.tag          = "ETH-RSI-W1";
    eth_rsi_w1_cfg.kind         = chimera::StrategyKind::RSI_REVERT;
    eth_rsi_w1_cfg.tf_secs      = 604800;
    eth_rsi_w1_cfg.lookback     = 14;
    eth_rsi_w1_cfg.hold_bars    = 4;
    eth_rsi_w1_cfg.sl_atr_mult  = 3.0;
    eth_rsi_w1_cfg.atr_period   = 14;
    eth_rsi_w1_cfg.rsi_threshold = 25.0;
    eth_rsi_w1_cfg.trail_arm_atr = 1.5;
    eth_rsi_w1_cfg.trail_dist_atr = 0.8;
    eth_rsi_w1_cfg.max_history  = 128;
    chimera::EdgeEngine eth_rsi_w1(eth_rsi_w1_cfg);
    wire_engine(eth_rsi_w1);

    // BTC W1 BOLLINGER — weekly lower-band touch reversals
    chimera::EdgeEngine::Config btc_boll_w1_cfg;
    btc_boll_w1_cfg.symbol       = "btcusdt";
    btc_boll_w1_cfg.tag          = "BTC-BOLL-W1";
    btc_boll_w1_cfg.kind         = chimera::StrategyKind::BOLLINGER;
    btc_boll_w1_cfg.tf_secs      = 604800;
    btc_boll_w1_cfg.lookback     = 20;     // 20-week Bollinger
    btc_boll_w1_cfg.hold_bars    = 3;      // hold 3 weeks
    btc_boll_w1_cfg.sl_atr_mult  = 3.5;
    btc_boll_w1_cfg.atr_period   = 14;
    btc_boll_w1_cfg.bb_k         = 2.5;    // wider bands for weekly (more conservative)
    btc_boll_w1_cfg.trail_arm_atr = 2.0;
    btc_boll_w1_cfg.trail_dist_atr = 1.0;
    btc_boll_w1_cfg.max_history  = 128;
    chimera::EdgeEngine btc_boll_w1(btc_boll_w1_cfg);
    wire_engine(btc_boll_w1);

    // ETH W1 BOLLINGER
    chimera::EdgeEngine::Config eth_boll_w1_cfg;
    eth_boll_w1_cfg.symbol       = "ethusdt";
    eth_boll_w1_cfg.tag          = "ETH-BOLL-W1";
    eth_boll_w1_cfg.kind         = chimera::StrategyKind::BOLLINGER;
    eth_boll_w1_cfg.tf_secs      = 604800;
    eth_boll_w1_cfg.lookback     = 20;
    eth_boll_w1_cfg.hold_bars    = 3;
    eth_boll_w1_cfg.sl_atr_mult  = 3.5;
    eth_boll_w1_cfg.atr_period   = 14;
    eth_boll_w1_cfg.bb_k         = 2.5;
    eth_boll_w1_cfg.trail_arm_atr = 2.0;
    eth_boll_w1_cfg.trail_dist_atr = 1.0;
    eth_boll_w1_cfg.max_history  = 128;
    chimera::EdgeEngine eth_boll_w1(eth_boll_w1_cfg);
    wire_engine(eth_boll_w1);

    // ══════════════════════════════════════════════════════════════════════
    // DISABLED ENGINES — No OOS edge after costs (Sessions 13-15)
    // ══════════════════════════════════════════════════════════════════════
    // ETH-BB-H6 (PF=0.72), SOL-DONCH-H6 (PF=0.83), XRP-DONCH-H1 (PF=0.82),
    // LINK-RSI-H6 (PF=1.17/4trades), BTC-OVERNIGHT-H1 (PF=0.31),
    // BTC-WEEKDAY-D1 (PF=0.44), DOGE-TSMOM-H4 (Nbr=49%),
    // BTC-TSMOM-H1 (PF=1.17/Nbr=76%), ETH-TSMOM-H1 (PF=1.13/Nbr=59%),
    // BNB-TSMOM-H1 (PF=1.11/Nbr=16%), DOGE-TSMOM-H1 (PF=1.11/Nbr=48%),
    // AVAX-TSMOM-H1 (PF=1.03/Nbr=8%)
    //
    // DISABLED ENGINES — Failed re-validation (Session 29c, 2026-05-17)
    // ──────────────────────────────────────────────────────────────────────
    // BNB-RSI-H6   (Orig PF=373.9 → OOS PF=1.08, 6 trades, no edge — inflated by 1 lucky window)
    // ETH-RSI-H16  (Orig PF=158.2 → OOS PF=0.49, 6 trades, recent PF=0.24 — actively losing)
    // NEAR-TSMOM-D3 (Orig PF=99.9 → OOS PF=0.86, 12 trades — edge not confirmed in extended OOS)
    // XRP-RSI-H12  (Orig PF=63.2 → OOS PF=1.09, 7 trades, recent PF=0.89 — breakeven, fading)

    // ══════════════════════════════════════════════════════════════════════
    // Register all active engines with backtest metadata
    // ══════════════════════════════════════════════════════════════════════

    // D1 engines (5)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_d1,   "btcusdt",  86400, "BTC-TSMOM-D1",   1.92, 1.67,  85,  24, 13});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_d1,   "ethusdt",  86400, "ETH-TSMOM-D1",   3.15, 3.17,  91,  26, 13});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_d1,   "solusdt",  86400, "SOL-TSMOM-D1",   2.25, 2.41,  89,  15, 13});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d1,  "linkusdt", 86400, "LINK-TSMOM-D1",  2.18, 1.92, 100,  23, 13});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_d1,   "bnbusdt",  86400, "BNB-TSMOM-D1",   3.16, 2.91,  90,  32, 14});

    // H12 engines (3)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h12,  "btcusdt",  43200, "BTC-TSMOM-H12",  3.63, 3.40,  96,  31, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h12, "dogeusdt", 43200, "DOGE-TSMOM-H12", 2.78, 3.66, 100,  82, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h12, "avaxusdt", 43200, "AVAX-TSMOM-H12", 2.61, 2.98,  87,  76, 14});

    // H6 engines (8) — NEW Session 15
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h6,   "xrpusdt",  21600, "XRP-TSMOM-H6",   2.68, 4.41, 100, 120, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h6,    "btcusdt",  21600, "BTC-TSMOM-H6",   2.59, 5.16, 100, 169, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h6,    "ethusdt",  21600, "ETH-TSMOM-H6",   2.07, 3.70, 100, 151, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h6,    "solusdt",  21600, "SOL-TSMOM-H6",   2.07, 3.25, 100, 127, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h6,    "bnbusdt",  21600, "BNB-TSMOM-H6",   2.07, 2.76, 100,  95, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h6,   "linkusdt", 21600, "LINK-TSMOM-H6",  2.07, 3.13, 100,  81, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h6,   "dogeusdt", 21600, "DOGE-TSMOM-H6",  1.72, 2.24,  77,  91, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h6,   "avaxusdt", 21600, "AVAX-TSMOM-H6",  1.37, 1.82,  67, 207, 15});

    // H4 engines (7)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h4,   "xrpusdt",  14400, "XRP-TSMOM-H4",   2.43, 5.80, 100, 267, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h4,    "bnbusdt",  14400, "BNB-TSMOM-H4",   1.91, 3.79, 100, 291, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h4,   "linkusdt", 14400, "LINK-TSMOM-H4",  1.91, 4.07,  95, 205, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h4,    "solusdt",  14400, "SOL-TSMOM-H4",   1.89, 3.82, 100, 208, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h4,    "btcusdt",  14400, "BTC-TSMOM-H4",   1.82, 3.54, 100, 167, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h4,    "ethusdt",  14400, "ETH-TSMOM-H4",   1.76, 3.26, 100, 196, 14});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h4,   "avaxusdt", 14400, "AVAX-TSMOM-H4",  1.47, 2.17,  83, 231, 14});

    // H1 engines (3) — Session 15
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h1,   "xrpusdt",   3600, "XRP-TSMOM-H1",   1.66, 3.73, 100, 327, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h1,    "solusdt",   3600, "SOL-TSMOM-H1",   1.40, 3.31, 100, 527, 15});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h1,   "linkusdt",  3600, "LINK-TSMOM-H1",  1.32, 3.08,  95, 798, 15});

    // H2 engines (5) — NEW Session 17
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h2,   "btcusdt",   7200, "BTC-TSMOM-H2",   1.99, 4.98, 100, 281, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h2,   "ethusdt",   7200, "ETH-TSMOM-H2",   1.50, 3.02, 100, 359, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h2,   "solusdt",   7200, "SOL-TSMOM-H2",   1.78, 4.17, 100, 340, 17});
    g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h2,   "xrpusdt",   7200, "XRP-TSMOM-H2",   2.00, 4.70, 100, 320, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h2,  "linkusdt",  7200, "LINK-TSMOM-H2",  1.69, 3.76, 100, 357, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h2,  "dogeusdt",  7200, "DOGE-TSMOM-H2",  1.21, 1.72,  93, 387, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h2,   "bnbusdt",   7200, "BNB-TSMOM-H2",   1.19, 1.12,  87, 436, 20});

    // H3 engines (6) — NEW Session 17 (no native Binance candles — cold-start from ticks)
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h3,   "btcusdt",  10800, "BTC-TSMOM-H3",   1.96, 3.52, 100, 156, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h3,   "ethusdt",  10800, "ETH-TSMOM-H3",   1.74, 3.65,  98, 278, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h3,   "solusdt",  10800, "SOL-TSMOM-H3",   1.92, 4.15,  93, 259, 17});
    g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h3,   "xrpusdt",  10800, "XRP-TSMOM-H3",   2.19, 4.70, 100, 243, 17});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h3,  "linkusdt", 10800, "LINK-TSMOM-H3",  1.94, 4.19, 100, 254, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h3,   "bnbusdt",  10800, "BNB-TSMOM-H3",   1.55, 2.74,  97, 349, 17});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h3,  "dogeusdt", 10800, "DOGE-TSMOM-H3",  1.25, 1.48,  87, 309, 20});

    // ── COUNTER-TREND engines (RSI_REVERT + BOLLINGER dip-buy) ─── Session 19 ──
    // TIER 1 — strong OOS edge + high neighbourhood stability
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi30_h3,   "ethusdt",  10800, "ETH-RSI30-H3",   2.41, 2.18,  92,  87, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi30_h4,   "ethusdt",  14400, "ETH-RSI30-H4",   2.13, 1.95,  88,  62, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi30_h3,  "dogeusdt", 10800, "DOGE-RSI30-H3",  1.98, 1.82,  85,  94, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_rsi25_h2,  "avaxusdt",  7200, "AVAX-RSI25-H2",  2.27, 2.05,  90, 143, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi25_h2,  "dogeusdt",  7200, "DOGE-RSI25-H2",  1.85, 1.71,  83, 118, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_rsi35_h3,   "btcusdt",  10800, "BTC-RSI35-H3",   1.92, 1.78,  87,  76, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_boll25_h3,  "bnbusdt",  10800, "BNB-BOLL25-H3",  2.08, 1.93,  86,  68, 19});
    // TIER 2 — moderate OOS edge, still deploying for shadow observation
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_boll25_h3,  "ethusdt",  10800, "ETH-BOLL25-H3",  1.74, 1.62,  81,  55, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_rsi25_h2,   "btcusdt",   7200, "BTC-RSI25-H2",   1.68, 1.55,  79, 102, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll30_h1,  "linkusdt",  3600, "LINK-BOLL30-H1", 1.59, 1.48,  77, 134, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_rsi30_h6,   "xrpusdt",  21600, "XRP-RSI30-H6",   1.82, 1.69,  84,  48, 19});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_rsi30_h2,   "xrpusdt",   7200, "XRP-RSI30-H2",   1.71, 1.58,  80, 112, 19});

    // ── NEW SYMBOL engines (Session 20) — NEAR/SUI/APT/ARB ─────────────────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d1,  "nearusdt", 86400, "NEAR-TSMOM-D1",  2.79, 2.61, 100,  46, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h12, "nearusdt", 43200, "NEAR-TSMOM-H12", 1.92, 3.03,  95, 126, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h6,  "nearusdt", 21600, "NEAR-TSMOM-H6",  1.85, 3.62, 100, 257, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h4,  "nearusdt", 14400, "NEAR-TSMOM-H4",  2.17, 3.59, 100, 209, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h3,  "nearusdt", 10800, "NEAR-TSMOM-H3",  1.75, 3.65,  87, 351, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h6,   "suiusdt",  21600, "SUI-TSMOM-H6",   1.80, 3.22, 100, 129, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h4,   "suiusdt",  14400, "SUI-TSMOM-H4",   1.44, 2.11,  88, 169, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT,  &apt_tsmom_h6,   "aptusdt",  21600, "APT-TSMOM-H6",   1.82, 3.32,  92, 149, 20});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB,  &arb_tsmom_h6,   "arbusdt",  21600, "ARB-TSMOM-H6",   1.48, 2.31,  80, 131, 20});

// ── Counter-trend on new symbols (Session 21) — NEAR/SUI/APT/ARB ────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h6, "nearusdt", 21600, "NEAR-RSI-H6", 3.47, 1.64, 88, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h4, "nearusdt", 14400, "NEAR-RSI-H4", 3.24, 1.74, 62, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h3, "nearusdt", 10800, "NEAR-RSI-H3", 2.39, 1.78, 47, 26, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h2, "nearusdt", 7200, "NEAR-RSI-H2", 2.24, 1.08, 42, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h3, "suiusdt", 10800, "SUI-RSI-H3", 5.87, 1.89, 49, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h2, "suiusdt", 7200, "SUI-RSI-H2", 2.05, 1.16, 49, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h4, "suiusdt", 14400, "SUI-RSI-H4", 1.62, 0.86, 42, 17, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h2, "aptusdt", 7200, "APT-RSI-H2", 3.31, 1.42, 99, 18, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h1, "aptusdt", 3600, "APT-RSI-H1", 1.81, 0.73, 83, 39, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h4, "aptusdt", 14400, "APT-RSI-H4", 2.53, 1.32, 83, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h6, "aptusdt", 21600, "APT-RSI-H6", 2.27, 1.49, 55, 17, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h3, "aptusdt", 10800, "APT-RSI-H3", 2.90, 1.61, 46, 12, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h2, "arbusdt", 7200, "ARB-RSI-H2", 4.80, 2.66, 100, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h4, "arbusdt", 14400, "ARB-RSI-H4", 2.46, 1.54, 84, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h1, "arbusdt", 3600, "ARB-RSI-H1", 1.85, 1.35, 69, 34, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h6, "arbusdt", 21600, "ARB-RSI-H6", 4.89, 2.40, 66, 12, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h3, "arbusdt", 10800, "ARB-RSI-H3", 3.71, 2.22, 66, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h3, "nearusdt", 10800, "NEAR-BOLL-H3", 1.80, 1.66, 100, 36, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h6, "nearusdt", 21600, "NEAR-BOLL-H6", 3.70, 1.32, 100, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h2, "nearusdt", 7200, "NEAR-BOLL-H2", 2.20, 2.24, 95, 52, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h4, "nearusdt", 14400, "NEAR-BOLL-H4", 4.16, 1.94, 66, 24, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_boll_h2, "aptusdt", 7200, "APT-BOLL-H2", 4.55, 3.05, 100, 29, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_boll_h4, "aptusdt", 14400, "APT-BOLL-H4", 2.70, 2.16, 81, 28, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_boll_h3, "aptusdt", 10800, "APT-BOLL-H3", 1.37, 0.95, 77, 40, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_boll_h6, "arbusdt", 21600, "ARB-BOLL-H6", 3.95, 2.39, 71, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_boll_h3, "arbusdt", 10800, "ARB-BOLL-H3", 1.44, 0.97, 64, 24, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_boll_h6, "aptusdt", 21600, "APT-BOLL-H6", 1.75, 0.91, 54, 14, 21});
    g_slots.push_back({chimera::SYM_SUI, &sui_boll_h6, "suiusdt", 21600, "SUI-BOLL-H6", 1.54, 1.05, 86, 146, 31});  // tier-A re-validated 5yr Binance
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_boll_h4, "suiusdt", 14400, "SUI-BOLL-H4", 1.78, 1.37, 48, 29, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_boll_h2, "arbusdt", 7200, "ARB-BOLL-H2", 1.27, 0.78, 40, 45, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_boll_h4, "arbusdt", 14400, "ARB-BOLL-H4", 2.50, 2.00, 40, 16, 21});

    // ── Exotic TFs + extended counter-trend (Session 21) — 100 engines ────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_h8, "btcusdt", 28800, "BTC-TSMOM-H8", 1.99, 2.55, 82, 77, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_h8, "ethusdt", 28800, "ETH-TSMOM-H8", 2.90, 5.10, 100, 121, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_h8, "solusdt", 28800, "SOL-TSMOM-H8", 2.16, 3.32, 70, 76, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_h8, "xrpusdt", 28800, "XRP-TSMOM-H8", 2.81, 2.92, 100, 52, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h8, "linkusdt", 28800, "LINK-TSMOM-H8", 2.95, 4.78, 100, 119, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h8, "nearusdt", 28800, "NEAR-TSMOM-H8", 2.10, 3.79, 97, 171, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h8, "bnbusdt", 28800, "BNB-TSMOM-H8", 2.86, 3.67, 100, 138, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h8, "dogeusdt", 28800, "DOGE-TSMOM-H8", 2.02, 2.54, 100, 107, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h8, "avaxusdt", 28800, "AVAX-TSMOM-H8", 1.90, 2.33, 77, 101, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_h8, "suiusdt", 28800, "SUI-TSMOM-H8", 2.27, 2.50, 81, 62, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_tsmom_h8, "aptusdt", 28800, "APT-TSMOM-H8", 2.54, 3.45, 100, 89, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_h8, "arbusdt", 28800, "ARB-TSMOM-H8", 2.01, 2.84, 50, 86, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_h16, "btcusdt", 57600, "BTC-TSMOM-H16", 5.16, 4.01, 100, 22, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_h16, "ethusdt", 57600, "ETH-TSMOM-H16", 4.39, 2.83, 100, 28, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_h16, "solusdt", 57600, "SOL-TSMOM-H16", 3.47, 3.77, 100, 54, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_h16, "xrpusdt", 57600, "XRP-TSMOM-H16", 4.72, 4.14, 100, 55, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h16, "linkusdt", 57600, "LINK-TSMOM-H16", 3.15, 3.17, 97, 39, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h16, "nearusdt", 57600, "NEAR-TSMOM-H16", 2.03, 1.65, 45, 29, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h16, "bnbusdt", 57600, "BNB-TSMOM-H16", 2.76, 2.70, 100, 61, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h16, "dogeusdt", 57600, "DOGE-TSMOM-H16", 2.16, 2.33, 92, 54, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h16, "avaxusdt", 57600, "AVAX-TSMOM-H16", 2.74, 2.40, 59, 24, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_h16, "suiusdt", 57600, "SUI-TSMOM-H16", 2.13, 2.16, 85, 40, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_tsmom_h16, "aptusdt", 57600, "APT-TSMOM-H16", 1.67, 1.65, 64, 49, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_h16, "arbusdt", 57600, "ARB-TSMOM-H16", 2.33, 2.84, 40, 43, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_d2, "btcusdt", 172800, "BTC-TSMOM-D2", 38.30, 7.17, 100, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_d2, "ethusdt", 172800, "ETH-TSMOM-D2", 5.99, 2.48, 88, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_d2, "solusdt", 172800, "SOL-TSMOM-D2", 5.97, 3.30, 82, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_d2, "xrpusdt", 172800, "XRP-TSMOM-D2", 26.86, 3.35, 100, 22, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d2, "linkusdt", 172800, "LINK-TSMOM-D2", 28.76, 3.56, 100, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d2, "nearusdt", 172800, "NEAR-TSMOM-D2", 4.09, 2.62, 53, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_d2, "bnbusdt", 172800, "BNB-TSMOM-D2", 13.09, 2.78, 100, 31, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_d2, "dogeusdt", 172800, "DOGE-TSMOM-D2", 4.99, 3.51, 100, 20, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_d2, "avaxusdt", 172800, "AVAX-TSMOM-D2", 4.48, 3.04, 93, 19, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_d2, "suiusdt", 172800, "SUI-TSMOM-D2", 3.84, 2.10, 91, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_d2, "arbusdt", 172800, "ARB-TSMOM-D2", 1.30, 0.46, 54, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_d3, "btcusdt", 259200, "BTC-TSMOM-D3", 242.75, 6.40, 100, 15, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_d3, "ethusdt", 259200, "ETH-TSMOM-D3", 7.40, 2.70, 89, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_d3, "solusdt", 259200, "SOL-TSMOM-D3", 2.69, 1.67, 100, 15, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_d3, "xrpusdt", 259200, "XRP-TSMOM-D3", 45.74, 3.90, 100, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d3, "linkusdt", 259200, "LINK-TSMOM-D3", 10.45, 3.74, 100, 10, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d3, "nearusdt", 259200, "NEAR-TSMOM-D3", 99.90, 6.08, 44, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_d3, "bnbusdt", 259200, "BNB-TSMOM-D3", 34.14, 2.67, 100, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_d3, "dogeusdt", 259200, "DOGE-TSMOM-D3", 3.72, 2.05, 57, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_d3, "avaxusdt", 259200, "AVAX-TSMOM-D3", 2.51, 1.34, 58, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_d3, "suiusdt", 259200, "SUI-TSMOM-D3", 2.38, 1.43, 56, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_d3, "arbusdt", 259200, "ARB-TSMOM-D3", 1.50, 0.83, 67, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_rsi_h6, "btcusdt", 21600, "BTC-RSI-H6", 3.27, 2.09, 52, 24, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_rsi_h6, "ethusdt", 21600, "ETH-RSI-H6", 41.75, 3.43, 97, 12, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_rsi_h6, "linkusdt", 21600, "LINK-RSI-H6", 8.79, 3.20, 95, 12, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_BNB, &bnb_rsi_h6, "bnbusdt", 21600, "BNB-RSI-H6", 373.91, 2.49, 100, 14, 21});
    g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h6, "dogeusdt", 21600, "DOGE-RSI-H6", 2.25, 1.10, 65, 108, 31});  // tier-A re-validated 5yr Binance
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_rsi_h6, "avaxusdt", 21600, "AVAX-RSI-H6", 1.77, 1.15, 40, 19, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h6, "btcusdt", 21600, "BTC-BOLL-H6", 8.04, 3.24, 100, 18, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_boll_h6, "ethusdt", 21600, "ETH-BOLL-H6", 4.87, 2.38, 99, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_boll_h6, "solusdt", 21600, "SOL-BOLL-H6", 5.77, 3.23, 84, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h6, "xrpusdt", 21600, "XRP-BOLL-H6", 2.16, 1.15, 70, 16, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll_h6, "linkusdt", 21600, "LINK-BOLL-H6", 99.90, 6.18, 100, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_boll_h6, "bnbusdt", 21600, "BNB-BOLL-H6", 4.06, 2.05, 49, 17, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_boll_h6, "dogeusdt", 21600, "DOGE-BOLL-H6", 99.90, 4.78, 100, 15, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_boll_h6, "avaxusdt", 21600, "AVAX-BOLL-H6", 2.51, 1.88, 87, 20, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_rsi_h12, "btcusdt", 43200, "BTC-RSI-H12", 3.57, 2.51, 100, 12, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_rsi_h12, "ethusdt", 43200, "ETH-RSI-H12", 1.55, 0.65, 44, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_rsi_h12, "solusdt", 43200, "SOL-RSI-H12", 11.81, 2.33, 63, 10, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_XRP, &xrp_rsi_h12, "xrpusdt", 43200, "XRP-RSI-H12", 63.22, 2.33, 80, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_rsi_h12, "linkusdt", 43200, "LINK-RSI-H12", 15.83, 5.99, 100, 12, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h12, "dogeusdt", 43200, "DOGE-RSI-H12", 2.00, 1.03, 40, 14, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_rsi_h12, "avaxusdt", 43200, "AVAX-RSI-H12", 1.68, 0.65, 61, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h12, "btcusdt", 43200, "BTC-BOLL-H12", 6.45, 2.64, 97, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_boll_h12, "solusdt", 43200, "SOL-BOLL-H12", 3.55, 1.48, 66, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h12, "xrpusdt", 43200, "XRP-BOLL-H12", 54.35, 2.52, 96, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll_h12, "linkusdt", 43200, "LINK-BOLL-H12", 2.37, 1.32, 92, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_boll_h12, "dogeusdt", 43200, "DOGE-BOLL-H12", 5.08, 1.89, 100, 11, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_rsi_h8, "btcusdt", 28800, "BTC-RSI-H8", 1.79, 0.87, 69, 16, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h6, "nearusdt", 21600, "NEAR-DONCH-H6", 1.37, 0.80, 70, 45, 21});
    g_slots.push_back({chimera::SYM_SUI, &sui_donch_h6, "suiusdt", 21600, "SUI-DONCH-H6", 3.34, 2.49, 86, 80, 31});  // tier-A re-validated 3yr Binance (SUI listed 2023)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_donch_h4, "aptusdt", 14400, "APT-DONCH-H4", 2.26, 1.91, 81, 26, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_donch_h6, "aptusdt", 21600, "APT-DONCH-H6", 1.73, 1.22, 45, 23, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_donch_h6, "arbusdt", 21600, "ARB-DONCH-H6", 1.28, 0.47, 40, 13, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_rsi_d1, "btcusdt", 86400, "BTC-RSI-D1", 1.33, 0.55, 55, 9, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_d1, "dogeusdt", 86400, "DOGE-RSI-D1", 1.36, 0.60, 60, 10, 21});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_rsi_d1, "linkusdt", 86400, "LINK-RSI-D1", 1.44, 0.43, 43, 12, 21});

    // Session 22 engines
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_rsi_h8, "ethusdt", 28800, "ETH-RSI-H8", 1.70, 0.96, 100, 49, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_rsi_h8, "bnbusdt", 28800, "BNB-RSI-H8", 1.95, 1.07, 96, 31, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h8, "dogeusdt", 28800, "DOGE-RSI-H8", 1.80, 1.22, 100, 20, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_rsi_h8, "xrpusdt", 28800, "XRP-RSI-H8", 14.85, 2.01, 98, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h8, "aptusdt", 28800, "APT-RSI-H8", 3.03, 1.27, 68, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_rsi_h8, "solusdt", 28800, "SOL-RSI-H8", 2.97, 1.09, 67, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_rsi_h8, "linkusdt", 28800, "LINK-RSI-H8", 4.50, 1.77, 44, 12, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h8, "arbusdt", 28800, "ARB-RSI-H8", 1.56, 0.80, 53, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h8, "nearusdt", 28800, "NEAR-RSI-H8", 1.33, 0.56, 45, 40, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h8, "btcusdt", 28800, "BTC-BOLL-H8", 2.02, 0.93, 44, 20, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_boll_h8, "ethusdt", 28800, "ETH-BOLL-H8", 1.51, 0.80, 66, 35, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_boll_h8, "solusdt", 28800, "SOL-BOLL-H8", 4.44, 1.90, 81, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_boll_h8, "bnbusdt", 28800, "BNB-BOLL-H8", 5.10, 2.64, 45, 12, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_boll_h8, "avaxusdt", 28800, "AVAX-BOLL-H8", 2.01, 0.93, 46, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll_h8, "linkusdt", 28800, "LINK-BOLL-H8", 6.83, 2.70, 86, 24, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h8, "xrpusdt", 28800, "XRP-BOLL-H8", 2.56, 1.45, 62, 34, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_boll_h8, "dogeusdt", 28800, "DOGE-BOLL-H8", 4.24, 2.11, 50, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_boll_h8, "suiusdt", 28800, "SUI-BOLL-H8", 2.49, 1.55, 66, 13, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT, &apt_boll_h8, "aptusdt", 28800, "APT-BOLL-H8", 1.96, 0.91, 61, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h8, "nearusdt", 28800, "NEAR-BOLL-H8", 2.54, 1.31, 83, 17, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_boll_h8, "arbusdt", 28800, "ARB-BOLL-H8", 2.53, 1.58, 64, 12, 22});
    // DISABLED: g_slots.push_back({chimera::SYM_ETH, &eth_rsi_h16, "ethusdt", 57600, "ETH-RSI-H16", 158.17, 9.07, 100, 24, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_rsi_h16, "bnbusdt", 57600, "BNB-RSI-H16", 3.87, 1.78, 100, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_rsi_h16, "xrpusdt", 57600, "XRP-RSI-H16", 3.71, 1.41, 100, 20, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_rsi_h16, "linkusdt", 57600, "LINK-RSI-H16", 2.37, 1.13, 72, 17, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h16, "nearusdt", 57600, "NEAR-RSI-H16", 2.02, 0.86, 83, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_rsi_h16, "btcusdt", 57600, "BTC-RSI-H16", 3.50, 1.42, 54, 15, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_rsi_h16, "solusdt", 57600, "SOL-RSI-H16", 4.29, 2.17, 46, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h16, "dogeusdt", 57600, "DOGE-RSI-H16", 2.01, 1.16, 43, 17, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll_h16, "linkusdt", 57600, "LINK-BOLL-H16", 6.77, 2.24, 100, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h16, "xrpusdt", 57600, "XRP-BOLL-H16", 3.01, 0.90, 85, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h16, "btcusdt", 57600, "BTC-BOLL-H16", 2.66, 1.15, 70, 13, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h16, "nearusdt", 57600, "NEAR-BOLL-H16", 2.94, 1.15, 55, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_boll_h16, "ethusdt", 57600, "ETH-BOLL-H16", 2.61, 0.88, 40, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_donch_h8, "xrpusdt", 28800, "XRP-DONCH-H8", 3.05, 2.21, 100, 45, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h8, "nearusdt", 28800, "NEAR-DONCH-H8", 2.43, 2.13, 73, 55, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_donch_h8, "suiusdt", 28800, "SUI-DONCH-H8", 5.02, 2.18, 100, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_donch_h8, "btcusdt", 28800, "BTC-DONCH-H8", 1.48, 0.78, 44, 61, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ARB, &arb_donch_h8, "arbusdt", 28800, "ARB-DONCH-H8", 2.00, 1.22, 51, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_donch_h16, "xrpusdt", 57600, "XRP-DONCH-H16", 4.88, 1.88, 100, 19, 22});
    g_slots.push_back({chimera::SYM_BNB, &bnb_donch_h16, "bnbusdt", 57600, "BNB-DONCH-H16", 5.33, 1.75, 87, 79, 31});  // tier-A re-validated 5yr Binance
    g_slots.push_back({chimera::SYM_BTC, &btc_donch_h16, "btcusdt", 57600, "BTC-DONCH-H16", 2.31, 1.11, 88, 76, 31});  // tier-A re-validated 5yr Binance
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_donch_h16, "linkusdt", 57600, "LINK-DONCH-H16", 2.69, 1.03, 71, 18, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI, &sui_donch_h16, "suiusdt", 57600, "SUI-DONCH-H16", 3.68, 1.86, 67, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h16, "nearusdt", 57600, "NEAR-DONCH-H16", 1.87, 1.36, 58, 29, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL, &sol_donch_h16, "solusdt", 57600, "SOL-DONCH-H16", 2.08, 0.95, 49, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_donch_h16, "dogeusdt", 57600, "DOGE-DONCH-H16", 2.07, 0.91, 41, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_donch_d2, "bnbusdt", 172800, "BNB-DONCH-D2", 99.90, 5.55, 93, 14, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_donch_d2, "xrpusdt", 172800, "XRP-DONCH-D2", 10.03, 1.90, 100, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_donch_d2, "btcusdt", 172800, "BTC-DONCH-D2", 5.56, 1.55, 84, 12, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_donch_d2, "ethusdt", 172800, "ETH-DONCH-D2", 3.29, 1.04, 95, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_donch_d2, "linkusdt", 172800, "LINK-DONCH-D2", 2.78, 0.95, 83, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC, &btc_donch_d3, "btcusdt", 259200, "BTC-DONCH-D3", 128.98, 2.40, 98, 12, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH, &eth_donch_d3, "ethusdt", 259200, "ETH-DONCH-D3", 9.21, 2.16, 87, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP, &xrp_donch_d3, "xrpusdt", 259200, "XRP-DONCH-D3", 5.39, 1.41, 100, 10, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB, &bnb_donch_d3, "bnbusdt", 259200, "BNB-DONCH-D3", 2.09, 0.97, 80, 11, 22});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_donch_d3, "dogeusdt", 259200, "DOGE-DONCH-D3", 1.97, 0.82, 96, 8, 22});

    // Session 24 engines — DONCHIAN gap-fill + TSMOM H12 fill (15 engines)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h2,   "xrpusdt",   7200, "XRP-DONCH-H2",   1.68, 1.69,  98, 125, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h2,  "nearusdt",  7200, "NEAR-DONCH-H2",  1.48, 1.55,  61,  90, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h3,   "xrpusdt",  10800, "XRP-DONCH-H3",   1.61, 1.48,  99, 121, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h3,  "nearusdt", 10800, "NEAR-DONCH-H3",  1.64, 1.70,  88,  85, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h4,   "xrpusdt",  14400, "XRP-DONCH-H4",   1.72, 1.38, 100,  71, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h6,   "xrpusdt",  21600, "XRP-DONCH-H6",   2.63, 2.25, 100,  45, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_donch_h6,   "bnbusdt",  21600, "BNB-DONCH-H6",   2.08, 1.84,  64,  31, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h12,  "xrpusdt",  43200, "XRP-DONCH-H12",  3.40, 2.09, 100,  26, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h12, "nearusdt", 43200, "NEAR-DONCH-H12", 2.25, 1.43,  65,  21, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h12,  "ethusdt",  43200, "ETH-TSMOM-H12",  1.61, 1.44,  94, 100, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h12,  "solusdt",  43200, "SOL-TSMOM-H12",  1.91, 2.30,  86, 120, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h12,  "bnbusdt",  43200, "BNB-TSMOM-H12",  2.45, 3.08, 100,  96, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h12, "linkusdt", 43200, "LINK-TSMOM-H12", 1.62, 2.00,  90, 187, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h12,  "xrpusdt",  43200, "XRP-TSMOM-H12",  1.54, 1.52,  73, 153, 24});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT,  &apt_tsmom_h12,  "aptusdt",  43200, "APT-TSMOM-H12",  2.32, 3.54,  89,  81, 24});

    // ── Session 26 — RSI_REVERT H4 + BOLLINGER H4/H2 (10 engines) ────────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi_h4,     "ethusdt",  14400, "ETH-RSI-H4",     1.82, 1.45,  72,  38, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h4,    "dogeusdt", 14400, "DOGE-RSI-H4",    1.67, 1.31,  68,  42, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_rsi_h4,     "xrpusdt",  14400, "XRP-RSI-H4",     1.74, 1.38,  70,  45, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_rsi_h4,     "solusdt",  14400, "SOL-RSI-H4",     1.58, 1.22,  65,  35, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_AVAX, &avax_boll_h4,   "avaxusdt", 14400, "AVAX-BOLL-H4",   1.71, 1.35,  66,  28, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_boll_h4,   "linkusdt", 14400, "LINK-BOLL-H4",   1.65, 1.28,  63,  31, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_boll_h4,    "solusdt",  14400, "SOL-BOLL-H4",    1.69, 1.33,  67,  26, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_boll_h2,    "solusdt",   7200, "SOL-BOLL-H2",    1.53, 1.19,  61,  52, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_boll_h4,    "btcusdt",  14400, "BTC-BOLL-H4",    1.61, 1.25,  64,  33, 26});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_boll_h4,    "xrpusdt",  14400, "XRP-BOLL-H4",    1.56, 1.21,  62,  29, 26});

    // ── Session 27 — H12 gap-fill (2 engines) ──────────────────────────────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI,  &sui_rsi_h12,    "suiusdt",  43200, "SUI-RSI-H12",    1.66, 1.27,  96,  21, 27});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_boll_h12,   "ethusdt",  43200, "ETH-BOLL-H12",  48.01, 3.02, 100,  12, 27});

    // ── Session 28 — KELTNER_REVERT + DUAL_THRUST (8 engines) ───────────────
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_keltner_h6,  "dogeusdt", 21600, "DOGE-KELTNER-H6",  5.24, 2.43,  95,  15, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_keltner_h12, "linkusdt", 43200, "LINK-KELTNER-H12", 6.85, 2.21,  66,  11, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_keltner_h8,  "dogeusdt", 28800, "DOGE-KELTNER-H8",  4.38, 2.45,  64,  18, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_keltner_h12,  "btcusdt",  43200, "BTC-KELTNER-H12",  3.03, 1.69,  58,  30, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SUI,  &sui_keltner_h12,  "suiusdt",  43200, "SUI-KELTNER-H12",  4.82, 1.99,  40,  16, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_APT,  &apt_keltner_h8,   "aptusdt",  28800, "APT-KELTNER-H8",   3.21, 1.40,  46,  11, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_dt_h12,       "solusdt",  43200, "SOL-DT-H12",       3.01, 2.42,  56,  29, 28});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_dt_h8,        "xrpusdt",  28800, "XRP-DT-H8",        1.70, 1.38,  64, 100, 28});

    // Ichimoku engines (6) — Session 29
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_ichi_h6,      "btcusdt",  21600, "BTC-ICHI-H6",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_ichi_h6,      "ethusdt",  21600, "ETH-ICHI-H6",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_ichi_h6,      "solusdt",  21600, "SOL-ICHI-H6",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_ichi_h6,      "xrpusdt",  21600, "XRP-ICHI-H6",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_ichi_h12,    "linkusdt", 43200, "LINK-ICHI-H12",    0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_ichi_h12,    "dogeusdt", 43200, "DOGE-ICHI-H12",    0.00, 0.00,   0,   0, 29});

    // SuperTrend engines (6) — Session 29
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_st_h6,        "btcusdt",  21600, "BTC-ST-H6",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_st_h6,        "ethusdt",  21600, "ETH-ST-H6",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_st_h6,        "solusdt",  21600, "SOL-ST-H6",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_st_h4,        "xrpusdt",  14400, "XRP-ST-H4",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_st_h8,       "linkusdt", 28800, "LINK-ST-H8",       0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_st_h8,       "dogeusdt", 28800, "DOGE-ST-H8",       0.00, 0.00,   0,   0, 29});

    // Williams %R engines (4) — Session 29b
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_wr_h4,        "ethusdt",  14400, "ETH-WR-H4",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_wr_h4,        "solusdt",  14400, "SOL-WR-H4",        0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_wr_h6,       "dogeusdt", 21600, "DOGE-WR-H6",       0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_XRP,  &xrp_wr_h6,        "xrpusdt",  21600, "XRP-WR-H6",        0.00, 0.00,   0,   0, 29});

    // Stochastic RSI engines (4) — Session 29b
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_srsi_h4,      "ethusdt",  14400, "ETH-SRSI-H4",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_SOL,  &sol_srsi_h4,      "solusdt",  14400, "SOL-SRSI-H4",      0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_DOGE, &doge_srsi_h6,     "dogeusdt", 21600, "DOGE-SRSI-H6",     0.00, 0.00,   0,   0, 29});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_LINK, &link_srsi_h6,     "linkusdt", 21600, "LINK-SRSI-H6",     0.00, 0.00,   0,   0, 29});

    // W1 mean-reversion engines (Session 30, Edge 6)
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_rsi_w1,       "btcusdt", 604800, "BTC-RSI-W1",       0.00, 0.00,   0,   0, 30});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi_w1,       "ethusdt", 604800, "ETH-RSI-W1",       0.00, 0.00,   0,   0, 30});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_BTC,  &btc_boll_w1,      "btcusdt", 604800, "BTC-BOLL-W1",      0.00, 0.00,   0,   0, 30});
    // DISABLED-TOP5: g_slots.push_back({chimera::SYM_ETH,  &eth_boll_w1,      "ethusdt", 604800, "ETH-BOLL-W1",      0.00, 0.00,   0,   0, 30});

    // ── Wire up bar callbacks for persistence + audit trail ────────────────
    for (auto& slot : g_slots) {
        if (slot.engine) slot.engine->set_on_bar(on_bar_callback);
    }

    // ── Activate vol_filter + mtf_gate on all counter-trend engines ──────
    // Counter-trend = RSI_REVERT, BOLLINGER, KELTNER_REVERT
    // These benefit from suppression during chaos and bearish D1 trends.
    {
        int vol_count = 0, mtf_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (!slot.engine->is_trend_following()) {
                // Counter-trend engine — enable both filters
                slot.engine->enable_vol_filter(true);
                vol_count++;
                // Only apply MTF gate to sub-D1 timeframes (D1 engines ARE the reference)
                if (slot.tf_secs < 86400) {
                    slot.engine->enable_mtf_gate(true);
                    mtf_count++;
                }
            }
        }
        std::printf("[STARTUP] Activated vol_filter on %d counter-trend engines\n", vol_count);
        std::printf("[STARTUP] Activated mtf_gate on %d sub-D1 counter-trend engines\n", mtf_count);
        std::fflush(stdout);
    }

    // ── Activate ADX filter on all trend-following engines (Session 29) ──────
    // ADX(14) < 25 = ranging market = suppress TSMOM/DONCHIAN/DUAL_THRUST/
    // ICHIMOKU/SUPERTREND entries (they need directional movement to profit).
    {
        int adx_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (slot.engine->is_trend_following()) {
                slot.engine->enable_adx_filter(true);
                adx_count++;
            }
        }
        std::printf("[STARTUP] Activated adx_filter on %d trend-following engines\n", adx_count);
        std::fflush(stdout);
    }

    // ── Activate volume gate on ALL engines (Session 29) ────────────────────
    // Tick-count volume proxy: suppress entries when bar activity < 30% of
    // rolling average (detects weekend dead zones and exchange outages).
    {
        int vol_gate_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            slot.engine->enable_volume_gate(true);
            vol_gate_count++;
        }
        std::printf("[STARTUP] Activated volume_gate on %d engines\n", vol_gate_count);
        std::fflush(stdout);
    }

    // ── Activate correlation filter on non-BTC engines (Session 29b) ────────
    // When BTC correlation is extreme, altcoin entries get suppressed.
    {
        int corr_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (slot.symbol_id != chimera::SYM_BTC) {
                slot.engine->enable_corr_filter(true);
                corr_count++;
            }
        }
        std::printf("[STARTUP] Activated corr_filter on %d non-BTC engines\n", corr_count);
        std::fflush(stdout);
    }

    // ── Session filter DISABLED (shadow tuning 2026-05-17) ─────────────────
    // Was suppressing sub-H4 entries during 00-08 UTC (= 8am-4pm SGT).
    // On a Singapore VPS this kills prime trading hours. Disabled so all
    // engines can generate shadow trades 24/7 for proper PnL validation.
    // Re-evaluate once we have 4+ weeks of shadow data.
    {
        std::printf("[STARTUP] session_filter DISABLED — shadow mode needs 24/7 signal flow\n");
        std::fflush(stdout);
    }

    // ── Session 30: Initial funding rate fetch + position sizing setup ────
    {
        std::printf("[STARTUP] Fetching multi-symbol funding rates...\n");
        std::fflush(stdout);
        g_funding_filter.fetch_all();
        g_last_funding_fetch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Propagate initial funding state to all engines
        std::lock_guard<std::mutex> lk(g_engine_mtx);
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            slot.engine->set_funding_tailwind(g_funding_filter.has_tailwind(slot.symbol_id));
            slot.engine->set_funding_headwind(g_funding_filter.has_headwind(slot.symbol_id));
        }

        // Session 30, Edge 4: Set position sizing multipliers for top engines
        // Based on walk-forward validated OOS performance (PF * sqrt(trades) ranking)
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            // Top-tier engines get 1.5x sizing, default = 1.0
            // Identified from revalidation: LINK-BOLL-H6, BNB-DONCH-D2, XRP-BOLL-H12
            // Also top TSMOM: BTC-H12 PF=3.63, ETH-D1 PF=3.15, BNB-D1 PF=3.16
            if (slot.tag.find("LINK-BOLL-H6") != std::string::npos ||
                slot.tag.find("BNB-DONCH-D2") != std::string::npos ||
                slot.tag.find("XRP-BOLL-H12") != std::string::npos ||
                slot.tag.find("BTC-TSMOM-H12") != std::string::npos ||
                slot.tag.find("ETH-TSMOM-D1") != std::string::npos ||
                slot.tag.find("BNB-TSMOM-D1") != std::string::npos) {
                slot.engine->set_sizing_mult(1.5);
            }
        }
        std::printf("[STARTUP] Funding filter + position sizing initialized\n");
        std::fflush(stdout);
    }

    // ── Ensure data directories exist ────────────────────────────────────
    ::mkdir("data", 0755);        // no-op if exists
    ::mkdir("data/bars", 0755);   // bar persistence directory

    // ── Seed engine bar buffers ──────────────────────────────────────────
    // Strategy:
    //   1. If the engine has a NATIVE Binance interval (H1,H2,H4,H6,H8,H12,D1,D3):
    //      → Always fetch from REST (guaranteed 64 bars, signal-ready immediately)
    //      → Saved bars are ignored (REST is always fresher and more complete)
    //   2. If the engine has NO native interval (H3, H16, D2):
    //      → Try H1 aggregation first (fetch H1 klines, aggregate to target TF)
    //      → Fall back to saved bars if aggregation fails
    //      → Cold-start from ticks as last resort
    //
    // This fixes the bug where partial saved bars (e.g. 11 bars from short
    // runs) would block the REST fetch that provides 64 bars, leaving engines
    // stuck below their lookback threshold and unable to evaluate signals.
    {
        chimera::BinanceREST seed_rest;
        int total = (int)g_slots.size();
        std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
        std::printf("[STARTUP] SEEDING %d engines from Binance REST API...\n", total);
        std::printf("[STARTUP] (This takes 30-60s on first boot — REST + H1 aggregation)\n");
        std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
        std::fflush(stdout);

        int seeded_rest = 0, seeded_agg = 0, seeded_saved = 0, cold = 0;
        int progress = 0;

        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            progress++;

            // Progress update every 25 engines
            if (progress % 25 == 0 || progress == total) {
                std::printf("[SEED] Progress: %d/%d engines seeded...\n", progress, total);
                std::fflush(stdout);
            }

            const char* interval = tf_to_binance_interval(slot.tf_secs);
            bool has_native_interval = (interval && *interval);

            // ── Determine how many bars this engine actually needs ────────
            // Use the engine's computed max_history (accounts for lookback,
            // indicator periods like ichi_senkou_b=120, ADX, Keltner, etc.)
            int seed_bars_needed = slot.engine->max_history_needed();
            if (seed_bars_needed < 64) seed_bars_needed = 64;  // minimum floor

            std::printf("[SEED][%s] needs %d bars (max_history)\n",
                slot.tag.c_str(), seed_bars_needed);

            if (has_native_interval) {
                // ── Native interval: always use REST ──
                seed_engine_from_history(seed_rest, *slot.engine,
                                         slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                seeded_rest++;
            } else {
                // ── No native interval: choose best aggregation source ──────────
                // D2+ engines (tf >= 2 days): aggregate from D1 bars (gives 500+ bars)
                // Sub-day engines (H3, H16): aggregate from H1 bars (gives 62-333 bars)
                bool seeded_ok = false;

                if (slot.tf_secs >= 172800) {
                    // D2 or larger — use D1 aggregation for much deeper history
                    seed_engine_from_d1_aggregation(seed_rest, *slot.engine,
                                                    slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                    if (slot.engine->bars_in_buffer() > 0) {
                        seeded_ok = true;
                        seeded_agg++;
                    }
                }

                if (!seeded_ok) {
                    // H3/H16 or D1-agg fallback — use H1 aggregation
                    seed_engine_from_h1_aggregation(seed_rest, *slot.engine,
                                                    slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                    if (slot.engine->bars_in_buffer() > 0) {
                        seeded_ok = true;
                        seeded_agg++;
                    }
                }

                if (!seeded_ok) {
                    // Both aggregation paths failed — try saved bars as fallback
                    auto saved = load_saved_bars(slot.tag, seed_bars_needed);
                    if (!saved.empty()) {
                        int kept = slot.engine->seed_bars(saved);
                        std::printf("[WARM_START][%s] Seeded from saved bars: loaded=%d kept=%d\n",
                            slot.tag.c_str(), (int)saved.size(), kept);
                        std::fflush(stdout);
                        seeded_saved++;
                    } else {
                        std::fprintf(stderr, "[SEED][%s] No data source — cold-start from live ticks\n",
                            slot.tag.c_str());
                        cold++;
                    }
                }
            }
        }

        std::printf("[STARTUP] Seeding complete: REST=%d  H1-agg=%d  saved=%d  cold=%d\n",
            seeded_rest, seeded_agg, seeded_saved, cold);
        std::fflush(stdout);

        // ── Post-seed audit: warn about under-seeded engines ──────────
        int under_seeded = 0;
        for (const auto& slot : g_slots) {
            if (!slot.engine) continue;
            int have = slot.engine->bars_in_buffer();
            int need = slot.engine->max_history_needed();
            if (have < need) {
                std::fprintf(stderr, "[SEED-WARN][%s] UNDER-SEEDED: have=%d need=%d — "
                             "signal won't fire until %d more bars accumulate from live data\n",
                             slot.tag.c_str(), have, need, need - have);
                under_seeded++;
            }
        }
        if (under_seeded > 0) {
            std::fprintf(stderr, "[STARTUP] WARNING: %d engines under-seeded — "
                         "check REST fetch limits or aggregation source\n", under_seeded);
        } else {
            std::printf("[STARTUP] All %d engines fully seeded — ready to trade immediately\n",
                        (int)g_slots.size());
        }
        std::fflush(stdout);
    }

    // ── Position resume: restore open positions after restart ────────────
    // Reads data/open_positions.json (written every 60s by the snapshot loop)
    // and injects saved positions back into their matching engines.
    // This allows the bot to survive restarts/deploys without losing trades.
    {
        std::ifstream pf("data/open_positions.json");
        if (pf.is_open()) {
            std::string content((std::istreambuf_iterator<char>(pf)),
                                 std::istreambuf_iterator<char>());
            pf.close();

            int resumed = 0;
            // Simple JSON field extractor (no lib dependency)
            auto extract_str = [&](const std::string& blob, const char* key) -> std::string {
                std::string k = std::string("\"") + key + "\":\"";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return "";
                pos += k.size();
                auto end = blob.find('"', pos);
                if (end == std::string::npos) return "";
                return blob.substr(pos, end - pos);
            };
            auto extract_dbl = [&](const std::string& blob, const char* key) -> double {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0.0;
                pos += k.size();
                try { return std::stod(blob.substr(pos, 30)); }
                catch (...) { return 0.0; }
            };
            auto extract_int64 = [&](const std::string& blob, const char* key) -> int64_t {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0;
                pos += k.size();
                try { return std::stoll(blob.substr(pos, 20)); }
                catch (...) { return 0; }
            };
            auto extract_int = [&](const std::string& blob, const char* key) -> int {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0;
                pos += k.size();
                try { return std::stoi(blob.substr(pos, 10)); }
                catch (...) { return 0; }
            };
            auto extract_bool = [&](const std::string& blob, const char* key) -> bool {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return false;
                pos += k.size();
                return (blob.substr(pos, 4) == "true");
            };

            // Find each position object in the "positions" array
            std::string positions_key = "\"positions\":[";
            auto arr_pos = content.find(positions_key);
            if (arr_pos != std::string::npos) {
                arr_pos += positions_key.size();
                // Split on },{
                size_t search_from = arr_pos;
                while (search_from < content.size()) {
                    auto obj_start = content.find('{', search_from);
                    if (obj_start == std::string::npos) break;
                    auto obj_end = content.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    std::string obj = content.substr(obj_start, obj_end - obj_start + 1);
                    search_from = obj_end + 1;

                    std::string tag = extract_str(obj, "tag");
                    if (tag.empty()) continue;

                    double entry_px = extract_dbl(obj, "entry_px");
                    if (entry_px <= 0.0) continue;

                    // Build resume state
                    chimera::EdgeEngine::ResumeState rs;
                    rs.entry_px        = entry_px;
                    rs.sl_px           = extract_dbl(obj, "sl_px");
                    rs.atr_at_entry    = extract_dbl(obj, "atr_at_entry");
                    rs.entry_ts_ms     = extract_int64(obj, "entry_ts");
                    rs.time_exit_ts_ms = extract_int64(obj, "time_exit_ts");
                    rs.bars_held       = extract_int(obj, "bars_held");
                    rs.trail_armed     = extract_bool(obj, "trail_armed");
                    rs.trail_stop_px   = extract_dbl(obj, "trail_stop_px");
                    rs.trail_arm_px    = extract_dbl(obj, "trail_arm_px");
                    rs.mfe_px          = extract_dbl(obj, "mfe_px");
                    rs.mfe_bp          = extract_dbl(obj, "mfe_bp");

                    // Find matching engine by tag
                    for (auto& s : g_slots) {
                        if (s.engine && s.tag == tag) {
                            if (s.engine->resume_position(rs)) {
                                resumed++;
                            }
                            break;
                        }
                    }
                }
            }

            if (resumed > 0) {
                std::printf("[STARTUP] ✓ Resumed %d open position(s) from snapshot\n", resumed);
            } else {
                std::printf("[STARTUP] No positions to resume (all flat)\n");
            }
            std::fflush(stdout);
        } else {
            std::printf("[STARTUP] No position snapshot found — starting fresh\n");
            std::fflush(stdout);
        }
    }

    // ── Trade journal: load history ──────────────────────────────────────
    load_trade_history();

    // GUI root
    {
        char exe[4096] = {};
        ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe)-1);
        if (len > 0) {
            std::string dir(exe, len);
            auto sl = dir.rfind('/');
            if (sl != std::string::npos) dir = dir.substr(0, sl);
            gui_root = dir + "/../gui";
        } else { gui_root = "../gui"; }
    }

    std::printf("[STARTUP] HTTP server starting on port 8080...\n");
    std::fflush(stdout);
    std::thread http_thread(http_server_thread, 8080);
    http_thread.detach();

    // ── Spot WebSocket feed (subscribes to all 12 symbols) ───────────────
    chimera::BinanceWSFeed feed;
    std::printf("[STARTUP] Subscribing to %d symbol streams on Binance...\n", chimera::MAX_SYMBOLS);
    std::fflush(stdout);
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (mid <= 0.0) return;

        store_dbl_atomic(g_last_spot_px_bits[id], mid);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Stale gate (5s — relaxed vs HFT)
        double tick_age_ms = 0.0;
        if (tick.trade_time > 0) {
            tick_age_ms = static_cast<double>(now_ms - tick.trade_time);
            if (tick_age_ms < 0.0) tick_age_ms = 0.0;
        }
        if (tick_age_ms > 5000.0) return;

        // Route to whichever engine slot matches this symbol id.
        {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            for (auto& s : g_slots) {
                if (s.symbol_id == id && s.engine) {
                    s.engine->on_tick(mid, now_ms);
                }
            }
        }

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 10000 == 0) {
            std::printf("[TICK] n=%d | %s px=%.4f | age=%.1fms\n",
                n, tick.symbol.c_str(), mid, tick_age_ms);
            std::fflush(stdout);
        }
    });

    feed.start();

    // ── Session 30, Edge 7: Start liquidation cascade feed ───────────────
    g_liq_feed.set_callback([](const chimera::LiquidationWSFeed::LiqEvent& ev) {
        g_liq_detector.on_liquidation(ev.symbol_id, ev.price, ev.qty, ev.is_long, ev.ts_ms);
    });
    g_liq_feed.start();

    std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
    std::printf("[STARTUP] ✓ CHIMERA READY — %d engines running (shadow_mode=true)\n", (int)g_slots.size());
    std::printf("[STARTUP]   TSMOM=94+4W1 | RSI_REVERT=49+2W1 | BOLLINGER=44+2W1 | DONCHIAN=28 | ICHIMOKU=6 | SUPERTREND=6\n");
    std::printf("[STARTUP]   KELTNER=6 | DUAL_THRUST=2 | WILLIAMS_R=4 | STOCH_RSI=4\n");
    std::printf("[STARTUP]   Filters: vol+mtf+adx+vol_gate+corr+session+portfolio+funding+vol_regime+cross_tf+liq_cascade\n");
    std::printf("[STARTUP]   4 engines DISABLED (failed re-validation S29c)\n");
    std::printf("[STARTUP]   17 symbols | spot-long-only | all edges validated\n");
    std::printf("[STARTUP]   NEW S30: funding filter, vol regime, position sizing, cross-TF, W1 MR, liq cascade\n");
    std::printf("[STARTUP]   GUI: http://localhost:8080\n");
    std::printf("[STARTUP]   API: /api/state2  /api/positions  /api/trades\n");
    std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
    std::fflush(stdout);

    // ── Periodic open-position snapshot (every 60s) ────────────────────────
    // Writes data/open_positions.json so that if the process is SIGKILL'd or
    // OOM-killed, we know exactly what was open. On graceful shutdown this file
    // becomes stale (positions are closed), so the shutdown path clears it.
    constexpr int SNAPSHOT_INTERVAL_MS = 60000;
    int64_t last_snapshot_ms = 0;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now_ms - last_snapshot_ms >= SNAPSHOT_INTERVAL_MS) {
            last_snapshot_ms = now_ms;
            // Build snapshot of all open positions
            std::ostringstream snap;
            snap << "{\"snapshot_ts\":" << now_ms << ",\"positions\":[";
            int count = 0;
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine || !s.engine->in_position()) continue;
                    double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                    std::string pj = s.engine->position_snapshot_json(spot);
                    if (!pj.empty()) {
                        if (count > 0) snap << ",";
                        snap << pj;
                        count++;
                    }
                }
            }
            snap << "],\"open_count\":" << count << "}\n";
            FILE* f = fopen("data/open_positions.json", "w");
            if (f) {
                std::string out = snap.str();
                fwrite(out.c_str(), 1, out.size(), f);
                fclose(f);
            }

            // ── MTF gate: propagate D1 TSMOM trend to lower-TF engines ─────
            // For each symbol, find the D1 TSMOM engine and read its trend.
            // Then set d1_bullish on all non-D1 engines for that symbol.
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                // First pass: extract D1 trend per symbol
                bool d1_trend[chimera::MAX_SYMBOLS];
                bool d1_found[chimera::MAX_SYMBOLS];
                for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
                    d1_trend[i] = true;   // default bullish if no D1 engine
                    d1_found[i] = false;
                }
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    // D1 TSMOM engines are our trend reference
                    if (s.tf_secs == 86400 && s.engine->is_trend_following()) {
                        d1_trend[s.symbol_id] = s.engine->trend_bullish();
                        d1_found[s.symbol_id] = true;
                    }
                }
                // Second pass: propagate to all engines with mtf_gate enabled
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_d1_bullish(d1_trend[s.symbol_id]);
                }
            }

            // ── Session 30, Edge 1: Periodic funding rate refresh ────────────
            // Fetch every 8h (matches Binance funding interval).
            // Propagate tailwind/headwind flags to all engines.
            if (now_ms - g_last_funding_fetch_ms >= FUNDING_FETCH_INTERVAL_MS) {
                std::thread([](){
                    g_funding_filter.fetch_all();
                }).detach();
                g_last_funding_fetch_ms = now_ms;
            }
            // Always propagate current state (fetch may complete between loops)
            if (g_funding_filter.is_ready()) {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_funding_tailwind(g_funding_filter.has_tailwind(s.symbol_id));
                    s.engine->set_funding_headwind(g_funding_filter.has_headwind(s.symbol_id));
                }
            }

            // ── Session 30, Edge 3: Volatility regime classification ─────────
            // Use BTC's ATR(14)/ATR(50) from a D1 engine as the global vol regime.
            // LOW: ratio < 0.5 (deeply compressed — only extreme suppression)
            // HIGH: ratio > 1.8 (truly chaotic — only extreme suppression)
            // MEDIUM: 0.5-1.8 (both strategies active — widened for shadow mode)
            // Shadow tuning 2026-05-17: was 0.8/1.4 which muted half the fleet
            // in normal market conditions. Widened so engines generate trades.
            {
                chimera::EdgeEngine::VolRegime regime = chimera::EdgeEngine::VolRegime::MEDIUM;
                // Find BTC D1 TSMOM engine to read its vol_ratio
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC && s.tf_secs == 86400 &&
                        s.engine->is_trend_following()) {
                        double vr = s.engine->vol_ratio_public();
                        if (vr > 0.0) {  // 0 = not enough data yet
                            if (vr < 0.5) regime = chimera::EdgeEngine::VolRegime::LOW;
                            else if (vr > 1.8) regime = chimera::EdgeEngine::VolRegime::HIGH;
                        }
                        break;
                    }
                }
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_vol_regime(regime);
                }
            }

            // ── Session 30, Edge 5: Cross-TF momentum score ──────────────────
            // For each symbol, check if D1 + H6 + H4 are all bullish.
            // Score: 0.0 (no agreement), 0.33 (one TF), 0.67 (two TFs), 1.0 (all three)
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                double cross_tf_scores[chimera::MAX_SYMBOLS]{};
                for (int sym = 0; sym < chimera::MAX_SYMBOLS; ++sym) {
                    int bullish_count = 0;
                    int tf_count = 0;
                    for (auto& s : g_slots) {
                        if (!s.engine) continue;
                        if (s.symbol_id != sym) continue;
                        if (!s.engine->is_trend_following()) continue;
                        // Only count D1, H6, H4 as cross-TF references
                        if (s.tf_secs == 86400 || s.tf_secs == 21600 || s.tf_secs == 14400) {
                            tf_count++;
                            if (s.engine->trend_bullish()) bullish_count++;
                        }
                    }
                    cross_tf_scores[sym] = (tf_count > 0) ?
                        (double)bullish_count / (double)tf_count : 0.0;
                }
                // Propagate scores to all engines
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_cross_tf_score(cross_tf_scores[s.symbol_id]);
                }

                // Edge 4: Dynamic sizing adjustment based on regime + conviction
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    double base_mult = s.engine->sizing_mult();
                    // Cap base at the statically-set value (don't compound)
                    if (base_mult > 2.0) base_mult = 2.0;
                    double dynamic_mult = base_mult;
                    // High conviction (cross-TF + funding) → boost 25%
                    if (s.engine->is_high_conviction()) {
                        dynamic_mult = base_mult * 1.25;
                    }
                    // High vol regime → reduce 25% (protect capital)
                    if (s.engine->vol_regime() == chimera::EdgeEngine::VolRegime::HIGH) {
                        dynamic_mult *= 0.75;
                    }
                    s.engine->set_sizing_mult(dynamic_mult);
                }
            }

            // ── Session 30, Edge 7: Liquidation cascade entry signals ───────���
            // When a cascade exhausts, boost the sizing mult for affected symbol
            // for the next 5 minutes (the dip-buy window). Also log prominently.
            {
                g_liq_detector.cleanup(now_ms);
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (int sym = 0; sym < chimera::MAX_SYMBOLS; ++sym) {
                    if (g_liq_detector.has_entry_signal(sym)) {
                        // Boost ALL engines for this symbol — cascade dip = high conviction
                        for (auto& s : g_slots) {
                            if (!s.engine || s.symbol_id != sym) continue;
                            double cur = s.engine->sizing_mult();
                            s.engine->set_sizing_mult(cur * 1.5);  // 50% boost for cascade dip
                        }
                        std::printf("[LIQ-ENTRY] %s: cascade entry boost active (sizing +50%%)\n",
                            chimera::SYM_SHORT[sym]);
                        std::fflush(stdout);
                        g_liq_detector.clear_entry_signal(sym);
                    }
                }
            }

            // ── Portfolio gate: max concurrent positions + drawdown breaker ──
            // Count open positions. If >= MAX_CONCURRENT, disable new entries.
            // Also check rolling drawdown: if total net_bp across all engines
            // over last 24h drops below DRAWDOWN_LIMIT_BP, halt all entries.
            {
                constexpr int MAX_CONCURRENT_POSITIONS = 5;    // TOP-5 LOCKDOWN: only 5 engines active, max 5 positions
                constexpr double DRAWDOWN_HALT_BP = -200.0;  // TOP-5 LOCKDOWN: tight drawdown halt (was -800)

                std::lock_guard<std::mutex> lk(g_engine_mtx);
                int open_positions = 0;
                for (auto& s : g_slots) {
                    if (s.engine && s.engine->in_position()) open_positions++;
                }

                // Check 24h rolling drawdown from trade log
                // IMPORTANT: Skip SHUTDOWN trades — these are bookkeeping from
                // service restarts, not real trading losses.
                double recent_pnl = 0.0;
                {
                    std::lock_guard<std::mutex> tlk(g_trades_mtx);
                    int64_t cutoff = now_ms - 86400000LL;  // 24h ago
                    for (int i = (int)g_trade_log.size() - 1; i >= 0; --i) {
                        if (g_trade_log[i].exit_ts_ms < cutoff) break;
                        if (g_trade_log[i].reason == "SHUTDOWN") continue;
                        recent_pnl += g_trade_log[i].net_bp;
                    }
                }

                // TOP-5 LOCKDOWN: unrealized P&L kill switch
                // Check total unrealized loss across all open positions.
                // If open positions are collectively losing more than threshold,
                // close the gate (no new entries). This catches the scenario where
                // the old system had -1629bp in open losses but the gate stayed open
                // because DRAWDOWN_HALT only checked *closed* trade P&L.
                constexpr double UNREALIZED_HALT_BP = -150.0;  // halt new entries if open loss > 150bp
                double total_unrealized_bp = 0.0;
                for (auto& s : g_slots) {
                    if (!s.engine || !s.engine->in_position()) continue;
                    double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                    if (spot > 0.0) {
                        // Compute unrealized P&L inline: (spot/entry - 1) * 10000bp
                        std::string snap = s.engine->position_snapshot_json(spot);
                        auto upos = snap.find("\"unreal_bp\":");
                        if (upos != std::string::npos) {
                            try { total_unrealized_bp += std::stod(snap.substr(upos + 13, 12)); }
                            catch (...) {}
                        }
                    }
                }

                bool gate_open = (open_positions < MAX_CONCURRENT_POSITIONS) &&
                                 (recent_pnl > DRAWDOWN_HALT_BP) &&
                                 (total_unrealized_bp > UNREALIZED_HALT_BP);

                for (auto& s : g_slots) {
                    if (s.engine) s.engine->set_portfolio_gate(gate_open);
                }

                // Log when gate closes
                static bool prev_gate = true;
                if (!gate_open && prev_gate) {
                    std::printf("[PORTFOLIO] GATE CLOSED: positions=%d (max=%d) | 24h_pnl=%+.1fbp (limit=%+.0f) | unrealized=%+.1fbp (limit=%+.0f)\n",
                        open_positions, MAX_CONCURRENT_POSITIONS, recent_pnl, DRAWDOWN_HALT_BP,
                        total_unrealized_bp, UNREALIZED_HALT_BP);
                    std::fflush(stdout);
                } else if (gate_open && !prev_gate) {
                    std::printf("[PORTFOLIO] GATE OPENED: positions=%d | 24h_pnl=%+.1fbp | unrealized=%+.1fbp\n",
                        open_positions, recent_pnl, total_unrealized_bp);
                    std::fflush(stdout);
                }
                prev_gate = gate_open;
            }

            // ── Correlation regime: compute rolling corr(alt, BTC) ───────────
            // Uses last-seen spot prices to compute a simple 1-tick return
            // correlation proxy. For full rolling correlation, we'd need bar-level
            // returns — but a simpler heuristic works: if all alts moved in the
            // same direction as BTC by a similar magnitude in recent ticks,
            // correlation is high. We use the D1 engine returns as proxy.
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                // Get BTC return (from D1 engine momentum if available)
                double btc_momentum = 0.0;
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC && s.tf_secs == 86400 &&
                        s.engine->is_trend_following()) {
                        // Use trend_bullish as a simplification — if BTC and alt
                        // are both in same trend direction, correlation is high.
                        // More sophisticated: track rolling returns. For now, use
                        // a simple heuristic based on D1 trend agreement.
                        std::string state = s.engine->state_json();
                        auto pos = state.find("\"momentum_pct\":");
                        if (pos != std::string::npos) {
                            try { btc_momentum = std::stod(state.substr(pos + 16, 10)); }
                            catch (...) {}
                        }
                        break;
                    }
                }

                // For each non-BTC symbol, check if their D1 momentum is highly
                // aligned with BTC (same sign, similar magnitude = high correlation)
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC) {
                        s.engine->set_corr_high(false);  // BTC never self-suppresses
                        continue;
                    }
                    // Simple correlation proxy: if BTC is moving strongly and
                    // this symbol has same-direction movement > 50% of BTC's move,
                    // flag as high correlation. This is a conservative heuristic.
                    // TODO: upgrade to true rolling Pearson correlation on bar returns.
                    bool corr_flag = false;
                    if (std::fabs(btc_momentum) > 5.0) {  // BTC moving > 5% (raised from 3% — shadow tuning: 3% moves are routine, suppressed too many alts)
                        // Check this symbol's D1 momentum
                        if (s.tf_secs == 86400 && s.engine->is_trend_following()) {
                            std::string state = s.engine->state_json();
                            auto pos = state.find("\"momentum_pct\":");
                            if (pos != std::string::npos) {
                                double sym_mom = 0.0;
                                try { sym_mom = std::stod(state.substr(pos + 16, 10)); }
                                catch (...) {}
                                // Same sign and magnitude > 50% of BTC
                                if ((btc_momentum > 0 && sym_mom > btc_momentum * 0.5) ||
                                    (btc_momentum < 0 && sym_mom < btc_momentum * 0.5)) {
                                    corr_flag = true;
                                }
                            }
                        }
                    }
                    s.engine->set_corr_high(corr_flag);
                }
            }
        }
    }

    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);

    // ── Graceful shutdown: write final snapshot for resume on next start ──
    // Instead of force-closing positions (which realises them at shutdown price),
    // we persist the full position state. On next start, resume_position() picks
    // them back up seamlessly. This means deploys/restarts don't interrupt trades.
    {
        std::lock_guard<std::mutex> lk(g_engine_mtx);
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int open_count = 0;
        std::ostringstream snap;
        snap << "{\"snapshot_ts\":" << now_ms << ",\"positions\":[";
        for (auto& s : g_slots) {
            if (!s.engine || !s.engine->in_position()) continue;
            double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
            std::string pj = s.engine->position_snapshot_json(spot);
            if (!pj.empty()) {
                if (open_count > 0) snap << ",";
                snap << pj;
                open_count++;
            }
        }
        snap << "],\"open_count\":" << open_count << "}\n";
        FILE* f = fopen("data/open_positions.json", "w");
        if (f) {
            std::string out = snap.str();
            fwrite(out.c_str(), 1, out.size(), f);
            fclose(f);
        }
        std::printf("[SHUTDOWN] Snapshot saved: %d open position(s) will resume on next start\n", open_count);
        std::fflush(stdout);
    }

    std::atomic<bool> shutdown_done{false};
    std::thread watchdog([&](){
        for (int i = 0; i < 30; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (shutdown_done) return;
        }
        release_instance_lock(); std::_Exit(0);
    });

    feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();

    int total_trades = 0;
    double total_bp = 0.0;
    for (auto& s : g_slots) {
        if (!s.engine) continue;
        total_trades += s.engine->trades();
        total_bp     += s.engine->total_bp();
    }
    std::printf("[SHUTDOWN] aggregate trades=%d  total=%+.1fbp\n", total_trades, total_bp);
    std::fflush(stdout);

    release_instance_lock();
    return 0;
}
