#pragma once
#include <chrono>
#include <atomic>

namespace chimera {

class FloodGuard {
public:
    bool allow()
    {
        auto now = clock::now();

        if (now - last_order_ < min_interval_)
            return false;

        last_order_ = now;
        return true;
    }

private:
    using clock = std::chrono::steady_clock;

    std::chrono::milliseconds min_interval_{150};
    clock::time_point last_order_{clock::now()};
};

}
