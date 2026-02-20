#pragma once
#include <functional>
#include <string>
#include <atomic>

namespace chimera {

class UserStream {
public:
    using BalanceCallback =
        std::function<void(const std::string&, double)>;

    using PositionCallback =
        std::function<void(const std::string&, double)>;

    void set_balance_callback(BalanceCallback cb)
    {
        balance_cb_ = cb;
    }

    void set_position_callback(PositionCallback cb)
    {
        position_cb_ = cb;
    }

    void update_balance(const std::string& asset,
                        double amount)
    {
        if (balance_cb_)
            balance_cb_(asset, amount);
    }

    void update_position(const std::string& symbol,
                         double qty)
    {
        if (position_cb_)
            position_cb_(symbol, qty);
    }

private:
    BalanceCallback balance_cb_;
    PositionCallback position_cb_;
};

}
