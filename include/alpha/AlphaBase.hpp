#pragma once

namespace chimera {

struct MarketState {
    double volatility_bps;
    double spread_bps;
    double slippage_bps;
    double mid_price;
    double depth_imbalance;
};

template<typename Derived>
class AlphaBase {
public:
    inline double compute(const MarketState& m) {
        return static_cast<Derived*>(this)->compute_impl(m);
    }
};

}
