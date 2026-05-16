// ============================================================================
// Chimera — Tier-2 long-only longer-timeframe edges (rewrite 2026-05-11)
//
// After full OOS backtest + 12,096-combo parameter sweep (2026-05-16):
//   - BTC-TSMOM-D1 is the ONLY engine with validated edge
//   - Optimized params: lookback=10, hold=12, sl=3.0, trail=1.0/0.4
//   - OOS PF=1.92, Sharpe=1.67, +3924bp, 75% WR, 85% neighbour stability
//   - All other engines disabled (no OOS edge after costs)
//
//   instance         symbol     strategy     tf     OOS PF  OOS Sharpe
//   ------------------------------------------------------------------------
//   btc_tsmom_d1     BTCUSDT    tsmom        D1     1.92    1.67
//
// DISABLED (no OOS edge — configs preserved below for reference):
//   eth_bb_h6        ETHUSDT    bollinger    H6     0.72   -0.59
//   sol_donch_h6     SOLUSDT    donchian     H6     0.83   -0.57
//   xrp_donch_h1     XRPUSDT    donchian     H1     0.82   -1.19
//   link_rsi_h6      LINKUSDT   rsi_revert   H6     1.17    0.14  (4 trades)
//   btc_overnight_h1 BTCUSDT    overnight    H1     0.31   -4.91
//   btc_weekday_d1   BTCUSDT    weekday      D1     0.44   -1.86
//
// All instances spot-LONG-only, shadow_mode = true by default. Promote to live
// only after 4 weeks of paper trades match backtest WR/PF within +/- 10%.
//
// COLD-START SEEDING: After constructing the engines and before starting the
// live tick feed, we fetch the most recent N OHLC bars from Binance REST
// (/api/v3/klines, public endpoint) and pre-populate each engine's closed-bar
// deque. Without this, BTC-TSMOM-D1 would need ~20 calendar days to evaluate
// its first signal because it would have to build the lookback window from
// live ticks one bar at a time. With seeding, the engine is signal-ready on
// bar 1 of live data.
//
// HTTP GUI :8080
//   GET  /api/state2  -> {
//       "build":"<hash>",
//       "spot_prices":{ "<symbol>": <last_tick_px>, ... 8 symbols },
//       "engines":[ <one EdgeEngine state_json> x 1 ]
//   }
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

// Build the structured JSON for /api/state2.
//
// Schema:
//   {
//     "build":       "<git short hash>",
//     "spot_prices": { "btcusdt": <px>, "ethusdt": <px>, ... 8 entries },
//     "engines":     [ <state_json>, ... ]
//   }
//
// "spot_prices" is the live last-tick price for every symbol the WebSocket feed
// subscribes to (BTC/ETH/SOL/BNB/AVAX/LINK/XRP/DOGE). It is independent of
// engine bar accumulation, so the GUI can show real prices on first paint even
// when no bar has closed yet (engines start with bars_in_buffer=0 and
// last_close=0 until their first bar window completes — though with
// seed_bars() at startup, last_close is non-zero almost immediately).
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

        if (strstr(req, "POST /api/kill")) {
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
// Pulls `limit` historical bars matching the engine's timeframe from Binance,
// converts to EdgeEngine::SeedBar, and hands them off to the engine.
// Logs success/failure but never aborts startup — engine falls back to live-
// tick warm-up if REST is unreachable.
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
    std::printf("[STARTUP] Chimera — Tier-2 Edge Engines | build=%s\n", BUILD_VERSION);
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
    // BTC-TSMOM-D1 — OPTIMIZED (2026-05-16 parameter sweep)
    //
    // Sweep: 12,096 combos on BTC D1 2022-2026, 80/20 IS/OOS split, 17bp cost
    // Result: PF=1.92, Sharpe=1.67, +3924bp, 75% WR, 85% neighbour stability
    //
    // Changes from original:
    //   lookback:       20 → 10  (faster trend detection)
    //   hold_bars:      12 → 12  (unchanged — sweet spot)
    //   sl_atr_mult:    3.0 → 3.0 (unchanged)
    //   trail_arm_atr:  1.0 → 1.0 (unchanged)
    //   trail_dist_atr: 0.5 → 0.4 (tighter trail — locks in more profit)
    //   round_trip_bp:  10 → 17  (realistic Binance spot + BNB discount)
    // ══════════════════════════════════════════════════════════════════════
    chimera::EdgeEngine::Config btc_cfg{
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
    chimera::EdgeEngine btc_tsmom_d1(btc_cfg);

    // ══════════════════════════════════════════════════════════════════════
    // DISABLED ENGINES — No OOS edge after costs (2026-05-16 backtest)
    //
    // Configs preserved for reference. To re-enable, uncomment the config,
    // constructor, g_slots.push_back, and seed_engine_from_history lines.
    // ══════════════════════════════════════════════════════════════════════

    // ── ETH-BB-H6 — DISABLED (OOS PF=0.72, Sharpe=-0.59) ────────────────
    // chimera::EdgeEngine::Config eth_cfg{
    //     .symbol         = "ethusdt",
    //     .tag            = "ETH-BB-H6",
    //     .kind           = chimera::StrategyKind::BOLLINGER,
    //     .tf_secs        = 21600,
    //     .lookback       = 20,
    //     .hold_bars      = 12,
    //     .sl_atr_mult    = 2.5,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 10.0,
    //     .max_history    = 64,
    // };
    // chimera::EdgeEngine eth_bb_h6(eth_cfg);

    // ── SOL-DONCH-H6 — DISABLED (OOS PF=0.83, Sharpe=-0.57) ─────────────
    // chimera::EdgeEngine::Config sol_cfg{
    //     .symbol         = "solusdt",
    //     .tag            = "SOL-DONCH-H6",
    //     .kind           = chimera::StrategyKind::DONCHIAN,
    //     .tf_secs        = 21600,
    //     .lookback       = 20,
    //     .hold_bars      = 24,
    //     .sl_atr_mult    = 2.5,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 10.0,
    //     .max_history    = 64,
    // };
    // chimera::EdgeEngine sol_donch_h6(sol_cfg);

    // ── XRP-DONCH-H1 — DISABLED (OOS PF=0.82, Sharpe=-1.19) ─────────────
    // chimera::EdgeEngine::Config xrp_cfg{
    //     .symbol         = "xrpusdt",
    //     .tag            = "XRP-DONCH-H1",
    //     .kind           = chimera::StrategyKind::DONCHIAN,
    //     .tf_secs        = 3600,
    //     .lookback       = 20,
    //     .hold_bars      = 24,
    //     .sl_atr_mult    = 2.5,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 10.0,
    //     .max_history    = 64,
    // };
    // chimera::EdgeEngine xrp_donch_h1(xrp_cfg);

    // ── LINK-RSI-H6 — DISABLED (OOS PF=1.17 but only 4 trades) ──────────
    // chimera::EdgeEngine::Config link_cfg{
    //     .symbol         = "linkusdt",
    //     .tag            = "LINK-RSI-H6",
    //     .kind           = chimera::StrategyKind::RSI_REVERT,
    //     .tf_secs        = 21600,
    //     .lookback       = 20,
    //     .hold_bars      = 8,
    //     .sl_atr_mult    = 2.0,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 10.0,
    //     .max_history    = 64,
    // };
    // chimera::EdgeEngine link_rsi_h6(link_cfg);

    // ── BTC-OVERNIGHT-H1 — DISABLED (OOS PF=0.31, Sharpe=-4.91) ─────────
    // chimera::EdgeEngine::Config overnight_cfg{
    //     .symbol         = "btcusdt",
    //     .tag            = "BTC-OVERNIGHT-H1",
    //     .kind           = chimera::StrategyKind::OVERNIGHT,
    //     .tf_secs        = 3600,
    //     .lookback       = 20,
    //     .hold_bars      = 2,
    //     .sl_atr_mult    = 1.5,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 17.0,
    //     .max_history    = 64,
    //     .trail_arm_atr  = 0.8,
    //     .trail_dist_atr = 0.4,
    //     .entry_hour_utc = 21,
    // };
    // chimera::EdgeEngine btc_overnight_h1(overnight_cfg);

    // ── BTC-WEEKDAY-D1 — DISABLED (OOS PF=0.44, Sharpe=-1.86) ───────────
    // chimera::EdgeEngine::Config weekday_cfg{
    //     .symbol         = "btcusdt",
    //     .tag            = "BTC-WEEKDAY-D1",
    //     .kind           = chimera::StrategyKind::WEEKDAY,
    //     .tf_secs        = 86400,
    //     .lookback       = 20,
    //     .hold_bars      = 3,
    //     .sl_atr_mult    = 2.0,
    //     .atr_period     = 14,
    //     .bb_k           = 2.0,
    //     .rsi_threshold  = 30.0,
    //     .round_trip_bp  = 17.0,
    //     .max_history    = 64,
    //     .trail_arm_atr  = 1.0,
    //     .trail_dist_atr = 0.5,
    //     .entry_hour_utc = 21,
    //     .entry_dow      = 1,
    //     .sma_len        = 5,
    // };
    // chimera::EdgeEngine btc_weekday_d1(weekday_cfg);

    // ══════════════════════════════════════════════════════════════════════
    // Register active engines only
    // ══════════════════════════════════════════════════════════════════════
    g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_d1});

    // ── Seed engine bar buffer from Binance REST klines ────────────────────
    {
        chimera::BinanceREST seed_rest;     // dedicated read-only client; no creds needed
        std::printf("[STARTUP] Seeding engine bar buffers from Binance REST klines...\n");
        std::fflush(stdout);

        seed_engine_from_history(seed_rest, btc_tsmom_d1,
                                 btc_cfg.symbol,  btc_cfg.tf_secs,  btc_cfg.tag,  64);

        std::printf("[STARTUP] Seeding complete.\n");
        std::fflush(stdout);
    }

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

    std::printf("[STARTUP] Spot feed live. 1 engine running (shadow_mode=true):\n");
    std::printf("[STARTUP]   BTC  tsmom  D1  (lookback=10, hold=12, sl=3.0*atr, trail=1.0/0.4*atr, cost=17bp)\n");
    std::printf("[STARTUP]   OOS PF=1.92, Sharpe=1.67, +3924bp, 75%% WR, 85%% neighbour stability\n");
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
