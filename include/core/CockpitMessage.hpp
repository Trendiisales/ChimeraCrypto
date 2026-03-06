#ifndef COCKPIT_MESSAGE_HPP
#define COCKPIT_MESSAGE_HPP

#include <string>
#include <vector>

/**
 * CockpitMessage - Data structure sent to GUI
 * 
 * Contains all real-time data the cockpit GUI needs:
 * - Current tick data
 * - Position information
 * - Risk metrics
 * - Microstructure analysis
 */
struct CockpitMessage {
    // Tick data
    std::string symbol;
    double bid = 0.0;
    double ask = 0.0;
    double mid = 0.0;
    double last = 0.0;
    double spread_bps = 0.0;
    uint64_t timestamp = 0;
    
    // Source
    bool from_crypto = false;
    bool from_mt5 = false;
    
    // Position data
    double position_size = 0.0;
    double entry_price = 0.0;
    double unrealized_pnl = 0.0;
    double realized_pnl = 0.0;
    
    // Risk metrics
    double exposure = 0.0;
    int open_positions = 0;
    double max_drawdown = 0.0;
    
    // Microstructure
    double ofi = 0.0;           // Order flow imbalance
    double imbalance = 0.0;     // Bid/ask imbalance
    double vpin = 0.0;          // Volume-sync probability informed trading
    double volatility = 0.0;    // Short-horizon volatility
    double bid_pressure = 0.0;
    double ask_pressure = 0.0;
    
    // Strategy signals
    std::string signal_action;  // HOLD, ENTER, EXIT, REVERSE
    std::string signal_side;    // BUY, SELL
    double signal_confidence = 0.0;
    
    // Latency
    double rtt_ms = 0.0;
    double feed_latency_ms = 0.0;
    
    // Account equity (per engine)
    double crypto_equity = 0.0;
    double crypto_pnl = 0.0;
    double mt5_equity = 0.0;
    double mt5_pnl = 0.0;
};

#endif // COCKPIT_MESSAGE_HPP
