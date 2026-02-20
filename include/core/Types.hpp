#pragma once
#include <string>

namespace chimera {

enum class OrderSide {
    BUY,
    SELL
};

inline const char* to_string(OrderSide s) {
    return (s == OrderSide::BUY) ? "BUY" : "SELL";
}

}
