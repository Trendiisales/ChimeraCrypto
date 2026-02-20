#pragma once
#include <unordered_map>
#include <deque>
#include <string>

namespace chimera {

class PortfolioAllocator {
public:
    void record_price(const std::string& symbol,
                      double price);

    double weight(const std::string& symbol) const;

private:
    std::unordered_map<std::string,
                       std::deque<double>> returns_;

    size_t max_samples_ = 100;

    double volatility(
        const std::deque<double>& r) const;
};

}
