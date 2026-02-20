#include "execution/ExecutionHeatmap.hpp"

namespace chimera {

void ExecutionHeatmap::record(
    const std::string& symbol,
    double latency_ms,
    double slippage_bps)
{
    auto& b = buckets_[symbol];

    b.latency.push_back(latency_ms);
    b.slippage.push_back(slippage_bps);

    if (b.latency.size() > max_samples_)
        b.latency.pop_front();

    if (b.slippage.size() > max_samples_)
        b.slippage.pop_front();
}

double ExecutionHeatmap::avg(
    const std::deque<double>& d) const
{
    if (d.empty())
        return 0.0;

    double sum = 0.0;
    for (double v : d)
        sum += v;

    return sum / d.size();
}

HeatmapStats ExecutionHeatmap::stats(
    const std::string& symbol) const
{
    HeatmapStats s;

    auto it = buckets_.find(symbol);
    if (it == buckets_.end())
        return s;

    s.avg_latency_ms =
        avg(it->second.latency);

    s.avg_slippage_bps =
        avg(it->second.slippage);

    s.samples =
        it->second.latency.size();

    return s;
}

}
