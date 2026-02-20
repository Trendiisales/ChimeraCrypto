#pragma once
#include <vector>

namespace chimera {

struct Level {
    double price = 0.0;
    double qty = 0.0;
};

struct Snapshot {
    std::vector<Level> bids;
    std::vector<Level> asks;
    unsigned long last_update_id = 0;
};

struct DepthEvent {
    std::vector<Level> bids;
    std::vector<Level> asks;
    unsigned long first_update_id = 0;
    unsigned long final_update_id = 0;
};

}
