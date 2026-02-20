#pragma once
#include <unordered_map>
#include <vector>
#include <string>

namespace chimera {

struct OpenOrder {
    std::string symbol;
    std::string side;
    double price;
    double orig_qty;
    double executed_qty;
};

class ExchangeTruth {
public:
    ExchangeTruth();

    bool refresh_account();
    bool refresh_open_orders();

    double asset_balance(const std::string& asset) const;
    bool balance_aligned(const std::string& asset,
                         double engine_balance,
                         double tolerance = 1e-6) const;

    const std::vector<OpenOrder>& open_orders() const;

private:
    std::unordered_map<std::string, double> balances_;
    std::vector<OpenOrder> open_orders_;

    std::string api_key_;
    std::string secret_key_;

    std::string sign(const std::string& query) const;
    std::string http_get(const std::string& url,
                         const std::string& headers) const;
};

}
