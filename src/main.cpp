#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include "live/BinanceWSFeed.hpp"
#include "telemetry/TelemetrySpine.hpp"
#include "telemetry/WsTelemetryServer.hpp"
#include "telemetry/DeskSnapshot.hpp"
#include "execution/NetworkLatencySystem.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include "core/SymbolIndex.hpp"
#include "core/BalancedEngine.hpp"
#include "engine/VolatilityExpansionEngine.hpp"
#include "engine/LiquidityVacuumEngine.hpp"
#include "engine/MultiSymbolAlignmentEngine.hpp"
#include "logging/TradeLogger.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include "core/SymbolIndex.hpp"
#include "core/BalancedEngine.hpp"

chimera::ExchangeLatencyEngine g_exchange_latency;




Chimera::NetworkLatencySystem g_network_latency;

static std::atomic<bool> g_running{true};

struct PriceCache {
    std::atomic<uint64_t> btc_bits{0};
    std::atomic<uint64_t> eth_bits{0};
    std::atomic<uint64_t> sol_bits{0};
    
    void set_btc(double val) {
        uint64_t bits;
        __builtin_memcpy(&bits, &val, sizeof(double));
        btc_bits.store(bits, std::memory_order_relaxed);
    }
    
    void set_eth(double val) {
        uint64_t bits;
        __builtin_memcpy(&bits, &val, sizeof(double));
        eth_bits.store(bits, std::memory_order_relaxed);
    }
    
    void set_sol(double val) {
        uint64_t bits;
        __builtin_memcpy(&bits, &val, sizeof(double));
        sol_bits.store(bits, std::memory_order_relaxed);
    }
    
    double get_btc() const {
        uint64_t bits = btc_bits.load(std::memory_order_relaxed);
        double val;
        __builtin_memcpy(&val, &bits, sizeof(double));
        return val;
    }
    
    double get_eth() const {
        uint64_t bits = eth_bits.load(std::memory_order_relaxed);
        double val;
        __builtin_memcpy(&val, &bits, sizeof(double));
        return val;
    }
    
    double get_sol() const {
        uint64_t bits = sol_bits.load(std::memory_order_relaxed);
        double val;
        __builtin_memcpy(&val, &bits, sizeof(double));
        return val;
    }
} price_cache;

void signal_handler(int) {
    g_running = false;
}

int main() {
    
    // Ultra controller
    chimera::BalancedEngine controller;
    
    // Immediate rejection stats test
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::printf("[IMMEDIATE-REJECTION-TEST] %s\n", controller.get_rejection_stats().c_str());
    std::fflush(stdout);
    
    // Per-symbol engine instances (disabled)
    chimera::VEConfig ve_cfg{7.0, 15.0, 9.0, 28.0, 1.8, 12.0, 500, 5000};
    chimera::LVConfig lv_cfg{7.0, 14.0, 8.0, 26.0, 14.0, 2.4, 0.30, 3.0, 64, 800};
    chimera::MSAConfig msa_cfg{7.0, 16.0, 10.0, 34.0, 18.0, 10.0, 64, 1200};
    
    std::unordered_map<std::string, chimera::VolatilityExpansionEngine> ve_engines;
    std::unordered_map<std::string, chimera::LiquidityVacuumEngine> lv_engines;
    
    for (const auto& s : {"btcusdt", "ethusdt", "solusdt"}) {
        ve_engines.emplace(s, chimera::VolatilityExpansionEngine(ve_cfg));
        lv_engines.emplace(s, chimera::LiquidityVacuumEngine(lv_cfg));
    }
    
    chimera::MultiSymbolAlignmentEngine msa_engine(msa_cfg);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    chimera::TelemetrySpine spine;
    chimera::WsTelemetryServer ws_server(9001, spine, "");
    ws_server.start();
    
    // Wire BalancedEngine to broadcast to GUI
    controller.set_gui_broadcast([&ws_server](const std::string& json_message) {
        ws_server.broadcast(json_message);
    });

    chimera::BinanceWSFeed feed;
    feed.add_symbol("btcusdt");
    feed.add_symbol("ethusdt");
    feed.add_symbol("solusdt");

    feed.set_callback([&](const chimera::MarketTick& tick) {
        if (tick.symbol == "btcusdt") price_cache.set_btc(tick.last_price);
        else if (tick.symbol == "ethusdt") price_cache.set_eth(tick.last_price);
        else if (tick.symbol == "solusdt") price_cache.set_sol(tick.last_price);
        
        // engine.update_book(tick.symbol, tick.bid, tick.ask, tick.bid_size, tick.ask_size);
        // engine.tick(tick.symbol);
        
        // Call engines
        if (!g_exchange_latency.ready()) return;
        
        auto now = std::chrono::system_clock::now();
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        double mid = (tick.bid + tick.ask) * 0.5;
        
        // Map symbol to index
        int sym_idx = -1;
        if (tick.symbol == "btcusdt") sym_idx = 0;
        else if (tick.symbol == "ethusdt") sym_idx = 1;
        else if (tick.symbol == "solusdt") sym_idx = 2;
        
        // Ultra controller
        auto id = chimera::symbol_to_id(tick.symbol);
        controller.on_tick(id, mid, now_ms, g_exchange_latency.p95());
        
        static int tick_count = 0;
        tick_count++;
        if (tick_count % 500 == 0) {
            std::printf("[DEBUG] %s | ticks=%d | px=%.2f | lat_p95=%.2fms\n",
                tick.symbol.c_str(), tick_count, mid, g_exchange_latency.p95());
            std::fflush(stdout);
        }
        
        // Old engines (disabled)
        /*        // Get per-symbol engines
        auto ve_it = ve_engines.find(tick.symbol);
        auto lv_it = lv_engines.find(tick.symbol);
        
        if (ve_it == ve_engines.end() || lv_it == lv_engines.end()) return;
        
        auto& ve = ve_it->second;
        auto& lv = lv_it->second;
        
        // Call engines
        if (sym_idx >= 0) {
            msa_engine.on_tick(sym_idx, mid, now_ms, g_exchange_latency.p95());
        }
        
        static int debug_count = 0;
        debug_count++;
        
        ve.on_tick(mid, now_ms, g_exchange_latency.p95());
        lv.on_book(tick.bid, tick.bid_size, tick.ask, tick.ask_size, now_ms, g_exchange_latency.p95());
        
        if (debug_count % 500 == 0) {
            std::printf("[DEBUG] %s | ticks=%d | px=%.2f | lat_p95=%.2fms\n",
                tick.symbol.c_str(), debug_count, mid, g_exchange_latency.p95());
            std::fflush(stdout);
        }
        
        // Check VE signals
        if (ve.has_signal()) {
            std::printf("[VE] ENTRY | %s | LONG=%d | px=%.2f\n", 
                tick.symbol.c_str(), ve.is_long() ? 1 : 0, ve.entry_price());
            std::fflush(stdout);
        }
        
        if (ve.exit_ready()) {
            std::printf("[VE] EXIT | %s | px=%.2f\n", 
                tick.symbol.c_str(), ve.exit_price());
            std::fflush(stdout);
        }
        
        // Check LV signals
        if (lv.enter_signal()) {
            std::printf("[LV] ENTRY | %s | LONG=%d | px=%.2f\n",
                tick.symbol.c_str(), lv.is_long() ? 1 : 0, lv.entry_price());
            std::fflush(stdout);
        }
        
        if (lv.exit_signal()) {
            std::printf("[LV] EXIT | %s | px=%.2f\n", 
                tick.symbol.c_str(), lv.exit_price());
            std::fflush(stdout);
        }
        
        if (msa_engine.enter_signal()) {
            std::printf("[MSA] ENTRY | sym=%d | LONG=%d | px=%.2f\n",
                msa_engine.symbol(), msa_engine.is_long() ? 1 : 0, msa_engine.entry_price());
            std::fflush(stdout);
        }
        
        if (msa_engine.exit_signal()) {
            std::printf("[MSA] EXIT | px=%.2f\n", msa_engine.exit_price());
            std::fflush(stdout);
        }
        */
    });

    feed.start();

    auto last_snapshot = std::chrono::steady_clock::now();
    chimera::DeskSnapshot snapshot;

    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_snapshot).count() >= 1) {
            
            snapshot.btc_price = price_cache.get_btc();
            snapshot.eth_price = price_cache.get_eth();
            snapshot.sol_price = price_cache.get_sol();
            
            snapshot.equity = 10000.0 + controller.get_total_pnl();
            snapshot.pnl = controller.get_realized_pnl();
            snapshot.unrealized_pnl = 0.0;
            snapshot.day_pnl = controller.get_total_pnl();
            snapshot.latency_ms = g_exchange_latency.latest();
            snapshot.orders_sent = controller.get_total_trades();
            snapshot.fills_received = controller.get_total_trades();
            snapshot.positions = controller.get_open_positions();
            snapshot.orders_blocked = 0;
            snapshot.governor = 0;
            snapshot.kill_switch = false;
            
            spine.publish(&snapshot);
            last_snapshot = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    feed.stop();
    ws_server.stop();
    
    return 0;
}
