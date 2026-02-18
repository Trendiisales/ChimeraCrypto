#pragma once
#include <vector>
#include <numeric>
#include <algorithm>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// EXECUTION QUALITY FEEDBACK ENGINE
// Tracks intended vs actual fills, measures slippage
// ═══════════════════════════════════════════════════════════════════

struct ExecutionRecord {
    double intended_price;
    double actual_price;
    double size;
    uint64_t timestamp_us;
};

class ExecutionQualityTracker {
private:
    static constexpr size_t MAX_RECORDS = 1000;
    std::vector<ExecutionRecord> records_;
    
public:
    ExecutionQualityTracker() {
        records_.reserve(MAX_RECORDS);
    }
    
    // Record a fill
    void record(double intended,
                double actual,
                double size) noexcept {
        uint64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        records_.push_back({intended, actual, size, now_us});
        
        // Keep only recent records
        if (records_.size() > MAX_RECORDS) {
            records_.erase(records_.begin());
        }
    }
    
    // Average slippage in basis points
    [[nodiscard]] double average_slippage_bps() const noexcept {
        if (records_.empty()) return 0.0;
        
        double total = 0.0;
        for (const auto& r : records_) {
            double diff = r.actual_price - r.intended_price;
            total += (diff / r.intended_price) * 10000.0;
        }
        
        return total / records_.size();
    }
    
    // Worst slippage in basis points
    [[nodiscard]] double worst_slippage_bps() const noexcept {
        if (records_.empty()) return 0.0;
        
        double worst = 0.0;
        for (const auto& r : records_) {
            double diff = r.actual_price - r.intended_price;
            double bps = std::abs((diff / r.intended_price) * 10000.0);
            worst = std::max(worst, bps);
        }
        
        return worst;
    }
    
    // Recent slippage (last N records)
    [[nodiscard]] double recent_slippage_bps(size_t n = 50) const noexcept {
        if (records_.empty()) return 0.0;
        
        size_t start = records_.size() > n ? records_.size() - n : 0;
        double total = 0.0;
        size_t count = 0;
        
        for (size_t i = start; i < records_.size(); ++i) {
            double diff = records_[i].actual_price - records_[i].intended_price;
            total += (diff / records_[i].intended_price) * 10000.0;
            count++;
        }
        
        return count > 0 ? total / count : 0.0;
    }
    
    // Number of recorded fills
    [[nodiscard]] size_t count() const noexcept {
        return records_.size();
    }
    
    // Quality score (0-1, where 1 = perfect fills)
    [[nodiscard]] double quality_score() const noexcept {
        if (records_.empty()) return 1.0;
        
        double avg_slip = std::abs(average_slippage_bps());
        
        // Score degrades with slippage
        // 0 bps = 1.0
        // 5 bps = 0.9
        // 10 bps = 0.8
        // 20+ bps = 0.5
        return std::max(0.5, 1.0 - (avg_slip / 50.0));
    }
};

} // namespace chimera
