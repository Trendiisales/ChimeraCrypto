#pragma once
#include "ExecutionPolicy.hpp"
#include "ExecutionTelemetry.hpp"
#include <chrono>

namespace chimera {

struct MarketSnapshot {
    double mid = 0.0;
    double spread = 0.0;
    double imbalance = 0.5;
    bool valid = false;
};

class ExecutionPolicyGovernor {
public:
    PolicyState evaluate(const ExecutionMetrics& exec,
                        const MarketSnapshot& snap,
                        bool bookReady) {
        PolicyState state;
        
        state.latencyMs = exec.avgRttMs;
        state.avgSlippage = exec.avgSlippageBps;  // FIXED: Now in BPS
        state.rejectRate = exec.rejectRate;
        
        // Check feed health
        if (!bookReady || !snap.valid) {
            state.mode = TradingMode::DISABLE;
            state.feedHealthy = false;
            state.reason = "Depth feed not ready";
            return state;
        }
        
        // Check reject rate
        if (exec.rejectRate > 0.05) {
            state.mode = TradingMode::DISABLE;
            state.exchangeStable = false;
            state.reason = "High reject rate";
            return state;
        }
        
        // Check latency - moderate spike
        if (exec.avgRttMs > 10.0 && exec.avgRttMs <= 20.0) {
            state.mode = TradingMode::REDUCE_SIZE;
            state.reason = "Elevated latency";
            return state;
        }
        
        // Check latency - severe spike
        if (exec.avgRttMs > 20.0) {
            state.mode = TradingMode::DISABLE;
            state.reason = "Severe latency";
            return state;
        }
        
        // Check slippage - FIXED: Now comparing BPS to BPS
        if (exec.avgSlippageBps > 5.0) {
            state.mode = TradingMode::POST_ONLY;
            state.reason = "High slippage";
            return state;
        }
        
        // All good
        state.mode = TradingMode::ENABLE;
        state.reason = "Normal operation";
        return state;
    }
};

} // namespace chimera
