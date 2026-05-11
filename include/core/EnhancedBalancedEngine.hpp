#pragma once

#include "core/BalancedEngine.hpp"
#include "core/StructuralEngine.hpp"
#include "telemetry/SimpleHttpServer.hpp"
#include <sstream>
#include <iomanip>

namespace chimera {

class EnhancedBalancedEngine {
public:
    EnhancedBalancedEngine() : http_server_(8080) {
        structural_[0] = StructuralEngine("btcusdt");
        structural_[1] = StructuralEngine("ethusdt");
        structural_[2] = StructuralEngine("solusdt");
        
        http_server_.set_state_callback([this]() {
            return generate_state_json();
        });
        
        if (!http_server_.start()) {
            std::fprintf(stderr, "[ENHANCED] Failed to start HTTP server on port 8080\n");
        }
        
        std::printf("\n");
        std::printf("╔════════════════════════════════════════════════════════════════╗\n");
        std::printf("║              STRUCTURAL ENGINE LAYER - ADDED                  ║\n");
        std::printf("╠════════════════════════════════════════════════════════════════╣\n");
        std::printf("║ Original BalancedEngine: MICRO trades (10-30bp)              ║\n");
        std::printf("║ + Structural Engine:     RIDERS (30-150bp)                    ║\n");
        std::printf("║                                                                ║\n");
        std::printf("║ Directional Dominance: ENFORCED                               ║\n");
        std::printf("║ Structural exits → 45-tick micro cooldown                     ║\n");
        std::printf("║ Portfolio cap: 2.0R per symbol (shared)                       ║\n");
        std::printf("║ GUI: http://143.198.89.54:8080                               ║\n");
        std::printf("╚════════════════════════════════════════════════════════════════╝\n");
        std::printf("\n");
        std::fflush(stdout);
    }
    
    ~EnhancedBalancedEngine() {
        http_server_.stop();
    }
    
    void on_tick(int id, const MarketTick& tick, int64_t ts, double latency_ms) {
        double price = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        balanced_.on_tick(id, tick, ts, latency_ms);
        update_market_state(id, price, ts);
        
        auto& ms = market_state_[id];
        structural_[id].evaluate(
            price,
            ms.vol_ratio,
            ms.displacement_bp,
            ms.regime,
            ms.vol_rising,
            ts,
            ms.available_R
        );
    }
    
    std::string get_rejection_stats() const { return balanced_.get_rejection_stats(); }
    double get_total_pnl() const { return balanced_.get_total_pnl(); }
    double get_realized_pnl() const { return balanced_.get_realized_pnl(); }
    int get_total_trades() const { return balanced_.get_total_trades(); }
    int get_open_positions() const { return balanced_.get_open_positions(); }
    
    std::string generate_state_json() {
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{";
        
        // Add prices at top level
        json << "\"btc_price\":" << market_state_[0].last_price << ",";
        json << "\"eth_price\":" << market_state_[1].last_price << ",";
        json << "\"sol_price\":" << market_state_[2].last_price << ",";
        
        const char* symbols[] = {"btcusdt", "ethusdt", "solusdt"};
        for (int i = 0; i < 3; i++) {
            auto stats = structural_[i].get_stats();
            auto& ms = market_state_[i];
            
            json << "\"" << symbols[i] << "\":{";
            json << "\"micro_active\":" << (ms.micro_active ? "true" : "false") << ",";
            json << "\"micro_entry_price\":" << ms.micro_entry_price << ",";
            json << "\"micro_mfe_bp\":" << ms.micro_mfe_bp << ",";
            json << "\"micro_mae_bp\":" << ms.micro_mae_bp << ",";
            json << "\"micro_total_pnl_bp\":" << (balanced_.get_realized_pnl() * 100) << ",";
            json << "\"micro_total_trades\":" << balanced_.get_total_trades() << ",";
            json << "\"structural_active\":" << (stats.active ? "true" : "false") << ",";
            json << "\"structural_size_R\":" << stats.size_R << ",";
            json << "\"structural_entry_price\":" << stats.entry_price << ",";
            json << "\"structural_mfe_bp\":" << stats.mfe_bp << ",";
            json << "\"structural_mae_bp\":" << stats.mae_bp << ",";
            json << "\"structural_total_pnl_bp\":" << stats.total_pnl_bp << ",";
            json << "\"structural_total_trades\":" << stats.total_trades << ",";
            json << "\"structural_win_rate\":" << stats.win_rate << ",";
            json << "\"structural_cooldown_active\":" << (stats.cooldown_active ? "true" : "false") << ",";
            json << "\"structural_cooldown_remaining\":" << stats.cooldown_remaining << ",";
            double portfolio_R = (ms.micro_active ? 1.0 : 0.0) + stats.size_R;
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
    SimpleHttpServer http_server_;
    
    struct SimpleMarketState {
        double last_price = 0.0;
        double vol_ratio = 1.0;
        double displacement_bp = 0.0;
        int regime = 0;
        bool vol_rising = false;
        double available_R = 2.0;
        bool micro_active = false;
        double micro_entry_price = 0.0;
        double micro_mfe_bp = 0.0;
        double micro_mae_bp = 0.0;
        std::deque<double> returns;
        double ema_vol = 0.0;
        int buildup_ticks = 0;
    };
    
    SimpleMarketState market_state_[3];
    
    void update_market_state(int id, double price, int64_t ts) {
        auto& ms = market_state_[id];
        
        if (ms.last_price > 0) {
            double ret = std::log(price / ms.last_price);
            ms.returns.push_back(ret);
            if (ms.returns.size() > 20) ms.returns.pop_front();
            
            double sum_sq = 0.0;
            for (double r : ms.returns) sum_sq += r * r;
            double short_vol = std::sqrt(sum_sq / ms.returns.size());
            
            if (ms.ema_vol == 0.0) ms.ema_vol = short_vol;
            else ms.ema_vol = 0.95 * ms.ema_vol + 0.05 * short_vol;
            
            ms.vol_ratio = (ms.ema_vol > 0) ? short_vol / ms.ema_vol : 1.0;
            ms.vol_rising = (short_vol > ms.ema_vol * 1.1);
            
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
        double micro_R_used = micro_positions > 0 ? 1.0 : 0.0;
        ms.available_R = 2.0 - micro_R_used - structural_[id].pos.size_R;
    }
};

} // namespace chimera
