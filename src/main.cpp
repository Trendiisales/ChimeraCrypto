// ============================================================================
// Chimera — H4/D1 Swing Engine + 8 paper-trading parallel engines
//          (Move 2 + Phase 1 + Multi-Day Trio + High-Freq Range MR)
//
// Strategies running concurrently (all spot-LONG-only):
//   1. SwingEngine  v9                     — H4 Donchian breakout, ETH-only (live shadow)
//   2. FundingWindowEngine                 — pre-funding basis snap-back, BTC + ETH
//   3. BasisMomentumEngine                 — perp→spot lead-lag, BTC + ETH
//   4. OrderbookImbalanceEngine            — short-term mean-reversion, BTC + ETH
//   5. EthBtcLeadLagEngine                 — Phase 1 ETH→BTC 1-3min lead-lag, BTC spot long
//                                             (spot-only; perp-feed-independent)
//   6. CoinbasePremiumMRevEngine           — Multi-day BTC mean reversion via
//                                             Coinbase-Binance premium (3-10 day hold)
//   7. FundingPersistenceFadeEngine        — Multi-day BTC mean reversion via sustained
//                                             negative funding (3-7 day hold; needs perp data)
//   8. VolCompressionBreakoutEngine        — 24h vol-squeeze + Donchian-24h breakout
//                                             (8-72h hold, BTC spot long)
//   9. RangeMeanReversionEngine            — 30-min Bollinger + RSI(14) high-freq mean
//                                             reversion, BTC + ETH (5-20 fires/sym/day)
//
// Engines 2-9 are paper-only via printf log lines. None have executors wired.
// SwingEngine alone has SpotExecutor wiring (and is in shadow_mode = true).
//
// Feeds:
//   spot WS   (BinanceWSFeed)              — bookTicker + aggTrade + depth5 (8 symbols)
//   spot WS   (CoinbaseWSFeed, BTC-USD)    — single-leg cross-venue reference for engine #6
//   perp WS   (PerpFeed)                   — markPrice + aggTrade for funding/basis/flow
//
// HTTP GUI :8080
//   GET  /api/state   → SwingEngine state_json (unchanged for legacy dashboard)
//   GET  /api/state2  → {"funding_window":[...],"basis_momentum":[...],
//                         "obi":[...],"eth_btc_leadlag":{...},
//                         "coinbase_premium_mrev":{...},
//                         "funding_persistence_fade":{...},
//                         "vol_compression_breakout":{...},
//                         "range_mean_reversion":[...]}
//   POST /api/kill    → kill_all on every engine (Swing + 8 paper)
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <mutex>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "live/BinanceWSFeed.hpp"
#include "live/CoinbaseWSFeed.hpp"
#include "live/PerpFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "core/SwingEngine.hpp"
#include "core/FundingWindowEngine.hpp"
#include "core/BasisMomentumEngine.hpp"
#include "core/OrderbookImbalanceEngine.hpp"
#include "core/EthBtcLeadLagEngine.hpp"
#include "core/CoinbasePremiumMRevEngine.hpp"
#include "core/FundingPersistenceFadeEngine.hpp"
#include "core/VolCompressionBreakoutEngine.hpp"
#include "core/RangeMeanReversionEngine.hpp"
#include "core/SymbolIndex.hpp"

#include "version_generated.hpp"
#include "execution/ExchangeLatencyEngine.hpp"

// Required by BinanceWSFeed.cpp (extern declaration)
chimera::ExchangeLatencyEngine g_exchange_latency;
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

// ── Tradable symbol set for the perp-aware paper engines ─────────────────────
// FundingWindow / BasisMomentum / OBI / RangeMeanReversion all run on BTC + ETH
// per their headers. Easy to expand later (need PerpFeed coverage for FW/BM and
// just spot for RMR on the new symbol).
static constexpr int PAPER_NUM_SYMBOLS = 2;
static constexpr int PAPER_SYMBOL_IDS[PAPER_NUM_SYMBOLS] = {
    chimera::SYM_BTC,   // 0
    chimera::SYM_ETH    // 1
};

// ── Single-instance lock ─────────────────────────────────────────────────────
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) { std::fprintf(stderr, "[FATAL] Cannot open lock file\n"); std::exit(1); }
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        char buf[32] = {};
        if (::read(g_lock_fd, buf, sizeof(buf)-1) < 0) { /* ignore — best-effort PID read */ }
        ::close(g_lock_fd);
        std::fprintf(stderr, "[FATAL] Chimera already running (PID %s)\n", buf); std::exit(1);
    }
    if (::ftruncate(g_lock_fd, 0) != 0) { /* ignore — best-effort truncate */ }
    char pidbuf[32]; int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    if (::write(g_lock_fd, pidbuf, len) < 0) { /* ignore — best-effort PID write */ }
    ::fsync(g_lock_fd);
    std::printf("[STARTUP] Instance lock acquired PID=%d\n", (int)::getpid());
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN); ::close(g_lock_fd); ::unlink(PID_LOCK_FILE); g_lock_fd = -1;
    }
}

// ── Shared engine pointers / mutex for HTTP server ───────────────────────────
// g_engine_mtx now protects SwingEngine + every paper engine (arrays + each
// singleton — Phase 1 EthBtcLeadLag, plus the multi-day trio, plus the new
// per-symbol high-frequency Range Mean Reversion array).
static chimera::SwingEngine*                    g_engine_ptr      = nullptr;
static std::mutex                               g_engine_mtx;
static chimera::SwingEngine*                    g_engine_ptr_kill = nullptr;
static chimera::FundingWindowEngine*            g_fwes_ptr        = nullptr;
static chimera::BasisMomentumEngine*            g_bmes_ptr        = nullptr;
static chimera::OrderbookImbalanceEngine*       g_obes_ptr        = nullptr;
static chimera::EthBtcLeadLagEngine*            g_ellaye_ptr      = nullptr;
static chimera::CoinbasePremiumMRevEngine*      g_cbprem_ptr      = nullptr;
static chimera::FundingPersistenceFadeEngine*   g_fpfe_ptr        = nullptr;
static chimera::VolCompressionBreakoutEngine*   g_vcbe_ptr        = nullptr;
static chimera::RangeMeanReversionEngine*       g_rmre_ptr        = nullptr;
static chimera::PerpFeed*                       g_perp_feed_ptr   = nullptr;

// Last-seen Coinbase BTC-USD price (atomic so the GUI / state_json can read it
// without taking g_engine_mtx). Updated from the Coinbase WS callback.
static std::atomic<uint64_t> g_last_cb_btc_px_bits{0};

// Last-seen spot mid per symbol — used by kill_all to flatten paper positions
// at a current price (the paper engines don't store the latest price on their own).
static std::atomic<uint64_t> g_last_spot_px_bits[chimera::MAX_SYMBOLS]{};

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
    if (got != (size_t)sz) buf.resize(got);  // partial read — keep what we got
    fclose(f);
    return buf;
}

static std::string gui_root;

// Build the structured JSON for /api/state2 — one array per perp-aware paper
// engine, plus a singleton object for each singleton engine, plus the new
// per-symbol range_mean_reversion array.
static std::string build_paper_state_json() {
    std::ostringstream js;
    js << "{";

    // ── funding_window ──
    js << "\"funding_window\":[";
    if (g_fwes_ptr) {
        for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            const int id = PAPER_SYMBOL_IDS[i];
            double spot  = load_dbl_atomic(g_last_spot_px_bits[id]);
            double frate = 0.0, basis = 0.0;
            if (g_perp_feed_ptr && g_perp_feed_ptr->ready(id)) {
                frate = g_perp_feed_ptr->funding_rate(id);
                basis = g_perp_feed_ptr->basis_bp(id, spot);
            }
            js << g_fwes_ptr[i].state_json(frate, basis, spot);
        }
    }
    js << "],";

    // ── basis_momentum ──
    js << "\"basis_momentum\":[";
    if (g_bmes_ptr) {
        for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            const int id = PAPER_SYMBOL_IDS[i];
            double spot  = load_dbl_atomic(g_last_spot_px_bits[id]);
            double basis = 0.0, flow = 0.0;
            if (g_perp_feed_ptr && g_perp_feed_ptr->ready(id)) {
                basis = g_perp_feed_ptr->basis_bp(id, spot);
                flow  = g_perp_feed_ptr->perp_flow_ratio(id);
            }
            js << g_bmes_ptr[i].state_json(basis, flow, spot);
        }
    }
    js << "],";

    // ── obi (OrderbookImbalance) ──
    js << "\"obi\":[";
    if (g_obes_ptr) {
        for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            const int id = PAPER_SYMBOL_IDS[i];
            double spot  = load_dbl_atomic(g_last_spot_px_bits[id]);
            double basis = 0.0;
            if (g_perp_feed_ptr && g_perp_feed_ptr->ready(id)) {
                basis = g_perp_feed_ptr->basis_bp(id, spot);
            }
            // Note: book_imbalance and spread_bps come from the latest tick,
            // which we don't cache outside the SwingEngine. So we report 0 here
            // — accurate values are in the [OBI-ENTRY] log lines.
            js << g_obes_ptr[i].state_json(/*book_imbalance=*/0.0,
                                            /*spread_bps=*/0.0,
                                            /*perp_basis_bp=*/basis,
                                            /*spot_price=*/spot);
        }
    }
    js << "],";

    // ── eth_btc_leadlag (Phase 1 spot-only engine, singleton) ──
    js << "\"eth_btc_leadlag\":";
    if (g_ellaye_ptr) {
        double btc_px = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
        double eth_px = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_ETH]);
        js << g_ellaye_ptr->state_json(btc_px, eth_px);
    } else {
        js << "{}";
    }
    js << ",";

    // ── coinbase_premium_mrev (Strategy A — multi-day, BTC singleton) ──
    js << "\"coinbase_premium_mrev\":";
    if (g_cbprem_ptr) {
        double btc_px = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
        double cb_px  = load_dbl_atomic(g_last_cb_btc_px_bits);
        js << g_cbprem_ptr->state_json(btc_px, cb_px);
    } else {
        js << "{}";
    }
    js << ",";

    // ── funding_persistence_fade (Strategy B — multi-day, BTC singleton) ──
    js << "\"funding_persistence_fade\":";
    if (g_fpfe_ptr) {
        double btc_px = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
        double frate  = 0.0;
        if (g_perp_feed_ptr && g_perp_feed_ptr->ready(chimera::SYM_BTC)) {
            frate = g_perp_feed_ptr->funding_rate(chimera::SYM_BTC);
        }
        js << g_fpfe_ptr->state_json(btc_px, frate);
    } else {
        js << "{}";
    }
    js << ",";

    // ── vol_compression_breakout (Strategy C — 8-72h, BTC singleton) ──
    js << "\"vol_compression_breakout\":";
    if (g_vcbe_ptr) {
        double btc_px = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
        js << g_vcbe_ptr->state_json(btc_px);
    } else {
        js << "{}";
    }
    js << ",";

    // ── range_mean_reversion (NEW — high-frequency, BTC + ETH per-symbol) ──
    js << "\"range_mean_reversion\":[";
    if (g_rmre_ptr) {
        for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
            if (i > 0) js << ",";
            const int id = PAPER_SYMBOL_IDS[i];
            double spot  = load_dbl_atomic(g_last_spot_px_bits[id]);
            js << g_rmre_ptr[i].state_json(spot);
        }
    }
    js << "]";

    js << "}";
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
        if (read(client, req, sizeof(req)-1) < 0) { /* ignore — req stays zero-initialised */ }

        std::string body;
        const char* ct = "application/json";
        int status = 200;

        if (strstr(req, "POST /api/kill")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            if (g_engine_ptr_kill) g_engine_ptr_kill->kill_all();
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
                const int id = PAPER_SYMBOL_IDS[i];
                double spot  = load_dbl_atomic(g_last_spot_px_bits[id]);
                if (g_fwes_ptr) g_fwes_ptr[i].kill_all(spot, now_ms);
                if (g_bmes_ptr) g_bmes_ptr[i].kill_all(spot, now_ms);
                if (g_obes_ptr) g_obes_ptr[i].kill_all(spot, now_ms);
                if (g_rmre_ptr) g_rmre_ptr[i].kill_all(spot, now_ms);
            }
            // Singleton engines (BTC-leg trades only) — kill with current BTC spot.
            const double btc_spot = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
            if (g_ellaye_ptr) g_ellaye_ptr->kill_all(btc_spot, now_ms);
            if (g_cbprem_ptr) g_cbprem_ptr->kill_all(btc_spot, now_ms);
            if (g_fpfe_ptr)   g_fpfe_ptr->kill_all(btc_spot, now_ms);
            if (g_vcbe_ptr)   g_vcbe_ptr->kill_all(btc_spot, now_ms);
            body = "{\"ok\":true}";
        } else if (strstr(req, "GET /api/state2")) {
            // NOTE: must be checked BEFORE /api/state (substring match).
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            body = build_paper_state_json();
        } else if (strstr(req, "GET /api/state")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            body = g_engine_ptr ? g_engine_ptr->state_json() : "{}";
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
        if (write(client, s.c_str(), s.size()) < 0) { /* ignore — client may have disconnected */ }
        close(client);
    }
    close(server_fd);
}

// ── Signal handler ────────────────────────────────────────────────────────────
static std::atomic<bool> g_running{true};
void signal_handler(int) { g_running = false; }

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    std::printf("[STARTUP] Chimera — Swing + 8 paper engines (Move 2 + Phase 1 + Multi-Day Trio + Range MR) | build=%s\n",
                BUILD_VERSION);
    std::fflush(stdout);

    acquire_instance_lock();
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Executor (spot only — shared with SwingEngine; the paper engines never call it)
    chimera::SpotExecutor executor;
    bool exec_ok = executor.init("config/binance_credentials.json");
    if (!exec_ok) {
        std::fprintf(stderr, "[STARTUP] WARNING: executor init failed — shadow mode only\n");
    }

    // ── Engine #1: SwingEngine v9 (H4 Donchian, ETH-only, live shadow) ──────
    chimera::SwingEngine engine;
    engine.shadow_mode = true;
    if (exec_ok) engine.set_executor(&executor);
    g_engine_ptr      = &engine;
    g_engine_ptr_kill = &engine;

    // ── Engine #2: FundingWindow on BTC + ETH (paper) ───────────────────────
    chimera::FundingWindowEngine fwes[PAPER_NUM_SYMBOLS] = {
        chimera::FundingWindowEngine(chimera::sym_full(chimera::SYM_BTC)),
        chimera::FundingWindowEngine(chimera::sym_full(chimera::SYM_ETH))
    };
    for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) fwes[i].shadow_mode = true;
    g_fwes_ptr = fwes;

    // ── Engine #3: BasisMomentum on BTC + ETH (paper) ───────────────────────
    chimera::BasisMomentumEngine bmes[PAPER_NUM_SYMBOLS] = {
        chimera::BasisMomentumEngine(chimera::sym_full(chimera::SYM_BTC)),
        chimera::BasisMomentumEngine(chimera::sym_full(chimera::SYM_ETH))
    };
    for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) bmes[i].shadow_mode = true;
    g_bmes_ptr = bmes;

    // ── Engine #4: OrderbookImbalance on BTC + ETH (paper) ──────────────────
    chimera::OrderbookImbalanceEngine obes[PAPER_NUM_SYMBOLS] = {
        chimera::OrderbookImbalanceEngine(chimera::sym_full(chimera::SYM_BTC)),
        chimera::OrderbookImbalanceEngine(chimera::sym_full(chimera::SYM_ETH))
    };
    for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) obes[i].shadow_mode = true;
    g_obes_ptr = obes;

    // ── Engine #5: EthBtcLeadLag — Phase 1, spot-only singleton (paper) ─────
    chimera::EthBtcLeadLagEngine ellaye;
    ellaye.shadow_mode = true;
    g_ellaye_ptr = &ellaye;

    // ── Engine #6: CoinbasePremiumMRev — multi-day BTC mean reversion ───────
    // Receives Binance BTC price from the spot tick callback and Coinbase
    // BTC-USD price from a separate CoinbaseWSFeed callback below. Buffers
    // ~24h of premium samples and trades on sustained discount.
    chimera::CoinbasePremiumMRevEngine cbprem;
    cbprem.shadow_mode = true;
    g_cbprem_ptr = &cbprem;

    // ── Engine #7: FundingPersistenceFade — multi-day BTC mean reversion ────
    // Reads Binance BTC funding rate via PerpFeed::funding_rate(SYM_BTC).
    // While the perp WS is silent (Tokyo IP block) funding_rate returns 0.0
    // and this engine self-disables via its trigger logic. Re-enables
    // automatically once perp data is restored.
    chimera::FundingPersistenceFadeEngine fpfe;
    fpfe.shadow_mode = true;
    g_fpfe_ptr = &fpfe;

    // ── Engine #8: VolCompressionBreakout — 24h vol-squeeze + Donchian ──────
    // Self-contained: maintains its own 24h ring of 60s-sampled BTC mid
    // prices. Cannot fire until ~23h after startup (warm-up period).
    chimera::VolCompressionBreakoutEngine vcbe;
    vcbe.shadow_mode = true;
    g_vcbe_ptr = &vcbe;

    // ── Engine #9: RangeMeanReversion on BTC + ETH (paper, high-frequency) ──
    // 30-bar 1-minute Bollinger + RSI(14). Fades lower-band touches when
    // RSI is oversold and the range vol-fraction is in the
    // mean-reversion-favourable band. Long-only spot. Warm-up ≈ 30 min.
    chimera::RangeMeanReversionEngine rmre[PAPER_NUM_SYMBOLS] = {
        chimera::RangeMeanReversionEngine(chimera::sym_full(chimera::SYM_BTC)),
        chimera::RangeMeanReversionEngine(chimera::sym_full(chimera::SYM_ETH))
    };
    for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) rmre[i].shadow_mode = true;
    g_rmre_ptr = rmre;

    // ── Perp WebSocket feed (powers FundingWindow + BasisMomentum + OBI's basis input) ──
    chimera::PerpFeed perp_feed;
    g_perp_feed_ptr = &perp_feed;

    // Set GUI root directory
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

    // HTTP GUI thread
    std::thread http_thread(http_server_thread, 8080);
    http_thread.detach();

    // Spot WebSocket feed
    chimera::BinanceWSFeed feed;
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (mid <= 0.0) return;

        // Cache last spot price for kill_all flattening (atomic, lock-free).
        store_dbl_atomic(g_last_spot_px_bits[id], mid);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Swing trades don't need sub-ms freshness but still drop very stale ticks
        double tick_age_ms = 0.0;
        if (tick.trade_time > 0) {
            tick_age_ms = static_cast<double>(now_ms - tick.trade_time);
            if (tick_age_ms < 0.0) tick_age_ms = 0.0;
        }
        if (tick_age_ms > 5000.0) return;  // 5s stale gate (relaxed vs HFT)

        {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            engine.update_price(id, mid);
            engine.on_tick(id, tick, now_ms);

            // ── Route BTC/ETH ticks through the perp-aware paper engines ─────
            int paper_slot = -1;
            for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
                if (PAPER_SYMBOL_IDS[i] == id) { paper_slot = i; break; }
            }
            if (paper_slot >= 0 && perp_feed.ready(id)) {
                const double frate = perp_feed.funding_rate(id);
                const double basis = perp_feed.basis_bp(id, mid);
                const double flow  = perp_feed.perp_flow_ratio(id);
                // available_R = 1.0 placeholder until Tier 1 risk wrapper exists.
                const double avail_R = 1.0;

                // FundingWindow — uses funding_rate + basis
                fwes[paper_slot].on_tick(mid, now_ms, frate, basis, avail_R);

                // BasisMomentum — uses basis + perp flow + vol_ratio.
                // vol_ratio = 1.0 placeholder (engine requires >= 0.7); replace
                // with a real per-symbol vol estimate once we have a regime
                // classifier in main.cpp.
                bmes[paper_slot].on_tick(mid, now_ms, basis, flow,
                                          /*vol_ratio=*/1.0, avail_R);

                // OrderbookImbalance — uses tick.book_imbalance + tick.spread_bps
                // + perp_basis. regime is hardcoded to 1 (GRIND) and vol_ratio
                // to 1.5 (above the 1.25 gate) — the spread / imbalance / basis
                // gates are doing the actual filtering. Refine when we have a
                // per-symbol regime classifier.
                obes[paper_slot].on_tick(mid, now_ms,
                                          /*book_imbalance=*/tick.book_imbalance,
                                          /*spread_bps=*/tick.spread_bps,
                                          /*vol_ratio=*/1.5,
                                          /*perp_basis_bp=*/basis,
                                          /*regime=*/1,
                                          avail_R);
            }

            // ── Engine #5: ETH→BTC Lead-Lag (Phase 1, spot-only) ─────────────
            // The engine self-routes: BTC/ETH only, others ignored.
            ellaye.on_tick(id, tick, now_ms, /*available_R=*/1.0);

            // ── Engines #6-#8: BTC-singleton multi-day / vol-regime engines ─
            // Each ignores any symbol_id != SYM_BTC internally, so calling on
            // every tick is safe. Strategy A (Coinbase Premium) needs the
            // BTC tick to update its Binance leg; the Coinbase leg comes via
            // a separate WS callback below.
            cbprem.update_binance_btc(id, tick, now_ms, /*available_R=*/1.0);

            // Strategy B (Funding Persistence) reads funding via PerpFeed.
            // While the perp WS is silent, frate is 0.0 here (no PerpFeed
            // ready) — the engine no-ops gracefully.
            const double frate_btc = (perp_feed.ready(chimera::SYM_BTC))
                                       ? perp_feed.funding_rate(chimera::SYM_BTC)
                                       : 0.0;
            fpfe.on_tick(id, tick, now_ms, frate_btc, /*available_R=*/1.0);

            // Strategy C (Vol Compression) is fully spot-only.
            vcbe.on_tick(id, tick, now_ms, /*available_R=*/1.0);

            // ── Engine #9: RangeMeanReversion (NEW, high-frequency) ──────────
            // BTC + ETH only; route per-symbol like FW/BM/OBI. Perp data not
            // required — pure spot mid-price + RSI on close-to-close deltas.
            // Fires every tick AFTER its 30-min warm-up; trades clustering
            // when the range-vol fraction is in [8 bp, 120 bp].
            if (paper_slot >= 0) {
                rmre[paper_slot].on_tick(tick, now_ms, /*available_R=*/1.0);
            }
        }

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 10000 == 0) {
            int rmre_total_trades = 0;
            double rmre_total_pnl = 0.0;
            if (g_rmre_ptr) {
                for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
                    rmre_total_trades += g_rmre_ptr[i].total_trades();
                    rmre_total_pnl    += g_rmre_ptr[i].total_pnl_bp();
                }
            }
            std::printf("[TICK] n=%d | %s px=%.4f | age=%.1fms | "
                        "swing_trades=%d | ell=%d/%.0fbp | cbprem=%d/%.0fbp | "
                        "fpfe=%d/%.0fbp | vcbe=%d/%.0fbp | rmre=%d/%.0fbp\n",
                n, tick.symbol.c_str(), mid, tick_age_ms,
                engine.total_trades(),
                ellaye.total_trades(),  ellaye.total_pnl_bp(),
                cbprem.total_trades(),  cbprem.total_pnl_bp(),
                fpfe.total_trades(),    fpfe.total_pnl_bp(),
                vcbe.total_trades(),    vcbe.total_pnl_bp(),
                rmre_total_trades,      rmre_total_pnl);
            std::fflush(stdout);
        }
    });

    // ── Coinbase BTC-USD feed (powers Strategy A, Coinbase Premium MRev) ────
    chimera::CoinbaseWSFeed coinbase_feed;
    coinbase_feed.set_callback([&](double cb_price, int64_t cb_ts) {
        if (cb_price <= 0.0) return;
        store_dbl_atomic(g_last_cb_btc_px_bits, cb_price);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        // Use exchange ts if reasonable; else local now.
        const int64_t use_ts = (cb_ts > 0 && cb_ts < now_ms + 5000 && cb_ts > now_ms - 30000)
                                 ? cb_ts : now_ms;

        std::lock_guard<std::mutex> lk(g_engine_mtx);
        if (g_cbprem_ptr) g_cbprem_ptr->update_coinbase_btc(cb_price, use_ts);
    });

    // Seed H4 + D1 indicators from Binance REST history
    engine.seed_from_history();

    feed.start();
    perp_feed.start();
    coinbase_feed.start();
    std::printf("[STARTUP] Spot feed live. SwingEngine running on 8 symbols (ETH-only trades).\n");
    std::printf("[STARTUP] Perp feed live. Paper engines on BTC + ETH:\n");
    std::printf("[STARTUP]   - FundingWindow (pre-funding basis snap-back)\n");
    std::printf("[STARTUP]   - BasisMomentum (perp→spot lead-lag)\n");
    std::printf("[STARTUP]   - OrderbookImbalance (short-term mean-reversion)\n");
    std::printf("[STARTUP] Phase 1 paper engine (spot-only, perp-feed-independent):\n");
    std::printf("[STARTUP]   - EthBtcLeadLag (1-3 min ETH leads -> BTC follower long)\n");
    std::printf("[STARTUP] Multi-day trio paper engines (BTC singleton):\n");
    std::printf("[STARTUP]   - CoinbasePremiumMRev (3-10 day, sustained CB-discount -> long)\n");
    std::printf("[STARTUP]   - FundingPersistenceFade (3-7 day, sustained -funding -> long; needs perp data)\n");
    std::printf("[STARTUP]   - VolCompressionBreakout (8-72h, 24h vol-squeeze + Donchian breakout)\n");
    std::printf("[STARTUP] High-frequency paper engine (BTC + ETH, spot-only):\n");
    std::printf("[STARTUP]   - RangeMeanReversion (30-min Bollinger + RSI(14), ~5-20 fires/sym/day)\n");
    std::printf("[STARTUP] Coinbase feed live (BTC-USD only) — powers CoinbasePremiumMRev.\n");
    std::printf("[STARTUP] All paper engines run in shadow_mode (printf log only, no executor).\n");
    std::printf("[STARTUP] GUI: http://localhost:8080  (state2 = paper engines JSON)\n");
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
    perp_feed.stop();
    coinbase_feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();

    int rmre_total_trades_final = 0;
    double rmre_total_pnl_final = 0.0;
    for (int i = 0; i < PAPER_NUM_SYMBOLS; ++i) {
        rmre_total_trades_final += rmre[i].total_trades();
        rmre_total_pnl_final    += rmre[i].total_pnl_bp();
    }
    std::printf("[SHUTDOWN] swing trades=%d swing pnl=%.3f%% | "
                "ell=%d/%.1fbp | cbprem=%d/%.1fbp | fpfe=%d/%.1fbp | vcbe=%d/%.1fbp | "
                "rmre=%d/%.1fbp\n",
                engine.total_trades(), engine.total_pnl_pct(),
                ellaye.total_trades(),  ellaye.total_pnl_bp(),
                cbprem.total_trades(),  cbprem.total_pnl_bp(),
                fpfe.total_trades(),    fpfe.total_pnl_bp(),
                vcbe.total_trades(),    vcbe.total_pnl_bp(),
                rmre_total_trades_final, rmre_total_pnl_final);
    std::fflush(stdout);
    release_instance_lock();
    return 0;
}
