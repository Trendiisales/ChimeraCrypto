#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>
#include <deque>
#include <cstdio>

namespace chimera {

// =============================================================
// ACCELERATION MODEL
// Tracks second derivative of price movement
// =============================================================

class AccelerationModel {
public:
    explicit AccelerationModel(int window = 12)
        : max_window(window) {}

    void update(double price) {
        if (!prices.empty()) {
            double bp = (price - prices.back()) / prices.back() * 10000.0;
            deltas.push_back(bp);
            if ((int)deltas.size() > max_window)
                deltas.pop_front();
        }
        prices.push_back(price);
        if ((int)prices.size() > max_window + 1)
            prices.pop_front();
    }

    double acceleration_bp() const {
        if (deltas.size() < 2) return 0.0;
        return deltas.back() - deltas[deltas.size() - 2];
    }

private:
    int max_window;
    std::deque<double> prices;
    std::deque<double> deltas;
};

// =============================================================
// CONVEX POSITION
// =============================================================

enum class ConvexDirection {
    NONE = 0,
    LONG = 1,
    SHORT = -1
};

struct ConvexPosition {
    ConvexDirection dir = ConvexDirection::NONE;
    double size_R = 0.0;
    double entry_price = 0.0;
    double mfe_bp = 0.0;
    double mae_bp = 0.0;
    bool active = false;
    int64_t entry_timestamp = 0;

    void reset() {
        dir = ConvexDirection::NONE;
        size_R = 0.0;
        entry_price = 0.0;
        mfe_bp = 0.0;
        mae_bp = 0.0;
        active = false;
        entry_timestamp = 0;
    }
};

// =============================================================
// CONVEX SHOCK ENGINE
// Acceleration rider: 20bp+ shock detection
// Profit-funded pyramiding: 0.25R → 1.0R
// Full exit on first decay
// =============================================================

class ConvexShockEngine {
public:
    static constexpr double ROUND_TRIP_COST_BP = 15.0; // 7.5bp/side with BNB discount (0.075%/side)
    ConvexPosition pos;
    
    std::string symbol;
    
    bool armed = false;
    int shock_ticks = 0;
    int cooldown_ticks = 0;
    
    double shock_reference_price = 0.0;
    
    double total_pnl_bp = 0.0;
    int total_trades = 0;
    int winning_trades = 0;
    double last_trade_entry_px = 0.0;
    double last_trade_pnl_bp   = 0.0;

    explicit ConvexShockEngine(const std::string& sym = "")
        : symbol(sym) {}

    void reset() {
        pos.reset();
        armed = false;
        shock_ticks = 0;
        shock_reference_price = 0.0;
    }

    void tick_cooldown() {
        if (cooldown_ticks > 0) {
            cooldown_ticks--;
        }
    }

    bool is_active() const {
        return pos.active;
    }

    void evaluate(double price,
                  double vol_ratio_smooth,
                  double short_vol,
                  double long_vol_ema,
                  double displacement_bp,
                  double acceleration_bp,  // Add this parameter
                  int regime,
                  int64_t timestamp,
                  double available_R)
    {
        // Use passed acceleration_bp instead of calculating
        double accel_bp = acceleration_bp;
        
        tick_cooldown();

        // =========================================================
        // ARMING LOGIC (Shock Detection)
        // =========================================================
        if (!pos.active && cooldown_ticks == 0) {

            bool shock_detected =
                std::abs(displacement_bp) >= 20.0 &&
                vol_ratio_smooth >= 1.6 &&
                std::abs(accel_bp) >= 10.0 &&
                short_vol > long_vol_ema;

            if (shock_detected) {
                armed = true;
                shock_ticks = 0;
                shock_reference_price = price;
            }

            if (armed) {
                shock_ticks++;

                // Confirm continuation (no immediate 50% retrace)
                double move_bp =
                    (price - shock_reference_price) /
                    shock_reference_price * 10000.0;

                if (std::abs(move_bp) >= 10.0) {

                    if (available_R >= 0.25) {

                        ConvexDirection dir =
                            displacement_bp > 0
                                ? ConvexDirection::LONG
                                : ConvexDirection::SHORT;

                        pos.dir = dir;
                        pos.size_R = 0.25;
                        pos.entry_price = price;
                        pos.entry_timestamp = timestamp;
                        pos.active = true;
                        pos.mfe_bp = 0.0;
                        pos.mae_bp = 0.0;

                        armed = false;

                        const char* dir_str =
                            (dir == ConvexDirection::LONG) ? "LONG" : "SHORT";

                        std::printf("[CONVEX-ENTER] %s | %s | size=0.25R | px=%.2f | disp=%.1fbp | accel=%.1fbp | vol=%.2f\n",
                            symbol.c_str(),
                            dir_str,
                            price,
                            displacement_bp,
                            accel_bp,
                            vol_ratio_smooth);
                        std::fflush(stdout);
                    }
                }

                // Disarm if shock fades quickly
                if (shock_ticks > 20 && std::abs(displacement_bp) < 15.0) {
                    armed = false;
                }
            }

            return;
        }

        // =========================================================
        // POSITION MANAGEMENT
        // =========================================================

        double pnl_bp =
            (price - pos.entry_price) / pos.entry_price * 10000.0;

        if (pos.dir == ConvexDirection::SHORT)
            pnl_bp *= -1.0;

        pos.mfe_bp = std::max(pos.mfe_bp, pnl_bp);
        pos.mae_bp = std::min(pos.mae_bp, pnl_bp);

        // =========================================================
        // PROFIT-FUNDED PYRAMIDING
        // =========================================================

        if (pos.size_R == 0.25 &&
            pos.mfe_bp >= 15.0 &&
            available_R >= 0.25 &&
            vol_ratio_smooth >= 1.6)
        {
            pos.size_R = 0.5;

            std::printf("[CONVEX-ADD1] %s | size=0.5R | mfe=%.1fbp\n",
                symbol.c_str(), pos.mfe_bp);
            std::fflush(stdout);
        }

        if (pos.size_R == 0.5 &&
            pos.mfe_bp >= 30.0 &&
            available_R >= 0.25 &&
            vol_ratio_smooth >= 1.6)
        {
            pos.size_R = 0.75;

            std::printf("[CONVEX-ADD2] %s | size=0.75R | mfe=%.1fbp\n",
                symbol.c_str(), pos.mfe_bp);
            std::fflush(stdout);
        }

        if (pos.size_R == 0.75 &&
            pos.mfe_bp >= 50.0 &&
            available_R >= 0.25 &&
            vol_ratio_smooth >= 1.6)
        {
            pos.size_R = 1.0;

            std::printf("[CONVEX-ADD3] %s | size=1.0R | mfe=%.1fbp\n",
                symbol.c_str(), pos.mfe_bp);
            std::fflush(stdout);
        }

        // =========================================================
        // DECAY DETECTION (FULL EXIT)
        // =========================================================

        bool decay = false;
        std::string reason;

        // Volatility collapse
        if (vol_ratio_smooth < 1.2) {
            decay = true;
            reason = "vol_collapse";
        }

        // 40% retrace of MFE
        if (pos.mfe_bp > 0 &&
            pnl_bp < (pos.mfe_bp * 0.6))
        {
            decay = true;
            reason = "retrace";
        }

        // Opposing acceleration
        if (std::abs(accel_bp) >= 12.0 &&
            ((accel_bp < 0 && pos.dir == ConvexDirection::LONG) ||
             (accel_bp > 0 && pos.dir == ConvexDirection::SHORT)))
        {
            decay = true;
            reason = "opposing_accel";
        }

        if (decay) {

            double gross_pnl = pnl_bp;
            double final_pnl = (pnl_bp - ROUND_TRIP_COST_BP) * pos.size_R;  // net after 15bp cost (BNB discount)

            total_pnl_bp += final_pnl;
            total_trades++;
            if (final_pnl > 0) winning_trades++;

            std::printf("[CONVEX-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | size=%.2fR | mfe=%.1f | mae=%.1f | reason=%s | total=%.1fbp\n",
                symbol.c_str(),
                final_pnl, gross_pnl, ROUND_TRIP_COST_BP,
                pos.size_R,
                pos.mfe_bp,
                pos.mae_bp,
                reason.c_str(),
                total_pnl_bp);
            std::fflush(stdout);

            cooldown_ticks = 30;
            last_trade_entry_px = pos.entry_price;
            last_trade_pnl_bp   = final_pnl;
            reset();
        }
    }

    double get_win_rate() const {
        return total_trades > 0
            ? static_cast<double>(winning_trades) / total_trades
            : 0.0;
    }

    struct Stats {
        bool active;
        ConvexDirection dir;
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
        s.active = pos.active;
        s.dir = pos.dir;
        s.size_R = pos.size_R;
        s.entry_price = pos.active ? pos.entry_price : last_trade_entry_px;
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
