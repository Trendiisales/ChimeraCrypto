#pragma once

namespace chimera {

class CostGate {
public:
    CostGate(double cost_bps,
             double slippage_bps);

    bool allow(double expected_edge_bps) const;

private:
    double cost_;
    double slip_;
};

}
