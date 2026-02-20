#include "risk/PortfolioAllocator.hpp"
#include <cmath>
#include <algorithm>

namespace chimera {

void PortfolioAllocator::record_price(
    const std::string& symbol,
    double price)
{
    auto& r = returns_[symbol];
    
    if (!r.empty()) {
        double prev = r.back();
        double ret = std::log(price / prev);
        r.push_back(ret);
        if (r.size() > max_samples_)
            r.pop_front();
    } else {
        r.push_back(price);
    }
}

double PortfolioAllocator::volatility(const std::deque<double>& r) const
{
    if (r.size() < 2) return 0.0;
    
    double mean = 0.0;
    for (double v : r) mean += v;
    mean /= r.size();
    
    double var = 0.0;
    for (double v : r)
        var += (v - mean) * (v - mean);
    var /= r.size();
    
    return std::sqrt(var);
}

double PortfolioAllocator::weight(const std::string& symbol) const
{
    auto it = returns_.find(symbol);
    if (it == returns_.end()) return 0.33; // Equal weight for 3 symbols
    
    double vol = volatility(it->second);
    
    // Prevent extreme weights
    if (vol < 0.0001) vol = 0.0001;
    
    double inv = 1.0 / vol;
    
    // Clamp to 0.2 - 2.0 range
    return std::max(0.2, std::min(inv, 2.0));
}

}
