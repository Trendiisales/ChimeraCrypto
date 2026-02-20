#include "metrics/PerformanceTracker.hpp"
#include <cmath>

using namespace chimera;

void PerformanceTracker::record(double pnl)
{
    returns_.push_back(pnl);
    if (returns_.size() > max_samples_)
        returns_.pop_front();
}

double PerformanceTracker::avg_return() const
{
    if (returns_.empty())
        return 0.0;

    double sum = 0.0;
    for (double r : returns_)
        sum += r;

    return sum / static_cast<double>(returns_.size());
}

double PerformanceTracker::sharpe() const
{
    if (returns_.size() < 10)
        return 0.0;

    double mean = avg_return();

    double var = 0.0;
    for (double r : returns_)
        var += (r - mean) * (r - mean);

    var /= static_cast<double>(returns_.size());
    double stddev = std::sqrt(var);

    if (stddev == 0.0)
        return 0.0;

    return mean / stddev;
}
