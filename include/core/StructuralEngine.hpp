#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>

namespace chimera {

// =============================================================
// STRUCTURAL ENGINE (30-150bp rider)
// Only this one engine - your BalancedEngine is already the micro
// =============================================================

enum class StructDirection {
    NONE = 0,
    LONG = 1,
    SHORT = -1
};

struct StructuralPosition {
    StructDirection dir = StructDirection::NONE;
    double size_R = 0.0;
    double entry_price = 0.0;
    double mfe_bp = 0.0;
    double mae_bp = 0.0;
    bool active = false;
    int64_t entry_timestamp = 0;
    
    void reset() {
        dir = StructDirection::NONE;
        size_R = 0.0;
        entry_price = 0.0;
        mfe_bp = 0.0;
        mae_bp = 0.0;
        active = false;
        entry_timestamp = 0;
    }
};

class StructuralEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = 8.0;
    StructuralPosition pos;
    StructDirection last_exit_direction = StructDirection::NONE;
    bool active = false;
    int buildup_ticks = 0;
    int cooldown_ticks = 0;
    
    std::string symbol;
    double total_pnl_bp = 0.0;
    int total_trades = 0;
    int winning_trades = 0;
    double last_trade_entry_px = 0.0;
    double last_trade_pnl_bp   = 0.0;

    explicit StructuralEngine(const std::string& sym = "") : symbol(sym) {}

    void reset() {
        pos.reset();
        active = false;
        buildup_ticks = 0;
    }

    // Check if micro engine should be locked to this direction
    bool is_micro_locked() const {
        return cooldown_ticks > 0;
    }
    
    StructDirection get_locked_direction() const {
        return last_exit_direction;
    }
    
    void tick_cooldown(double vol_ratio_smooth) {
        if (cooldown_ticks > 0) {
            cooldown_ticks--;
            // Early clear if volatility collapses
            if (vol_ratio_smooth < 1.2) {
                cooldown_ticks = 0;
                last_exit_direction = StructDirection::NONE;
            }
        }
        if (cooldown_ticks == 0) {
            last_exit_direction = StructDirection::NONE;
        }
    }

    // Called on each tick
    void evaluate(double price, 
                  double vol_ratio_smooth,
                  double displacement_bp,
                  int regime,  // 0=DEAD, 1=GRIND, 2=BUILDUP
                  bool long_vol_rising,
                  int64_t timestamp,
                  double available_R)  // From risk governor
    {
        const int REGIME_BUILDUP = 2;
        
        // Track buildup duration
        if (regime == REGIME_BUILDUP) {
            buildup_ticks++;
        } else {
            buildup_ticks = 0;
        }
        
        // Tick down cooldown
        tick_cooldown(vol_ratio_smooth);

        // Entry logic
        if (!active) {
            // Structural entry conditions (30-150bp rider)
            if (vol_ratio_smooth > 1.4 &&
                buildup_ticks > 40 &&
                std::abs(displacement_bp) > 20.0 &&
                long_vol_rising &&
                available_R >= 0.5)  // Need at least 0.5R available
            {
                StructDirection dir = displacement_bp > 0 ? StructDirection::LONG : StructDirection::SHORT;

                pos.dir = dir;
                pos.size_R = 0.5;  // Initial conservative size
                pos.entry_price = price;
                pos.active = true;
                pos.entry_timestamp = timestamp;
                pos.mfe_bp = 0.0;
                pos.mae_bp = 0.0;
                active = true;
                
                const char* dir_str = (dir == StructDirection::LONG) ? "LONG" : "SHORT";
                std::printf("[STRUCTURAL-ENTER] %s | %s | size=0.5R | px=%.2f | vol_ratio=%.2f | disp=%.1fbp\n",
                    symbol.c_str(), dir_str, price, vol_ratio_smooth, displacement_bp);
                std::fflush(stdout);
            }
            return;
        }

        // Position management (active position)
        double pnl_bp = (price - pos.entry_price) / pos.entry_price * 10000.0;
        if (pos.dir == StructDirection::SHORT) pnl_bp *= -1.0;
        
        pos.mfe_bp = std::max(pos.mfe_bp, pnl_bp);
        pos.mae_bp = std::min(pos.mae_bp, pnl_bp);

        // Pyramiding logic
        // Add 1: Persistence confirmed (20bp MFE + strong vol)
        if (pos.size_R == 0.5 &&
            pos.mfe_bp > 20.0 &&
            vol_ratio_smooth > 1.5 &&
            available_R >= 0.5)
        {
            pos.size_R = 1.0;
            std::printf("[STRUCTURAL-ADD1] %s | size=0.5R->1.0R | mfe=%.1fbp | vol_ratio=%.2f\n",
                symbol.c_str(), pos.mfe_bp, vol_ratio_smooth);
            std::fflush(stdout);
        }

        // Add 2: Acceleration confirmed (40bp MFE + very strong vol)
        if (pos.size_R == 1.0 &&
            pos.mfe_bp > 40.0 &&
            vol_ratio_smooth > 1.7 &&
            available_R >= 0.5)
        {
            pos.size_R = 1.5;
            std::printf("[STRUCTURAL-ADD2] %s | size=1.0R->1.5R | mfe=%.1fbp | vol_ratio=%.2f\n",
                symbol.c_str(), pos.mfe_bp, vol_ratio_smooth);
            std::fflush(stdout);
        }

        // Exit conditions
        bool should_exit = false;
        std::string exit_reason;
        
        if (vol_ratio_smooth < 1.0) {
            should_exit = true;
            exit_reason = "vol_collapse";
        } else if (regime != REGIME_BUILDUP) {
            should_exit = true;
            exit_reason = "regime_change";
        }
        
        if (should_exit) {
            double gross_pnl  = pnl_bp;
            double final_pnl  = (pnl_bp - ROUND_TRIP_COST_BP) * pos.size_R;  // net after 8bp cost
            total_pnl_bp += final_pnl;
            total_trades++;
            if (final_pnl > 0) winning_trades++;
            
            std::printf("[STRUCTURAL-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | size=%.1fR | mfe=%.1f | mae=%.1f | reason=%s | total=%.1fbp\n",
                symbol.c_str(), final_pnl, gross_pnl, ROUND_TRIP_COST_BP, pos.size_R, pos.mfe_bp, pos.mae_bp,
                exit_reason.c_str(), total_pnl_bp);
            std::fflush(stdout);
            
            // Start cooldown for micro engine (45 ticks)
            last_exit_direction  = pos.dir;
            cooldown_ticks       = 45;
            last_trade_entry_px  = pos.entry_price;
            last_trade_pnl_bp    = final_pnl;
            reset();
        }
    }
    
    double get_win_rate() const {
        return total_trades > 0 ? (double)winning_trades / total_trades : 0.0;
    }
    
    // For GUI
    struct Stats {
        bool active;
        double size_R;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double total_pnl_bp;
        int total_trades;
        double win_rate;
        bool cooldown_active;
        int cooldown_remaining;
    };
    
    Stats get_stats() const {
        Stats s;
        s.active = active;
        s.size_R = pos.size_R;
        s.entry_price = (pos.active) ? pos.entry_price : last_trade_entry_px;
        s.mfe_bp = pos.mfe_bp;
        s.mae_bp = pos.mae_bp;
        s.total_pnl_bp = total_pnl_bp;
        s.total_trades = total_trades;
        s.win_rate = get_win_rate();
        s.cooldown_active = (cooldown_ticks > 0);
        s.cooldown_remaining = cooldown_ticks;
        return s;
    }
};

} // namespace chimera
