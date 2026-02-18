#pragma once

namespace chimera {

struct EdgeInputs {
    double signalStrength = 0.0;
    double imbalance = 0.0;
    double volatility = 0.0;
};

class EdgeEstimator {
public:
    double estimateEdgeBps(const EdgeInputs& in) {
        double edge = 0.0;
        
        // Signal strength contribution (0-10 bps)
        edge += in.signalStrength * 10.0;
        
        // Imbalance contribution (0-5 bps)
        edge += in.imbalance * 5.0;
        
        // Volatility contribution (0-2 bps)
        edge += in.volatility * 2.0;
        
        return edge;
    }
};

} // namespace chimera
