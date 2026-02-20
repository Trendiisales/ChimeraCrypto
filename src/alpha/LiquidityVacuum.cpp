#include "alpha/LiquidityVacuum.hpp"
#include <algorithm>

using namespace chimera;

void LiquidityVacuum::record(double bid_qty, double ask_qty)
{
    bid_hist_.push_back(bid_qty);
    ask_hist_.push_back(ask_qty);

    if (bid_hist_.size() > max_samples_)
        bid_hist_.pop_front();

    if (ask_hist_.size() > max_samples_)
        ask_hist_.pop_front();
}

double LiquidityVacuum::score() const
{
    if (bid_hist_.size() < 5)
        return 0.0;

    double bid_now = bid_hist_.back();
    double ask_now = ask_hist_.back();

    double bid_avg = 0;
    double ask_avg = 0;

    for (auto b : bid_hist_) bid_avg += b;
    for (auto a : ask_hist_) ask_avg += a;

    bid_avg /= bid_hist_.size();
    ask_avg /= ask_hist_.size();

    double bid_drop = (bid_avg - bid_now) / (bid_avg + 1e-9);
    double ask_drop = (ask_avg - ask_now) / (ask_avg + 1e-9);

    return ask_drop - bid_drop;
}
