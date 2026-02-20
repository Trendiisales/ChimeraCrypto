#include "alpha/ImbalanceVelocity.hpp"

using namespace chimera;

void ImbalanceVelocity::record(double bid_qty, double ask_qty)
{
    double imb = (bid_qty - ask_qty) / (bid_qty + ask_qty + 1e-9);
    imbalance_hist_.push_back(imb);

    if (imbalance_hist_.size() > max_samples_)
        imbalance_hist_.pop_front();
}

double ImbalanceVelocity::velocity_score() const
{
    if (imbalance_hist_.size() < 5)
        return 0.0;

    double recent = imbalance_hist_.back();
    double prev = imbalance_hist_[imbalance_hist_.size()-5];

    return recent - prev;
}
