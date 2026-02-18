#pragma once
#include <vector>
#include <random>
#include <algorithm>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// MONTE CARLO STRESS ENGINE
// Tests expectancy robustness against order randomisation
// ═══════════════════════════════════════════════════════════════════

class MonteCarloStress {
private:
    std::mt19937 rng_;
    
public:
    MonteCarloStress()
        : rng_(std::random_device{}())
    {
    }
    
    struct Result {
        double worst_drawdown;
        double avg_return;
        double best_return;
        double worst_return;
        double median_return;
    };
    
    // Simulate random order permutations
    [[nodiscard]] Result simulate(const std::vector<double>& trade_r_values,
                                  int iterations = 5000) noexcept {
        Result result{};
        result.worst_drawdown = 0.0;
        result.avg_return = 0.0;
        result.best_return = -1e9;
        result.worst_return = 1e9;
        
        if (trade_r_values.empty()) return result;
        
        std::vector<double> all_returns;
        all_returns.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            auto shuffled = trade_r_values;
            std::shuffle(shuffled.begin(), shuffled.end(), rng_);
            
            double equity = 0.0;
            double peak = 0.0;
            double worst_dd = 0.0;
            
            for (double r : shuffled) {
                equity += r;
                peak = std::max(peak, equity);
                double dd = peak - equity;
                worst_dd = std::max(worst_dd, dd);
            }
            
            result.worst_drawdown = std::max(result.worst_drawdown, worst_dd);
            result.avg_return += equity;
            
            all_returns.push_back(equity);
            result.best_return = std::max(result.best_return, equity);
            result.worst_return = std::min(result.worst_return, equity);
        }
        
        result.avg_return /= iterations;
        
        // Calculate median
        std::sort(all_returns.begin(), all_returns.end());
        result.median_return = all_returns[all_returns.size() / 2];
        
        return result;
    }
    
    // Quick stress test (fewer iterations for real-time use)
    [[nodiscard]] Result quick_stress(const std::vector<double>& trade_r_values) noexcept {
        return simulate(trade_r_values, 500);
    }
};

} // namespace chimera
