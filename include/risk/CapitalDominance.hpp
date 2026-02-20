#pragma once
#include <algorithm>

namespace chimera {

class CapitalDominance {
public:
    double allocate(double equity,
                    double edge_score,
                    double total_edge,
                    double volatility_bps) const {
        
        if (total_edge <= 0.0) return 0.0;
        
        double edge_ratio = edge_score / total_edge;
        double vol_adjust = std::clamp(30.0 / volatility_bps, 0.5, 1.5);
        
        return equity * edge_ratio * vol_adjust;
    }
};

}
