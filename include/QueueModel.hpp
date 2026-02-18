#pragma once
#include <algorithm>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// QUEUE POSITION MODEL
// Models time priority and queue decay for limit orders
// ═══════════════════════════════════════════════════════════════════

class QueueModel {
private:
    double position_ratio_;  // 0.0 = front of queue, 1.0 = back of queue
    double decay_speed_;
    
public:
    QueueModel(double initial_position_ratio,
               double decay_speed)
        : position_ratio_(initial_position_ratio),
          decay_speed_(decay_speed)
    {
    }
    
    // Update queue position based on trade flow
    void update(double trade_flow_ratio) noexcept {
        // Trade flow pushes you back in queue
        position_ratio_ -= trade_flow_ratio * decay_speed_;
        position_ratio_ = std::max(0.0, position_ratio_);
    }
    
    // Reset to back of queue (e.g., after order modification)
    void reset_to_back() noexcept {
        position_ratio_ = 1.0;
    }
    
    // Move to front (e.g., after price improvement)
    void move_to_front() noexcept {
        position_ratio_ = 0.0;
    }
    
    // Get current position
    [[nodiscard]] double position_ratio() const noexcept {
        return position_ratio_;
    }
    
    // Get fill probability based on position
    [[nodiscard]] double fill_probability() const noexcept {
        // Front of queue = high probability
        // Back of queue = low probability
        return 1.0 - position_ratio_;
    }
};

} // namespace chimera
