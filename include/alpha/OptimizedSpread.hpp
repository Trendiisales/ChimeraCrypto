#pragma once
#include "alpha/AlphaBase.hpp"

namespace chimera {

class OptimizedSpread : public AlphaBase<OptimizedSpread> {
public:
    inline double compute_impl(const MarketState& m) const {
        return (10.0 - m.spread_bps) * 5.0;
    }
};

}
