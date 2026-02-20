#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include "engine/InstitutionalEngine.hpp"
#include "runtime/EngineRuntime.hpp"
#include "telemetry/TelemetrySpine.hpp"
#include "telemetry/WsTelemetryServer.hpp"
#include "live/BinanceWSFeed.hpp"

std::atomic<int> tick_count{0};
std::atomic<double> btc_price{0};
std::atomic<double> eth_price{0};
std::atomic<double> sol_price{0};
std::atomic<double> current_latency_ms{2.5}; // Default 2.5ms

std::atomic<int> winning_trades{0};
std::atomic<int> losing_trades{0};
double last_total_pnl = 0;

void print_snapshot(chimera::InstitutionalEngine* engine) {
    double realized = engine->get_realized_pnl();
    double unrealized = engine->get_unrealized_pnl();
    double total_pnl = realized + unrealized;
    
    std::cout << "\n┌───────────────────────────────────────────────────────────┐\n";
    std::cout << "│ TUNED SYSTEM SNAPSHOT                                     │\n";
    std::cout << "├───────────────────────────────────────────────────────────┤\n";
    std::cout << "│ P&L                                                       │\n";
    std::cout << "│   Total:      " << std::setw(10) << std::fixed << std::setprecision(2) 
              << total_pnl << " " << (total_pnl >= 0 ? "✓" : "✗") << "                          │\n";
    std::cout << "│   Realized:   " << std::setw(10) << realized << "                           │\n";
    std::cout << "│   Unrealized: " << std::setw(10) << unrealized << "                           │\n";
    std::cout << "├───────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Orders: " << std::setw(4) << engine->total_shadow_orders() 
              << " | Blocked: " << std::setw(4) << engine->get_blocked_orders()
              << " | Positions: " << engine->num_positions() << "                │\n";
    std::cout << "│ BTC: " << std::setw(8) << std::setprecision(4) << engine->get_position(chimera::SymbolID::BTC)
              << " | $" << std::setw(8) << std::setprecision(2) << btc_price.load() << "                │\n";
    std::cout << "│ ETH: " << std::setw(8) << std::setprecision(4) << engine->get_position(chimera::SymbolID::ETH)
              << " | $" << std::setw(8) << std::setprecision(2) << eth_price.load() << "                │\n";
    std::cout << "│ SOL: " << std::setw(8) << std::setprecision(4) << engine->get_position(chimera::SymbolID::SOL)
              << " | $" << std::setw(8) << std::setprecision(2) << sol_price.load() << "                │\n";
    std::cout << "├───────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Governor: " << std::setw(10) << engine->get_governor_state() 
              << " | RTT: " << std::setw(6) << std::setprecision(1) << current_latency_ms.load() << "ms         │\n";
    std::cout << "│ Cost Floor Enforcement: ACTIVE                            │\n";
    std::cout << "└───────────────────────────────────────────────────────────┘\n\n";
}

void publish_telemetry(chimera::TelemetrySpine* spine, 
                      chimera::InstitutionalEngine* engine,
                      std::chrono::steady_clock::time_point start_time)
{
    static chimera::DeskSnapshot snapshot;
    static double prev_btc = 0, prev_eth = 0, prev_sol = 0;
    
    auto now = std::chrono::steady_clock::now();
    double elapsed_hours = std::chrono::duration<double>(now - start_time).count() / 3600.0;
    
    double cur_btc = btc_price.load();
    double cur_eth = eth_price.load();
    double cur_sol = sol_price.load();
    
    if (prev_btc > 0 && cur_btc > 0) snapshot.btc_change_pct = ((cur_btc - prev_btc) / prev_btc) * 100.0;
    if (prev_eth > 0 && cur_eth > 0) snapshot.eth_change_pct = ((cur_eth - prev_eth) / prev_eth) * 100.0;
    if (prev_sol > 0 && cur_sol > 0) snapshot.sol_change_pct = ((cur_sol - prev_sol) / prev_sol) * 100.0;
    
    prev_btc = cur_btc;
    prev_eth = cur_eth;
    prev_sol = cur_sol;
    
    double realized = engine->get_realized_pnl();
    double unrealized = engine->get_unrealized_pnl();
    double total_pnl = engine->get_total_pnl();
    
    snapshot.btc_price = cur_btc;
    snapshot.eth_price = cur_eth;
    snapshot.sol_price = cur_sol;
    snapshot.equity = engine->get_equity() + total_pnl;
    snapshot.pnl = realized;
    snapshot.unrealized_pnl = unrealized;
    snapshot.day_pnl = total_pnl;
    snapshot.drawdown = (total_pnl < 0) ? std::abs(total_pnl) : 0;
    
    snapshot.latency_ms = current_latency_ms.load();
    snapshot.orders_sent = engine->total_shadow_orders();
    snapshot.fills_received = snapshot.orders_sent;
    snapshot.positions = engine->num_positions();
    snapshot.orders_blocked = engine->get_blocked_orders();
    
    double btc_pos = engine->get_position(chimera::SymbolID::BTC);
    double eth_pos = engine->get_position(chimera::SymbolID::ETH);
    double sol_pos = engine->get_position(chimera::SymbolID::SOL);
    
    snapshot.exposure_usd = std::abs(btc_pos * cur_btc) + 
                           std::abs(eth_pos * cur_eth) + 
                           std::abs(sol_pos * cur_sol);
    
    snapshot.governor = engine->get_governor_state();
    snapshot.kill_switch = engine->is_halted();
    
    int total_trades = snapshot.orders_sent;
    if (total_trades > 0) {
        double current_pnl = total_pnl;
        if (current_pnl > last_total_pnl) winning_trades++;
        else if (current_pnl < last_total_pnl) losing_trades++;
        last_total_pnl = current_pnl;
        
        int trades_counted = winning_trades.load() + losing_trades.load();
        if (trades_counted > 0) {
            snapshot.win_rate = static_cast<double>(winning_trades.load()) / trades_counted;
        }
        
        if (elapsed_hours > 0 && total_pnl != 0) {
            double annualized_return = (total_pnl / engine->get_equity()) * (8760 / elapsed_hours);
            snapshot.sharpe_ratio = annualized_return / 0.15;
        }
    }
    
    snapshot.trades_today = total_trades;
    snapshot.uptime_hours = elapsed_hours;
    snapshot.mode = "SHADOW";
    snapshot.healthy = true;
    
    auto last_order = engine->get_last_order();
    snapshot.last_order_symbol = last_order.symbol.c_str();
    snapshot.last_order_side = last_order.side.c_str();
    snapshot.last_order_size = last_order.size;
    snapshot.last_order_price = last_order.price;
    snapshot.last_order_usd = last_order.usd;
    snapshot.last_order_signal = last_order.signal;
    snapshot.last_order_conviction = last_order.conviction;
    snapshot.last_order_cost_floor = last_order.cost_floor;
    snapshot.last_order_time = last_order.time.c_str();
    
    snapshot.btc_position = btc_pos;
    snapshot.eth_position = eth_pos;
    snapshot.sol_position = sol_pos;
    
    spine->publish(&snapshot);
}

int main()
{
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  ChimeraCrypto - TUNED SYSTEM\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    chimera::InstitutionalEngine engine(10000.0);
    chimera::EngineRuntime runtime(engine);
    runtime.start();
    
    chimera::TelemetrySpine spine;
    chimera::WsTelemetryServer ws_server(9001, spine, "");
    ws_server.start();
    
    std::cout << "[✓] Cost Floor: 6.5bps ACTIVE\n";
    std::cout << "[✓] Latency: ~2.5ms (estimated)\n";
    std::cout << "[✓] GUI: https://154.45.251.118:9443\n\n";
    
    chimera::BinanceWSFeed binance;
    binance.add_symbol("btcusdt");
    binance.add_symbol("ethusdt");
    binance.add_symbol("solusdt");
    
    binance.set_callback([&](const chimera::MarketTick& tick) {
        tick_count++;
        
        // Store latency - use tick value if valid, else keep default
        if (tick.rtt_ms > 0.1 && tick.rtt_ms < 100.0) {
            current_latency_ms.store(tick.rtt_ms);
        }
        
        if (tick.symbol == "btcusdt") btc_price.store(tick.last_price);
        else if (tick.symbol == "ethusdt") eth_price.store(tick.last_price);
        else if (tick.symbol == "solusdt") sol_price.store(tick.last_price);
        
        runtime.post_market_event(tick.symbol, [&, tick]() {
            engine.update_book(tick.symbol, tick.bid, tick.ask, tick.bid_size, tick.ask_size);
        });
        
        if (tick_count % 10 == 0) {
            runtime.post_execution_event(tick.symbol, [&, symbol=tick.symbol]() {
                engine.tick(symbol);
            });
        }
    });
    
    binance.start();
    std::cout << "[LIVE] System Active\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    auto last_snapshot = start_time;
    
    while (true) {
        publish_telemetry(&spine, &engine, start_time);
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_snapshot).count() >= 10) {
            print_snapshot(&engine);
            last_snapshot = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
