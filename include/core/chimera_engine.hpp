// chimera_engine.hpp - Modern C++17 HFT Engine with CRTP
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <immintrin.h>  // SIMD

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// CONFIGURATION - Compile-time constants
// ═══════════════════════════════════════════════════════════════════

struct Config {
    static constexpr size_t MAX_SYMBOLS = 4;
    static constexpr size_t BOOK_DEPTH = 20;
    static constexpr size_t TRADE_BUFFER = 10000;
    
    static constexpr double MAX_PORTFOLIO_RISK = 0.20;
    static constexpr double MAX_SYMBOL_RISK = 0.05;
    static constexpr double BASE_RISK_PER_TRADE = 0.002;
    
    static constexpr double TAKER_FEE = 0.0004;
    static constexpr double MIN_STOP_BPS = 5.0;
    static constexpr double COST_COVERAGE_MULT = 1.8;
    
    static constexpr int CONSEC_LOSS_LIMIT = 3;
    static constexpr int LOSS_WINDOW = 10;
    static constexpr int LOSS_COUNT_LIMIT = 4;
};

// ═══════════════════════════════════════════════════════════════════
// LEVEL 2 BOOK - Cache-aligned, SIMD-friendly
// ═══════════════════════════════════════════════════════════════════

struct alignas(64) Level {
    double price;
    double size;
    
    constexpr Level() noexcept : price(0.0), size(0.0) {}
    constexpr Level(double p, double s) noexcept : price(p), size(s) {}
};

struct alignas(128) OrderBook {
    std::array<Level, Config::BOOK_DEPTH> bids;
    std::array<Level, Config::BOOK_DEPTH> asks;
    double mid{0.0};
    double spread{0.0};
    uint64_t last_update_us{0};
    
    // SIMD-accelerated imbalance calculation
    [[nodiscard]] inline double imbalance() const noexcept {
        double bid_vol = 0.0, ask_vol = 0.0;
        
#ifdef __AVX2__
        // Process 4 levels at a time with AVX2
        __m256d bid_sum = _mm256_setzero_pd();
        __m256d ask_sum = _mm256_setzero_pd();
        
        for (size_t i = 0; i < Config::BOOK_DEPTH; i += 4) {
            __m256d bid_sizes = _mm256_set_pd(
                bids[i+3].size, bids[i+2].size,
                bids[i+1].size, bids[i].size
            );
            __m256d ask_sizes = _mm256_set_pd(
                asks[i+3].size, asks[i+2].size,
                asks[i+1].size, asks[i].size
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
        for (const auto& b : bids) bid_vol += b.size;
        for (const auto& a : asks) ask_vol += a.size;
#endif
        
        double total = bid_vol + ask_vol;
        return total > 0.0 ? bid_vol / total : 0.5;
    }
};

// ═══════════════════════════════════════════════════════════════════
// LOSS CLUSTER GOVERNOR - Template-based, zero overhead
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
        return allows_trade() ? 1.0 : 0.5;
    }
};

// ═══════════════════════════════════════════════════════════════════
// SLIPPAGE REALITY ENGINE - Lock-free, cache-efficient
// ═══════════════════════════════════════════════════════════════════

template<size_t HistorySize = 500>
class SlippageEngine {
private:
    struct Sample {
        double intended_entry;
        double actual_entry;
        double intended_exit;
        double actual_exit;
        uint8_t regime;  // Pack tightly
    };
    
    alignas(64) std::array<Sample, HistorySize> samples_{};
    std::atomic<size_t> index_{0};
    std::atomic<size_t> count_{0};
    
    std::array<std::atomic<double>, 4> regime_slip_{};  // Per-regime averages
    
public:
    constexpr SlippageEngine() noexcept {
        for (auto& s : regime_slip_) {
            s.store(0.0, std::memory_order_relaxed);
        }
    }
    
    inline void record(double intended_entry, double actual_entry,
                      double intended_exit, double actual_exit,
                      uint8_t regime) noexcept {
        size_t idx = index_.fetch_add(1, std::memory_order_relaxed) % HistorySize;
        
        samples_[idx] = {intended_entry, actual_entry, intended_exit, actual_exit, regime};
        
        size_t c = count_.load(std::memory_order_relaxed);
        if (c < HistorySize) {
            count_.store(c + 1, std::memory_order_relaxed);
        }
        
        // Update regime-specific slip
        double entry_slip = (actual_entry - intended_entry) / intended_entry * 10000.0;
        double exit_slip = (actual_exit - intended_exit) / intended_exit * 10000.0;
        double total_slip = entry_slip + exit_slip;
        
        double old = regime_slip_[regime].load(std::memory_order_relaxed);
        double updated = 0.95 * old + 0.05 * total_slip;
        regime_slip_[regime].store(updated, std::memory_order_relaxed);
    }
    
    [[nodiscard]] inline double expected_slip(uint8_t regime) const noexcept {
        return regime_slip_[regime].load(std::memory_order_relaxed);
    }
};

// ═══════════════════════════════════════════════════════════════════
// STRATEGY INTERFACE - CRTP for zero-cost polymorphism
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
// MICRO STRATEGY - Concrete implementation
// ═══════════════════════════════════════════════════════════════════

class MicroStrategy : public Strategy<MicroStrategy> {
private:
    double threshold_{0.65};
    
public:
    [[nodiscard]] inline double compute_confidence_impl(const OrderBook& book) noexcept {
        double imb = book.imbalance();
        double spread_norm = std::min(book.spread / book.mid * 10000.0 / 5.0, 1.0);
        
        // Imbalance score
        double imb_score = 1.0 - std::abs(imb - 0.5) * 2.0;
        
        // Spread score (tighter = better)
        double spread_score = 1.0 - spread_norm;
        
        // Weighted combination
        return 0.6 * imb_score + 0.4 * spread_score;
    }
    
    [[nodiscard]] constexpr bool should_enter_impl(double confidence, double threshold) const noexcept {
        return confidence >= threshold;
    }
    
    constexpr void set_threshold(double t) noexcept { threshold_ = t; }
    [[nodiscard]] constexpr double threshold() const noexcept { return threshold_; }
};

// ═══════════════════════════════════════════════════════════════════
// MAIN ENGINE - Brings it all together
// ═══════════════════════════════════════════════════════════════════

template<typename StrategyType>
class Engine {
private:
    std::array<OrderBook, Config::MAX_SYMBOLS> books_;
    LossClusterGovernor<> loss_gov_;
    SlippageEngine<> slip_engine_;
    StrategyType strategy_;
    
    double equity_{10000.0};
    std::atomic<bool> disable_trading_{false};
    
public:
    constexpr Engine() noexcept = default;
    explicit Engine(double starting_equity) noexcept : equity_(starting_equity) {}
    
    inline void update_book(size_t sym, const std::array<Level, Config::BOOK_DEPTH>& bids,
                           const std::array<Level, Config::BOOK_DEPTH>& asks) noexcept {
        books_[sym].bids = bids;
        books_[sym].asks = asks;
        books_[sym].mid = (bids[0].price + asks[0].price) / 2.0;
        books_[sym].spread = asks[0].price - bids[0].price;
    }
    
    inline void tick(size_t sym) noexcept {
        if (disable_trading_.load(std::memory_order_relaxed)) return;
        if (!loss_gov_.allows_trade()) return;
        
        double conf = strategy_.compute_confidence(books_[sym]);
        
        if (strategy_.should_enter(conf, strategy_.threshold())) {
            // Execute trade
        }
    }
    
    inline void record_trade_result(bool win) noexcept {
        loss_gov_.record(win);
    }
};

} // namespace chimera
