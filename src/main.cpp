// ============================================================================
// Chimera — main entry point
// HTTP server is owned by QuadEngineBalancedEngine (port 8080)
// main.cpp only manages feed, executor, and signal handler.
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <sys/file.h>   // flock()
#include <fcntl.h>      // open()
#include <unistd.h>     // getpid(), write(), close()

#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "core/QuadEngineBalancedEngine.hpp"
#include "core/LiquidationFeed.hpp"
#include "core/market_data/FundingRateFetcher.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include "execution/NetworkLatencySystem.hpp"
#include "core/SymbolIndex.hpp"
#include "config/TradingConfig.hpp"

// ── SINGLE-INSTANCE LOCK ─────────────────────────────────────────────────────
// Uses an advisory flock() on a PID file.
// The lock is automatically released by the OS if the process crashes or exits.
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) {
        std::fprintf(stderr, "[FATAL] Cannot open lock file %s\n", PID_LOCK_FILE);
        std::exit(1);
    }

    // Non-blocking exclusive lock — fails immediately if another instance holds it
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        // Read the PID of the existing instance from the file
        char buf[32] = {};
        ::read(g_lock_fd, buf, sizeof(buf) - 1);
        ::close(g_lock_fd);
        std::fprintf(stderr,
            "\n╔══════════════════════════════════════════════════╗\n"
            "║  CHIMERA ALREADY RUNNING — SECOND INSTANCE BLOCKED  ║\n"
            "╠══════════════════════════════════════════════════╣\n"
            "║  Existing PID: %-35s║\n"
            "║  Lock file:    %-35s║\n"
            "║  To stop the running instance: kill %s       ║\n"
            "║  Or: pkill chimera                               ║\n"
            "╚══════════════════════════════════════════════════╝\n",
            buf, PID_LOCK_FILE, buf);
        std::exit(1);
    }

    // Write our PID into the lock file for diagnostics
    ::ftruncate(g_lock_fd, 0);
    char pidbuf[32];
    int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    ::write(g_lock_fd, pidbuf, len);
    ::fsync(g_lock_fd);
    // Note: g_lock_fd intentionally left open — closing it releases the flock
    std::printf("[STARTUP] Instance lock acquired | PID=%d | lock=%s\n",
                (int)::getpid(), PID_LOCK_FILE);
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN);
        ::close(g_lock_fd);
        ::unlink(PID_LOCK_FILE);
        g_lock_fd = -1;
    }
}
// ─────────────────────────────────────────────────────────────────────────────

chimera::ExchangeLatencyEngine g_exchange_latency;
chimera::FundingRateFetcher    g_funding;
Chimera::NetworkLatencySystem  g_network_latency;

static std::atomic<bool> g_running{true};
static std::atomic<int>  g_sig_count{0};

struct PriceCache {
    std::atomic<uint64_t> bits[chimera::MAX_SYMBOLS] = {};
    void set(int id, double v) {
        uint64_t b; __builtin_memcpy(&b, &v, 8);
        bits[id].store(b, std::memory_order_relaxed);
    }
    double get(int id) const {
        uint64_t b = bits[id].load(std::memory_order_relaxed);
        double v; __builtin_memcpy(&v, &b, 8); return v;
    }
} price_cache;

void signal_handler(int sig) {
    int n = g_sig_count.fetch_add(1) + 1;
    g_running = false;
    if (n == 1) {
        // First Ctrl+C — request clean shutdown
        std::fprintf(stderr, "\n[SHUTDOWN] Signal %d — stopping cleanly (Ctrl+C again to force kill)\n", sig);
    } else {
        // Second Ctrl+C — something is hanging, force exit immediately
        std::fprintf(stderr, "\n[SHUTDOWN] Forced exit.\n");
        release_instance_lock();
        std::_Exit(0);
    }
}

int main() {
    // ── 0. Single-instance lock — must be first ───────────────────────────────
    acquire_instance_lock();

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ── 1. Executor / API keys ───────────────────────────────────────────────
    chimera::SpotExecutor executor;
    bool executor_ok = executor.init("config/binance_credentials.json");
    if (!executor_ok) {
        std::fprintf(stderr,
            "[STARTUP] WARNING: executor init failed — orders will be skipped.\n"
            "[STARTUP] Edit config/binance_credentials.json with real api_key/api_secret.\n");
    }

    // ── 2. Engine (owns HTTP server on port 8080) ─────────────────────────────
    chimera::QuadEngineBalancedEngine controller;

    if (executor_ok) {
        controller.set_executor(&executor);
        std::printf("[STARTUP] Executor wired | shadow=%s\n",
                    executor.is_shadow() ? "YES (paper)" : "NO — LIVE");
        std::fflush(stdout);
    }

    // Funding rate background fetch
    std::thread([&]() {
        g_funding.fetch();
        controller.set_funding_fetcher(&g_funding);
    }).detach();

    // ── 2b. Liquidation feed — Binance futures forceOrder stream ─────────────
    chimera::LiquidationFeed liq_feed;
    liq_feed.set_callback([&](const chimera::LiquidationEvent& ev) {
        // Route to engine — thread safe (atomic flag in LiquidationEngine)
        controller.liq_engine().on_liquidation(ev);
        if (ev.symbol_id >= 0) {
            std::printf("[LIQ] SHORT LIQ %s | $%.0f | price=%.2f\n",
                chimera::sym_short(ev.symbol_id), ev.notional_usd, ev.price);
            std::fflush(stdout);
        }
    });
    liq_feed.start();
    std::printf("[STARTUP] Liquidation feed started (fstream.binance.com)\n");
    std::fflush(stdout);

    // ── 3. WebSocket feed ─────────────────────────────────────────────────────
    chimera::BinanceWSFeed feed;
    // Add all symbols — order must match SymbolIndex.hpp (BTC=0, ETH=1, SOL=2, ...)
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        price_cache.set(id, mid);

        if (tick.trade_time > 0) {
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            g_exchange_latency.record(now_ms, tick.trade_time);
        }

        if (!g_exchange_latency.ready()) return;

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        controller.on_tick(id, tick, now_ms, g_exchange_latency.p95());

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 1000 == 0) {
            std::printf("[TICK] n=%d | %s px=%.2f | lat_p95=%.2fms | fills=%d\n",
                n, tick.symbol.c_str(), mid,
                g_exchange_latency.p95(),
                executor_ok ? executor.fills() : 0);
            std::fflush(stdout);
        }
    });

    feed.start();
    std::printf("[STARTUP] Feed live. Calibrating latency...\n");
    std::printf("[CONFIG] Regime thresholds: GRIND→BUILDUP=%.2f  BUILDUP→BREAKOUT=%.2f\n",
        chimera::TradingConfig::REGIME_GRIND_EXIT_TO_BUILDUP,
        chimera::TradingConfig::REGIME_BUILDUP_TO_BREAKOUT);
    std::printf("[CONFIG] LeadLag BTC threshold: %.1fbp  Target max: %.1fbp\n",
        chimera::TradingConfig::LEADLAG_BTC_THRESHOLD_BP,
        chimera::TradingConfig::LEADLAG_TARGET_MAX_BP);
    std::printf("[CONFIG] IMBAL threshold: %.2f  Spread max: %.1fbp\n",
        chimera::TradingConfig::IMBALANCE_THRESHOLD,
        chimera::TradingConfig::IMBALANCE_MAX_SPREAD_BPS);
    std::fflush(stdout);

    // ── 4. Main loop ──────────────────────────────────────────────────────────
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ── 5. Shutdown ───────────────────────────────────────────────────────────
    std::printf("\n[SHUTDOWN] Stopping feed...\n");
    std::fflush(stdout);

    // Watchdog: if feed.stop() hangs for >3s, force exit
    std::atomic<bool> shutdown_done{false};
    std::thread watchdog([&](){
        for (int i = 0; i < 30; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (shutdown_done) return;
        }
        std::fprintf(stderr, "[SHUTDOWN] Feed stop timeout — forcing exit\n");
        release_instance_lock();
        std::_Exit(0);
    });

    feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();
    std::printf("[SHUTDOWN] fills=%d errors=%d trades=%d pnl=%.2fbp\n",
                executor_ok ? executor.fills()  : 0,
                executor_ok ? executor.errors() : 0,
                controller.get_total_trades(),
                controller.get_total_pnl());
    std::printf("[SHUTDOWN] Clean exit.\n");
    std::fflush(stdout);
    release_instance_lock();
    return 0;
}
