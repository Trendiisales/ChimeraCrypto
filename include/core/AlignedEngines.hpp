#pragma once
#include <cstdint>
#include <cstring>

namespace chimera {

struct alignas(64) SymbolState {
    double last_price;
    double entry_price;
    double vol_short;
    double vol_long;
    int64_t last_exit_ms;
    bool in_position;
    bool long_side;

    void reset() {
        std::memset(this, 0, sizeof(*this));
    }
};

}
