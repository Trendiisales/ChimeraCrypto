// ============================================================================
// Chimera — main entry point
//
// Shadow mode (default): Live Binance WebSocket feed, real API keys loaded +
// validated, all orders signed and logged as [SHADOW-ORDER], nothing POSTed.
// Set shadow_mode=false in config/binance_credentials.json for live trading.
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
#include "telemetry/TelemetrySpine.hpp"
#include "telemetry/DeskSnapshot.hpp"
#include "telemetry/SimpleHttpServer.hpp"
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

    std::printf("╔══════════════════════════════════════════════════════════════╗\n");
    std::printf("║              CHIMERA CRYPTO — STARTING                      ║\n");
    std::printf("╚══════════════════════════════════════════════════════════════╝\n");
    std::fflush(stdout);

    // ── 1. Executor / API keys ───────────────────────────────────────────────
    chimera::SpotExecutor executor;
    bool executor_ok = executor.init("config/binance_credentials.json");
    if (!executor_ok) {
        std::fprintf(stderr,
            "[STARTUP] WARNING: executor init failed — orders will be skipped.\n"
            "[STARTUP] Edit config/binance_credentials.json with real api_key/api_secret.\n");
    }

    // ── 2. Engine ────────────────────────────────────────────────────────────
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

    // ── 3. GUI HTTP server ────────────────────────────────────────────────────
    chimera::TelemetrySpine   spine;
    chimera::DeskSnapshot     snapshot;
    chimera::SimpleHttpServer http_server(8080);
    http_server.set_state_callback([&]() -> std::string {
        return std::string(snapshot.to_json());
    });
    if (!http_server.start()) {
        std::fprintf(stderr, "[STARTUP] HTTP server failed on port 8080\n");
    }

    // ── 4. WebSocket feed ─────────────────────────────────────────────────────
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

        // Record latency from exchange timestamp
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

    // ── 5. Main loop — 1Hz snapshot ──────────────────────────────────────────
    auto t0       = std::chrono::steady_clock::now();
    auto last_snap = t0;

    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_snap).count() >= 1) {
            double uptime_s = std::chrono::duration<double>(now - t0).count();
            snapshot.btc_price      = price_cache.get(0);
            snapshot.eth_price      = price_cache.get(1);
            snapshot.sol_price      = price_cache.get(2);
            snapshot.equity         = 10000.0 + controller.get_total_pnl();
            snapshot.pnl            = controller.get_realized_pnl();
            snapshot.day_pnl        = controller.get_total_pnl();
            snapshot.latency_ms     = g_exchange_latency.ready() ? g_exchange_latency.p95() : 0.0;
            snapshot.orders_sent    = executor_ok ? executor.fills() : 0;
            snapshot.fills_received = executor_ok ? executor.fills() : 0;
            snapshot.positions      = controller.get_open_positions();
            snapshot.governor       = "ACTIVE";
            snapshot.kill_switch    = false;
            snapshot.uptime_hours   = uptime_s / 3600.0;
            snapshot.mode           = (executor_ok && !executor.is_shadow()) ? "LIVE" : "SHADOW";
            snapshot.healthy        = true;
            spine.publish(&snapshot);
            last_snap = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // ── 6. Shutdown ───────────────────────────────────────────────────────────
    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);
    feed.stop();
    http_server.stop();
    std::printf("[SHUTDOWN] fills=%d errors=%d trades=%d pnl=%.2fbp\n",
                executor_ok ? executor.fills()  : 0,
                executor_ok ? executor.errors() : 0,
                controller.get_total_trades(),
                controller.get_total_pnl());
    std::printf("[SHUTDOWN] Clean exit.\n");
    std::fflush(stdout);
    return 0;
}
