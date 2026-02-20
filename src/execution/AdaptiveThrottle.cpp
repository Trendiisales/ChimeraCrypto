#include "execution/AdaptiveThrottle.hpp"

using namespace chimera;

void AdaptiveThrottle::record_latency(double ms)
{
    avg_latency_ = 0.9 * avg_latency_ + 0.1 * ms;
}

bool AdaptiveThrottle::allow() const
{
    return avg_latency_ < 15.0;
}
