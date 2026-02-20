#include "execution/ExecutionGovernor.hpp"

namespace chimera {

ExecutionGovernor::ExecutionGovernor(
    double latency_limit_ms,
    double slippage_limit_bps)
    : latency_limit_ms_(latency_limit_ms),
      slippage_limit_bps_(slippage_limit_bps)
{
}

void ExecutionGovernor::record(
    double latency_ms,
    double slippage_bps)
{
    latency_samples_.push_back(latency_ms);
    slippage_samples_.push_back(slippage_bps);

    if (latency_samples_.size() > max_samples_)
        latency_samples_.pop_front();

    if (slippage_samples_.size() > max_samples_)
        slippage_samples_.pop_front();
}

double ExecutionGovernor::avg(
    const std::deque<double>& d) const
{
    if (d.empty())
        return 0.0;

    double sum = 0.0;
    for (double v : d)
        sum += v;

    return sum / d.size();
}

bool ExecutionGovernor::allow_trading() const
{
    double lat = avg(latency_samples_);
    double slip = avg(slippage_samples_);

    if (lat > latency_limit_ms_)
        return false;

    if (slip > slippage_limit_bps_)
        return false;

    return true;
}

}
