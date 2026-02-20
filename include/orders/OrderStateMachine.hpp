#pragma once
#include <unordered_map>
#include <string>

namespace chimera {

enum class OrderStatus {
    NEW,
    ACKED,
    PARTIAL,
    FILLED,
    CANCELED,
    REJECTED
};

struct OrderInfo {
    std::string symbol;
    double original_qty = 0.0;
    double filled_qty = 0.0;
    OrderStatus status = OrderStatus::NEW;
};

class OrderStateMachine {
public:
    void on_new(const std::string& clordid,
                const std::string& symbol,
                double qty);

    void on_ack(const std::string& clordid);

    void on_fill(const std::string& clordid,
                 double fill_qty);

    void on_cancel(const std::string& clordid);

    void on_reject(const std::string& clordid);

    const OrderInfo* get(
        const std::string& clordid) const;

private:
    std::unordered_map<std::string,
                       OrderInfo> orders_;
};

}
