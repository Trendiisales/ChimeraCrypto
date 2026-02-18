#pragma once
#include <random>
#include <algorithm>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// SLIPPAGE REALITY MODEL
// Injects realistic execution degradation into sim and live metrics
// ═══════════════════════════════════════════════════════════════════

class SlippageModel {
private:
    double base_bps_;
    double vol_multiplier_;
    double burst_multiplier_;
    
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
    
public:
    SlippageModel(double base_bps,
                  double vol_multiplier,
                  double burst_multiplier)
        : base_bps_(base_bps),
          vol_multiplier_(vol_multiplier),
          burst_multiplier_(burst_multiplier),
          rng_(std::random_device{}()),
          dist_(0.0, 1.0)
    {
    }
    
    // Apply slippage to a price
    [[nodiscard]] double apply(double price,
                              bool is_buy,
                              double volatility,
                              double burst_factor) noexcept {
        double slip_bps = base_bps_;
        
        // Volatility increases slippage
        slip_bps += volatility * vol_multiplier_;
        
        // Burst increases slippage
        slip_bps += burst_factor * burst_multiplier_;
        
        // Add random noise (±20% of slip)
        double random_noise = dist_(rng_) * 0.2 * slip_bps;
        slip_bps += random_noise;
        
        // Convert bps to price adjustment
        double slip = price * (slip_bps / 10000.0);
        
        // Buys slip up, sells slip down
        if (is_buy) {
            return price + slip;
        } else {
            return price - slip;
        }
    }
    
    // Get expected slippage in bps (for display/logging)
    [[nodiscard]] double expected_bps(double volatility, double burst_factor) const noexcept {
        return base_bps_ + (volatility * vol_multiplier_) + (burst_factor * burst_multiplier_);
    }
};

} // namespace chimera
