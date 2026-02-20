#include "alpha/SpreadCompression.hpp"

using namespace chimera;

void SpreadCompression::record(double bid, double ask)
{
    spread_hist_.push_back(ask - bid);
    if (spread_hist_.size() > max_samples_)
        spread_hist_.pop_front();
}

double SpreadCompression::compression_score() const
{
    if (spread_hist_.size() < 5)
        return 0.0;

    double latest = spread_hist_.back();
    double avg = 0;
    for (auto s : spread_hist_)
        avg += s;
    avg /= spread_hist_.size();

    return (avg - latest) / (avg + 1e-9);
}
