#pragma once
#include <string>
#include <sstream>
#include <iomanip>

namespace chimera {

// JSON message builders for GUI WebSocket feed
class GuiMessageBuilder {
public:
    // Signal rejection message
    static std::string signal_reject(const std::string& symbol, const std::string& layer, const std::string& reason) {
        std::ostringstream oss;
        oss << "{\"type\":\"signal_reject\","
            << "\"symbol\":\"" << symbol << "\","
            << "\"layer\":\"" << layer << "\","
            << "\"reason\":\"" << reason << "\"}";
        return oss.str();
    }
    
    // Signal acceptance message
    static std::string signal_accept(const std::string& symbol, const std::string& layer, double signal_strength) {
        std::ostringstream oss;
        oss << "{\"type\":\"signal_accept\","
            << "\"symbol\":\"" << symbol << "\","
            << "\"layer\":\"" << layer << "\","
            << "\"strength\":" << std::fixed << std::setprecision(2) << signal_strength << "}";
        return oss.str();
    }
    
    // Position entry message
    static std::string position_enter(const std::string& symbol, const std::string& layer, 
                                     double price, int regime, double size_mult) {
        std::ostringstream oss;
        oss << "{\"type\":\"position_enter\","
            << "\"symbol\":\"" << symbol << "\","
            << "\"layer\":\"" << layer << "\","
            << "\"price\":" << std::fixed << std::setprecision(2) << price << ","
            << "\"regime\":" << regime << ","
            << "\"size_mult\":" << size_mult << "}";
        return oss.str();
    }
    
    // Position exit message
    static std::string position_exit(const std::string& symbol, const std::string& layer,
                                    double pnl_bps, int64_t hold_ms, double latency_ms) {
        std::ostringstream oss;
        oss << "{\"type\":\"position_exit\","
            << "\"symbol\":\"" << symbol << "\","
            << "\"layer\":\"" << layer << "\","
            << "\"pnl_bps\":" << std::fixed << std::setprecision(2) << pnl_bps << ","
            << "\"hold_ms\":" << hold_ms << ","
            << "\"latency_ms\":" << std::fixed << std::setprecision(1) << latency_ms << "}";
        return oss.str();
    }
    
    // Regime update message
    static std::string regime_update(const std::string& symbol, int regime, 
                                    double price, int short_n, double short_vol, double long_vol) {
        std::ostringstream oss;
        oss << "{\"type\":\"regime_update\","
            << "\"symbol\":\"" << symbol << "\","
            << "\"regime\":" << regime << ","
            << "\"price\":" << std::fixed << std::setprecision(2) << price << ","
            << "\"short_n\":" << short_n << ","
            << "\"short_vol\":" << std::fixed << std::setprecision(2) << short_vol << ","
            << "\"long_vol\":" << std::fixed << std::setprecision(2) << long_vol << "}";
        return oss.str();
    }
    
    // Engine health message
    static std::string engine_health(double event_loop_ms, double eval_ms, 
                                    int stall_count, int samples) {
        std::ostringstream oss;
        oss << "{\"type\":\"engine_health\","
            << "\"event_loop_ms\":" << std::fixed << std::setprecision(2) << event_loop_ms << ","
            << "\"eval_ms\":" << std::fixed << std::setprecision(2) << eval_ms << ","
            << "\"stall_count\":" << stall_count << ","
            << "\"samples\":" << samples << "}";
        return oss.str();
    }
    
    // Performance summary message
    static std::string performance_summary(double equity, double day_pnl, int trades, 
                                          int open_positions, int loss_streak) {
        std::ostringstream oss;
        oss << "{\"type\":\"performance_summary\","
            << "\"equity\":" << std::fixed << std::setprecision(2) << equity << ","
            << "\"day_pnl\":" << day_pnl << ","
            << "\"trades\":" << trades << ","
            << "\"open_positions\":" << open_positions << ","
            << "\"loss_streak\":" << loss_streak << "}";
        return oss.str();
    }
    
    // Latency band PnL message
    static std::string latency_band_pnl(const std::string& band, int trades, 
                                       double total_pnl, double avg_pnl) {
        std::ostringstream oss;
        oss << "{\"type\":\"latency_band_pnl\","
            << "\"band\":\"" << band << "\","
            << "\"trades\":" << trades << ","
            << "\"total_pnl\":" << std::fixed << std::setprecision(2) << total_pnl << ","
            << "\"avg_pnl\":" << avg_pnl << "}";
        return oss.str();
    }
    
    // Rejection summary message
    static std::string rejection_summary(const std::string& key, int count, const std::string& reason) {
        std::ostringstream oss;
        oss << "{\"type\":\"rejection_summary\","
            << "\"key\":\"" << key << "\","
            << "\"count\":" << count << ","
            << "\"reason\":\"" << reason << "\"}";
        return oss.str();
    }
};

} // namespace chimera
