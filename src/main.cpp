// ============================================================================
// Chimera — main entry point
// HTTP server is owned by QuadEngineBalancedEngine (port 8080)
// main.cpp only manages feed, executor, and signal handler.
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "core/QuadEngineBalancedEngine.hpp"
#include "core/market_data/FundingRateFetcher.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include "execution/NetworkLatencySystem.hpp"
#include "core/SymbolIndex.hpp"

chimera::ExchangeLatencyEngine g_exchange_latency;
chimera::FundingRateFetcher    g_funding;
Chimera::NetworkLatencySystem  g_network_latency;

static std::atomic<bool> g_running{true};

struct PriceCache {
    std::atomic<uint64_t> bits[3] = {};
    void set(int id, double v) {
        uint64_t b; __builtin_memcpy(&b, &v, 8);
        bits[id].store(b, std::memory_order_relaxed);
    }
    double get(int id) const {
        uint64_t b = bits[id].load(std::memory_order_relaxed);
        double v; __builtin_memcpy(&v, &b, 8); return v;
    }
} price_cache;

void signal_handler(int) { g_running = false; }

int main() {
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

    // ── 3. WebSocket feed ─────────────────────────────────────────────────────
    chimera::BinanceWSFeed feed;
    feed.add_symbol("btcusdt");
    feed.add_symbol("ethusdt");
    feed.add_symbol("solusdt");

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = -1;
        if      (tick.symbol == "btcusdt") id = 0;
        else if (tick.symbol == "ethusdt") id = 1;
        else if (tick.symbol == "solusdt") id = 2;
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
    std::fflush(stdout);

    // ── 4. Main loop ──────────────────────────────────────────────────────────
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ── 5. Shutdown ───────────────────────────────────────────────────────────
    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);
    feed.stop();
    std::printf("[SHUTDOWN] fills=%d errors=%d trades=%d pnl=%.2fbp\n",
                executor_ok ? executor.fills()  : 0,
                executor_ok ? executor.errors() : 0,
                controller.get_total_trades(),
                controller.get_total_pnl());
    std::printf("[SHUTDOWN] Clean exit.\n");
    std::fflush(stdout);
    return 0;
}
