#pragma once
#include <atomic>
#include <chrono>
#include <cstring>

namespace chimera {

class LatencyKillSwitch {
public:
    void record_rtt(double ms)
    {
        uint64_t bits;
        std::memcpy(&bits, &ms, sizeof(double));
        last_rtt_bits_.store(bits, std::memory_order_relaxed);
        
        if (ms > max_latency_ms_)
            halted_.store(true, std::memory_order_release);
    }

    bool halted() const
    {
        return halted_.load(std::memory_order_acquire);
    }
    
    double last_rtt() const
    {
        uint64_t bits = last_rtt_bits_.load(std::memory_order_relaxed);
        double result;
        std::memcpy(&result, &bits, sizeof(double));
        return result;
    }

    void reset()
    {
        halted_.store(false, std::memory_order_release);
    }

private:
    double max_latency_ms_ = 80.0;
    std::atomic<uint64_t> last_rtt_bits_{0};  // FIXED: atomic double -> atomic uint64_t
    std::atomic<bool> halted_{false};
};

}
