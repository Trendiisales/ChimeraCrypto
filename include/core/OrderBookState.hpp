#pragma once
#include <cstdint>
#include <algorithm>

namespace chimera {

struct alignas(64) OrderBookState {
    double best_bid;
    double best_ask;
    double bid_size;
    double ask_size;

    inline double spread_bps() const {
        double mid = (best_bid + best_ask) * 0.5;
        return (best_ask - best_bid) / mid * 10000.0;
    }

    inline double imbalance() const {
        double total = bid_size + ask_size;
        if (total == 0.0) return 0.5;
        return bid_size / total;
    }
};

}
