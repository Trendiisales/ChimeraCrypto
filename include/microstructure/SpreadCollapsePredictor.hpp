#pragma once
#include <deque>
#include <cstdint>
#include <cstddef>

namespace chimera {

struct SpreadCollapseSignal {
    bool active;
    double compression;
    uint64_t timestamp_ns;
};

class SpreadCollapsePredictor {
public:
    SpreadCollapsePredictor(size_t window, double compression_threshold);
    SpreadCollapseSignal update(double spread, uint64_t timestamp_ns);

private:
    size_t window_;
    double threshold_;
    std::deque<double> spreads_;
};

}
