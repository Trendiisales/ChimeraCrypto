#include "live/SpotExecutor.hpp"
#include <iostream>

namespace chimera {

SpotExecutor::SpotExecutor(bool shadow)
    : shadow_(shadow)
{
}

void SpotExecutor::execute(
    const std::string& symbol,
    bool is_buy,
    double qty,
    double price)
{
    if (shadow_)
    {
        std::cout
            << "[SHADOW] "
            << symbol
            << (is_buy ? " BUY " : " SELL ")
            << qty
            << " @ "
            << price
            << "\n";
    }
    else
    {
        std::cout
            << "[LIVE] "
            << symbol
            << " EXEC\n";
    }
}

}
