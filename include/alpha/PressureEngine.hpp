#pragma once
#include <cstdint>

namespace chimera {

struct Signal
{
    bool fire = false;
    bool is_buy = true;
    double expected_edge_bps = 0.0;
};

class PressureEngine
{
public:
    Signal evaluate(
        double bid,
        double ask,
        double bid_size,
        double ask_size,
        double last_trade_bid,
        double last_trade_ask);
};

} // namespace chimera
