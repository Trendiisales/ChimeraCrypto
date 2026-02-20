#include "execution/LatencyTracker.hpp"

namespace chimera {

void LatencyTracker::stamp_send(const std::string& symbol)
{
    stamps_[symbol] = clock::now();
}

double LatencyTracker::stamp_recv(const std::string& symbol)
{
    auto it = stamps_.find(symbol);
    if (it == stamps_.end())
        return 0.0;

    return std::chrono::duration<double,std::milli>(
        clock::now() - it->second).count();
}

}
