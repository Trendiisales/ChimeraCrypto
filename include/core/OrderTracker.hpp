#pragma once
#include <array>
#include <cstdint>
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
        size_t idx = static_cast<size_t>(id);
        orders_[idx].active = true;
        orders_[idx].price = price;
        orders_[idx].size = size;
        orders_[idx].is_buy = is_buy;
        orders_[idx].timestamp = __builtin_ia32_rdtsc();
        total_orders_++;
    }
    
    int total_orders() const { return total_orders_; }
    
private:
    std::array<TrackedOrder, static_cast<size_t>(SymbolID::COUNT)> orders_;
    int total_orders_ = 0;
};

}
