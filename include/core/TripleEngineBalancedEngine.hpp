#pragma once

#include "core/BalancedEngine.hpp"
#include "core/StructuralEngine.hpp"
#include "core/ConvexShockEngine.hpp"
#include "telemetry/SimpleHttpServer.hpp"
#include <sstream>
#include <iomanip>

namespace chimera {

// =============================================================
// TRIPLE ENGINE BALANCED ENGINE
// Micro (BalancedEngine) + Structural + Convex
// Enforces directional dominance
// Shared 2.0R cap per symbol across all engines
// =============================================================

class TripleEngineBalancedEngine {
public:
    TripleEngineBalancedEngine() : http_server_(8080) {
        // Initialize structural engines
        structural_[0] = StructuralEngine("btcusdt");
        structural_[1] = StructuralEngine("ethusdt");
        structural_[2] = StructuralEngine("solusdt");
        
        // Initialize convex engines
        convex_[0] = ConvexShockEngine("btcusdt");
        convex_[1] = ConvexShockEngine("ethusdt");
        convex_[2] = ConvexShockEngine("solusdt");
        
        // Start HTTP server
        http_server_.set_state_callback([this]() {
            return generate_state_json();
        });
        
        if (!http_server_.start()) {
            std::fprintf(stderr, "[TRIPLE-ENGINE] Failed to start HTTP server on port 8080\n");
        }
        
        std::printf("\n");
        std::printf("╔════════════════════════════════════════════════════════════════╗\n");
        std::printf("║           TRIPLE ENGINE VOLATILITY FRAMEWORK                  ║\n");
        std::printf("╠════════════════════════════════════════════════════════════════╣\n");
        std::printf("║ MICRO (BalancedEngine):    10-30bp (15bp min)                 ║\n");
        std::printf("║ STRUCTURAL:                30-150bp riders                     ║\n");
        std::printf("║ CONVEX SHOCK:              20bp+ acceleration                  ║\n");
        std::printf("║                                                                ║\n");
        std::printf("║ Directional Dominance:     ENFORCED                            ║\n");
        std::printf("║ Portfolio Cap:             2.0R per symbol (6.0R total)        ║\n");
        std::printf("║ GUI:                       http://143.198.89.54:8080          ║\n");
        std::printf("╚════════════════════════════════════════════════════════════════╝\n");
        std::printf("\n");
        std::fflush(stdout);
    }
    
    ~TripleEngineBalancedEngine() {
        http_server_.stop();
    }
    
    void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        // 1. Run original BalancedEngine (micro)
        balanced_.on_tick(id, tick, ts, latency_ms);
        
        // 2. Update market state for structural/convex
        update_market_state(id, price, ts);
        
        auto& ms = market_state_[id];
        
        // 3. Calculate available R (2.0R cap per symbol)
        double micro_R = balanced_.get_open_positions() > 0 ? 1.0 : 0.0;
        double structural_R = structural_[id].pos.size_R;
        double convex_R = convex_[id].pos.size_R;
        double used_R = micro_R + structural_R + convex_R;
        double available_R = std::max(0.0, 2.0 - used_R);
        
        // 4. Run structural engine
        structural_[id].evaluate(
            price,
            ms.vol_ratio,
            ms.displacement_bp,
            ms.regime,
            ms.vol_rising,
            ts,
            available_R
        );
        
        // 5. Update available R after structural changes
        structural_R = structural_[id].pos.size_R;
        used_R = micro_R + structural_R + convex_R;
        available_R = std::max(0.0, 2.0 - used_R);
        
        // 6. Run convex engine
        convex_[id].evaluate(
            price,
            ms.vol_ratio,
            ms.short_vol,
            ms.ema_vol,
            ms.displacement_bp,
            ms.acceleration_bp,  // Pass calculated acceleration
            ms.regime,
            ts,
            available_R
        );
        
        // 7. Enforce directional dominance
        enforce_directional_dominance(id);
    }
    
    // Delegate getters to BalancedEngine
    std::string get_rejection_stats() const { return balanced_.get_rejection_stats(); }
    double get_total_pnl() const { return balanced_.get_total_pnl(); }
    double get_realized_pnl() const { return balanced_.get_realized_pnl(); }
    int get_total_trades() const { return balanced_.get_total_trades(); }
    int get_open_positions() const { return balanced_.get_open_positions(); }
    
    std::string generate_state_json() {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{";
        
        // Add prices
        json << "\"btc_price\":" << market_state_[0].last_price << ",";
        json << "\"eth_price\":" << market_state_[1].last_price << ",";
        json << "\"sol_price\":" << market_state_[2].last_price << ",";
        
        const char* symbols[] = {"btcusdt", "ethusdt", "solusdt"};
        for (int i = 0; i < 3; i++) {
            auto structural_stats = structural_[i].get_stats();
            auto convex_stats = convex_[i].get_stats();
            auto& ms = market_state_[i];
            
            json << "\"" << symbols[i] << "\":{";
            
            // Micro stats
            json << "\"micro_active\":" << (ms.micro_active ? "true" : "false") << ",";
            json << "\"micro_entry_price\":" << ms.micro_entry_price << ",";
            json << "\"micro_mfe_bp\":" << ms.micro_mfe_bp << ",";
            json << "\"micro_mae_bp\":" << ms.micro_mae_bp << ",";
            json << "\"micro_total_pnl_bp\":" << (balanced_.get_realized_pnl() * 100) << ",";
            json << "\"micro_total_trades\":" << balanced_.get_total_trades() << ",";
            
            // Structural stats
            json << "\"structural_active\":" << (structural_stats.active ? "true" : "false") << ",";
            json << "\"structural_size_R\":" << structural_stats.size_R << ",";
            json << "\"structural_entry_price\":" << structural_stats.entry_price << ",";
            json << "\"structural_mfe_bp\":" << structural_stats.mfe_bp << ",";
            json << "\"structural_mae_bp\":" << structural_stats.mae_bp << ",";
            json << "\"structural_total_pnl_bp\":" << structural_stats.total_pnl_bp << ",";
            json << "\"structural_total_trades\":" << structural_stats.total_trades << ",";
            json << "\"structural_win_rate\":" << structural_stats.win_rate << ",";
            json << "\"structural_cooldown_active\":" << (structural_stats.cooldown_active ? "true" : "false") << ",";
            json << "\"structural_cooldown_remaining\":" << structural_stats.cooldown_remaining << ",";
            
            // Convex stats
            json << "\"convex_active\":" << (convex_stats.active ? "true" : "false") << ",";
            json << "\"convex_size_R\":" << convex_stats.size_R << ",";
            json << "\"convex_entry_price\":" << convex_stats.entry_price << ",";
            json << "\"convex_mfe_bp\":" << convex_stats.mfe_bp << ",";
            json << "\"convex_mae_bp\":" << convex_stats.mae_bp << ",";
            json << "\"convex_total_pnl_bp\":" << convex_stats.total_pnl_bp << ",";
            json << "\"convex_total_trades\":" << convex_stats.total_trades << ",";
            json << "\"convex_win_rate\":" << convex_stats.win_rate << ",";
            
            // Combined
            double micro_R = ms.micro_active ? 1.0 : 0.0;
            double portfolio_R = micro_R + structural_stats.size_R + convex_stats.size_R;
            json << "\"portfolio_R\":" << portfolio_R;
            
            json << "}";
            if (i < 2) json << ",";
        }
        
        json << "}";
        return json.str();
    }

private:
    BalancedEngine balanced_;
    StructuralEngine structural_[3];
    ConvexShockEngine convex_[3];
    SimpleHttpServer http_server_;
    
    struct SimpleMarketState {
        double last_price = 0.0;
        double vol_ratio = 1.0;
        double short_vol = 0.0;
        double ema_vol = 0.0;
        double displacement_bp = 0.0;
        double acceleration_bp = 0.0;  // Add acceleration tracking
        int regime = 0;
        bool vol_rising = false;
        
        bool micro_active = false;
        double micro_entry_price = 0.0;
        double micro_mfe_bp = 0.0;
        double micro_mae_bp = 0.0;
        
        std::deque<double> returns;
        std::deque<double> price_deltas;  // Add for acceleration calculation
        int buildup_ticks = 0;
    };
    
    SimpleMarketState market_state_[3];
    
    void update_market_state(int id, double price, int64_t ts) {
        auto& ms = market_state_[id];
        
        if (ms.last_price > 0) {
            // Calculate return
            double ret = std::log(price / ms.last_price);
            ms.returns.push_back(ret);
            if (ms.returns.size() > 20) ms.returns.pop_front();
            
            // Calculate price delta in bp
            double delta_bp = (price - ms.last_price) / ms.last_price * 10000.0;
            ms.price_deltas.push_back(delta_bp);
            if (ms.price_deltas.size() > 12) ms.price_deltas.pop_front();
            
            // Calculate acceleration (change in delta)
            if (ms.price_deltas.size() >= 2) {
                ms.acceleration_bp = ms.price_deltas.back() - 
                                    ms.price_deltas[ms.price_deltas.size() - 2];
            }
            
            double sum_sq = 0.0;
            for (double r : ms.returns) sum_sq += r * r;
            ms.short_vol = std::sqrt(sum_sq / ms.returns.size());
            
            if (ms.ema_vol == 0.0) ms.ema_vol = ms.short_vol;
            else ms.ema_vol = 0.95 * ms.ema_vol + 0.05 * ms.short_vol;
            
            ms.vol_ratio = (ms.ema_vol > 0) ? ms.short_vol / ms.ema_vol : 1.0;
            ms.vol_rising = (ms.short_vol > ms.ema_vol * 1.1);
            
            if (ms.vol_ratio > 1.3) {
                ms.regime = 2;  // BUILDUP
                ms.buildup_ticks++;
            } else if (ms.vol_ratio > 0.8) {
                ms.regime = 1;  // GRIND
                ms.buildup_ticks = 0;
            } else {
                ms.regime = 0;  // DEAD
                ms.buildup_ticks = 0;
            }
            
            static double anchor_price[3] = {0, 0, 0};
            static int tick_counter = 0;
            tick_counter++;
            
            if (tick_counter % 20 == 0) anchor_price[id] = price;
            if (anchor_price[id] > 0) {
                ms.displacement_bp = (price - anchor_price[id]) / anchor_price[id] * 10000.0;
            }
        }
        
        ms.last_price = price;
        
        int micro_positions = balanced_.get_open_positions();
        ms.micro_active = (micro_positions > 0);
    }
    
    void enforce_directional_dominance(int id) {
        // If structural has position, force convex to match direction or exit
        if (structural_[id].pos.active && convex_[id].pos.active) {
            auto convex_dir = convex_[id].pos.dir;
            auto struct_dir = structural_[id].pos.dir;
            
            bool conflict = false;
            if (struct_dir == StructDirection::LONG && convex_dir == ConvexDirection::SHORT) {
                conflict = true;
            }
            if (struct_dir == StructDirection::SHORT && convex_dir == ConvexDirection::LONG) {
                conflict = true;
            }
            
            if (conflict) {
                std::printf("[DOMINANCE] %s | Convex forced exit - conflicts with Structural %s\n",
                    convex_[id].symbol.c_str(),
                    struct_dir == StructDirection::LONG ? "LONG" : "SHORT");
                std::fflush(stdout);
                
                // Force convex exit
                double pnl = (market_state_[id].last_price - convex_[id].pos.entry_price) / 
                            convex_[id].pos.entry_price * 10000.0;
                if (convex_[id].pos.dir == ConvexDirection::SHORT) pnl *= -1.0;
                
                convex_[id].total_pnl_bp += pnl * convex_[id].pos.size_R;
                convex_[id].total_trades++;
                if (pnl > 0) convex_[id].winning_trades++;
                
                convex_[id].reset();
                convex_[id].cooldown_ticks = 45;  // Extended cooldown after forced exit
            }
        }
    }
};

} // namespace chimera
