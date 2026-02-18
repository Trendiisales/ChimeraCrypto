#pragma once

namespace chimera {

struct CostInputs {
    double spreadBps = 0.0;
    double makerFeeBps = 2.0;
    double takerFeeBps = 7.5;
    
    double avgSlippageBps = 0.0;  // FIXED: Now in BPS
    double latencyMs = 0.0;
};

class CostModel {
public:
    double estimateCostBps(const CostInputs& in, bool aggressive) {
        double fee = aggressive ? in.takerFeeBps : in.makerFeeBps;
        
        // Latency penalty: 0.02 bps per ms
        double latencyPenalty = in.latencyMs * 0.02;
        
        // All in BPS
        return in.spreadBps
             + fee
             + in.avgSlippageBps
             + latencyPenalty;
    }
};

} // namespace chimera
