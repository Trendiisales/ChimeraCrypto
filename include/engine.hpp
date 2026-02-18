// engine.hpp - Modern C++17 HFT Engine with CRTP
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef __AVX2__
#include <immintrin.h>  // SIMD
#endif

#include "config.hpp"
#include "types.hpp"

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
// SLIPPAGE REALITY ENGINE
// ═══════════════════════════════════════════════════════════════════

template<size_t HistorySize = 500>
class SlippageEngine {
private:
    struct Sample {
        double intended_entry;
        double actual_entry;
        double intended_exit;
        double actual_exit;
        uint8_t regime;
    };
    
    alignas(64) std::array<Sample, HistorySize> samples_{};
    std::atomic<size_t> index_{0};
    std::atomic<size_t> count_{0};
    
    std::array<std::atomic<double>, 4> regime_slip_{};
    
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
// MICRO STRATEGY
// ═══════════════════════════════════════════════════════════════════

class MicroStrategy : public Strategy<MicroStrategy> {
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
        // Use hysteresis to prevent oscillation at boundary
        return confidence > threshold + 0.01;
    }
    
    constexpr void set_threshold(double t) noexcept { threshold_ = t; }
    [[nodiscard]] constexpr double threshold() const noexcept { return threshold_; }
};

// ═══════════════════════════════════════════════════════════════════
// MAIN ENGINE
// ═══════════════════════════════════════════════════════════════════

template<typename StrategyType>
class Engine {
private:
    std::array<OrderBook, Config::MAX_SYMBOLS> books_;
    LossClusterGovernor<> loss_gov_;
    SlippageEngine<> slip_engine_;
    StrategyType strategy_;
    
    std::atomic<double> equity_{10000.0};
    std::atomic<bool> disable_trading_{false};
    
public:
    constexpr Engine() noexcept = default;
    explicit Engine(double starting_equity) noexcept : equity_(starting_equity) {}
    
    inline void update_book(size_t sym, const std::array<Level, Config::BOOK_DEPTH>& bids,
                           const std::array<Level, Config::BOOK_DEPTH>& asks) noexcept {
        if (sym >= Config::MAX_SYMBOLS) return;
        
        books_[sym].bids = bids;
        books_[sym].asks = asks;
        books_[sym].mid = (bids[0].price + asks[0].price) / 2.0;
        books_[sym].spread = asks[0].price - bids[0].price;
    }
    
    inline void tick(size_t sym) noexcept {
        if (sym >= Config::MAX_SYMBOLS) return;
        if (disable_trading_.load(std::memory_order_relaxed)) return;
        if (!loss_gov_.allows_trade()) return;
        
        // Only print debug every 5 seconds
        static uint64_t last_debug_us[Config::MAX_SYMBOLS] = {0};
        auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        bool should_print = (now_us - last_debug_us[sym]) > 5000000; // 5 seconds
        
        // Don't calculate if book is empty
        if (books_[sym].mid < 0.01) {
            return;  // Book not populated yet
        }
        
        double conf = strategy_.compute_confidence(books_[sym]);
        double imb = compute_imbalance(books_[sym]);
        double spread_bps = (books_[sym].spread / books_[sym].mid) * 10000.0;
        
        if (should_print) {
            const char* symbol_name = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
            printf("[MICRO_DEBUG] %s: imb=%.6f spread_bps=%.2f conf=%.2f thresh=%.2f mid=%.2f\n",
                   symbol_name, imb, spread_bps, conf, strategy_.threshold(), books_[sym].mid);
            last_debug_us[sym] = now_us;
        }
        
        if (strategy_.should_enter(conf, strategy_.threshold())) {
            const char* symbol_name = (sym == 0) ? "BTC" : (sym == 1) ? "ETH" : "SOL";
            printf("[SIGNAL] %s: ENTRY SIGNAL conf=%.2f >= thresh=%.2f\n", 
                   symbol_name, conf, strategy_.threshold());
        }
    }
    
    inline void record_trade_result(bool win) noexcept {
        loss_gov_.record(win);
    }
    
    [[nodiscard]] double get_equity() const noexcept {
        return equity_.load(std::memory_order_relaxed);
    }
};

} // namespace chimera
