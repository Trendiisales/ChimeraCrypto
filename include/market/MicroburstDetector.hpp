#pragma once
#include <deque>
#include <cstddef>

namespace chimera {

class MicroburstDetector {
public:
    void record(double bid,
                double ask,
                double bid_size,
                double ask_size);

    void record_tick(double bid,
                     double ask,
                     double bid_size,
                     double ask_size)
    {
        record(bid, ask, bid_size, ask_size);
    }

    bool collapse_detected() const;
    bool refill_detected() const;

    bool burst_detected() const
    {
        return collapse_detected();
    }

private:
    struct Snapshot {
        double spread;
        double depth;
    };

    std::deque<Snapshot> window_;
    size_t max_samples_ = 20;
};

}
