#include "risk/CapitalAllocator.hpp"

namespace chimera {

CapitalAllocator::CapitalAllocator(double max_leverage)
    : max_leverage_(max_leverage)
{
}

double CapitalAllocator::max_leverage() const
{
    return max_leverage_;
}

}
