#pragma once
#include <unordered_map>
#include <string>
#include <chrono>

namespace chimera {

class LatencyTracker {
public:
    using clock = std::chrono::steady_clock;

    void stamp_send(const std::string& symbol);
    double stamp_recv(const std::string& symbol);

private:
    std::unordered_map<std::string, clock::time_point> stamps_;
};

}
