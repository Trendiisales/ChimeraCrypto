#pragma once
#include <cstdint>
#include <ctime>

namespace chimera {

class PnLGovernor {
public:
    PnLGovernor() : realized_bps_(0.0), daily_limit_(-120.0), kill_(false), last_reset_day_(-1) {}

    inline void record(double pnl_bps) {
        reset_if_new_day();
        realized_bps_ += pnl_bps;
        if (realized_bps_ <= daily_limit_) kill_ = true;
    }

    // Call from main loop each tick — resets daily PnL at UTC midnight
    inline void reset_if_new_day() {
        std::time_t now = std::time(nullptr);
        std::tm* utc = std::gmtime(&now);
        int today = utc->tm_yday + utc->tm_year * 366; // unique day key
        if (last_reset_day_ != today) {
            realized_bps_  = 0.0;
            kill_          = false;
            last_reset_day_ = today;
        }
    }

    inline bool blocked() const { return kill_; }

private:
    double   realized_bps_;
    double   daily_limit_;
    bool     kill_;
    int      last_reset_day_;
};

}
