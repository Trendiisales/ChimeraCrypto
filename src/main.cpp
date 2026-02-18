#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include "InstitutionalEngine.hpp"
#include "types.hpp"
#include "config.hpp"
#include "binance_client.hpp"
#include "http_dashboard.hpp"
#include "TelemetryServer.hpp"
#include "DepthManager.hpp"
#include "ExecutionSimulator.hpp"

namespace {
    std::atomic<bool> g_shutdown{false};
    
    void signal_handler(int) {
        g_shutdown.store(true, std::memory_order_release);
    }
}

int main() {
    // Install signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "═══════════════════════════════════════════════════════\n"
              << "  CHIMERA CRYPTO - INSTITUTIONAL HFT ENGINE\n"
              << "═══════════════════════════════════════════════════════\n"
              << "  Optimizations:     AVX2 SIMD, CRTP, Lock-free\n"
              << "  Core Features:     Loss Cluster, Liquidity Shock\n"
              << "  Institutional:     Slippage Model, Execution Quality\n"
              << "                     Monte Carlo Stress, Portfolio Envelope\n"
              << "                     Trade Imbalance, Burst Detection\n"
              << "                     Absorption Detection, Queue Model\n"
              << "  Microstructure:    Book Impact, Partial Fills\n"
              << "  Dashboard:         http://localhost:8888\n"
              << "═══════════════════════════════════════════════════════\n\n";
    
    // Create institutional engine
    chimera::InstitutionalEngine<> engine(10000.0);
    std::cout << "[ENGINE] Institutional Engine initialized with $10,000 equity\n";
    
    // Create depth managers with REST snapshot bootstrap
    chimera::DepthManager btc_depth("BTCUSDT");
    chimera::DepthManager eth_depth("ETHUSDT");
    
    std::cout << "[DEPTH] L2 bootstrap will trigger automatically on first WebSocket delta\n";
    
    auto btc_view = btc_depth.book();
    auto eth_view = eth_depth.book();
    std::cout << "[DEPTH] BTC ready=" << btc_view.ready << " mid=" << btc_view.mid << "\n";
    std::cout << "[DEPTH] ETH ready=" << eth_view.ready << " mid=" << eth_view.mid << "\n";
    
    // Create execution simulator
    chimera::ExecutionSimulator exec_sim(10000.0);
    std::cout << "[EXECUTION] Paper execution simulator initialized\n";
    
    // Wire quality tracker from engine to exec_sim
    exec_sim.set_quality_tracker(&engine.get_exec_quality_mutable());
    exec_sim.set_portfolio(&engine.get_portfolio_mutable());
    exec_sim.set_trade_results(&engine.get_trade_results());
    std::cout << "[EXECUTION] Quality tracker, portfolio, and trade results wired\n";
    
    // Create telemetry state
    chimera::TelemetryState telemetry;
    
    // Initialize with starting values IMMEDIATELY
    telemetry.risk.peak_equity = 10000.0;
    telemetry.risk.current_equity = 10000.0;
    telemetry.ops.ws_status = "STARTING";
    telemetry.state.current_state = "WARMUP";
    telemetry.state.warmup_ticks_remaining = 500;
    
    // Add symbols
    telemetry.symbols[0].symbol = "BTC";
    telemetry.symbols[1].symbol = "ETH";
    telemetry.symbols[2].symbol = "SOL";
    
    engine.set_telemetry(&telemetry);
    engine.set_execution_sim(&exec_sim);
    engine.set_depth_managers(&btc_depth, &eth_depth);
    std::cout << "[TELEMETRY] Telemetry system initialized\n";
    std::cout << "[ENGINE] Execution simulator and depth managers wired\n";
    
    // Start telemetry WebSocket server
    chimera::TelemetryServer telemetry_server;
    if (telemetry_server.start(&telemetry)) {
        std::cout << "[TELEMETRY] WebSocket server started on port 9000\n";
        std::cout << "[TELEMETRY] Operator console: http://154.45.251.118:8888\n";
    } else {
        std::cerr << "[TELEMETRY] Failed to start WebSocket server\n";
    }
    
    std::cout << "[ENGINE] All institutional layers active:\n";
    std::cout << "         ✓ Trade Aggregator (100ms VWAP buckets)\n";
    std::cout << "         ✓ Trade Imbalance Engine (aggressor, burst, absorption)\n";
    std::cout << "         ✓ Slippage Reality Model\n";
    std::cout << "         ✓ Execution Quality Tracker\n";
    std::cout << "         ✓ Monte Carlo Stress Engine\n";
    std::cout << "         ✓ Portfolio Envelope (DD limits + kill switch)\n";
    std::cout << "         ✓ Exchange Microstructure Model\n";
    std::cout << "         ✓ Queue Position Model\n\n";
    
    // Start HTTP dashboard
    chimera::HttpDashboard dashboard;
    dashboard.set_telemetry(&telemetry);
    dashboard.set_execution_sim(&exec_sim);
    dashboard.set_depth_managers(&btc_depth, &eth_depth);
    
    if (dashboard.start(8888)) {
        std::cout << "[HTTP] Dashboard started on http://localhost:8888\n";
        std::cout << "[HTTP] Serving LIVE execution metrics from ExecutionSimulator\n";
    } else {
        std::cerr << "[HTTP] Failed to start dashboard\n";
    }
    
    // Create Binance WebSocket client
    chimera::BinanceClient binance;
    
    // Track prices and latency
    double btc_price = 0.0;
    double eth_price = 0.0;
    auto last_trade_time = std::chrono::steady_clock::now();
    double latency_ms = 0.0;
    
    // Setup callbacks with NEW signature (includes quantity)
    binance.on_trade([&](size_t sym, double price, double quantity) {
        // Calculate latency
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_trade_time);
        latency_ms = elapsed.count();
        last_trade_time = now;
        
        if (sym == 0) {  // BTC
            btc_price = price;
        } else if (sym == 1) {  // ETH
            eth_price = price;
            
            // Get real metrics from engine
            const auto& portfolio = engine.get_portfolio();
            // const auto& quality = engine.get_exec_quality();
            
            // Calculate regime based on actual state
            std::string regime = "OBSERVE";
            if (portfolio.killed()) {
                regime = "KILLED";
            } else if (portfolio.current_exposure() > 100.0) {
                regime = "LONG";
            } else if (portfolio.current_exposure() < -100.0) {
                regime = "SHORT";
            } else if (portfolio.current_exposure() > 0.01) {
                regime = "TRADING";
            }
            
            // Real metrics from institutional layers
            // double spread_loss = quality.count() > 0 ? quality.average_slippage_bps() / 100.0 : 0.0;
            // double adverse = quality.count() > 0 ? quality.worst_slippage_bps() / 100.0 : 0.0;
            // double pressure = engine.get_trade_imbalance(1);  // ETH imbalance
            // double vol_percentile = std::min(100.0, engine.get_volatility(1) / 10.0 * 100.0);  // Normalize to 0-100
            
        }
        
        // Feed to trade aggregator (replaces direct engine.tick)
        engine.add_trade(sym, price, quantity);
        
    });
    
    binance.on_book_update([&btc_depth, &eth_depth, &engine](
        size_t sym,
        uint64_t U, uint64_t u,
        const std::vector<chimera::Level>& bids,
        const std::vector<chimera::Level>& asks) {
        
        // Route to appropriate depth manager
        if (sym == 0) {  // BTC
            btc_depth.handleWsDepth(U, u, bids, asks);
            
            // Update engine book from depth manager
            auto view = btc_depth.book();
            if (view.mid > 0) {
                std::array<chimera::Level, chimera::Config::BOOK_DEPTH> full_bids{};
                std::array<chimera::Level, chimera::Config::BOOK_DEPTH> full_asks{};
                full_bids[0].price = view.bestBid;
                full_bids[0].size = 1.0;
                full_asks[0].price = view.bestAsk;
                full_asks[0].size = 1.0;
                engine.update_book(sym, full_bids, full_asks);
            }
        } else if (sym == 1) {  // ETH
            eth_depth.handleWsDepth(U, u, bids, asks);
            
            // Update engine book from depth manager
            auto view = eth_depth.book();
            if (view.mid > 0) {
                std::array<chimera::Level, chimera::Config::BOOK_DEPTH> full_bids{};
                std::array<chimera::Level, chimera::Config::BOOK_DEPTH> full_asks{};
                full_bids[0].price = view.bestBid;
                full_bids[0].size = 1.0;
                full_asks[0].price = view.bestAsk;
                full_asks[0].size = 1.0;
                engine.update_book(sym, full_bids, full_asks);
            }
        }
    });
    
    // Connect to Binance
    std::cout << "[BINANCE] Connecting to WebSocket...\n";
    if (!binance.connect()) {
        std::cerr << "[BINANCE] Failed to connect\n";
        return 1;
    }
    
    // Wait for connection
    for (int i = 0; i < 50 && !binance.is_connected(); ++i) {
        binance.service(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!binance.is_connected()) {
        std::cerr << "[BINANCE] Connection timeout\n";
        return 1;
    }
    
    std::cout << "[BINANCE] Connected!\n";
    std::cout << "[ENGINE] Starting main loop with institutional layers...\n";
    std::cout << "[FILTERS] Dust filter active: BTC min 0.001, ETH min 0.01\n";
    std::cout << "[LOGGING] Aggregated signals only (no raw tick spam)\n\n";
    
    size_t tick_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (!g_shutdown.load(std::memory_order_acquire)) {
        // Service WebSocket (non-blocking)
        binance.service(0);
        
        // Tick engine for both symbols
        engine.tick(0);  // BTC
        engine.tick(1);  // ETH
        
        // Tick execution simulator for exit logic
        chimera::OrderBook btc_book{}, eth_book{};
        auto btc_view = btc_depth.book();
        auto eth_view = eth_depth.book();
        btc_book.mid = btc_view.mid;
        btc_book.bids[0].price = btc_view.bestBid;
        btc_book.asks[0].price = btc_view.bestAsk;
        eth_book.mid = eth_view.mid;
        eth_book.bids[0].price = eth_view.bestBid;
        eth_book.asks[0].price = eth_view.bestAsk;
        
        exec_sim.on_tick("BTC", btc_book);
        exec_sim.on_tick("ETH", eth_book);
        
        ++tick_count;
        
        // Update telemetry every 100 ticks (~100ms)
        if (tick_count % 100 == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            
            telemetry.runtime_seconds = elapsed.count();
            telemetry.ops.uptime_seconds = elapsed.count();
            telemetry.ops.ws_status = binance.is_connected() ? "CONNECTED" : "DISCONNECTED";
            
            // Decrement warmup
            if (telemetry.state.warmup_ticks_remaining > 0) {
                telemetry.state.warmup_ticks_remaining -= 100;
            }
            
            engine.update_telemetry();
        }
        
        // Print quality report every 60 seconds
        static uint64_t last_report_us = 0;
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        if (now_us - last_report_us > 60000000) {  // 60 seconds
            const auto& quality = engine.get_exec_quality();
            const auto& portfolio = engine.get_portfolio();
            
            std::cout << "\n═══════════════════════════════════════════════════════\n";
            std::cout << "[QUALITY REPORT] " << tick_count << " ticks\n";
            std::cout << "  Execution Quality:  " << std::fixed << std::setprecision(2) 
                      << quality.quality_score() * 100.0 << "%\n";
            std::cout << "  Avg Slippage:       " << quality.average_slippage_bps() << " bps\n";
            std::cout << "  Worst Slippage:     " << quality.worst_slippage_bps() << " bps\n";
            std::cout << "  Fill Count:         " << quality.count() << "\n";
            std::cout << "  Equity:             $" << exec_sim.equity() << "\n";
            std::cout << "  Realized PnL:       $" << exec_sim.realized_pnl() << "\n";
            std::cout << "  Daily Drawdown:     $" << portfolio.daily_dd() << "\n";
            std::cout << "  Current Exposure:   $" << portfolio.current_exposure() << "\n";
            std::cout << "  Peak Equity:        $" << portfolio.peak_equity() << "\n";
            std::cout << "  Kill Switch:        " << (portfolio.killed() ? "TRIGGERED" : "ACTIVE") << "\n";
            std::cout << "═══════════════════════════════════════════════════════\n\n";
            
            last_report_us = now_us;
        }
        
        // Sleep 1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "\n[SHUTDOWN] Engine stopped after " << tick_count << " ticks (" << duration << "s)\n";
    
    // Final quality report
    const auto& quality = engine.get_exec_quality();
    const auto& portfolio = engine.get_portfolio();
    
    std::cout << "\n═══════════════════════════════════════════════════════\n";
    std::cout << "FINAL QUALITY REPORT\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "Execution Quality:  " << std::fixed << std::setprecision(2) 
              << quality.quality_score() * 100.0 << "%\n";
    std::cout << "Avg Slippage:       " << quality.average_slippage_bps() << " bps\n";
    std::cout << "Worst Slippage:     " << quality.worst_slippage_bps() << " bps\n";
    std::cout << "Total Fills:        " << quality.count() << "\n";
    std::cout << "Final Equity:       $" << engine.get_equity() << "\n";
    std::cout << "Peak Equity:        $" << portfolio.peak_equity() << "\n";
    std::cout << "Daily DD:           $" << portfolio.daily_dd() << "\n";
    std::cout << "Kill Switch:        " << (portfolio.killed() ? "TRIGGERED" : "NEVER TRIGGERED") << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    
    binance.disconnect();
    dashboard.stop();
    telemetry_server.stop();
    
    std::cout << "[TELEMETRY] Server stopped\n";
    
    return 0;
}
