#include "live/CostGate.hpp"

namespace chimera {

CostGate::CostGate(double cost_bps,
                   double slippage_bps)
    : cost_(cost_bps),
      slip_(slippage_bps) {}

bool CostGate::allow(double edge) const
{
    return edge > (cost_ + slip_);
}

}
