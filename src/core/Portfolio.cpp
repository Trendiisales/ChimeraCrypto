#include "core/Portfolio.hpp"

namespace chimera {

void Portfolio::apply_fill(const std::string& symbol,
                           double price,
                           double qty)
{
    positions_[symbol] += qty;
    cash_ -= price * qty;
}

double Portfolio::equity() const
{
    return cash_;
}

double Portfolio::position(const std::string& symbol) const
{
    auto it = positions_.find(symbol);
    if (it == positions_.end())
        return 0.0;

    return it->second;
}

}
