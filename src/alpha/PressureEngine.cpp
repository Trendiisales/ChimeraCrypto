#include "alpha/PressureEngine.hpp"

namespace chimera {

Signal PressureEngine::evaluate(
    double bid,
    double ask,
    double bid_size,
    double ask_size,
    double last_trade_bid,
    double last_trade_ask)
{
    Signal s;

    double imbalance = (bid_size - ask_size) /
                       (bid_size + ask_size + 1e-9);

    double spread = ask - bid;
    double mid = (bid + ask) * 0.5;

    double spread_bps = (spread / mid) * 10000.0;

    double raw_edge = imbalance * 20.0;   // amplify imbalance
    double net_edge = raw_edge - spread_bps;

    if (net_edge > 6.5)
    {
        s.fire = true;
        s.is_buy = imbalance > 0;
        s.expected_edge_bps = net_edge;
    }

    return s;
}

} // namespace chimera
