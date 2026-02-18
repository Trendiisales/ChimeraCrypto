#pragma once
#include <unordered_map>
#include <deque>
#include <string>
#include <cstddef>

namespace chimera {

struct CorrelationSignal {
    bool active;
    std::string leader;
    std::string follower;
    double correlation;
};

class LiquidityCorrelationEngine {
public:
    LiquidityCorrelationEngine(size_t window, double threshold);
    void update(const std::string& symbol, double imbalance);
    CorrelationSignal detect() const;

private:
    size_t window_;
    double threshold_;
    std::unordered_map<std::string, std::deque<double>> history_;
    double correlation(const std::deque<double>& a, const std::deque<double>& b) const;
};

}
