#pragma once
#include <vector>
#include <cstdint>

namespace chimera {

struct Slice {
    double qty;
    uint64_t delayMs;
};

class OrderSlicer {
public:
    std::vector<Slice> slice(double totalQty, int slices, 
                            uint64_t totalDurationMs) {
        std::vector<Slice> out;

        double perSlice = totalQty / slices;
        uint64_t delay = totalDurationMs / slices;

        for (int i = 0; i < slices; ++i) {
            out.push_back({ perSlice, delay });
        }

        return out;
    }
};

} // namespace chimera
