#pragma once

#include <deque>
#include <cmath>
#include <algorithm>

namespace chimera {

// ============================================================================
// COMPRESSION ENGINE - MICROSTRUCTURE MEAN REVERSION
// ============================================================================
//
// Operates in tight spread, low volatility regimes (80% of market time)
// Captures order book imbalance and micro mean reversion
// 
// Based on observed behavior:
// - vol_ratio typically 0.7-1.1
// - Toxicity 0.12-0.14 (mild churn, not toxic)
// - Latency 10-17ms stable
// - Spread stable
// - Thousands of weak_volatility rejections = opportunity
//
// Strategy:
// - Monitor bid/ask imbalance
// - Track micro delta (5-tick momentum)
// - Enter on mean deviation + imbalance confirmation
// - Exit fast (25ms max)
// - Target 1.1bp (just above cost)
//
// ============================================================================

struct CompressionSignal {
    enum class Direction {
        NONE,
        LONG,   // Bid imbalance + price below mean
        SHORT   // Ask imbalance + price above mean
    };
    
    Direction direction = Direction::NONE;
    double imbalance = 0.5;           // 0-1, 0.5 = balanced
    double micro_delta_bp = 0.0;      // 5-tick momentum in bp
    double mean_deviation_bp = 0.0;   // Distance from 20-tick EMA
    double confidence = 0.0;          // 0-1 signal strength
};

class CompressionEngine {
public:
    // ========================================================================
    // CONFIGURATION (Data-Aligned)
    // ========================================================================
    
    // Activation thresholds
    static constexpr double ACTIVATION_VOL_RATIO = 1.12;         // Allow up to 1.12
    static constexpr double ACTIVATION_TOXICITY = 0.18;          // Allow 0.12-0.14 range
    static constexpr double ACTIVATION_DISPLACEMENT_BP = 1.5;    // Micro movements only
    static constexpr double ACTIVATION_SPREAD_MULT = 1.10;       // Spread <= median × 1.10
    static constexpr double ACTIVATION_LATENCY_MS = 25.0;        // p95 must be clean
    
    // Deactivation threshold
    static constexpr double DEACTIVATION_VOL_RATIO = 1.25;       // Clear separation from expansion
    
    // Entry signal thresholds
    static constexpr double LONG_IMBALANCE_THRESHOLD = 0.58;     // Bid > 58%
    static constexpr double SHORT_IMBALANCE_THRESHOLD = 0.42;    // Ask > 58% (1-0.42)
    static constexpr double MICRO_DELTA_THRESHOLD_BP = 0.7;      // 5-tick momentum
    static constexpr double MEAN_DEVIATION_THRESHOLD_BP = 0.35;  // Distance from EMA
    
    // Impulse threshold (cross spread only when very strong)
    static constexpr double IMPULSE_IMBALANCE_THRESHOLD = 0.68;  // Very strong imbalance
    
    // Safety filters
    static constexpr double MAX_RECENT_MOMENTUM_BP = 1.8;        // Block if trending
    
    // Risk parameters
    static constexpr double TARGET_BP = 1.1;                     // Just above 2bp cost
    static constexpr double STOP_BP = -1.8;                      // Tight stop
    static constexpr double MAX_HOLD_MS = 25.0;                  // Speed-based exit
    
    // Execution weights
    static constexpr double MAKER_WEIGHT = 0.70;                 // Prefer posting
    static constexpr double IMPULSE_WEIGHT = 0.30;               // Limited crossing
    
    // Capital allocation
    static constexpr double BASE_WEIGHT = 0.25;                  // Conservative base
    static constexpr double MIN_WEIGHT = 0.15;                   // Floor (poor performance)
    static constexpr double MAX_WEIGHT = 0.30;                   // Ceiling (great performance)
    
    // Performance adaptation thresholds
    static constexpr double HIGH_WIN_RATE = 0.70;                // Scale up if > 70%
    static constexpr double LOW_WIN_RATE = 0.55;                 // Scale down if < 55%
    static constexpr int PERFORMANCE_WINDOW = 50;                // Rolling window size
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    struct State {
        // Price tracking
        std::deque<double> prices;              // Last 20 ticks for EMA
        double short_mean_ema = 0.0;            // 20-tick EMA
        bool ema_initialized = false;
        
        // Momentum tracking
        std::deque<double> returns;             // Last 3 ticks for momentum filter
        
        // Performance tracking
        std::deque<bool> recent_wins;           // Last 50 trades
        int total_trades = 0;
        int wins = 0;
        double total_pnl_bp = 0.0;
        
        // Position tracking
        bool in_position = false;
        int64_t entry_ts = 0;
        double entry_price = 0.0;
        CompressionSignal::Direction entry_direction = CompressionSignal::Direction::NONE;
        
        void reset() {
            prices.clear();
            returns.clear();
            recent_wins.clear();
            short_mean_ema = 0.0;
            ema_initialized = false;
            total_trades = 0;
            wins = 0;
            total_pnl_bp = 0.0;
            in_position = false;
            entry_ts = 0;
            entry_price = 0.0;
            entry_direction = CompressionSignal::Direction::NONE;
        }
    };
    
    State state_;
    
    // ========================================================================
    // ACTIVATION / DEACTIVATION
    // ========================================================================
    
    bool should_activate(double vol_ratio_smooth, 
                        double displacement_bp,
                        double toxicity,
                        double spread_bp,
                        double median_spread_bp,
                        double lat_p95_ms) const {
        
        return vol_ratio_smooth <= ACTIVATION_VOL_RATIO &&
               displacement_bp <= ACTIVATION_DISPLACEMENT_BP &&
               toxicity <= ACTIVATION_TOXICITY &&
               spread_bp <= median_spread_bp * ACTIVATION_SPREAD_MULT &&
               lat_p95_ms <= ACTIVATION_LATENCY_MS;
    }
    
    bool should_deactivate(double vol_ratio_smooth) const {
        return vol_ratio_smooth >= DEACTIVATION_VOL_RATIO;
    }
    
    // ========================================================================
    // PRICE TRACKING & EMA
    // ========================================================================
    
    void update_price(double price) {
        // Update 20-tick EMA
        if (!state_.ema_initialized) {
            state_.short_mean_ema = price;
            state_.ema_initialized = true;
        } else {
            // EMA with alpha = 0.1 (roughly 20-tick memory)
            constexpr double alpha = 0.1;
            state_.short_mean_ema = alpha * price + (1.0 - alpha) * state_.short_mean_ema;
        }
        
        // Track recent prices for momentum calculation
        state_.prices.push_back(price);
        if (state_.prices.size() > 20) {
            state_.prices.pop_front();
        }
        
        // Calculate log return for momentum filter
        if (state_.prices.size() >= 2) {
            double prev_price = state_.prices[state_.prices.size() - 2];
            double log_return = std::log(price / prev_price);
            
            state_.returns.push_back(log_return);
            if (state_.returns.size() > 3) {
                state_.returns.pop_front();
            }
        }
    }
    
    // ========================================================================
    // SIGNAL GENERATION
    // ========================================================================
    
    CompressionSignal generate_signal(double price,
                                     double bid_size,
                                     double ask_size) {
        
        CompressionSignal sig;
        
        // Calculate order book imbalance
        double total_size = bid_size + ask_size;
        if (total_size < 1e-9) {
            return sig;  // No signal if book is empty
        }
        
        sig.imbalance = bid_size / total_size;
        
        // Calculate micro delta (5-tick momentum)
        sig.micro_delta_bp = calculate_micro_delta_bp();
        
        // Calculate mean deviation
        sig.mean_deviation_bp = (price - state_.short_mean_ema) / state_.short_mean_ema * 10000.0;
        
        // Check for excessive recent momentum (safety filter)
        double recent_momentum_bp = calculate_recent_momentum_bp();
        if (std::abs(recent_momentum_bp) > MAX_RECENT_MOMENTUM_BP) {
            return sig;  // Don't mean-revert into momentum burst
        }
        
        // LONG signal: Bid imbalance + price below mean + positive micro delta
        if (sig.imbalance >= LONG_IMBALANCE_THRESHOLD &&
            sig.micro_delta_bp >= MICRO_DELTA_THRESHOLD_BP &&
            sig.mean_deviation_bp <= -MEAN_DEVIATION_THRESHOLD_BP) {
            
            sig.direction = CompressionSignal::Direction::LONG;
            sig.confidence = calculate_confidence(sig.imbalance, 
                                                 sig.micro_delta_bp,
                                                 sig.mean_deviation_bp,
                                                 true);
        }
        // SHORT signal: Ask imbalance + price above mean + negative micro delta
        else if (sig.imbalance <= SHORT_IMBALANCE_THRESHOLD &&
                sig.micro_delta_bp <= -MICRO_DELTA_THRESHOLD_BP &&
                sig.mean_deviation_bp >= MEAN_DEVIATION_THRESHOLD_BP) {
            
            sig.direction = CompressionSignal::Direction::SHORT;
            sig.confidence = calculate_confidence(1.0 - sig.imbalance,  // Ask imbalance
                                                 -sig.micro_delta_bp,
                                                 -sig.mean_deviation_bp,
                                                 false);
        }
        
        return sig;
    }
    
    // ========================================================================
    // EXECUTION DECISION
    // ========================================================================
    
    bool should_use_impulse(const CompressionSignal& sig) const {
        // Only cross spread if imbalance is very strong
        return (sig.direction == CompressionSignal::Direction::LONG &&
                sig.imbalance >= IMPULSE_IMBALANCE_THRESHOLD) ||
               (sig.direction == CompressionSignal::Direction::SHORT &&
                sig.imbalance <= (1.0 - IMPULSE_IMBALANCE_THRESHOLD));
    }
    
    double get_execution_weight(const CompressionSignal& sig) const {
        return should_use_impulse(sig) ? IMPULSE_WEIGHT : MAKER_WEIGHT;
    }
    
    // ========================================================================
    // POSITION MANAGEMENT
    // ========================================================================
    
    bool should_exit(double current_price, int64_t current_ts) const {
        if (!state_.in_position) return false;
        
        // Calculate current PnL
        double pnl_bp = calculate_pnl_bp(current_price);
        
        // Time stop (mandatory)
        int64_t hold_ms = (current_ts - state_.entry_ts) / 1000;
        if (hold_ms >= MAX_HOLD_MS) {
            return true;
        }
        
        // Target hit
        if (pnl_bp >= TARGET_BP) {
            return true;
        }
        
        // Stop hit
        if (pnl_bp <= STOP_BP) {
            return true;
        }
        
        return false;
    }
    
    double calculate_pnl_bp(double current_price) const {
        if (!state_.in_position) return 0.0;
        
        double pnl_bp;
        if (state_.entry_direction == CompressionSignal::Direction::LONG) {
            pnl_bp = (current_price - state_.entry_price) / state_.entry_price * 10000.0;
        } else {
            pnl_bp = (state_.entry_price - current_price) / state_.entry_price * 10000.0;
        }
        
        return pnl_bp;
    }
    
    // ========================================================================
    // ENTRY / EXIT
    // ========================================================================
    
    void enter(double price, int64_t ts, CompressionSignal::Direction direction) {
        state_.in_position = true;
        state_.entry_price = price;
        state_.entry_ts = ts;
        state_.entry_direction = direction;
    }
    
    void exit(double price, int64_t ts) {
        double pnl_bp = calculate_pnl_bp(price);
        
        // Record trade
        state_.total_trades++;
        state_.total_pnl_bp += pnl_bp;
        
        bool win = pnl_bp > 0;
        if (win) state_.wins++;
        
        state_.recent_wins.push_back(win);
        if (state_.recent_wins.size() > PERFORMANCE_WINDOW) {
            state_.recent_wins.pop_front();
        }
        
        // Reset position
        state_.in_position = false;
        state_.entry_price = 0.0;
        state_.entry_ts = 0;
        state_.entry_direction = CompressionSignal::Direction::NONE;
    }
    
    // ========================================================================
    // PERFORMANCE ADAPTATION
    // ========================================================================
    
    double get_adaptive_weight() const {
        if (state_.recent_wins.size() < 20) {
            return BASE_WEIGHT;  // Not enough data
        }
        
        // Calculate recent win rate
        int recent_wins = 0;
        for (bool w : state_.recent_wins) {
            if (w) recent_wins++;
        }
        double win_rate = (double)recent_wins / state_.recent_wins.size();
        
        // Adapt weight based on performance
        double weight = BASE_WEIGHT;
        
        if (win_rate > HIGH_WIN_RATE) {
            weight = MAX_WEIGHT;  // Scale up
        } else if (win_rate < LOW_WIN_RATE) {
            weight = MIN_WEIGHT;  // Scale down
        }
        
        return weight;
    }
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    double get_win_rate() const {
        return state_.total_trades > 0 ? (double)state_.wins / state_.total_trades : 0.0;
    }
    
    double get_avg_pnl() const {
        return state_.total_trades > 0 ? state_.total_pnl_bp / state_.total_trades : 0.0;
    }
    
    int get_total_trades() const {
        return state_.total_trades;
    }
    
    double get_total_pnl() const {
        return state_.total_pnl_bp;
    }
    
    bool is_in_position() const {
        return state_.in_position;
    }
    
private:
    // ========================================================================
    // HELPERS
    // ========================================================================
    
    double calculate_micro_delta_bp() const {
        // Calculate 5-tick momentum
        // Use last 3 returns (approximates 5-tick if ticks are close)
        if (state_.returns.size() < 3) return 0.0;
        
        double sum = 0.0;
        for (double r : state_.returns) {
            sum += r;
        }
        
        // Convert to bp
        return sum * 10000.0;
    }
    
    double calculate_recent_momentum_bp() const {
        // Same as micro_delta for now
        // Could be extended to different window
        return std::abs(calculate_micro_delta_bp());
    }
    
    double calculate_confidence(double imbalance_strength,
                               double delta_strength,
                               double deviation_strength,
                               bool is_long) const {
        // Combine signal components into confidence score
        // Normalize each to 0-1 range
        
        double imb_conf = (imbalance_strength - 0.5) * 2.0;  // 0.5-1.0 → 0-1.0
        imb_conf = std::clamp(imb_conf, 0.0, 1.0);
        
        double delta_conf = delta_strength / 2.0;  // 0-2bp → 0-1.0
        delta_conf = std::clamp(delta_conf, 0.0, 1.0);
        
        double dev_conf = deviation_strength / 1.0;  // 0-1bp → 0-1.0
        dev_conf = std::clamp(dev_conf, 0.0, 1.0);
        
        // Weighted average (imbalance most important)
        return 0.5 * imb_conf + 0.3 * delta_conf + 0.2 * dev_conf;
    }
};

} // namespace chimera
