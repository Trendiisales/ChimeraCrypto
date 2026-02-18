#pragma once
#include "CostModel.hpp"
#include "EdgeEstimator.hpp"

namespace chimera {

class DynamicEdgeGate {
public:
    bool allowTrade(const CostInputs& costIn,
                   const EdgeInputs& edgeIn,
                   bool aggressive) {
        double expectedEdge = edge_.estimateEdgeBps(edgeIn);
        double costFloor = cost_.estimateCostBps(costIn, aggressive);
        
        lastExpectedEdge_ = expectedEdge;
        lastCostFloor_ = costFloor;
        
        // Only trade if expected edge exceeds cost
        return expectedEdge > costFloor;
    }
    
    double lastEdge() const { return lastExpectedEdge_; }
    double lastCost() const { return lastCostFloor_; }
    
private:
    CostModel cost_;
    EdgeEstimator edge_;
    double lastExpectedEdge_ = 0.0;
    double lastCostFloor_ = 0.0;
};

} // namespace chimera
