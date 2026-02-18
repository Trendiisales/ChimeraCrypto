#pragma once
#include "ExecutionEvent.hpp"
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace chimera {

enum class OrderSide {
    BUY,
    SELL
};

class ExecutionTracker {
public:
    void recordSend(const std::string& id, double intendedPrice, OrderSide side) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = orders_[id];
        state.sendTs = now();
        state.intendedPrice = intendedPrice;
        state.side = side;
    }
    
    ExecutionEvent recordAck(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = orders_[id];
        
        state.ackTs = now();
        
        ExecutionEvent e;
        e.orderId = id;
        e.type = ExecType::NEW_ACK;
        e.sendTs = state.sendTs;
        e.ackTs = state.ackTs;
        e.rttMs = (state.ackTs - state.sendTs) / 1000.0;
        
        return e;
    }
    
    ExecutionEvent recordFill(const std::string& id, double fillPrice, double qty) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = orders_[id];
        
        state.fillTs = now();
        
        ExecutionEvent e;
        e.orderId = id;
        e.type = ExecType::FILL;
        e.sendTs = state.sendTs;
        e.ackTs = state.ackTs;
        e.fillTs = state.fillTs;
        e.price = fillPrice;
        e.qty = qty;
        e.rttMs = (state.fillTs - state.sendTs) / 1000.0;
        
        // CRITICAL FIX: Side-aware slippage in BPS
        double rawDelta = fillPrice - state.intendedPrice;
        if (state.side == OrderSide::SELL) {
            rawDelta = state.intendedPrice - fillPrice;  // Flip for sells
        }
        
        // Convert to basis points (BPS)
        e.slippage = (rawDelta / state.intendedPrice) * 10000.0;
        
        orders_.erase(id);
        
        return e;
    }
    
    ExecutionEvent recordReject(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& state = orders_[id];
        
        ExecutionEvent e;
        e.orderId = id;
        e.type = ExecType::REJECT;
        e.sendTs = state.sendTs;
        e.rejected = true;
        
        orders_.erase(id);
        
        return e;
    }
    
private:
    struct OrderState {
        uint64_t sendTs = 0;
        uint64_t ackTs = 0;
        uint64_t fillTs = 0;
        double intendedPrice = 0.0;
        OrderSide side = OrderSide::BUY;
    };
    
    uint64_t now() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    std::mutex mutex_;
    std::unordered_map<std::string, OrderState> orders_;
};

} // namespace chimera
