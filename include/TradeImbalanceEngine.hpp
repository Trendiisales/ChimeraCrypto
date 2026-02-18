#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include "types.hpp"
#include "config.hpp"

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// TRADE IMBALANCE ENGINE
// Aggressor classification, burst detection, absorption detection
// ═══════════════════════════════════════════════════════════════════

template<size_t MaxSymbols = Config::MAX_SYMBOLS>
class TradeImbalanceEngine {
private:
    static constexpr size_t WINDOW_SIZE = 50;
    static constexpr double DECAY_FACTOR = 0.85;
    static constexpr uint64_t DECAY_INTERVAL_US = 100000;  // 100ms
    static constexpr double BURST_THRESHOLD = 3.0;
    static constexpr double ABSORPTION_THRESHOLD = 0.8;
    
    struct ImbalanceState {
        double buy_pressure{0.0};
        double sell_pressure{0.0};
        uint64_t last_decay_us{0};
        
        // Burst detection
        std::array<double, WINDOW_SIZE> recent_volumes{};
        size_t volume_index{0};
        size_t volume_count{0};
        
        // Absorption tracking
        double last_volume{0.0};
        double acceleration{0.0};
    };
    
    std::array<ImbalanceState, MaxSymbols> states_{};
    
    [[nodiscard]] inline uint64_t now_us() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    
    inline void apply_decay(size_t sym) noexcept {
        uint64_t now = now_us();
        
        if (now - states_[sym].last_decay_us >= DECAY_INTERVAL_US) {
            states_[sym].buy_pressure *= DECAY_FACTOR;
            states_[sym].sell_pressure *= DECAY_FACTOR;
            states_[sym].last_decay_us = now;
        }
    }
    
    [[nodiscard]] inline double classify_aggressor(size_t sym, double price, 
                                                   const std::array<Level, 5>& bids,
                                                   const std::array<Level, 5>& asks) const noexcept {
        if (bids[0].price < 0.01 || asks[0].price < 0.01) return 0.0;
        
        double mid = (bids[0].price + asks[0].price) / 2.0;
        
        // Trade closer to ask = buy aggression
        // Trade closer to bid = sell aggression
        if (price > mid) {
            double distance = (price - mid) / (asks[0].price - mid);
            return std::clamp(distance, 0.0, 1.0);  // Positive = buy
        } else {
            double distance = (mid - price) / (mid - bids[0].price);
            return -std::clamp(distance, 0.0, 1.0);  // Negative = sell
        }
    }
    
    [[nodiscard]] inline bool detect_burst(size_t sym, double volume) noexcept {
        auto& state = states_[sym];
        
        // Add to window
        state.recent_volumes[state.volume_index] = volume;
        state.volume_index = (state.volume_index + 1) % WINDOW_SIZE;
        if (state.volume_count < WINDOW_SIZE) state.volume_count++;
        
        if (state.volume_count < 10) return false;  // Need minimum samples
        
        // Calculate recent average
        double sum = 0.0;
        for (size_t i = 0; i < state.volume_count; ++i) {
            sum += state.recent_volumes[i];
        }
        double avg = sum / state.volume_count;
        
        // Burst if current volume is 3x+ average
        return volume >= avg * BURST_THRESHOLD;
    }
    
    [[nodiscard]] inline bool detect_absorption(size_t sym, double current_volume) noexcept {
        auto& state = states_[sym];
        
        if (state.last_volume < 0.001) {
            state.last_volume = current_volume;
            return false;
        }
        
        // Calculate acceleration (rate of volume change)
        state.acceleration = (current_volume - state.last_volume) / (state.last_volume + 1e-9);
        state.last_volume = current_volume;
        
        // Absorption = high volume but decelerating
        return (current_volume > 0.1 && state.acceleration < -ABSORPTION_THRESHOLD);
    }
    
public:
    constexpr TradeImbalanceEngine() noexcept = default;
    
    struct ImbalanceSignal {
        double imbalance;       // -1.0 to 1.0 (negative = sell, positive = buy)
        bool is_burst;
        bool is_absorption;
        double acceleration;
    };
    
    // Process aggregated trade (from TradeAggregator)
    [[nodiscard]] inline ImbalanceSignal process_trade(size_t sym, 
                                                       double vwap, 
                                                       double volume,
                                                       const std::array<Level, 5>& bids,
                                                       const std::array<Level, 5>& asks) noexcept {
        if (sym >= MaxSymbols) return {0.0, false, false, 0.0};
        
        apply_decay(sym);
        
        // Classify aggressor
        double aggressor_score = classify_aggressor(sym, vwap, bids, asks);
        
        // Update pressure
        if (aggressor_score > 0) {
            states_[sym].buy_pressure += volume * aggressor_score;
        } else {
            states_[sym].sell_pressure += volume * std::abs(aggressor_score);
        }
        
        // Detect burst and absorption
        bool burst = detect_burst(sym, volume);
        bool absorption = detect_absorption(sym, volume);
        
        // Calculate net imbalance
        double total_pressure = states_[sym].buy_pressure + states_[sym].sell_pressure;
        double imbalance = total_pressure > 0.0 
            ? (states_[sym].buy_pressure - states_[sym].sell_pressure) / total_pressure
            : 0.0;
        
        return {imbalance, burst, absorption, states_[sym].acceleration};
    }
    
    // Get current imbalance without processing new trade
    [[nodiscard]] inline double get_imbalance(size_t sym) const noexcept {
        if (sym >= MaxSymbols) return 0.0;
        
        double total = states_[sym].buy_pressure + states_[sym].sell_pressure;
        return total > 0.0 
            ? (states_[sym].buy_pressure - states_[sym].sell_pressure) / total
            : 0.0;
    }
};

} // namespace chimera
