#pragma once
#include <cstdint>

namespace chimera {

/**
 * Order book state snapshot for a single symbol at a point in time.
 * 
 * This is the fundamental input to all microstructure analysis engines.
 * Updated on every tick with current market conditions.
 */
struct BookState {
    // Order book liquidity
    double bid_depth = 0.0;          // Total size on bid side
    double ask_depth = 0.0;          // Total size on ask side
    double bid_price = 0.0;          // Best bid
    double ask_price = 0.0;          // Best ask
    double mid_price = 0.0;          // (bid + ask) / 2
    double spread_bps = 0.0;         // ((ask - bid) / mid) * 10000
    
    // Book imbalance (-1 to +1)
    // Positive = bid depth > ask depth (buying pressure)
    // Negative = ask depth > bid depth (selling pressure)
    double imbalance = 0.0;          // (bid_depth - ask_depth) / (bid_depth + ask_depth)
    
    // Queue density (0 to 1)
    // Normalized measure of total liquidity
    // 1.0 = very deep book, 0.0 = thin book
    double queue_density = 0.0;
    
    // Trade flow
    double trade_volume = 0.0;           // Total volume traded in window
    double aggressive_buy_volume = 0.0;  // Market buy (taker) volume
    double aggressive_sell_volume = 0.0; // Market sell (taker) volume
    
    // Volatility metrics
    double short_range = 0.0;        // Short-term price range (e.g., 10-tick)
    double long_range = 0.0;         // Long-term price range (e.g., 200-tick)
    
    // Funding (for perp markets, 0.0 for spot)
    double funding_rate = 0.0;       // Per 8h funding rate
    
    // Timing
    int64_t timestamp_ms = 0;        // Millisecond timestamp
    double latency_ms = 0.0;         // Observed latency for this update
    
    /**
     * Calculate derived metrics from raw book data
     */
    void update_derived() {
        // Spread
        if (bid_price > 0.0 && ask_price > 0.0) {
            mid_price = (bid_price + ask_price) / 2.0;
            spread_bps = ((ask_price - bid_price) / mid_price) * 10000.0;
        }
        
        // Imbalance
        double total_depth = bid_depth + ask_depth;
        if (total_depth > 1e-6) {
            imbalance = (bid_depth - ask_depth) / total_depth;
        } else {
            imbalance = 0.0;
        }
        
        // Queue density (normalized to [0, 1])
        // Assumption: "normal" depth is 1.0, anything above increases density
        queue_density = std::min(total_depth, 2.0) / 2.0;
    }
    
    /**
     * Reset all fields to zero
     */
    void reset() {
        bid_depth = 0.0;
        ask_depth = 0.0;
        bid_price = 0.0;
        ask_price = 0.0;
        mid_price = 0.0;
        spread_bps = 0.0;
        imbalance = 0.0;
        queue_density = 0.0;
        trade_volume = 0.0;
        aggressive_buy_volume = 0.0;
        aggressive_sell_volume = 0.0;
        short_range = 0.0;
        long_range = 0.0;
        funding_rate = 0.0;
        timestamp_ms = 0;
        latency_ms = 0.0;
    }
};

} // namespace chimera
