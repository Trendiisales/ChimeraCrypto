// ============================================================================
// Chimera — Tier-2 long-only longer-timeframe edges (rewrite 2026-05-11)
//
// After full OOS backtest + multi-symbol parameter sweep (2026-05-16/17):
//   - 4 existing D1 engines (Session 13)
//   - 10 NEW H4 engines (Session 14 — new timeframe discovery)
//   - 3 NEW H12 engines (Session 14)
//   - 1 NEW D1 engine: BNB (Session 14)
//   - Total: 18 engines, all shadow mode, all TSMOM, all long-only
//
// SESSION 14 DISCOVERY:
//   H4 and H12 timeframes were never tested before Session 14.
//   Scanner v2 found 61% of H12 combos and 51% of H4 combos profitable
//   (vs only 6.9% on D1). Full optimizer runs confirmed with neighbourhood
//   stability >= 83% on every deployed engine.
//
//   instance              symbol     strat   tf    OOS PF  OOS Shrp  Nbr%  Trades
//   ──────────────────────────────────────────────────────────────────────────────
//   EXISTING D1 ENGINES (Session 13):
//   btc_tsmom_d1          BTCUSDT    TSMOM   D1    1.92    1.67      85%     24
//   eth_tsmom_d1          ETHUSDT    TSMOM   D1    3.15    3.17      91%     26
//   sol_tsmom_d1          SOLUSDT    TSMOM   D1    2.25    2.41      89%     15
//   link_tsmom_d1         LINKUSDT   TSMOM   D1    2.18    1.92     100%     23
//
//   NEW H4 ENGINES (Session 14):
//   xrp_tsmom_h4          XRPUSDT    TSMOM   H4    2.43    5.80     100%    267
//   bnb_tsmom_h4          BNBUSDT    TSMOM   H4    1.91    3.79     100%    291
//   link_tsmom_h4         LINKUSDT   TSMOM   H4    1.91    4.07      95%    205
//   sol_tsmom_h4          SOLUSDT    TSMOM   H4    1.89    3.82     100%    208
//   btc_tsmom_h4          BTCUSDT    TSMOM   H4    1.82    3.54     100%    167
//   eth_tsmom_h4          ETHUSDT    TSMOM   H4    1.76    3.26     100%    196
//
//   NEW H12 ENGINES (Session 14):
//   btc_tsmom_h12         BTCUSDT    TSMOM   H12   3.63    3.40      96%     31
//   doge_tsmom_h12        DOGEUSDT   TSMOM   H12   2.78    3.66     100%     82
//   avax_tsmom_h12        AVAXUSDT   TSMOM   H12   2.61    2.98      87%     76
//
//   NEW D1 ENGINE (Session 14):
//   bnb_tsmom_d1          BNBUSDT    TSMOM   D1    3.16    2.91      90%     32
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
// deque. H4 = "4h", H12 = "12h", D1 = "1d" — all valid Binance intervals.
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
};

static std::vector<EngineSlot>  g_slots;
static std::mutex               g_engine_mtx;

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

// Build the structured JSON for /api/state2.
static std::string build_state_json() {
    std::ostringstream js;
    js << "{\"build\":\"" << BUILD_VERSION << "\",";

    // ── spot_prices ─────────────────────────────────────────────────────────
    js << "\"spot_prices\":{";
    js << std::fixed << std::setprecision(6);
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
        if (i > 0) js << ",";
        double px = load_dbl_atomic(g_last_spot_px_bits[i]);
        js << "\"" << chimera::sym_full(i) << "\":" << px;
    }
    js << "},";

    // ── engines ─────────────────────────────────────────────────────────────
    js << "\"engines\":[";
    for (size_t i = 0; i < g_slots.size(); ++i) {
        if (i > 0) js << ",";
        if (g_slots[i].engine) js << g_slots[i].engine->state_json();
        else js << "null";
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

        if (strstr(req, "GET /api/trades")) {
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

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    std::printf("[STARTUP] Chimera — Tier-2 Edge Engines (18 active) | build=%s\n", BUILD_VERSION);
    std::fflush(stdout);

    acquire_instance_lock();
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Executor — kept for the day we flip shadow_mode off. Engines run in
    // shadow until then and never call the executor.
    chimera::SpotExecutor executor;
    bool exec_ok = executor.init("config/binance_credentials.json");
    if (!exec_ok) {
        std::fprintf(stderr, "[STARTUP] WARNING: executor init failed — shadow mode only\n");
    }
    (void)exec_ok;

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION A: EXISTING D1 ENGINES (Session 13) ─────────────────────
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
    };
    chimera::EdgeEngine btc_tsmom_d1(btc_d1_cfg);
    btc_tsmom_d1.set_on_trade(on_trade_callback);

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
    };
    chimera::EdgeEngine eth_tsmom_d1(eth_d1_cfg);
    eth_tsmom_d1.set_on_trade(on_trade_callback);

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
    };
    chimera::EdgeEngine sol_tsmom_d1(sol_d1_cfg);
    sol_tsmom_d1.set_on_trade(on_trade_callback);

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
    };
    chimera::EdgeEngine link_tsmom_d1(link_d1_cfg);
    link_tsmom_d1.set_on_trade(on_trade_callback);

    // ENGINE A5: BNB-TSMOM-D1 — NEW Session 14 — PF=3.16, Sharpe=2.91, Nbr=90%
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
    };
    chimera::EdgeEngine bnb_tsmom_d1(bnb_d1_cfg);
    bnb_tsmom_d1.set_on_trade(on_trade_callback);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION B: NEW H4 ENGINES (Session 14 — never tested before) ────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE B1: XRP-TSMOM-H4 — PF=2.43, Sharpe=5.80, 267 trades, Nbr=100%
    // 98.7% of ALL parameter combos profitable — strongest H4 edge found
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
    };
    chimera::EdgeEngine xrp_tsmom_h4(xrp_h4_cfg);
    xrp_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B2: BNB-TSMOM-H4 — PF=1.91, Sharpe=3.79, 291 trades, Nbr=100%
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
    };
    chimera::EdgeEngine bnb_tsmom_h4(bnb_h4_cfg);
    bnb_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B3: LINK-TSMOM-H4 — PF=1.91, Sharpe=4.07, 205 trades, Nbr=95%
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
    };
    chimera::EdgeEngine link_tsmom_h4(link_h4_cfg);
    link_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B4: SOL-TSMOM-H4 — PF=1.89, Sharpe=3.82, 208 trades, Nbr=100%
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
    };
    chimera::EdgeEngine sol_tsmom_h4(sol_h4_cfg);
    sol_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B5: BTC-TSMOM-H4 — PF=1.82, Sharpe=3.54, 167 trades, Nbr=100%
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
    };
    chimera::EdgeEngine btc_tsmom_h4(btc_h4_cfg);
    btc_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B6: ETH-TSMOM-H4 — PF=1.76, Sharpe=3.26, 196 trades, Nbr=100%
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
    };
    chimera::EdgeEngine eth_tsmom_h4(eth_h4_cfg);
    eth_tsmom_h4.set_on_trade(on_trade_callback);

    // ENGINE B7: AVAX-TSMOM-H4 — PF=1.47, Sharpe=2.17, 231 trades, Nbr=83%
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
    };
    chimera::EdgeEngine avax_tsmom_h4(avax_h4_cfg);
    avax_tsmom_h4.set_on_trade(on_trade_callback);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION C: NEW H12 ENGINES (Session 14 — never tested before) ───
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE C1: BTC-TSMOM-H12 — PF=3.63, Sharpe=3.40, 31 trades, Nbr=96%
    // 94.6% of ALL parameter combos profitable — most robust edge in the system
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
    };
    chimera::EdgeEngine btc_tsmom_h12(btc_h12_cfg);
    btc_tsmom_h12.set_on_trade(on_trade_callback);

    // ENGINE C2: DOGE-TSMOM-H12 — PF=2.78, Sharpe=3.66, 82 trades, Nbr=100%
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
    };
    chimera::EdgeEngine doge_tsmom_h12(doge_h12_cfg);
    doge_tsmom_h12.set_on_trade(on_trade_callback);

    // ENGINE C3: AVAX-TSMOM-H12 — PF=2.61, Sharpe=2.98, 76 trades, Nbr=87%
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
    };
    chimera::EdgeEngine avax_tsmom_h12(avax_h12_cfg);
    avax_tsmom_h12.set_on_trade(on_trade_callback);

    // ══════════════════════════════════════════════════════════════════════
    // DISABLED ENGINES — No OOS edge after costs (Sessions 13-14)
    // ══════════════════════════════════════════════════════════════════════

    // ── ETH-BB-H6 — DISABLED (OOS PF=0.72, Sharpe=-0.59) ────────────────
    // chimera::EdgeEngine::Config eth_bb_cfg{
    //     .symbol = "ethusdt", .tag = "ETH-BB-H6",
    //     .kind = chimera::StrategyKind::BOLLINGER, .tf_secs = 21600,
    //     .lookback = 20, .hold_bars = 12, .sl_atr_mult = 2.5,
    //     .atr_period = 14, .bb_k = 2.0, .rsi_threshold = 30.0,
    //     .round_trip_bp = 10.0, .max_history = 64,
    // };

    // ── SOL-DONCH-H6 — DISABLED (OOS PF=0.83, Sharpe=-0.57) ─────────────
    // chimera::EdgeEngine::Config sol_donch_cfg{
    //     .symbol = "solusdt", .tag = "SOL-DONCH-H6",
    //     .kind = chimera::StrategyKind::DONCHIAN, .tf_secs = 21600,
    //     .lookback = 20, .hold_bars = 24, .sl_atr_mult = 2.5,
    //     .atr_period = 14, .bb_k = 2.0, .rsi_threshold = 30.0,
    //     .round_trip_bp = 10.0, .max_history = 64,
    // };

    // ── XRP-DONCH-H1 — DISABLED (OOS PF=0.82, Sharpe=-1.19) ─────────────
    // ── LINK-RSI-H6 — DISABLED (OOS PF=1.17 but only 4 trades) ──────────
    // ── BTC-OVERNIGHT-H1 — DISABLED (OOS PF=0.31, Sharpe=-4.91) ─────────
    // ── BTC-WEEKDAY-D1 — DISABLED (OOS PF=0.44, Sharpe=-1.86) ───────────
    // ── DOGE-TSMOM-H4 — DISABLED (PF=1.28 but Nbr=49% — isolated peak) ─

    // ══════════════════════════════════════════════════════════════════════
    // Register all active engines
    // ══════════════════════════════════════════════════════════════════════

    // D1 engines
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_d1});
    g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_d1});
    g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_d1});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d1});
    g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_d1});

    // H4 engines
    g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h4});
    g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h4});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h4});
    g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h4});
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h4});
    g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h4});
    g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h4});

    // H12 engines
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h12});
    g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h12});
    g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h12});

    // ── Seed engine bar buffers from Binance REST klines ────────────────────
    {
        chimera::BinanceREST seed_rest;     // dedicated read-only client; no creds needed
        std::printf("[STARTUP] Seeding 18 engine bar buffers from Binance REST klines...\n");
        std::fflush(stdout);

        // D1 engines
        seed_engine_from_history(seed_rest, btc_tsmom_d1,
                                 btc_d1_cfg.symbol,  btc_d1_cfg.tf_secs,  btc_d1_cfg.tag,  64);
        seed_engine_from_history(seed_rest, eth_tsmom_d1,
                                 eth_d1_cfg.symbol,  eth_d1_cfg.tf_secs,  eth_d1_cfg.tag,  64);
        seed_engine_from_history(seed_rest, sol_tsmom_d1,
                                 sol_d1_cfg.symbol,  sol_d1_cfg.tf_secs,  sol_d1_cfg.tag,  64);
        seed_engine_from_history(seed_rest, link_tsmom_d1,
                                 link_d1_cfg.symbol, link_d1_cfg.tf_secs, link_d1_cfg.tag, 64);
        seed_engine_from_history(seed_rest, bnb_tsmom_d1,
                                 bnb_d1_cfg.symbol,  bnb_d1_cfg.tf_secs,  bnb_d1_cfg.tag,  64);

        // H4 engines
        seed_engine_from_history(seed_rest, xrp_tsmom_h4,
                                 xrp_h4_cfg.symbol,  xrp_h4_cfg.tf_secs,  xrp_h4_cfg.tag,  64);
        seed_engine_from_history(seed_rest, bnb_tsmom_h4,
                                 bnb_h4_cfg.symbol,  bnb_h4_cfg.tf_secs,  bnb_h4_cfg.tag,  64);
        seed_engine_from_history(seed_rest, link_tsmom_h4,
                                 link_h4_cfg.symbol, link_h4_cfg.tf_secs, link_h4_cfg.tag, 64);
        seed_engine_from_history(seed_rest, sol_tsmom_h4,
                                 sol_h4_cfg.symbol,  sol_h4_cfg.tf_secs,  sol_h4_cfg.tag,  64);
        seed_engine_from_history(seed_rest, btc_tsmom_h4,
                                 btc_h4_cfg.symbol,  btc_h4_cfg.tf_secs,  btc_h4_cfg.tag,  64);
        seed_engine_from_history(seed_rest, eth_tsmom_h4,
                                 eth_h4_cfg.symbol,  eth_h4_cfg.tf_secs,  eth_h4_cfg.tag,  64);
        seed_engine_from_history(seed_rest, avax_tsmom_h4,
                                 avax_h4_cfg.symbol, avax_h4_cfg.tf_secs, avax_h4_cfg.tag, 64);

        // H12 engines
        seed_engine_from_history(seed_rest, btc_tsmom_h12,
                                 btc_h12_cfg.symbol,  btc_h12_cfg.tf_secs,  btc_h12_cfg.tag,  64);
        seed_engine_from_history(seed_rest, doge_tsmom_h12,
                                 doge_h12_cfg.symbol, doge_h12_cfg.tf_secs, doge_h12_cfg.tag, 64);
        seed_engine_from_history(seed_rest, avax_tsmom_h12,
                                 avax_h12_cfg.symbol, avax_h12_cfg.tf_secs, avax_h12_cfg.tag, 64);

        std::printf("[STARTUP] Seeding complete.\n");
        std::fflush(stdout);
    }

    // ── Trade journal: ensure data dir exists and load history ──────────
    ::mkdir("data", 0755);  // no-op if exists
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

    std::thread http_thread(http_server_thread, 8080);
    http_thread.detach();

    // ── Spot WebSocket feed (subscribes to all 8 symbols — keeps the feed
    //    config matched to SymbolIndex for GUI spot price strip). ──────────
    chimera::BinanceWSFeed feed;
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

    std::printf("[STARTUP] Spot feed live. 18 engines running (shadow_mode=true):\n");
    std::printf("[STARTUP]   ── D1 engines (5) ──────────────────────────────────────────\n");
    std::printf("[STARTUP]   BTC   TSMOM  D1   LB=10  HB=12 SL=3.0  T=1.0/0.4  cost=17bp  PF=1.92 Shrp=1.67 Nbr= 85%%\n");
    std::printf("[STARTUP]   ETH   TSMOM  D1   LB=25  HB=8  SL=2.5  T=0.8/0.4  cost=17bp  PF=3.15 Shrp=3.17 Nbr= 91%%\n");
    std::printf("[STARTUP]   SOL   TSMOM  D1   LB=10  HB=20 SL=2.0  T=0.5/0.3  cost=20bp  PF=2.25 Shrp=2.41 Nbr= 89%%\n");
    std::printf("[STARTUP]   LINK  TSMOM  D1   LB=40  HB=20 SL=2.0  T=1.0/0.8  cost=22bp  PF=2.18 Shrp=1.92 Nbr=100%%\n");
    std::printf("[STARTUP]   BNB   TSMOM  D1   LB=10  HB=20 SL=2.5  T=1.2/0.4  cost=20bp  PF=3.16 Shrp=2.91 Nbr= 90%%  ← NEW\n");
    std::printf("[STARTUP]   ── H4 engines (7) — NEW TIMEFRAME ─────────────────────────\n");
    std::printf("[STARTUP]   XRP   TSMOM  H4   LB=30  HB=20 SL=1.5  T=0.5/0.4  cost=20bp  PF=2.43 Shrp=5.80 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   BNB   TSMOM  H4   LB=40  HB=16 SL=3.0  T=0.5/0.3  cost=20bp  PF=1.91 Shrp=3.79 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   LINK  TSMOM  H4   LB=30  HB=24 SL=2.5  T=0.5/0.3  cost=22bp  PF=1.91 Shrp=4.07 Nbr= 95%%  ← NEW\n");
    std::printf("[STARTUP]   SOL   TSMOM  H4   LB=40  HB=12 SL=3.0  T=0.5/0.3  cost=20bp  PF=1.89 Shrp=3.82 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   BTC   TSMOM  H4   LB=25  HB=16 SL=4.0  T=0.8/0.3  cost=17bp  PF=1.82 Shrp=3.54 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   ETH   TSMOM  H4   LB=40  HB=24 SL=3.0  T=0.5/0.4  cost=17bp  PF=1.76 Shrp=3.26 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   AVAX  TSMOM  H4   LB=40  HB=24 SL=2.5  T=0.5/0.4  cost=22bp  PF=1.47 Shrp=2.17 Nbr= 83%%  ← NEW\n");
    std::printf("[STARTUP]   ── H12 engines (3) — NEW TIMEFRAME ────────────────────────\n");
    std::printf("[STARTUP]   BTC   TSMOM  H12  LB=15  HB=24 SL=4.0  T=1.2/0.8  cost=17bp  PF=3.63 Shrp=3.40 Nbr= 96%%  ← NEW\n");
    std::printf("[STARTUP]   DOGE  TSMOM  H12  LB=35  HB=24 SL=2.0  T=0.5/0.4  cost=22bp  PF=2.78 Shrp=3.66 Nbr=100%%  ← NEW\n");
    std::printf("[STARTUP]   AVAX  TSMOM  H12  LB=30  HB=12 SL=4.0  T=0.5/0.4  cost=22bp  PF=2.61 Shrp=2.98 Nbr= 87%%  ← NEW\n");
    std::printf("[STARTUP] GUI: http://localhost:8080\n");
    std::fflush(stdout);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);

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
