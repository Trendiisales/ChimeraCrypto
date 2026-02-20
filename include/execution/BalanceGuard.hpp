#pragma once
#include <cmath>
#include <atomic>

namespace chimera {

class BalanceGuard {
public:
    void set_tolerance(double t)
    {
        tolerance_ = t;
    }

    bool check(double expected,
               double exchange_balance)
    {
        if (std::fabs(expected - exchange_balance) > tolerance_)
        {
            halted_ = true;
            return false;
        }
        return true;
    }

    bool halted() const
    {
        return halted_;
    }

    void reset()
    {
        halted_ = false;
    }

private:
    double tolerance_ = 0.50;
    std::atomic<bool> halted_{false};
};

}
