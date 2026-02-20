#pragma once
#include <cmath>
#include <cstdint>

namespace chimera {

struct alignas(64) ImpulseState {
    double last_price;
    double last_move_bps;
    int64_t last_ts;
};

class ImpulseMatrix {
public:
    void reset() {
        for (int i = 0; i < 3; ++i) {
            states_[i].last_price = 0.0;
            states_[i].last_move_bps = 0.0;
            states_[i].last_ts = 0;
        }
    }

    inline void update(int id, double price, int64_t ts) {
        auto& s = states_[id];
        if (s.last_price > 0.0) {
            double move = (price - s.last_price) / s.last_price * 10000.0;
            s.last_move_bps = move;
        }
        s.last_price = price;
        s.last_ts = ts;
    }

    inline bool follower_signal(int leader, int follower, int64_t now) {
        const auto& L = states_[leader];
        const auto& F = states_[follower];

        if (now - L.last_ts > 120) return false;

        if (std::fabs(L.last_move_bps) > 18.0 && std::fabs(F.last_move_bps) < 6.0) {
            return true;
        }
        return false;
    }

private:
    ImpulseState states_[3];
};

}
