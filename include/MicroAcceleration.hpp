#pragma once
#include <deque>
#include <cmath>
#include <chrono>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// MICRO ACCELERATION ENGINE
// Time-normalized + volatility-normalized acceleration
// Prevents 500+ spikes, clamps to [-10, +10]
// ═══════════════════════════════════════════════════════════════════

class MicroAcceleration {
private:
    bool initialized_{false};
    double last_price_{0.0};
    std::chrono::high_resolution_clock::time_point last_time_;
    std::deque<double> velocity_history_;
    static constexpr size_t WINDOW = 50;
    
public:
    void on_tick(double price) noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        
        if (!initialized_) {
            last_price_ = price;
            last_time_ = now;
            initialized_ = true;
            return;
        }
        
        // Time delta in seconds
        double dt = std::chrono::duration<double>(now - last_time_).count();
        if (dt <= 0.000001) dt = 0.000001;  // Prevent division by zero
        
        // Velocity = price change per second
        double velocity = (price - last_price_) / dt;
        
        velocity_history_.push_back(velocity);
        if (velocity_history_.size() > WINDOW) {
            velocity_history_.pop_front();
        }
        
        last_price_ = price;
        last_time_ = now;
    }
    
    // Returns normalized acceleration (z-score, clamped to [-10, +10])
    [[nodiscard]] double get_normalized_accel() const noexcept {
        if (velocity_history_.size() < 10) return 0.0;  // Need minimum samples
        
        // Calculate mean velocity
        double mean = 0.0;
        for (double v : velocity_history_) {
            mean += v;
        }
        mean /= velocity_history_.size();
        
        // Calculate standard deviation
        double var = 0.0;
        for (double v : velocity_history_) {
            var += (v - mean) * (v - mean);
        }
        var /= velocity_history_.size();
        
        double stddev = std::sqrt(var);
        if (stddev == 0.0) return 0.0;
        
        // Current velocity z-score
        double current = velocity_history_.back();
        double z = (current - mean) / stddev;
        
        // HARD CLAMP to prevent spikes
        z = std::max(-5.0, std::min(5.0, z));
        
        return z;
    }
    
    [[nodiscard]] bool has_sufficient_samples() const noexcept {
        return velocity_history_.size() >= 50;  // Need 50 samples for stability
    }
};

} // namespace chimera
