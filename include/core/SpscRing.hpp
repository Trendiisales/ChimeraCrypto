#pragma once
#include <atomic>
#include <cstddef>

namespace chimera {

template<typename T, size_t N>
class SpscRing {
public:
    SpscRing() : head_(0), tail_(0) {}

    bool push(const T& v) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) % N;
        if (next == tail_.load(std::memory_order_acquire))
            return false;
        buffer_[head] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire))
            return false;
        out = buffer_[tail];
        tail_.store((tail + 1) % N, std::memory_order_release);
        return true;
    }

private:
    T buffer_[N];
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

}
