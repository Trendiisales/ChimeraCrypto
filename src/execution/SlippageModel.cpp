#include "execution/SlippageModel.hpp"

using namespace chimera;

void SlippageModel::record(double edge_bps, double realized_bps)
{
    double error = edge_bps - realized_bps;
    errors_.push_back(error);

    if (errors_.size() > max_samples_)
        errors_.pop_front();
}

double SlippageModel::predict(double edge_bps) const
{
    if (errors_.empty())
        return 0.0;

    double avg = 0;
    for (auto e : errors_)
        avg += e;

    avg /= errors_.size();
    return avg;
}
