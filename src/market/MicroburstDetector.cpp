#include "market/MicroburstDetector.hpp"

namespace chimera {

void MicroburstDetector::record(
    double bid,
    double ask,
    double bid_size,
    double ask_size)
{
    Snapshot s;
    s.spread = ask - bid;
    s.depth = bid_size + ask_size;

    window_.push_back(s);

    if (window_.size() > max_samples_)
        window_.pop_front();
}

bool MicroburstDetector::collapse_detected() const
{
    if (window_.size() < 2)
        return false;

    double depth_change =
        window_.back().depth -
        window_.front().depth;

    return depth_change < -1500;
}

bool MicroburstDetector::refill_detected() const
{
    if (window_.size() < 2)
        return false;

    double depth_change =
        window_.back().depth -
        window_.front().depth;

    return depth_change > 1500;
}

}
