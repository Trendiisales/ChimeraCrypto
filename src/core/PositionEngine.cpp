#include "core/PositionEngine.hpp"
#include <cmath>

namespace chimera {

PositionEngine::PositionEngine(double cost_bps)
    : cost_bps_(cost_bps)
{
}

bool PositionEngine::in_position() const
{
    return active_;
}

void PositionEngine::enter(bool is_buy,
                           double price,
                           double size,
                           double expected_edge_bps)
{
    active_ = true;
    is_buy_ = is_buy;
    entry_price_ = price;
    size_ = size;

    target_bps_ =
        cost_bps_ + expected_edge_bps * 0.6;
}

bool PositionEngine::should_exit(
    double current_price,
    bool refill_detected,
    double expected_edge_bps)
{
    if (!active_)
        return false;

    double move_bps =
        is_buy_
        ? ((current_price - entry_price_) / entry_price_) * 10000.0
        : ((entry_price_ - current_price) / entry_price_) * 10000.0;

    if (move_bps >= target_bps_)
        return true;

    if (move_bps <= -stop_bps_)
        return true;

    if (refill_detected)
        return true;

    if (expected_edge_bps < cost_bps_)
        return true;

    return false;
}

void PositionEngine::exit()
{
    active_ = false;
}

}
