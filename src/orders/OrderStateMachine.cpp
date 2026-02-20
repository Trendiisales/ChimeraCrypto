#include "orders/OrderStateMachine.hpp"

namespace chimera {

void OrderStateMachine::on_new(
    const std::string& clordid,
    const std::string& symbol,
    double qty)
{
    orders_[clordid] =
        OrderInfo{symbol, qty, 0.0,
                  OrderStatus::NEW};
}

void OrderStateMachine::on_ack(
    const std::string& clordid)
{
    auto it = orders_.find(clordid);
    if (it == orders_.end())
        return;

    it->second.status =
        OrderStatus::ACKED;
}

void OrderStateMachine::on_fill(
    const std::string& clordid,
    double fill_qty)
{
    auto it = orders_.find(clordid);
    if (it == orders_.end())
        return;

    it->second.filled_qty += fill_qty;

    if (it->second.filled_qty >=
        it->second.original_qty)
    {
        it->second.status =
            OrderStatus::FILLED;
    }
    else
    {
        it->second.status =
            OrderStatus::PARTIAL;
    }
}

void OrderStateMachine::on_cancel(
    const std::string& clordid)
{
    auto it = orders_.find(clordid);
    if (it == orders_.end())
        return;

    it->second.status =
        OrderStatus::CANCELED;
}

void OrderStateMachine::on_reject(
    const std::string& clordid)
{
    auto it = orders_.find(clordid);
    if (it == orders_.end())
        return;

    it->second.status =
        OrderStatus::REJECTED;
}

const OrderInfo* OrderStateMachine::get(
    const std::string& clordid) const
{
    auto it = orders_.find(clordid);
    if (it == orders_.end())
        return nullptr;

    return &it->second;
}

}
