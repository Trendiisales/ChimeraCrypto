#pragma once
#include <atomic>
#include <string>

namespace chimera {

class RiskGovernor {
public:
    explicit RiskGovernor(double initial_equity)
        : equity_(initial_equity)
    {}

    inline bool allow_global() const
    {
        return !halted_.load(std::memory_order_relaxed);
    }

    inline void force_halt(const std::string&)
    {
        halted_.store(true, std::memory_order_relaxed);
    }

    inline void clear_halt()
    {
        halted_.store(false, std::memory_order_relaxed);
    }

    inline bool is_halted() const
    {
        return halted_.load(std::memory_order_relaxed);
    }

private:
    double equity_;
    std::atomic<bool> halted_{false};
};

}
