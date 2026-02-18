#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include "types.hpp"

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// EXCHANGE MICROSTRUCTURE MODEL
// Models queue position, fill probability, book impact, liquidity
// ═══════════════════════════════════════════════════════════════════

struct BookLevel {
    double price;
    double size;
};

struct BookSnapshot {
    std::vector<BookLevel> bids;
    std::vector<BookLevel> asks;
    double spread_bps;
};

class ExchangeMicrostructure {
private:
    double base_fill_prob_;
    double impact_coeff_;
    double liquidity_decay_coeff_;
    double burst_widen_coeff_;
    
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
    
public:
    ExchangeMicrostructure(double base_fill_prob,
                           double impact_coeff,
                           double liquidity_decay_coeff,
                           double burst_widen_coeff)
        : base_fill_prob_(base_fill_prob),
          impact_coeff_(impact_coeff),
          liquidity_decay_coeff_(liquidity_decay_coeff),
          burst_widen_coeff_(burst_widen_coeff),
          rng_(std::random_device{}()),
          dist_(0.0, 1.0)
    {
    }
    
    struct FillResult {
        double average_price;
        double filled_qty;
        double slippage_bps;
        bool fully_filled;
    };
    
    // Execute trade with realistic book impact
    [[nodiscard]] FillResult execute(double requested_qty,
                                     bool is_buy,
                                     const BookSnapshot& book,
                                     double volatility,
                                     double burst_factor,
                                     bool taker) noexcept {
        FillResult result{};
        result.average_price = 0.0;
        result.filled_qty = 0.0;
        result.slippage_bps = 0.0;
        result.fully_filled = false;
        
        const auto& side = is_buy ? book.asks : book.bids;
        
        if (side.empty()) return result;
        
        double remaining = requested_qty;
        double total_cost = 0.0;
        
        // Liquidity thins during volatility
        double liquidity_multiplier = 1.0 - (volatility * liquidity_decay_coeff_);
        liquidity_multiplier = std::max(0.1, liquidity_multiplier);
        
        // Spread widens during bursts (calculated but could be used for future enhancements)
        // double effective_spread_widen = book.spread_bps * (1.0 + burst_factor * burst_widen_coeff_);
        
        // Walk the book
        for (const auto& level : side) {
            if (remaining <= 0.0) break;
            
            double available = level.size * liquidity_multiplier;
            double fill_here = std::min(available, remaining);
            
            // Calculate price impact
            double impact = impact_coeff_ * (fill_here / (available + 1e-9));
            
            double adjusted_price = level.price;
            if (is_buy) {
                adjusted_price += adjusted_price * impact;
            } else {
                adjusted_price -= adjusted_price * impact;
            }
            
            total_cost += adjusted_price * fill_here;
            result.filled_qty += fill_here;
            remaining -= fill_here;
        }
        
        // Calculate average fill price and slippage
        if (result.filled_qty > 0.0) {
            result.average_price = total_cost / result.filled_qty;
            
            double reference_price = is_buy ? book.asks.front().price : book.bids.front().price;
            double diff = result.average_price - reference_price;
            result.slippage_bps = (diff / reference_price) * 10000.0;
        }
        
        result.fully_filled = (remaining <= 1e-9);
        
        return result;
    }
    
    // Maker fill probability (for limit orders)
    [[nodiscard]] bool maker_fill_probability(double queue_position_ratio,
                                              double volatility) noexcept {
        double prob = base_fill_prob_ * (1.0 - queue_position_ratio) * (1.0 - volatility);
        prob = std::clamp(prob, 0.0, 1.0);
        
        return dist_(rng_) < prob;
    }
    
    // Convert Level array to BookSnapshot
    [[nodiscard]] static BookSnapshot convert_book(const std::array<Level, 5>& bids,
                                                   const std::array<Level, 5>& asks) noexcept {
        BookSnapshot snapshot;
        
        for (const auto& b : bids) {
            if (b.price > 0.0) {
                snapshot.bids.push_back({b.price, b.size});
            }
        }
        
        for (const auto& a : asks) {
            if (a.price > 0.0) {
                snapshot.asks.push_back({a.price, a.size});
            }
        }
        
        if (!snapshot.bids.empty() && !snapshot.asks.empty()) {
            double spread = snapshot.asks.front().price - snapshot.bids.front().price;
            double mid = (snapshot.bids.front().price + snapshot.asks.front().price) / 2.0;
            snapshot.spread_bps = (spread / mid) * 10000.0;
        }
        
        return snapshot;
    }
};

} // namespace chimera
