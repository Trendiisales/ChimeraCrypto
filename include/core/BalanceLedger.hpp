#pragma once
#include <atomic>
#include <cstring>

namespace chimera {

class BalanceLedger {
public:
    void set_expected(double usdt)
    {
        uint64_t bits;
        std::memcpy(&bits, &usdt, sizeof(double));
        expected_usdt_bits_.store(bits, std::memory_order_relaxed);
    }

    void update_expected(double delta)
    {
        double current = expected();
        set_expected(current + delta);
    }

    double expected() const
    {
        uint64_t bits = expected_usdt_bits_.load(std::memory_order_relaxed);
        double result;
        std::memcpy(&result, &bits, sizeof(double));
        return result;
    }

private:
    std::atomic<uint64_t> expected_usdt_bits_{0};  // FIXED: atomic double -> atomic uint64_t
};

}
