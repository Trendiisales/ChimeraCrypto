// InstitutionalEngine.hpp - Production-Grade HFT Engine with All Institutional Layers
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>  // SIMD
#endif

#include "config.hpp"
#include "types.hpp"
#include "TradeAggregator.hpp"
#include "TradeImbalanceEngine.hpp"
#include "SlippageModel.hpp"
#include "ExecutionQualityTracker.hpp"
#include "MonteCarloStress.hpp"
#include "PortfolioEnvelope.hpp"
#include "ExchangeMicrostructure.hpp"
#include "QueueModel.hpp"
#include "TelemetryState.hpp"
#include "MicroAcceleration.hpp"
#include "ArmController.hpp"
#include "SignalLatch.hpp"
#include "ExecutionSimulator.hpp"
#include "DepthManager.hpp"

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// ORDER BOOK EXTENSIONS - SIMD processing
// ═══════════════════════════════════════════════════════════════════

inline double compute_imbalance(const OrderBook& book) noexcept {
    double bid_vol = 0.0, ask_vol = 0.0;
    
#ifdef __AVX2__
    // Process 4 levels at a time with AVX2
    __m256d bid_sum = _mm256_setzero_pd();
    __m256d ask_sum = _mm256_setzero_pd();
    
    for (size_t i = 0; i < Config::BOOK_DEPTH; i += 4) {
        __m256d bid_sizes = _mm256_set_pd(
            book.bids[i+3].size, book.bids[i+2].size,
            book.bids[i+1].size, book.bids[i].size
        );
        __m256d ask_sizes = _mm256_set_pd(
            book.asks[i+3].size, book.asks[i+2].size,
            book.asks[i+1].size, book.asks[i].size
        );
        
        bid_sum = _mm256_add_pd(bid_sum, bid_sizes);
        ask_sum = _mm256_add_pd(ask_sum, ask_sizes);
    }
    
    double bid_arr[4], ask_arr[4];
    _mm256_storeu_pd(bid_arr, bid_sum);
    _mm256_storeu_pd(ask_arr, ask_sum);
    
    bid_vol = bid_arr[0] + bid_arr[1] + bid_arr[2] + bid_arr[3];
    ask_vol = ask_arr[0] + ask_arr[1] + ask_arr[2] + ask_arr[3];
#else
    // Scalar fallback
    for (const auto& b : book.bids) bid_vol += b.size;
    for (const auto& a : book.asks) ask_vol += a.size;
#endif
    
    double total = bid_vol + ask_vol;
    return total > 0.0 ? bid_vol / total : 0.5;
}

// ═══════════════════════════════════════════════════════════════════
// LOSS CLUSTER GOVERNOR
// ═══════════════════════════════════════════════════════════════════

template<size_t WindowSize = Config::LOSS_WINDOW>
class LossClusterGovernor {
private:
    std::array<bool, WindowSize> recent_{};
    size_t index_{0};
    size_t count_{0};
    int consecutive_losses_{0};
    
public:
    constexpr LossClusterGovernor() noexcept = default;
    
    inline void record(bool win) noexcept {
        recent_[index_] = win;
        index_ = (index_ + 1) % WindowSize;
        if (count_ < WindowSize) ++count_;
        
        if (win) {
            consecutive_losses_ = 0;
        } else {
            ++consecutive_losses_;
        }
    }
    
    [[nodiscard]] constexpr bool allows_trade() const noexcept {
        if (consecutive_losses_ >= Config::CONSEC_LOSS_LIMIT) {
            return false;
        }
        
        if (count_ >= WindowSize) {
            int losses = 0;
            for (size_t i = 0; i < count_; ++i) {
                if (!recent_[i]) ++losses;
            }
            return losses < Config::LOSS_COUNT_LIMIT;
        }
        
        return true;
    }
    
    [[nodiscard]] constexpr double size_multiplier() const noexcept {
        return allows_trade() ? 1.0 : Config::LOSS_CLUSTER_SIZE_CUT;
    }
};

// ═══════════════════════════════════════════════════════════════════
// STRATEGY INTERFACE - CRTP
// ═══════════════════════════════════════════════════════════════════

template<typename Derived>
class Strategy {
public:
    [[nodiscard]] inline double compute_confidence(const OrderBook& book) noexcept {
        return static_cast<Derived*>(this)->compute_confidence_impl(book);
    }
    
    [[nodiscard]] inline bool should_enter(double confidence, double threshold) const noexcept {
        return static_cast<const Derived*>(this)->should_enter_impl(confidence, threshold);
    }
    
protected:
    ~Strategy() = default;
};

// ═══════════════════════════════════════════════════════════════════
// INSTITUTIONAL MICRO STRATEGY
// Now includes imbalance, burst, and absorption signals
// ═══════════════════════════════════════════════════════════════════

class InstitutionalMicroStrategy : public Strategy<InstitutionalMicroStrategy> {
private:
    double threshold_{Config::MICRO_BASE_THRESHOLD};
    
public:
    [[nodiscard]] inline double compute_confidence_impl(const OrderBook& book) noexcept {
        double imb = compute_imbalance(book);
        double spread_norm = std::min(book.spread / book.mid * 10000.0 / 5.0, 1.0);
        
        double imb_score = 1.0 - std::abs(imb - 0.5) * 2.0;
        double spread_score = 1.0 - spread_norm;
        
        return 0.6 * imb_score + 0.4 * spread_score;
    }
    
    [[nodiscard]] constexpr bool should_enter_impl(double confidence, double threshold) const noexcept {
        // Use hysteresis to prevent oscillation
        return confidence > threshold + 0.01;
    }
    
    constexpr void set_threshold(double t) noexcept { threshold_ = t; }
    [[nodiscard]] constexpr double threshold() const noexcept { return threshold_; }
};

// ═══════════════════════════════════════════════════════════════════
// INSTITUTIONAL ENGINE - Full Production System
// ═══════════════════════════════════════════════════════════════════

template<typename StrategyType = InstitutionalMicroStrategy>
class InstitutionalEngine {
private:
    std::array<OrderBook, Config::MAX_SYMBOLS> books_;
    
    // Core strategy
    StrategyType strategy_;
    
    // Institutional layers
    LossClusterGovernor<> loss_gov_;
    TradeAggregator<> trade_agg_;
    TradeImbalanceEngine<> imbalance_engine_;
    SlippageModel slippage_model_;
    ExecutionQualityTracker exec_quality_;
    MonteCarloStress monte_carlo_;
    PortfolioEnvelope portfolio_;
    ExchangeMicrostructure micro_model_;
    
    // Acceleration + Arm Control
    std::array<MicroAcceleration, Config::MAX_SYMBOLS> accel_engines_;
    ArmController arm_controller_;
    
    // Signal gating
    SignalLatch signal_latch_;
    
    // External components (set via pointers)
    TelemetryState* telemetry_{nullptr};
    void* exec_sim_{nullptr};  // ExecutionSimulator* (void to avoid circular dependency)
    void* btc_depth_{nullptr}; // DepthManager*
    void* eth_depth_{nullptr}; // DepthManager*
    
    std::atomic<double> equity_{10000.0};
    std::atomic<bool> disable_trading_{false};
    
    // Volatility estimate (simple EMA)
    std::array<double, Config::MAX_SYMBOLS> volatility_{};
    
    // Trade history for Monte Carlo
    std::vector<double> trade_results_;
    
public:
    constexpr InstitutionalEngine() noexcept 
        : slippage_model_(2.0, 15.0, 20.0),  // base 2bps, vol multiplier, burst multiplier
          portfolio_(500.0, 5000.0, 1000.0),  // max daily DD, max exposure, kill threshold
          micro_model_(0.85, 0.002, 0.6, 0.8)  // fill prob, impact coeff, liquidity decay, burst widen
    {
        for (auto& v : volatility_) v = 0.0;
        
        // Setup trade aggregator callback
        trade_agg_.on_aggregated_trade([this](size_t sym, double vwap, double volume, uint32_t tick_count) {
            this->on_aggregated_trade(sym, vwap, volume, tick_count);
        });
    }
    
    explicit InstitutionalEngine(double starting_equity) noexcept 
        : InstitutionalEngine() 
    {
        equity_.store(starting_equity, std::memory_order_relaxed);
        portfolio_.update_equity(starting_equity);
        portfolio_.reset_kill();  // Reset kill switch on startup
    }
    
    // Update order book
    inline void update_book(size_t sym, const std::array<Level, Config::BOOK_DEPTH>& bids,
                           const std::array<Level, Config::BOOK_DEPTH>& asks) noexcept {
        if (sym >= Config::MAX_SYMBOLS) return;
        
        books_[sym].bids = bids;
        books_[sym].asks = asks;
        books_[sym].mid = (bids[0].price + asks[0].price) / 2.0;
        books_[sym].spread = asks[0].price - bids[0].price;
    }
    
    // Add raw trade tick (will be aggregated)
    inline void add_trade(size_t sym, double price, double quantity) noexcept {
        trade_agg_.add_trade(sym, price, quantity);
    }
    
    // Callback for aggregated trades
    void on_aggregated_trade(size_t sym, double vwap, double volume, uint32_t tick_count) noexcept {
        if (sym >= Config::MAX_SYMBOLS) return;
        
        // Convert book to first 5 levels for imbalance engine
        std::array<Level, 5> bids_5, asks_5;
        for (size_t i = 0; i < 5; ++i) {
            bids_5[i] = books_[sym].bids[i];
            asks_5[i] = books_[sym].asks[i];
        }
        
        // Process through imbalance engine
        auto signal = imbalance_engine_.process_trade(sym, vwap, volume, bids_5, asks_5);
        
        // Update volatility estimate (simple)
        double spread_bps = (books_[sym].spread / books_[sym].mid) * 10000.0;
        volatility_[sym] = 0.95 * volatility_[sym] + 0.05 * spread_bps;
        
        // Log aggregated signal (not raw ticks)
        static uint64_t last_log_us[Config::MAX_SYMBOLS] = {0};
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        if (now_us - last_log_us[sym] > 10000000) {  // Every 5 seconds
            const char* symbol_name = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
            printf("[MICRO] %s: imb=%.2f burst=%s absorp=%s accel=%.2f vol=%.2f ticks=%u\n",
                   symbol_name,
                   signal.imbalance,
                   signal.is_burst ? "YES" : "NO",
                   signal.is_absorption ? "YES" : "NO",
                   signal.acceleration,
                   volume,
                   tick_count);
            last_log_us[sym] = now_us;
        }
    }
    
    // Main tick function
    inline void tick(size_t sym) noexcept {
        if (sym >= Config::MAX_SYMBOLS) return;
        if (disable_trading_.load(std::memory_order_relaxed)) return;
        
        // Update arm controller every tick
        arm_controller_.on_tick();
        
        // Check book readiness
        bool book_ready = (books_[sym].mid > 0.01 && 
                           books_[sym].bids[0].price > 0.01 && 
                           books_[sym].asks[0].price > 0.01 &&
                           books_[sym].bids[0].size > 0.0001 &&
                           books_[sym].asks[0].size > 0.0001);
        bool vol_ready = accel_engines_[sym].has_sufficient_samples();
        bool dd_breach = portfolio_.daily_dd() > 500.0;
        
        // CRITICAL: Log readiness every 5 seconds
        static uint64_t last_readiness_us[Config::MAX_SYMBOLS] = {0};
        auto readiness_now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        if (readiness_now_us - last_readiness_us[sym] > 10000000) {
            const char* sym_name = (sym == 0) ? "BTC" : "ETH";
            printf("[READINESS] %s: book_ready=%s vol_ready=%s dd_breach=%s armed=%s\n",
                   sym_name,
                   book_ready ? "YES" : "NO",
                   vol_ready ? "YES" : "NO", 
                   dd_breach ? "YES" : "NO",
                   arm_controller_.is_armed() ? "YES" : "NO");
            printf("[BOOK_CHECK] %s: mid=%.2f bid[0].price=%.2f bid[0].size=%.4f\n",
                   sym_name, books_[sym].mid, books_[sym].bids[0].price, books_[sym].bids[0].size);
            last_readiness_us[sym] = readiness_now_us;
        }
        
        // Update arm controller with readiness
        arm_controller_.update(book_ready, vol_ready, dd_breach);
        
        // HARD BLOCK if not armed
        if (!arm_controller_.is_armed()) {
            return;
        }
        
        if (portfolio_.killed()) return;
        if (!loss_gov_.allows_trade()) {
            arm_controller_.trigger_loss_cluster();
            return;
        }
        
        // Flush aggregator periodically
        trade_agg_.tick();
        
        // Don't calculate if book is empty
        if (books_[sym].mid < 0.01) {
            return;  // Book not populated yet
        }
        
        // Update acceleration engine with price
        accel_engines_[sym].on_tick(books_[sym].mid);
        
        double conf = strategy_.compute_confidence(books_[sym]);
        double imb = compute_imbalance(books_[sym]);
        double spread_bps = (books_[sym].spread / books_[sym].mid) * 10000.0;
        
        // Get NORMALIZED acceleration (already clamped to [-10, +10] in MicroAcceleration)
        double norm_accel = accel_engines_[sym].get_normalized_accel();
        
        // Update volatility
        volatility_[sym] = 0.95 * volatility_[sym] + 0.05 * spread_bps;
        
        // Get imbalance signal
        double trade_imbalance = imbalance_engine_.get_imbalance(sym);
        
        // Enhanced confidence combining micro + imbalance
        double enhanced_conf = 0.7 * conf + 0.3 * (std::abs(trade_imbalance));
        
        // Debug output every 5 seconds
        static uint64_t last_debug_us[Config::MAX_SYMBOLS] = {0};
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        bool should_print = (now_us - last_debug_us[sym]) > 10000000;
        
        if (should_print) {
            const char* symbol_name = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
            
            // Diagnostic: Check book sizes
            double bid_total = 0.0, ask_total = 0.0;
            for (const auto& b : books_[sym].bids) bid_total += b.size;
            for (const auto& a : books_[sym].asks) ask_total += a.size;
            
            printf("[DEBUG] %s: conf=%.2f enh_conf=%.2f imb=%.2f trade_imb=%.2f spread_bps=%.2f vol=%.2f mid=%.2f\n",
                   symbol_name, conf, enhanced_conf, imb, trade_imbalance, spread_bps, volatility_[sym], books_[sym].mid);
            printf("[BOOK] %s: bid_vol=%.2f ask_vol=%.2f bid[0]=%.2f@%.2f ask[0]=%.2f@%.2f\n",
                   symbol_name, bid_total, ask_total, books_[sym].bids[0].price, books_[sym].bids[0].size,
                   books_[sym].asks[0].price, books_[sym].asks[0].size);
            printf("[ACCEL] %s: norm_accel=%.2f (NO MORE 500+ SPIKES!)\n",
                   symbol_name, norm_accel);
            printf("[ARM] state=%s reason=%s warmup_remaining=%u\n",
                   arm_controller_.state_string().c_str(),
                   arm_controller_.reason_string().c_str(),
                   arm_controller_.warmup_remaining());
            printf("[PORTFOLIO] DD=%.2f exposure=%.2f equity=%.2f quality=%.2f killed=%s\n",
                   portfolio_.daily_dd(), portfolio_.current_exposure(), 
                   equity_.load(std::memory_order_relaxed), exec_quality_.quality_score(),
                   portfolio_.killed() ? "YES" : "NO");
            last_debug_us[sym] = now_us;
        }
        
        // Entry logic with RISING-EDGE trigger (NOT level-triggered)
        const char* symbol_name = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
        std::string symbol_str(symbol_name);
        
        // Check execution simulator position status
        bool hasPosition = false;
        if (exec_sim_) {
            auto* sim = reinterpret_cast<ExecutionSimulator*>(exec_sim_);
            hasPosition = sim->has_open_position();
        }
        
        // CRITICAL: Use SignalLatch for rising-edge detection
        if (signal_latch_.shouldEnter(symbol_str, enhanced_conf, strategy_.threshold(), hasPosition)) {
            if (portfolio_.allow_trade(100.0) && exec_sim_ && btc_depth_ && eth_depth_) {
                auto* sim = reinterpret_cast<ExecutionSimulator*>(exec_sim_);
                auto* btc_dm = reinterpret_cast<DepthManager*>(btc_depth_);
                auto* eth_dm = reinterpret_cast<DepthManager*>(eth_depth_);
                
                // Get book from appropriate depth manager
                auto& depth_mgr = (sym == 0) ? *btc_dm : *eth_dm;
                auto depth_view = depth_mgr.book();
                
                // Build OrderBook for execution
                chimera::OrderBook book{};
                book.mid = depth_view.mid;
                book.bids[0].price = depth_view.bestBid;
                book.asks[0].price = depth_view.bestAsk;
                book.bids[0].size = 1.0;  // Simplified
                book.asks[0].size = 1.0;
                
                // Determine side based on imbalance
                Side side = (imb > 0) ? Side::BUY : Side::SELL;
                
                // Calculate stop and target (0.1% stop, 0.15% target for HFT)
                double stop = (side == Side::BUY) ? book.mid * 0.999 : book.mid * 1.001;
                double target = (side == Side::BUY) ? book.mid * 1.0015 : book.mid * 0.9985;
                
                // Execute entry via simulator
                sim->enter(symbol_str, side, book, stop, target, 0.01);
                
                // Register REAL notional exposure (qty * price)
                double notional_exposure = 0.01 * book.mid;
                portfolio_.register_exposure(notional_exposure);
            }
        }
        
        // CRITICAL: Check exit conditions on EVERY tick for open positions
        if (exec_sim_) {
            auto* sim = reinterpret_cast<ExecutionSimulator*>(exec_sim_);
            OrderBook book;
            book.mid = books_[sym].mid;
            book.bids[0].price = books_[sym].bids[0].price;
            book.bids[0].size = books_[sym].bids[0].size;
            book.asks[0].price = books_[sym].asks[0].price;
            book.asks[0].size = books_[sym].asks[0].size;
            
            const char* symbol_str = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
            sim->on_tick(symbol_str, book);
        }
    }
    
    // Record trade result
    inline void record_trade_result(bool win, double r_value) noexcept {
        loss_gov_.record(win);
        trade_results_.push_back(r_value);
        
        // Run Monte Carlo stress test every 100 trades
        if (trade_results_.size() >= 100 && trade_results_.size() % 100 == 0) {
            auto mc_result = monte_carlo_.quick_stress(trade_results_);
            printf("[MONTE_CARLO] worst_dd=%.2f avg_return=%.2f median=%.2f\n",
                   mc_result.worst_drawdown, mc_result.avg_return, mc_result.median_return);
        }
    }
    
    // Getters
    [[nodiscard]] double get_equity() const noexcept {
        return equity_.load(std::memory_order_relaxed);
    }
    
    [[nodiscard]] const ExecutionQualityTracker& get_exec_quality() const noexcept {
        return exec_quality_;
    }
    
    [[nodiscard]] ExecutionQualityTracker& get_exec_quality_mutable() noexcept {
        return exec_quality_;
    }
    
    [[nodiscard]] const PortfolioEnvelope& get_portfolio() const noexcept {
        return portfolio_;
    }
    
    [[nodiscard]] PortfolioEnvelope& get_portfolio_mutable() noexcept {
        return portfolio_;
    }
    
    [[nodiscard]] std::vector<double>& get_trade_results() noexcept {
        return trade_results_;
    }
    
    [[nodiscard]] double get_trade_imbalance(size_t sym) const noexcept {
        return imbalance_engine_.get_imbalance(sym);
    }
    
    [[nodiscard]] double get_volatility(size_t sym) const noexcept {
        if (sym >= Config::MAX_SYMBOLS) return 0.0;
        return volatility_[sym];
    }
    
    // Set telemetry (called from main)
    void set_telemetry(TelemetryState* telem) noexcept {
        telemetry_ = telem;
    }
    
    void set_execution_sim(void* exec_sim) noexcept {
        exec_sim_ = exec_sim;
    }
    
    void set_depth_managers(void* btc_depth, void* eth_depth) noexcept {
        btc_depth_ = btc_depth;
        eth_depth_ = eth_depth;
    }
    
    // Update telemetry snapshot
    inline void update_telemetry() noexcept {
        if (!telemetry_) return;
        
        std::lock_guard<std::mutex> lock(telemetry_->mtx);
        
        // Portfolio
        telemetry_->total_pnl = equity_.load(std::memory_order_relaxed) - 10000.0;
        telemetry_->total_trades = trade_results_.size();
        
        // Risk
        telemetry_->risk.daily_drawdown = portfolio_.daily_dd();
        telemetry_->risk.peak_equity = portfolio_.peak_equity();
        telemetry_->risk.current_equity = equity_.load(std::memory_order_relaxed);
        telemetry_->risk.exposure = portfolio_.current_exposure();
        telemetry_->risk.risk_used_pct = (portfolio_.daily_dd() / 500.0) * 100.0;
        telemetry_->risk.exposure_pct = (portfolio_.current_exposure() / 5000.0) * 100.0;
        telemetry_->risk.kill_active = portfolio_.killed();
        telemetry_->risk.kill_reason = portfolio_.killed() ? "DD_BREACH" : "NONE";
        
        // Execution quality
        telemetry_->exec.fills = exec_quality_.count();
        telemetry_->exec.avg_entry_slippage = exec_quality_.average_slippage_bps();
        telemetry_->exec.worst_slippage = exec_quality_.worst_slippage_bps();
        
        // Engine state
        if (portfolio_.killed()) {
            telemetry_->state.current_state = "KILLED";
            telemetry_->state.gating_reason = "KILL_SWITCH_ACTIVE";
            telemetry_->state.armed = false;
        } else if (telemetry_->state.warmup_ticks_remaining > 0) {
            telemetry_->state.current_state = "WARMUP";
            telemetry_->state.gating_reason = "COLLECTING_SAMPLES";
            telemetry_->state.armed = false;
        } else if (!loss_gov_.allows_trade()) {
            telemetry_->state.current_state = "LOSS_CLUSTER";
            telemetry_->state.gating_reason = "CONSECUTIVE_LOSSES";
            telemetry_->state.armed = false;
        } else {
            telemetry_->state.current_state = "ACTIVE";
            telemetry_->state.gating_reason = "READY";
            telemetry_->state.armed = true;
        }
        
        // Update symbols
        for (size_t sym = 0; sym < std::min(Config::MAX_SYMBOLS, telemetry_->symbols.size()); ++sym) {
            auto& s = telemetry_->symbols[sym];
            s.price = books_[sym].mid;
            s.imbalance = compute_imbalance(books_[sym]);
            s.spread_bps = (books_[sym].spread / books_[sym].mid) * 10000.0;
            s.volatility = volatility_[sym];
            s.confidence = strategy_.compute_confidence(books_[sym]);
            
            // Book depth
            s.bid_depth = 0.0;
            s.ask_depth = 0.0;
            for (const auto& b : books_[sym].bids) s.bid_depth += b.size;
            for (const auto& a : books_[sym].asks) s.ask_depth += a.size;
            s.top_bid = books_[sym].bids[0].price;
            s.top_ask = books_[sym].asks[0].price;
        }
    }
};

} // namespace chimera
