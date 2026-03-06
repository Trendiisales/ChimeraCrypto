#ifndef COCKPIT_SERIALIZER_HPP
#define COCKPIT_SERIALIZER_HPP

#include "CockpitMessage.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

/**
 * CockpitSerializer - Converts CockpitMessage to JSON for GUI
 */
class CockpitSerializer {
public:
    // Single symbol message
    static std::string to_json(const CockpitMessage& m) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        
        ss << "{";
        ss << "\"type\":\"TICK\",";
        ss << "\"symbol\":\"" << m.symbol << "\",";
        ss << "\"bid\":" << m.bid << ",";
        ss << "\"ask\":" << m.ask << ",";
        ss << "\"mid\":" << m.mid << ",";
        ss << "\"last\":" << m.last << ",";
        ss << "\"spread_bps\":" << m.spread_bps << ",";
        ss << "\"timestamp\":" << m.timestamp << ",";
        ss << "\"from_crypto\":" << (m.from_crypto ? "true" : "false") << ",";
        ss << "\"from_mt5\":" << (m.from_mt5 ? "true" : "false") << ",";
        
        // Position
        ss << "\"position_size\":" << m.position_size << ",";
        ss << "\"entry_price\":" << m.entry_price << ",";
        ss << "\"unrealized_pnl\":" << m.unrealized_pnl << ",";
        ss << "\"realized_pnl\":" << m.realized_pnl << ",";
        
        // Risk
        ss << "\"exposure\":" << m.exposure << ",";
        ss << "\"open_positions\":" << m.open_positions << ",";
        ss << "\"max_drawdown\":" << m.max_drawdown << ",";
        
        // Microstructure
        ss << "\"ofi\":" << m.ofi << ",";
        ss << "\"imbalance\":" << m.imbalance << ",";
        ss << "\"vpin\":" << m.vpin << ",";
        ss << "\"volatility\":" << m.volatility << ",";
        ss << "\"bid_pressure\":" << m.bid_pressure << ",";
        ss << "\"ask_pressure\":" << m.ask_pressure << ",";
        
        // Signal
        ss << "\"signal_action\":\"" << m.signal_action << "\",";
        ss << "\"signal_side\":\"" << m.signal_side << "\",";
        ss << "\"signal_confidence\":" << m.signal_confidence << ",";
        
        // Latency
        ss << "\"rtt_ms\":" << m.rtt_ms << ",";
        ss << "\"feed_latency_ms\":" << m.feed_latency_ms << ",";
        
        // Account equity
        ss << "\"crypto_equity\":" << m.crypto_equity << ",";
        ss << "\"crypto_pnl\":" << m.crypto_pnl << ",";
        ss << "\"mt5_equity\":" << m.mt5_equity << ",";
        ss << "\"mt5_pnl\":" << m.mt5_pnl;
        
        ss << "}";
        
        return ss.str();
    }
    
    // Batch message with all symbols - sends ONE message
    static std::string to_batch_json(const std::vector<CockpitMessage>& messages, 
                                      double feed_latency_ms, double rtt_ms) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        
        ss << "{";
        ss << "\"type\":\"STATE\",";
        ss << "\"feed_latency_ms\":" << feed_latency_ms << ",";
        ss << "\"rtt_ms\":" << rtt_ms << ",";
        ss << "\"symbols\":[";
        
        bool first = true;
        for (const auto& m : messages) {
            if (!first) ss << ",";
            first = false;
            
            ss << "{";
            ss << "\"symbol\":\"" << m.symbol << "\",";
            ss << "\"bid\":" << m.bid << ",";
            ss << "\"ask\":" << m.ask << ",";
            ss << "\"mid\":" << m.mid << ",";
            ss << "\"last\":" << m.last << ",";
            ss << "\"spread_bps\":" << m.spread_bps << ",";
            ss << "\"from_crypto\":" << (m.from_crypto ? "true" : "false") << ",";
            ss << "\"from_mt5\":" << (m.from_mt5 ? "true" : "false") << ",";
            ss << "\"ofi\":" << m.ofi << ",";
            ss << "\"imbalance\":" << m.imbalance << ",";
            ss << "\"vpin\":" << m.vpin << ",";
            ss << "\"volatility\":" << m.volatility << ",";
            ss << "\"position_size\":" << m.position_size << ",";
            ss << "\"unrealized_pnl\":" << m.unrealized_pnl;
            ss << "}";
        }
        
        ss << "]}";
        
        return ss.str();
    }
};

#endif // COCKPIT_SERIALIZER_HPP
