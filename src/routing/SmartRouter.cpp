#include "routing/SmartRouter.hpp"
#include <limits>

namespace chimera {

void SmartRouter::update_venue(
    const std::string& venue,
    double latency_ms,
    double slippage_bps,
    double fee_bps)
{
    venues_[venue] =
        VenueStats{latency_ms,
                   slippage_bps,
                   fee_bps};
}

double SmartRouter::score(
    const VenueStats& v) const
{
    return v.avg_slippage_bps +
           v.fee_bps +
           (v.avg_latency_ms * 0.1);
}

std::string SmartRouter::best_venue() const
{
    double best_score =
        std::numeric_limits<double>::max();

    std::string best;

    for (const auto& [venue, stats] : venues_)
    {
        double s = score(stats);
        if (s < best_score)
        {
            best_score = s;
            best = venue;
        }
    }

    return best;
}

}
