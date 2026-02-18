#pragma once
#include "ExecutionEvent.hpp"
#include <atomic>
#include <mutex>

namespace chimera {

struct ExecutionMetrics {
    double avgRttMs = 0.0;
    double worstRttMs = 0.0;
    double avgSlippageBps = 0.0;  // FIXED: Now in BPS
    double rejectRate = 0.0;
    uint64_t totalTrades = 0;
    uint64_t rejects = 0;
};

class ExecutionTelemetry {
public:
    void update(const ExecutionEvent& e) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (e.type == ExecType::FILL) {
            metrics_.totalTrades++;
            
            // Exponential moving average
            metrics_.avgRttMs = 0.9 * metrics_.avgRttMs + 0.1 * e.rttMs;
            
            // FIXED: Slippage already in BPS from tracker
            metrics_.avgSlippageBps = 0.9 * metrics_.avgSlippageBps + 0.1 * e.slippage;
            
            if (e.rttMs > metrics_.worstRttMs) {
                metrics_.worstRttMs = e.rttMs;
            }
        }
        
        if (e.type == ExecType::REJECT) {
            metrics_.rejects++;
        }
        
        if (metrics_.totalTrades + metrics_.rejects > 0) {
            metrics_.rejectRate = double(metrics_.rejects) / double(metrics_.totalTrades + metrics_.rejects);
        }
    }
    
    ExecutionMetrics snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }
    
private:
    mutable std::mutex mutex_;
    ExecutionMetrics metrics_;
};

} // namespace chimera
