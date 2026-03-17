#pragma once
// ============================================================================
// StatArbEngine  BTC/ETH Cointegration Spread Trading
// ============================================================================
// Edge: BTC and ETH share a macro crypto driver. Their log-price ratio is
// cointegrated and mean-reverts over 2-4 hour windows.
//
// Signal:
//   spread = log(BTC_price) - log(ETH_price) * hedge_ratio
//   When spread deviates > ENTRY_ZSCORE stddevs from rolling mean:
//     -> fade the deviation (long the cheap leg direction on spot)
//   Exit: spread returns to within EXIT_ZSCORE stddevs of mean
//
// Spot-only implementation:
//   Long signal (spread too low = BTC cheap vs ETH): long BTC
//   Short signal (spread too high = BTC expensive vs ETH): long ETH
//   (We cannot short on spot, so we express both sides as longs on cheap leg)
//
// Parameters calibrated to crypto market structure:
//   Rolling window: 480 ticks (~4 hours at typical tick rate)
//   Entry Z-score: 2.0 stddevs
//   Exit Z-score:  0.5 stddevs (partial mean reversion is sufficient)
//   Max hold: 4 hours (macro mean reversion is slow)
//   TP: 25bp, SL: 8bp (wider than scalp strategies, this is a structural trade)
// ============================================================================
#include "core/SymbolIndex.hpp"
#include <deque>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <string>
#include <cstdint>

namespace chimera {

class StatArbEngine {
public:
    // Configuration
    static constexpr int     SPREAD_WINDOW       = 480;   // ~4h of ticks
    static constexpr double  ENTRY_ZSCORE        = 2.0;   // stddevs to enter
    static constexpr double  EXIT_ZSCORE         = 0.4;   // stddevs to exit (quick mean reversion capture)
    static constexpr double  HEDGE_RATIO         = 1.0;   // log-spread (no fixed ratio needed)
    static constexpr double  TP_BP               = 20.0;  // gross take-profit
    static constexpr double  SL_BP               = 6.0;   // stop loss
    static constexpr int64_t MAX_HOLD_MS         = 4 * 3600000LL; // 4 hours
    static constexpr int64_t COOLDOWN_MS         = 1800000LL;     // 30 min between trades
    static constexpr int     MIN_WINDOW_SAMPLES  = 120;   // need 1hr of data before trading

    enum class StatArbSignal { NONE, LONG_BTC, LONG_ETH };

    struct Position {
        bool     active       = false;
        StatArbSignal signal  = StatArbSignal::NONE;
        double   entry_spread = 0.0;
        double   entry_price  = 0.0;  // price of the leg we bought
        int      leg_id       = -1;   // which symbol we're long
        int64_t  entry_ts     = 0;
        double   peak_profit  = 0.0;
        double   mfe          = 0.0;
        double   mae          = 0.0;
    };

    struct Stats {
        int    total_trades  = 0;
        int    wins          = 0;
        double total_pnl_bp  = 0.0;
        double best_trade    = 0.0;
        double worst_trade   = 0.0;
    };

    StatArbEngine() {}

    // Called every tick for BTC (id=0) and ETH (id=1)
    void update_price(int id, double price) {
        if (id == 0) btc_price_ = price;
        if (id == 1) eth_price_ = price;

        if (btc_price_ <= 0.0 || eth_price_ <= 0.0) return;

        // Compute log spread: log(BTC) - log(ETH)
        // We normalise both to USD so the spread is dimensionless ratio
        double spread = std::log(btc_price_) - std::log(eth_price_);
        spread_history_.push_back(spread);
        if ((int)spread_history_.size() > SPREAD_WINDOW)
            spread_history_.pop_front();

        current_spread_ = spread;
    }

    // Returns signal and which leg to buy, or NONE
    // Call after update_price for each tick
    StatArbSignal check_signal(double& z_out) const {
        z_out = 0.0;
        if ((int)spread_history_.size() < MIN_WINDOW_SAMPLES) return StatArbSignal::NONE;

        double mean = 0.0, var = 0.0;
        for (double v : spread_history_) mean += v;
        mean /= spread_history_.size();
        for (double v : spread_history_) var += (v - mean) * (v - mean);
        var /= spread_history_.size();
        double stddev = std::sqrt(var);
        if (stddev < 1e-8) return StatArbSignal::NONE;

        z_out = (current_spread_ - mean) / stddev;

        if (z_out < -ENTRY_ZSCORE) return StatArbSignal::LONG_BTC;  // BTC cheap, buy BTC
        if (z_out >  ENTRY_ZSCORE) return StatArbSignal::LONG_ETH;  // ETH cheap, buy ETH
        return StatArbSignal::NONE;
    }

    // Check if open position should be closed
    // Returns true if should exit, sets reason
    bool check_exit(double current_price, double entry_price,
                    int64_t ts, std::string& reason) {
        if (!pos_.active) return false;

        double pnl_bp = (current_price - entry_price) / entry_price * 10000.0;
        pos_.mfe = std::max(pos_.mfe, pnl_bp);
        pos_.mae = std::min(pos_.mae, pnl_bp);
        pos_.peak_profit = std::max(pos_.peak_profit, pnl_bp);

        // TP
        if (pnl_bp >= TP_BP) { reason = "TP"; return true; }
        // SL
        if (pnl_bp <= -SL_BP) { reason = "SL"; return true; }
        // Max hold
        if (ts - pos_.entry_ts > MAX_HOLD_MS) { reason = "TIMEOUT"; return true; }

        // Spread mean reversion exit: spread has returned toward mean
        double z = 0.0;
        StatArbSignal sig = check_signal(z);
        // If spread has reverted past exit zone, close
        bool spread_reverted = false;
        if (pos_.signal == StatArbSignal::LONG_BTC && z > -EXIT_ZSCORE) spread_reverted = true;
        if (pos_.signal == StatArbSignal::LONG_ETH && z <  EXIT_ZSCORE) spread_reverted = true;
        if (spread_reverted && pnl_bp > 0.5) { // only exit if slightly profitable
            reason = "SPREAD_REVERT";
            return true;
        }

        // Trailing stop: once in profit by 8bp, trail at 50% of peak
        if (pos_.peak_profit >= 8.0) {
            double floor = pos_.peak_profit * 0.5;
            if (pnl_bp <= floor) { reason = "TRAIL"; return true; }
        }

        return false;
    }

    void open(StatArbSignal sig, int leg_id, double price, int64_t ts) {
        pos_.active      = true;
        pos_.signal      = sig;
        pos_.leg_id      = leg_id;
        pos_.entry_price = price;
        pos_.entry_spread = current_spread_;
        pos_.entry_ts    = ts;
        pos_.peak_profit = 0.0;
        pos_.mfe         = 0.0;
        pos_.mae         = 0.0;
        last_entry_ts_   = ts;
    }

    void close(double exit_price, const std::string& reason, int64_t ts) {
        if (!pos_.active) return;
        double pnl_bp = (exit_price - pos_.entry_price) / pos_.entry_price * 10000.0;
        // Subtract round trip cost (maker: ~4bp)
        pnl_bp -= 4.0;
        stats_.total_trades++;
        if (pnl_bp > 0) stats_.wins++;
        stats_.total_pnl_bp += pnl_bp;
        if (stats_.total_trades == 1 || pnl_bp > stats_.best_trade) stats_.best_trade = pnl_bp;
        if (stats_.total_trades == 1 || pnl_bp < stats_.worst_trade) stats_.worst_trade = pnl_bp;
        std::printf("[STATARB-EXIT] %s | pnl=%.2fbp | why=%s | spread_entry=%.4f | spread_now=%.4f | hold=%ldms\n",
            pos_.leg_id == 0 ? "BTC" : "ETH",
            pnl_bp, reason.c_str(),
            pos_.entry_spread, current_spread_,
            ts - pos_.entry_ts);
        std::fflush(stdout);
        pos_.active = false;
        pos_.signal = StatArbSignal::NONE;
        last_exit_ts_ = ts;
    }

    bool has_position() const { return pos_.active; }
    int  position_leg() const { return pos_.leg_id; }
    int64_t last_entry_ts() const { return last_entry_ts_; }
    int64_t last_exit_ts()  const { return last_exit_ts_;  }
    const Position& position() const { return pos_; }
    const Stats&    stats()    const { return stats_; }
    double current_spread()    const { return current_spread_; }
    double spread_zscore()     const {
        double z = 0.0; check_signal(z); return z;
    }

    bool in_cooldown(int64_t ts) const {
        return (ts - last_exit_ts_) < COOLDOWN_MS;
    }

private:
    double btc_price_     = 0.0;
    double eth_price_     = 0.0;
    double current_spread_ = 0.0;
    std::deque<double> spread_history_;

    Position pos_;
    Stats    stats_;
    int64_t  last_entry_ts_ = 0;
    int64_t  last_exit_ts_  = 0;
};

} // namespace chimera
