#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

/**
 * StructuralExpectancyMonitor - Real-Time Execution Audit Layer
 * 
 * Measures gross edge vs structural cost to detect when the system
 * is trading negative expectancy BEFORE it bleeds capital.
 * 
 * Tracks:
 * - Gross edge (pre-cost PnL)
 * - Structural cost (spread + slip + fees)
 * - Net edge (gross - cost)
 * - Win rate
 * - Expectancy per latency band
 * - Expectancy per regime
 */
class StructuralExpectancyMonitor {
public:

    struct TradeInput {
        double pnl_bps;
        double spread_bps;
        double slippage_bps;
        double fee_bps;
        double expected_move_bps;
        double latency_ms;
        int    regime;  // 0 neutral, 1 burst, 2 revert
    };

private:

    double gross_ema_ = 0.0;
    double cost_ema_  = 0.0;
    double net_ema_   = 0.0;

    double win_rate_ema_ = 0.0;
    double trade_count_  = 0.0;

    double latency_band_net_[3] = {0}; // <5ms, 5-10ms, >10ms
    double regime_net_[3]       = {0};

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    double ema(double prev, double v, double a) {
        return prev * (1.0 - a) + v * a;
    }

    int latency_band(double lat) {
        if (lat < 5.0) return 0;
        if (lat < 10.0) return 1;
        return 2;
    }

public:

    void record(const TradeInput& t) {

        double structural_cost =
            t.spread_bps
          + t.slippage_bps
          + t.fee_bps;

        double net_edge =
            t.pnl_bps - structural_cost;

        gross_ema_ = ema(gross_ema_, t.pnl_bps, 0.05);
        cost_ema_  = ema(cost_ema_, structural_cost, 0.05);
        net_ema_   = ema(net_ema_, net_edge, 0.05);

        trade_count_ += 1.0;

        double win = (net_edge > 0.0) ? 1.0 : 0.0;
        win_rate_ema_ = ema(win_rate_ema_, win, 0.05);

        int band = latency_band(t.latency_ms);
        latency_band_net_[band] =
            ema(latency_band_net_[band], net_edge, 0.05);

        if (t.regime >= 0 && t.regime < 3)
            regime_net_[t.regime] =
                ema(regime_net_[t.regime], net_edge, 0.05);
    }

    double net_expectancy() const {
        return net_ema_;
    }

    double gross_expectancy() const {
        return gross_ema_;
    }

    double structural_cost() const {
        return cost_ema_;
    }

    double win_rate() const {
        return win_rate_ema_;
    }

    double latency_band_expectancy(int band) const {
        if (band < 0 || band > 2) return 0.0;
        return latency_band_net_[band];
    }

    double regime_expectancy(int regime) const {
        if (regime < 0 || regime > 2) return 0.0;
        return regime_net_[regime];
    }

    bool structural_negative() const {
        return net_ema_ < 0.0;
    }

    bool slip_expanding() const {
        return cost_ema_ > gross_ema_;
    }
};

} // namespace chimera
