#pragma once
#include <cmath>

namespace chimera {

// =============================================================
// ECONOMIC GATE
// Final execution barrier - ensures all trades meet minimum
// expected edge threshold relative to cost base
// Cost base = 6.5bp (spread + slip + commission)
// Minimum required = 1.5x cost = 10bp
// =============================================================

class EconomicGate {
public:
    EconomicGate(double cost_bp = 6.5)
        : cost_base_bp(cost_bp)
    {}

    bool allow(double expected_move_bp) const
    {
        double min_required = cost_base_bp * 1.5; // 1.5R multiple = 10bp minimum
        
        return std::abs(expected_move_bp) >= min_required;
    }
    
    double get_min_required() const {
        return cost_base_bp * 1.5;
    }

private:
    double cost_base_bp;
};

} // namespace chimera
