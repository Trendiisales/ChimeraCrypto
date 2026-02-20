#pragma once
#include <atomic>
#include <vector>

namespace chimera {

template<typename T>
class LockFreeRing {
public:
    explicit LockFreeRing(size_t capacity)
        : buffer_(capacity),
          capacity_(capacity)
    {}

    bool push(const T& item)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next =
            (head + 1) % capacity_;

        if (next ==
            tail_.load(std::memory_order_acquire))
            return false;

        buffer_[head] = item;

        head_.store(next,
                    std::memory_order_release);

        return true;
    }

    bool pop(T& item)
    {
        size_t tail =
            tail_.load(std::memory_order_relaxed);

        if (tail ==
            head_.load(std::memory_order_acquire))
            return false;

        item = buffer_[tail];

        tail_.store((tail + 1) % capacity_,
                    std::memory_order_release);

        return true;
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;

    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

}
