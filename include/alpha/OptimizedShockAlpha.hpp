#pragma once
#include "alpha/AlphaBase.hpp"

namespace chimera {

class OptimizedShockAlpha : public AlphaBase<OptimizedShockAlpha> {
public:
    inline double compute_impl(const MarketState& m) const {
        return m.volatility_bps * 0.02;
    }
};

}
