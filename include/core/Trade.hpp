#pragma once
#include <string>
#include "Types.hpp"

namespace chimera {

struct Trade {
    std::string symbol;
    OrderSide side;
    double price = 0.0;
    double qty = 0.0;
};

}
