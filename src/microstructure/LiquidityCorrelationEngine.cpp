#include "microstructure/LiquidityCorrelationEngine.hpp"
#include <cmath>

namespace chimera {

LiquidityCorrelationEngine::LiquidityCorrelationEngine(size_t window, double threshold)
    : window_(window), threshold_(threshold) {}

void LiquidityCorrelationEngine::update(const std::string& symbol, double imbalance) {
    auto& h = history_[symbol];
    h.push_back(imbalance);
    if (h.size() > window_)
        h.pop_front();
}

double LiquidityCorrelationEngine::correlation(const std::deque<double>& a, const std::deque<double>& b) const {
    if (a.size() != b.size() || a.size() < 2)
        return 0.0;

    double mean_a = 0, mean_b = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        mean_a += a[i];
        mean_b += b[i];
    }
    mean_a /= a.size();
    mean_b /= b.size();

    double num = 0, den_a = 0, den_b = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        double da = a[i] - mean_a;
        double db = b[i] - mean_b;
        num += da * db;
        den_a += da * da;
        den_b += db * db;
    }

    if (den_a == 0 || den_b == 0)
        return 0.0;

    return num / std::sqrt(den_a * den_b);
}

CorrelationSignal LiquidityCorrelationEngine::detect() const {
    CorrelationSignal sig{};
    sig.active = false;

    for (auto it1 = history_.begin(); it1 != history_.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != history_.end(); ++it2) {
            double corr = correlation(it1->second, it2->second);
            if (std::abs(corr) > threshold_) {
                sig.active = true;
                sig.leader = it1->first;
                sig.follower = it2->first;
                sig.correlation = corr;
                return sig;
            }
        }
    }

    return sig;
}

}
