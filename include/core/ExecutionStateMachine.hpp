#pragma once
#include <cstdint>

namespace chimera {

enum ExecState : uint8_t {
    FLAT,
    POSTED,
    FILLED,
    EXITING
};

struct alignas(64) ExecutionStateMachine {
    ExecState state;
    double entry_price;
    int64_t entry_ts;

    void reset() {
        state = FLAT;
        entry_price = 0.0;
        entry_ts = 0;
    }

    inline void enter(double price, int64_t ts) {
        state = POSTED;
        entry_price = price;
        entry_ts = ts;
    }

    inline void filled() {
        state = FILLED;
    }

    inline bool should_exit(double price, int64_t ts, double target, double stop) {
        double move = (price - entry_price) / entry_price * 10000.0;

        if (move >= target || move <= -stop || ts - entry_ts > 5000)
            return true;
        return false;
    }

    inline void exit() {
        state = EXITING;
    }

    inline void flat() {
        state = FLAT;
    }
};

}
