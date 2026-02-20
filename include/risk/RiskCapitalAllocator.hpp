#pragma once

namespace chimera {

class RiskCapitalAllocator {
public:
    explicit RiskCapitalAllocator(double max_leverage = 3.0)
        : max_leverage_(max_leverage) {}

    double max_leverage() const { return max_leverage_; }

private:
    double max_leverage_;
};

}
