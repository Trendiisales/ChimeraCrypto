#pragma once
#include <atomic>
#include <cstddef>

namespace chimera {

template<size_t N>
class LockFreeRingBuffer {
public:
    void push(double value) {
        size_t head = head_.load(std::memory_order_relaxed);
        data_[head] = value;
        head_.store((head + 1) % N, std::memory_order_release);
        
        size_t cnt = count_.load(std::memory_order_relaxed);
        if (cnt < N)
            count_.store(cnt + 1, std::memory_order_release);
    }

    double latest() const {
        size_t cnt = count_.load(std::memory_order_acquire);
        if (cnt == 0) return 0.0;
        size_t head = head_.load(std::memory_order_acquire);
        size_t idx = (head == 0) ? N - 1 : head - 1;
        return data_[idx];
    }

    double mean() const {
        size_t cnt = count_.load(std::memory_order_acquire);
        if (cnt == 0) return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < cnt; ++i)
            sum += data_[i];
        return sum / cnt;
    }

private:
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> count_{0};
    alignas(64) double data_[N];
};

}

namespace Chimera {

class NetworkLatencySystem {
public:
    void record_market_rtt(double ms) {
        buffer_.push(ms);
    }

    double latest() const { return buffer_.latest(); }
    double mean() const { return buffer_.mean(); }
    
    double p95() const {
        return latest() * 1.3;
    }

private:
    chimera::LockFreeRingBuffer<300> buffer_;
};

}
