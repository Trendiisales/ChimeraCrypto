#pragma once
#include <deque>
#include <cstdint>
#include <cstddef>

namespace chimera {

struct ImbalanceAccelSignal {
    bool active;
    double acceleration;
    uint64_t timestamp_ns;
};

class ImbalanceAcceleration {
public:
    ImbalanceAcceleration(size_t window, double threshold);
    ImbalanceAccelSignal update(double imbalance, uint64_t timestamp_ns);

private:
    size_t window_;
    double threshold_;
    std::deque<double> history_;
};

}
