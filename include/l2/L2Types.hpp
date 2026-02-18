#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace chimera {

struct L2Level {
    double price;
    double quantity;
};

struct DepthEvent {
    std::string symbol;
    uint64_t U;  // first update id
    uint64_t u;  // final update id
    std::vector<L2Level> bids;
    std::vector<L2Level> asks;
};

struct Snapshot {
    std::string symbol;
    uint64_t lastUpdateId;
    std::vector<L2Level> bids;
    std::vector<L2Level> asks;
};

}
