#pragma once

namespace chimera {

/**
 * Global market environment state.
 * 
 * Aggregates cross-symbol conditions that affect capital allocation
 * and risk management decisions.
 */
struct MarketEnv {
    // Volatility metrics (aggregated across all symbols)
    double short_range = 0.0;        // Average short-term volatility
    double long_range = 1.0;         // Average long-term volatility
    double vol_ratio = 1.0;          // short_range / long_range
    double vol_acceleration = 0.0;   // Change in vol_ratio
    
    // Spread conditions
    double spread_bps = 1.0;         // Average spread across symbols
    
    // Book conditions
    double book_imbalance = 0.0;     // Average imbalance across symbols
    double queue_density = 1.0;      // Average queue depth
    
    // Funding (for perp markets)
    double funding_rate = 0.0;       // Average funding rate
    
    // Latency regime
    double latency_ms = 0.0;         // Current p95 latency
    bool net_clean = true;           // Latency < threshold (e.g., 8ms)
    
    /**
     * Reset to neutral state
     */
    void reset() {
        short_range = 0.0;
        long_range = 1.0;
        vol_ratio = 1.0;
        vol_acceleration = 0.0;
        spread_bps = 1.0;
        book_imbalance = 0.0;
        queue_density = 1.0;
        funding_rate = 0.0;
        latency_ms = 0.0;
        net_clean = true;
    }
};

} // namespace chimera
