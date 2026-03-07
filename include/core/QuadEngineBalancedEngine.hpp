#pragma once

#include "core/BalancedEngine.hpp"
#include "core/StructuralEngine.hpp"
#include "core/ConvexShockEngine.hpp"
#include "core/CompressionBreakoutEngine.hpp"
#include "core/RegimeStateAllocator.hpp"
#include "telemetry/SimpleHttpServer.hpp"
#include <sstream>
#include <iomanip>
#include <vector>

namespace chimera {

// =============================================================
// QUAD ENGINE BALANCED ENGINE
// Micro + Structural + Convex + Compression
// With Regime State Allocator for dynamic capital scaling
// =============================================================

class QuadEngineBalancedEngine {
public:
    QuadEngineBalancedEngine() : http_server_(8080) {
        // Initialize signal engines
        structural_[0] = StructuralEngine("btcusdt");
        structural_[1] = StructuralEngine("ethusdt");
        structural_[2] = StructuralEngine("solusdt");
        
        convex_[0] = ConvexShockEngine("btcusdt");
        convex_[1] = ConvexShockEngine("ethusdt");
        convex_[2] = ConvexShockEngine("solusdt");
        
        compression_[0] = CompressionBreakoutEngine("btcusdt");
        compression_[1] = CompressionBreakoutEngine("ethusdt");
        compression_[2] = CompressionBreakoutEngine("solusdt");
        
        // Initialize capital allocators vector
        allocator_.emplace_back("btcusdt");
        allocator_.emplace_back("ethusdt");
        allocator_.emplace_back("solusdt");
        
        http_server_.set_state_callback([this]() {
            return generate_state_json();
        });
        
        if (!http_server_.start()) {
            std::fprintf(stderr, "[QUAD-ENGINE] Failed to start HTTP server\n");
        }
        
        std::printf("\n");
        std::printf("╔════════════════════════════════════════════════════════════════╗\n");
        std::printf("║         QUAD ENGINE + REGIME ALLOCATOR FRAMEWORK              ║\n");
        std::printf("╠════════════════════════════════════════════════════════════════╣\n");
        std::printf("║ SIGNAL ENGINES:                                                ║\n");
        std::printf("║   • MICRO (BalancedEngine):    10-30bp (15bp min)             ║\n");
        std::printf("║   • STRUCTURAL:                30-150bp riders                 ║\n");
        std::printf("║   • CONVEX SHOCK:              20bp+ acceleration              ║\n");
        std::printf("║   • COMPRESSION BREAKOUT:      Tight range → expansion         ║\n");
        std::printf("║                                                                ║\n");
        std::printf("║ CAPITAL INTELLIGENCE:                                          ║\n");
        std::printf("║   • Regime State Allocator:    Dynamic capital scaling        ║\n");
        std::printf("║     - DEAD (0.0x)              Kill all trading                ║\n");
        std::printf("║     - COMPRESSION (0.5x)       Conservative sizing             ║\n");
        std::printf("║     - EXPANSION (1.0x)         Normal sizing                   ║\n");
        std::printf("║     - SHOCK (1.5x)             Aggressive scaling              ║\n");
        std::printf("║                                                                ║\n");
        std::printf("║ Portfolio Cap: 2.0R base → 0-3.0R dynamic per symbol          ║\n");
        std::printf("║ GUI: http://154.45.251.118:8080                               ║\n");
        std::printf("╚════════════════════════════════════════════════════════════════╝\n");
        std::printf("\n");
        std::fflush(stdout);
    }
    
    ~QuadEngineBalancedEngine() {
        http_server_.stop();
    }
    
    void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        // Derive scalar price for engines that don't need full tick
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;

        // 1. Run original BalancedEngine (micro) - passes full tick for real data
        balanced_.on_tick(id, tick, ts, latency_ms);
        
        // 2. Update market state
        update_market_state(id, price, ts);
        
        auto& ms = market_state_[id];
        
        // 3. Run Regime State Allocator FIRST (capital intelligence layer)
        allocator_[id].evaluate(
            ms.vol_ratio,
            ms.short_vol,
            ms.ema_vol,
            ms.displacement_bp,
            ms.acceleration_bp
        );
        
        // 4. Get dynamic capital cap (base 2.0R * regime multiplier)
        double base_cap = 2.0;
        double dynamic_cap = allocator_[id].allowed_R(base_cap);
        
        // 5. Calculate current usage
        double micro_R = balanced_.get_open_positions() > 0 ? 1.0 : 0.0;
        double structural_R = structural_[id].pos.size_R;
        double convex_R = convex_[id].pos.size_R;
        double compression_R = compression_[id].pos.size_R;
        double used_R = micro_R + structural_R + convex_R + compression_R;
        double available_R = std::max(0.0, dynamic_cap - used_R);
        
        // 6. Run signal engines with dynamic capital allocation + ECONOMIC GATES
        
        // STRUCTURAL GATE: 15bp+ displacement, 1.4+ vol ratio
        bool allow_structural = 
            std::abs(ms.displacement_bp) >= 15.0 &&
            ms.vol_ratio >= 1.4 &&
            allocator_[id].get_state() != VolState::DEAD;
        
        if (allow_structural) {
            structural_[id].evaluate(
                price,
                ms.vol_ratio,
                ms.displacement_bp,
                ms.regime,
                ms.vol_rising,
                ts,
                available_R
            );
        }
        
        // Update available R after structural
        structural_R = structural_[id].pos.size_R;
        used_R = micro_R + structural_R + convex_R + compression_R;
        available_R = std::max(0.0, dynamic_cap - used_R);
        
        // CONVEX GATE: 30bp+ displacement, 15bp+ acceleration, 1.8+ vol ratio
        bool allow_convex =
            std::abs(ms.displacement_bp) >= 30.0 &&
            std::abs(ms.acceleration_bp) >= 15.0 &&
            ms.vol_ratio >= 1.8;
        
        if (allow_convex) {
            convex_[id].evaluate(
                price,
                ms.vol_ratio,
                ms.short_vol,
                ms.ema_vol,
                ms.displacement_bp,
                ms.acceleration_bp,
                ms.regime,
                ts,
                available_R
            );
        }
        
        // Update available R after convex
        convex_R = convex_[id].pos.size_R;
        used_R = micro_R + structural_R + convex_R + compression_R;
        available_R = std::max(0.0, dynamic_cap - used_R);
        
        // COMPRESSION GATE: Allow compression detection always
        // (it has its own 100-tick arming + 15bp breakout logic)
        compression_[id].evaluate(
            price,
            ms.vol_ratio,
            ms.displacement_bp,
            ms.short_vol,
            ms.ema_vol,
            ms.acceleration_bp,
            ms.regime,
            ts,
            available_R
        );
        
        // 7. Enforce directional dominance
        enforce_directional_dominance(id);
    }
    
    // Delegate getters
    std::string get_rejection_stats() const { return balanced_.get_rejection_stats(); }
    double get_total_pnl() const { return balanced_.get_total_pnl(); }
    double get_realized_pnl() const { return balanced_.get_realized_pnl(); }
    int get_total_trades() const { return balanced_.get_total_trades(); }
    int get_open_positions() const { return balanced_.get_open_positions(); }
    void set_funding_fetcher(chimera::FundingRateFetcher* f) { balanced_.set_funding_fetcher(f); }
    void set_executor(chimera::SpotExecutor* e)              { balanced_.set_executor(e); }
    
    std::string generate_state_json() {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{";
        
        json << "\"btc_price\":" << market_state_[0].last_price << ",";
        json << "\"eth_price\":" << market_state_[1].last_price << ",";
        json << "\"sol_price\":" << market_state_[2].last_price << ",";
        
        const char* symbols[] = {"btcusdt", "ethusdt", "solusdt"};
        for (int i = 0; i < 3; i++) {
            auto structural_stats = structural_[i].get_stats();
            auto convex_stats = convex_[i].get_stats();
            auto compression_stats = compression_[i].get_stats();
            auto& ms = market_state_[i];
            
            json << "\"" << symbols[i] << "\":{";
            
            // Regime allocator state
            json << "\"regime_state\":\"" << allocator_[i].get_state_name() << "\",";
            json << "\"regime_multiplier\":" << allocator_[i].get_multiplier() << ",";
            json << "\"dynamic_cap_R\":" << allocator_[i].allowed_R(2.0) << ",";
            
            // Micro
            json << "\"micro_active\":" << (ms.micro_active ? "true" : "false") << ",";
            json << "\"micro_total_pnl_bp\":" << (balanced_.get_realized_pnl() * 100) << ",";
            json << "\"micro_total_trades\":" << balanced_.get_total_trades() << ",";
            
            // Structural
            json << "\"structural_active\":" << (structural_stats.active ? "true" : "false") << ",";
            json << "\"structural_size_R\":" << structural_stats.size_R << ",";
            json << "\"structural_mfe_bp\":" << structural_stats.mfe_bp << ",";
            json << "\"structural_total_pnl_bp\":" << structural_stats.total_pnl_bp << ",";
            json << "\"structural_total_trades\":" << structural_stats.total_trades << ",";
            
            // Convex
            json << "\"convex_active\":" << (convex_stats.active ? "true" : "false") << ",";
            json << "\"convex_size_R\":" << convex_stats.size_R << ",";
            json << "\"convex_mfe_bp\":" << convex_stats.mfe_bp << ",";
            json << "\"convex_total_pnl_bp\":" << convex_stats.total_pnl_bp << ",";
            json << "\"convex_total_trades\":" << convex_stats.total_trades << ",";
            
            // Compression
            json << "\"compression_active\":" << (compression_stats.active ? "true" : "false") << ",";
            json << "\"compression_size_R\":" << compression_stats.size_R << ",";
            json << "\"compression_mfe_bp\":" << compression_stats.mfe_bp << ",";
            json << "\"compression_total_pnl_bp\":" << compression_stats.total_pnl_bp << ",";
            json << "\"compression_total_trades\":" << compression_stats.total_trades << ",";
            json << "\"compression_ticks\":" << compression_stats.compression_ticks << ",";
            
            // Portfolio
            double micro_R = ms.micro_active ? 1.0 : 0.0;
            double portfolio_R = micro_R + structural_stats.size_R + convex_stats.size_R + compression_stats.size_R;
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
    CompressionBreakoutEngine compression_[3];
    std::vector<RegimeStateAllocator> allocator_;  // Use vector instead of array
    SimpleHttpServer http_server_;
    
    struct SimpleMarketState {
        double last_price = 0.0;
        double vol_ratio = 1.0;
        double short_vol = 0.0;
        double ema_vol = 0.0;
        double displacement_bp = 0.0;
        double acceleration_bp = 0.0;
        int regime = 0;
        bool vol_rising = false;
        
        bool micro_active = false;
        
        std::deque<double> returns;
        std::deque<double> price_deltas;
        int buildup_ticks = 0;

        // Per-symbol anchor for displacement (was incorrectly static/shared)
        double anchor_price = 0.0;
        int tick_counter = 0;
    };
    
    SimpleMarketState market_state_[3];
    
    void update_market_state(int id, double price, int64_t ts) {
        auto& ms = market_state_[id];
        
        if (ms.last_price > 0) {
            double ret = std::log(price / ms.last_price);
            ms.returns.push_back(ret);
            if (ms.returns.size() > 20) ms.returns.pop_front();
            
            double delta_bp = (price - ms.last_price) / ms.last_price * 10000.0;
            ms.price_deltas.push_back(delta_bp);
            if (ms.price_deltas.size() > 12) ms.price_deltas.pop_front();
            
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
                ms.regime = 2;
                ms.buildup_ticks++;
            } else if (ms.vol_ratio > 0.8) {
                ms.regime = 1;
                ms.buildup_ticks = 0;
            } else {
                ms.regime = 0;
                ms.buildup_ticks = 0;
            }
            
            if (ms.tick_counter % 20 == 0) ms.anchor_price = price;
            if (ms.anchor_price > 0) {
                ms.displacement_bp = (price - ms.anchor_price) / ms.anchor_price * 10000.0;
            }
        }
        
        ms.last_price = price;
        ms.tick_counter++;
        ms.micro_active = (balanced_.get_open_positions() > 0);
    }
    
    void enforce_directional_dominance(int id) {
        // Structural has priority over convex and compression
        if (structural_[id].pos.active) {
            auto struct_dir = structural_[id].pos.dir;
            
            // Check convex conflict
            if (convex_[id].pos.active) {
                auto convex_dir = convex_[id].pos.dir;
                bool conflict = false;
                if (struct_dir == StructDirection::LONG && convex_dir == ConvexDirection::SHORT) conflict = true;
                if (struct_dir == StructDirection::SHORT && convex_dir == ConvexDirection::LONG) conflict = true;
                
                if (conflict) {
                    std::printf("[DOMINANCE] %s | Convex forced exit - conflicts with Structural\n",
                        convex_[id].symbol.c_str());
                    std::fflush(stdout);
                    convex_[id].reset();
                    convex_[id].cooldown_ticks = 45;
                }
            }
            
            // Check compression conflict
            if (compression_[id].pos.active) {
                auto comp_dir = compression_[id].pos.dir;
                bool conflict = false;
                if (struct_dir == StructDirection::LONG && comp_dir == CompressionDirection::SHORT) conflict = true;
                if (struct_dir == StructDirection::SHORT && comp_dir == CompressionDirection::LONG) conflict = true;
                
                if (conflict) {
                    std::printf("[DOMINANCE] %s | Compression forced exit - conflicts with Structural\n",
                        compression_[id].symbol.c_str());
                    std::fflush(stdout);
                    compression_[id].pos.reset();
                    compression_[id].cooldown_ticks = 45;
                }
            }
        }
    }
};

} // namespace chimera
