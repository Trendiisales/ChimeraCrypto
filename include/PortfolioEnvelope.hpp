#pragma once
#include <cmath>
#include <algorithm>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// INSTITUTIONAL PORTFOLIO ENVELOPE
// Hard capital survivability governor with kill switch
// ═══════════════════════════════════════════════════════════════════

class PortfolioEnvelope {
private:
    double max_daily_dd_;
    double max_concurrent_exposure_;
    double kill_threshold_;
    
    double peak_equity_{0.0};
    double current_equity_{0.0};
    double daily_dd_{0.0};
    double current_exposure_{0.0};
    
    bool killed_{false};
    uint64_t day_start_us_{0};
    
    [[nodiscard]] inline uint64_t now_us() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    
    inline void check_day_reset() noexcept {
        uint64_t now = now_us();
        
        // Reset daily DD every 24 hours
        if (day_start_us_ == 0) {
            day_start_us_ = now;
        } else if (now - day_start_us_ >= 86400000000ULL) {  // 24 hours in microseconds
            daily_dd_ = 0.0;
            day_start_us_ = now;
        }
    }
    
public:
    PortfolioEnvelope(double max_daily_dd,
                      double max_concurrent_exposure,
                      double kill_threshold)
        : max_daily_dd_(max_daily_dd),
          max_concurrent_exposure_(max_concurrent_exposure),
          kill_threshold_(kill_threshold)
    {
    }
    
    // Update current equity (call after each trade or periodically)
    void update_equity(double equity) noexcept {
        check_day_reset();
        
        if (equity > peak_equity_) {
            peak_equity_ = equity;
        }
        
        current_equity_ = equity;
        daily_dd_ = peak_equity_ - current_equity_;
    }
    
    // Check if new trade is allowed
    [[nodiscard]] bool allow_trade(double new_exposure) const noexcept {
        if (killed_) {
            return false;
        }
        
        if (daily_dd_ >= max_daily_dd_) {
            return false;
        }
        
        if (current_exposure_ + new_exposure > max_concurrent_exposure_) {
            return false;
        }
        
        return true;
    }
    
    // Register new exposure (call when opening position)
    void register_exposure(double exposure) noexcept {
        current_exposure_ += exposure;
    }
    
    // Release exposure (call when closing position)
    void release_exposure(double exposure) noexcept {
        current_exposure_ -= exposure;
        if (current_exposure_ < 0) {
            current_exposure_ = 0;
        }
    }
    
    // Evaluate kill switch
    void evaluate_kill() noexcept {
        if (daily_dd_ >= kill_threshold_) {
            killed_ = true;
        }
    }
    
    // Getters
    [[nodiscard]] bool killed() const noexcept { return killed_; }
    [[nodiscard]] double daily_dd() const noexcept { return daily_dd_; }
    [[nodiscard]] double current_exposure() const noexcept { return current_exposure_; }
    [[nodiscard]] double peak_equity() const noexcept { return peak_equity_; }
    
    // Manual reset (use with caution)
    void reset_kill() noexcept { killed_ = false; }
    
    // Force new day (for testing)
    void force_day_reset() noexcept {
        daily_dd_ = 0.0;
        day_start_us_ = now_us();
    }
};

} // namespace chimera
