#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include "core/Symbol.hpp"

namespace chimera {

struct TrackedOrder {
    bool active = false;
    double price = 0.0;
    double size = 0.0;
    bool is_buy = false;
    uint64_t timestamp = 0;
};

class OrderTracker {
public:
    void track(SymbolID id, bool is_buy, double price, double size) {
        TrackedOrder order;
        order.active = true;
        order.price = price;
        order.size = size;
        order.is_buy = is_buy;
        order.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        orders_[id] = order;
        total_orders_++;
    }
    
    int total_orders() const { return total_orders_; }
    
private:
    std::unordered_map<SymbolID, TrackedOrder> orders_;
    int total_orders_ = 0;
};

}
