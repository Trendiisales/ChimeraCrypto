#pragma once
#include <string>
#include <unordered_map>

namespace chimera {

struct VenueStats {
    double avg_latency_ms = 0.0;
    double avg_slippage_bps = 0.0;
    double fee_bps = 0.0;
};

class SmartRouter {
public:
    void update_venue(const std::string& venue,
                      double latency_ms,
                      double slippage_bps,
                      double fee_bps);

    std::string best_venue() const;

private:
    std::unordered_map<std::string,
                       VenueStats> venues_;

    double score(const VenueStats& v) const;
};

}
