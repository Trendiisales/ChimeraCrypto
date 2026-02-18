#pragma once
#include <atomic>
#include <chrono>

namespace chimera {

class RestThrottle {
public:
    static bool allow(int weight = 50) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - windowStart_).count();
        
        // Reset window every minute
        if (elapsed >= 60) {
            windowStart_ = now;
            weightUsed_ = 0;
        }
        
        // Keep 500 weight buffer (Binance limit is 6000/min, we stay at 4500)
        if (weightUsed_ + weight > 4500) {
            return false;
        }
        
        weightUsed_ += weight;
        return true;
    }
    
    static int getWeightUsed() {
        return weightUsed_.load();
    }
    
private:
    static inline std::atomic<int> weightUsed_{0};
    static inline std::chrono::steady_clock::time_point windowStart_ = std::chrono::steady_clock::now();
};

} // namespace chimera
