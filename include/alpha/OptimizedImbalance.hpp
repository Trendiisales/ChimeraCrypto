#pragma once
#include "alpha/AlphaBase.hpp"

namespace chimera {

class OptimizedImbalance : public AlphaBase<OptimizedImbalance> {
public:
    inline double compute_impl(const MarketState& m) const {
        return m.depth_imbalance * 100.0;
    }
};

}
