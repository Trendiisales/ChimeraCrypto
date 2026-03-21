#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>
#include <deque>
#include <cstdio>

namespace chimera {

// =============================================================
// COMPRESSION BREAKOUT ENGINE
// Volatility Compression → Expansion
// Detects prolonged compression
// Enters on breakout of tight range
// Profit-funded pyramiding
// Full exit on first decay
// =============================================================

enum class CompressionDirection {
    NONE = 0,
    LONG = 1,
    SHORT = -1
};

struct CompressionPosition {
    CompressionDirection dir = CompressionDirection::NONE;
    double size_R = 0.0;
    double entry_price = 0.0;
    double mfe_bp = 0.0;
    double mae_bp = 0.0;
    bool active = false;

    void reset() {
        dir = CompressionDirection::NONE;
        size_R = 0.0;
        entry_price = 0.0;
        mfe_bp = 0.0;
        mae_bp = 0.0;
        active = false;
    }
};

class CompressionBreakoutEngine {
public:
    CompressionPosition pos;
    
    std::string symbol;
    
    int compression_ticks = 0;
    int cooldown_ticks = 0;
    
    double total_pnl_bp = 0.0;
    int total_trades = 0;
    int winning_trades = 0;
    double last_trade_entry_px = 0.0;
    double last_trade_pnl_bp   = 0.0;

    explicit CompressionBreakoutEngine(const std::string& sym = "")
        : symbol(sym) {}

    void evaluate(double price,
                  double vol_ratio_smooth,
                  double displacement_bp,
                  double short_vol,
                  double long_vol_ema,
                  double acceleration_bp,
                  int regime,
                  int64_t timestamp,
                  double available_R)
    {
        track_range(price);
        if (cooldown_ticks > 0)
            cooldown_ticks--;

        // =====================================================
        // PHASE 1: COMPRESSION DETECTION
        // =====================================================
        if (!pos.active && cooldown_ticks == 0) {

            bool compression =
                vol_ratio_smooth < 0.8 &&
                rolling_range_bp() < 15.0 &&
                short_vol < long_vol_ema;

            if (compression) {
                compression_ticks++;
            } else {
                compression_ticks = 0;
            }

            bool armed = compression_ticks >= 100;

            if (armed) {
                bool breakout =
                    std::abs(displacement_bp) >= 15.0 &&
                    std::abs(acceleration_bp) >= 8.0 &&
                    short_vol > long_vol_ema;

                if (breakout && available_R >= 0.5) {

                    pos.dir = displacement_bp > 0
                        ? CompressionDirection::LONG
                        : CompressionDirection::SHORT;

                    pos.size_R = 0.5;
                    pos.entry_price = price;
                    pos.mfe_bp = 0.0;
                    pos.mae_bp = 0.0;
                    pos.active = true;

                    const char* dir_str =
                        (pos.dir == CompressionDirection::LONG) ? "LONG" : "SHORT";

                    std::printf("[COMPRESSION-ENTER] %s | %s | size=0.5R | px=%.2f | range=%.1fbp | comp_ticks=%d\n",
                                symbol.c_str(),
                                dir_str,
                                price,
                                rolling_range_bp(),
                                compression_ticks);
                    std::fflush(stdout);

                    compression_ticks = 0;
                }
            }

            return;
        }

        // =====================================================
        // POSITION MANAGEMENT
        // =====================================================

        double pnl_bp =
            (price - pos.entry_price) / pos.entry_price * 10000.0;

        if (pos.dir == CompressionDirection::SHORT)
            pnl_bp *= -1.0;

        pos.mfe_bp = std::max(pos.mfe_bp, pnl_bp);
        pos.mae_bp = std::min(pos.mae_bp, pnl_bp);

        // =====================================================
        // PROFIT-FUNDED PYRAMIDING
        // =====================================================

        if (pos.size_R == 0.5 &&
            pos.mfe_bp >= 15.0 &&
            available_R >= 0.25)
        {
            pos.size_R = 0.75;
            std::printf("[COMPRESSION-ADD1] %s | size=0.75R | mfe=%.1fbp\n",
                        symbol.c_str(),
                        pos.mfe_bp);
            std::fflush(stdout);
        }

        if (pos.size_R == 0.75 &&
            pos.mfe_bp >= 30.0 &&
            available_R >= 0.25)
        {
            pos.size_R = 1.0;
            std::printf("[COMPRESSION-ADD2] %s | size=1.0R | mfe=%.1fbp\n",
                        symbol.c_str(),
                        pos.mfe_bp);
            std::fflush(stdout);
        }

        // =====================================================
        // DECAY DETECTION (FULL EXIT)
        // =====================================================

        bool decay = false;
        std::string reason;

        if (vol_ratio_smooth < 1.1) {
            decay = true;
            reason = "vol_collapse";
        }

        if (pos.mfe_bp > 0 &&
            pnl_bp < pos.mfe_bp * 0.6)
        {
            decay = true;
            reason = "retrace";
        }

        if (std::abs(acceleration_bp) >= 10.0 &&
            ((acceleration_bp < 0 && pos.dir == CompressionDirection::LONG) ||
             (acceleration_bp > 0 && pos.dir == CompressionDirection::SHORT)))
        {
            decay = true;
            reason = "opposing_accel";
        }

        if (decay) {

            double gross_pnl = pnl_bp;
            double final_pnl = (pnl_bp - ROUND_TRIP_COST_BP) * pos.size_R;  // net after 8bp cost

            total_pnl_bp += final_pnl;
            total_trades++;
            if (final_pnl > 0)
                winning_trades++;

            std::printf("[COMPRESSION-EXIT] %s | net=%.2fbp (gross=%.2f cost=%.1f) | size=%.2fR | mfe=%.1f | mae=%.1f | reason=%s | total=%.1fbp\n",
                        symbol.c_str(),
                        final_pnl, gross_pnl, ROUND_TRIP_COST_BP,
                        pos.size_R,
                        pos.mfe_bp,
                        pos.mae_bp,
                        reason.c_str(),
                        total_pnl_bp);
            std::fflush(stdout);

            cooldown_ticks = 40;
            last_trade_entry_px = pos.entry_price;
            last_trade_pnl_bp   = final_pnl;
            pos.reset();
        }
    }

    double get_win_rate() const {
        return total_trades > 0
            ? static_cast<double>(winning_trades) / total_trades
            : 0.0;
    }

    struct Stats {
        bool active;
        CompressionDirection dir;
        double size_R;
        double entry_price;
        double mfe_bp;
        double mae_bp;
        double total_pnl_bp;
        int total_trades;
        double win_rate;
        bool cooldown_active;
        int cooldown_remaining;
        int compression_ticks;
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
        s.compression_ticks = compression_ticks;
        return s;
    }

private:
    std::deque<double> price_window;
    static constexpr int range_window = 50;

    void track_range(double price) {
        price_window.push_back(price);
        if ((int)price_window.size() > range_window)
            price_window.pop_front();
    }

    double rolling_range_bp() const {
        if (price_window.empty()) return 0.0;
        double min_p = *std::min_element(price_window.begin(), price_window.end());
        double max_p = *std::max_element(price_window.begin(), price_window.end());
        if (min_p == 0.0) return 0.0;
        return (max_p - min_p) / min_p * 10000.0;
    }
};

} // namespace chimera
