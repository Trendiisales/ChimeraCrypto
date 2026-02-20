#pragma once
#include <cstdio>
#include <cstdint>

namespace chimera {

enum class DecisionType {
    ACCEPTED,
    REJECTED_SPREAD,
    REJECTED_EDGE,
    REJECTED_LATENCY,
    REJECTED_COST_FLOOR,
    REJECTED_COOLDOWN,
    REJECTED_POSITION_LIMIT
};

class TradeLogger {
public:
    static void log_decision(
        const char* engine_name,
        DecisionType decision,
        double edge_bps,
        double required_bps,
        double spread_bps,
        double latency_ms,
        const char* symbol)
    {
        const char* reason = "";
        switch (decision) {
            case DecisionType::ACCEPTED:
                std::printf("[TRADE] ✓ %s | %s | edge=%.2f required=%.2f spread=%.2f lat=%.2f\n",
                    engine_name, symbol, edge_bps, required_bps, spread_bps, latency_ms);
                break;
            case DecisionType::REJECTED_SPREAD:
                reason = "SPREAD";
                break;
            case DecisionType::REJECTED_EDGE:
                reason = "EDGE";
                break;
            case DecisionType::REJECTED_LATENCY:
                reason = "LATENCY";
                break;
            case DecisionType::REJECTED_COST_FLOOR:
                reason = "COST_FLOOR";
                break;
            case DecisionType::REJECTED_COOLDOWN:
                reason = "COOLDOWN";
                break;
            case DecisionType::REJECTED_POSITION_LIMIT:
                reason = "POSITION_LIMIT";
                break;
        }
        
        if (decision != DecisionType::ACCEPTED) {
            std::printf("[REJECT] ✗ %s | %s | %s | edge=%.2f req=%.2f\n",
                engine_name, symbol, reason, edge_bps, required_bps);
        }
        
        std::fflush(stdout);
    }
    
    static void log_fill(
        const char* engine_name,
        const char* symbol,
        const char* side,
        double price,
        double size,
        double pnl_bps)
    {
        std::printf("[FILL] %s | %s %s | px=%.2f sz=%.4f | pnl=%.2f bps\n",
            engine_name, side, symbol, price, size, pnl_bps);
        std::fflush(stdout);
    }
};

}
