#pragma once
#include <cmath>

namespace chimera {

class LadderManager {
public:
    bool should_reprice(double current_price, double best_bid, double best_ask, bool is_bid) {
        if(is_bid) return current_price < best_bid;
        else return current_price > best_ask;
    }

    double ladder_price(double best_bid, double best_ask, bool is_bid) {
        double tick = 0.01;
        if(is_bid) return best_bid - tick;
        else return best_ask + tick;
    }
};

}
