#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace chimera {

struct DepthLevel {
    double price = 0.0;
    double qty = 0.0;
};

struct DiffEvent {
    uint64_t U = 0;  // First update ID in event
    uint64_t u = 0;  // Final update ID in event
    
    std::vector<DepthLevel> bids;
    std::vector<DepthLevel> asks;
};

struct Snapshot {
    uint64_t lastUpdateId = 0;
    std::vector<DepthLevel> bids;
    std::vector<DepthLevel> asks;
};

} // namespace chimera
