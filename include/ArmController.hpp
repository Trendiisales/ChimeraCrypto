#pragma once
#include <string>
#include <cstdint>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// ARM CONTROLLER + KILL SWITCH STATE MACHINE
// Deterministic warmup → armed transitions
// Clear kill reason visibility
// ═══════════════════════════════════════════════════════════════════

enum class KillReason {
    NONE,
    DRAWDOWN_LIMIT,
    LATENCY_SPIKE,
    INVALID_SIGNAL,
    MANUAL_LOCK,
    LOSS_CLUSTER
};

enum class ArmState {
    DISARMED,
    WARMING,
    ARMED,
    KILLED
};

class ArmController {
private:
    KillReason reason_{KillReason::NONE};
    ArmState state_{ArmState::WARMING};
    uint32_t warmup_ticks_{0};
    static constexpr uint32_t WARMUP_REQUIRED = 500;  // 500 ticks to arm
    
public:
    // Call every tick
    void on_tick() noexcept {
        if (state_ == ArmState::KILLED) {
            return;  // Once killed, stay killed
        }
        
        if (state_ == ArmState::WARMING) {
            warmup_ticks_++;
            if (warmup_ticks_ >= WARMUP_REQUIRED) {
                state_ = ArmState::ARMED;
                reason_ = KillReason::NONE;
            }
        }
    }
    
    // Update with readiness checks - CRITICAL
    void update(bool book_ready, bool vol_ready, bool dd_breach) noexcept {
        if (dd_breach && state_ != ArmState::KILLED) {
            reason_ = KillReason::DRAWDOWN_LIMIT;
            state_ = ArmState::KILLED;
            return;
        }
        
        if (state_ == ArmState::KILLED) {
            return;  // Stay killed
        }
        
        // If in warming, check if we can arm
        if (state_ == ArmState::WARMING) {
            if (warmup_ticks_ >= WARMUP_REQUIRED && book_ready && vol_ready) {
                state_ = ArmState::ARMED;
                reason_ = KillReason::NONE;
            }
        }
    }
    
    // Update with drawdown check
    void update_drawdown(double dd, double max_dd) noexcept {
        if (dd > max_dd && state_ != ArmState::KILLED) {
            reason_ = KillReason::DRAWDOWN_LIMIT;
            state_ = ArmState::KILLED;
        }
    }
    
    // Trigger on loss cluster
    void trigger_loss_cluster() noexcept {
        if (state_ == ArmState::ARMED) {
            reason_ = KillReason::LOSS_CLUSTER;
            state_ = ArmState::DISARMED;  // Soft lock, can re-arm
        }
    }
    
    // Manual kill
    void manual_lock() noexcept {
        reason_ = KillReason::MANUAL_LOCK;
        state_ = ArmState::KILLED;
    }
    
    // Reset to warmup
    void reset() noexcept {
        reason_ = KillReason::NONE;
        state_ = ArmState::WARMING;
        warmup_ticks_ = 0;
    }
    
    // Getters
    [[nodiscard]] bool is_armed() const noexcept {
        return state_ == ArmState::ARMED;
    }
    
    [[nodiscard]] bool is_killed() const noexcept {
        return state_ == ArmState::KILLED;
    }
    
    [[nodiscard]] ArmState state() const noexcept {
        return state_;
    }
    
    [[nodiscard]] uint32_t warmup_remaining() const noexcept {
        if (state_ == ArmState::WARMING && warmup_ticks_ < WARMUP_REQUIRED) {
            return WARMUP_REQUIRED - warmup_ticks_;
        }
        return 0;
    }
    
    [[nodiscard]] std::string reason_string() const noexcept {
        switch (reason_) {
            case KillReason::NONE: return "NONE";
            case KillReason::DRAWDOWN_LIMIT: return "DRAWDOWN_LIMIT";
            case KillReason::LATENCY_SPIKE: return "LATENCY_SPIKE";
            case KillReason::INVALID_SIGNAL: return "INVALID_SIGNAL";
            case KillReason::MANUAL_LOCK: return "MANUAL_LOCK";
            case KillReason::LOSS_CLUSTER: return "LOSS_CLUSTER";
        }
        return "UNKNOWN";
    }
    
    [[nodiscard]] std::string state_string() const noexcept {
        switch (state_) {
            case ArmState::DISARMED: return "DISARMED";
            case ArmState::WARMING: return "WARMING";
            case ArmState::ARMED: return "ARMED";
            case ArmState::KILLED: return "KILLED";
        }
        return "UNKNOWN";
    }
};

} // namespace chimera
