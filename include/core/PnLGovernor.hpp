#pragma once
#include <cstdint>

namespace chimera {

class PnLGovernor {
public:
    PnLGovernor() : realized_bps_(0.0), daily_limit_(-120.0), kill_(false) {}

    inline void record(double pnl_bps) {
        realized_bps_ += pnl_bps;
        if (realized_bps_ <= daily_limit_) kill_ = true;
    }

    inline bool blocked() const { return kill_; }

private:
    double realized_bps_;
    double daily_limit_;
    bool kill_;
};

}
