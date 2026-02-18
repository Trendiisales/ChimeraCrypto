#pragma once
#include <array>
#include <cstdint>
#include <chrono>
#include <functional>
#include "types.hpp"
#include "config.hpp"

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// TRADE AGGREGATOR - 100ms VWAP Buckets
// Prevents raw tick spam, produces meaningful trade signals
// ═══════════════════════════════════════════════════════════════════

template<size_t MaxSymbols = Config::MAX_SYMBOLS>
class TradeAggregator {
private:
    struct Bucket {
        double volume_weighted_price{0.0};
        double total_volume{0.0};
        uint32_t tick_count{0};
        uint64_t window_start_us{0};
        bool has_data{false};
    };
    
    static constexpr uint64_t BUCKET_WINDOW_US = 100000;  // 100ms
    static constexpr double MIN_VOLUME_ETH = 0.01;        // Dust filter
    static constexpr double MIN_VOLUME_BTC = 0.001;
    
    std::array<Bucket, MaxSymbols> buckets_{};
    
    std::function<void(size_t, double, double, uint32_t)> on_aggregated_trade_;
    
    [[nodiscard]] inline uint64_t now_us() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    
    [[nodiscard]] inline double min_volume(size_t sym) const noexcept {
        if (sym == 0) return MIN_VOLUME_BTC;  // BTC
        if (sym == 1) return MIN_VOLUME_ETH;  // ETH
        return MIN_VOLUME_ETH;                // Default
    }
    
    inline void flush_bucket(size_t sym) noexcept {
        if (!buckets_[sym].has_data) return;
        
        double vwap = buckets_[sym].volume_weighted_price / buckets_[sym].total_volume;
        
        if (on_aggregated_trade_) {
            on_aggregated_trade_(sym, vwap, buckets_[sym].total_volume, buckets_[sym].tick_count);
        }
        
        // Reset bucket
        buckets_[sym] = Bucket{};
    }
    
public:
    constexpr TradeAggregator() noexcept = default;
    
    // Add raw trade tick
    inline void add_trade(size_t sym, double price, double quantity) noexcept {
        if (sym >= MaxSymbols) return;
        
        // Apply dust filter
        if (quantity < min_volume(sym)) {
            return;  // Ignore dust trades
        }
        
        uint64_t now = now_us();
        
        // Initialize or check window
        if (!buckets_[sym].has_data) {
            buckets_[sym].window_start_us = now;
            buckets_[sym].has_data = true;
        } else if (now - buckets_[sym].window_start_us >= BUCKET_WINDOW_US) {
            // Window expired, flush and start new
            flush_bucket(sym);
            buckets_[sym].window_start_us = now;
            buckets_[sym].has_data = true;
        }
        
        // Accumulate into bucket
        buckets_[sym].volume_weighted_price += price * quantity;
        buckets_[sym].total_volume += quantity;
        buckets_[sym].tick_count++;
    }
    
    // Force flush all buckets (call periodically or on shutdown)
    inline void flush_all() noexcept {
        for (size_t i = 0; i < MaxSymbols; ++i) {
            flush_bucket(i);
        }
    }
    
    // Set callback for aggregated trades
    void on_aggregated_trade(std::function<void(size_t, double, double, uint32_t)> callback) {
        on_aggregated_trade_ = std::move(callback);
    }
    
    // Periodic tick to flush expired buckets
    inline void tick() noexcept {
        uint64_t now = now_us();
        
        for (size_t sym = 0; sym < MaxSymbols; ++sym) {
            if (buckets_[sym].has_data && 
                now - buckets_[sym].window_start_us >= BUCKET_WINDOW_US) {
                flush_bucket(sym);
            }
        }
    }
};

} // namespace chimera
