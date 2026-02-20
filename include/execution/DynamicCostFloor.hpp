#pragma once
#include <algorithm>

namespace chimera {

class DynamicCostFloor {
public:
    static constexpr double BASE_FLOOR = 6.5;
    
    double compute(double spread_bps, 
                   double slippage_bps,
                   double latency_ms,
                   double volatility_bps) const {
        
        double latency_penalty = latency_ms * volatility_bps * 0.01;
        double total = spread_bps + slippage_bps + BASE_FLOOR + latency_penalty;
        
        return std::clamp(total * 1.5, BASE_FLOOR, 50.0);
    }
};

}
