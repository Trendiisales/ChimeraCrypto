#pragma once
#include "types.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace chimera {

// Forward declarations
class ExecutionQualityTracker;
class PortfolioEnvelope;

enum class Side { BUY, SELL };

struct ExecPosition {
    bool open = false;
    Side side;
    double entry = 0.0;
    double stop = 0.0;
    double target = 0.0;
    double qty = 0.0;
    std::string symbol;
};

struct TradeFill {
    double price = 0.0;
    double qty = 0.0;
    double slippage_bps = 0.0;
    double pnl = 0.0;
};

class ExecutionSimulator {
public:
    ExecutionSimulator(double start_equity) : equity_(start_equity) {}
    
    bool can_enter() const {
        return !position_.open;
    }
    
    void enter(const std::string& symbol, Side side, const OrderBook& book, 
               double stop, double target, double qty) {
        if (!can_enter() || book.mid < 0.01) return;
        
        // Entry price with slippage
        double entry_price = (side == Side::BUY) ? book.asks[0].price : book.bids[0].price;
        
        // Estimate slippage
        double mid = book.mid;
        double slip_bps = std::abs((entry_price - mid) / mid) * 10000.0;
        
        position_.open = true;
        position_.side = side;
        position_.entry = entry_price;
        position_.stop = stop;
        position_.target = target;
        position_.qty = qty;
        position_.symbol = symbol;
        
        total_slippage_bps_ += slip_bps;
        worst_slippage_bps_ = std::max(worst_slippage_bps_, slip_bps);
        fills_++;
        
        // Update quality tracker
        if (quality_tracker_) {
            auto* tracker = reinterpret_cast<ExecutionQualityTracker*>(quality_tracker_);
            tracker->record(book.mid, entry_price, qty);  // intended, actual, size
        }
        
        printf("[FILL] ENTRY %s %s @ %.2f slip=%.2f bps qty=%.4f\n",
               symbol.c_str(),
               (side == Side::BUY ? "BUY" : "SELL"),
               entry_price, slip_bps, qty);
    }
    
    void on_tick(const std::string& symbol, const OrderBook& book) {
        if (!position_.open || book.mid < 0.01) return;
        
        // CRITICAL: Only process if this book matches the open position symbol
        if (position_.symbol != symbol) return;
        
        // Check for exit
        double price = (position_.side == Side::BUY) ? book.bids[0].price : book.asks[0].price;
        
        bool should_exit = false;
        
        if (position_.side == Side::BUY) {
            if (price <= position_.stop || price >= position_.target) {
                should_exit = true;
            }
        } else {
            if (price >= position_.stop || price <= position_.target) {
                should_exit = true;
            }
        }
        
        // Debug: Print exit check
        static int debug_counter = 0;
        if (++debug_counter % 100 == 0) {
            printf("[EXIT_CHECK] %s: price=%.2f stop=%.2f target=%.2f should_exit=%s\n",
                   symbol.c_str(), price, position_.stop, position_.target, 
                   should_exit ? "YES" : "NO");
        }
        
        if (should_exit) {
            // Calculate PnL
            double pnl = 0.0;
            if (position_.side == Side::BUY) {
                pnl = (price - position_.entry) * position_.qty;
            } else {
                pnl = (position_.entry - price) * position_.qty;
            }
            
            // Exit slippage
            double mid = book.mid;
            double slip_bps = std::abs((price - mid) / mid) * 10000.0;
            
            total_slippage_bps_ += slip_bps;
            worst_slippage_bps_ = std::max(worst_slippage_bps_, slip_bps);
            fills_++;
            
            equity_ += pnl;
            realized_pnl_ += pnl;
            
            bool win = (pnl > 0);
            if (win) wins_++; else losses_++;
            
            // Update quality tracker
            if (quality_tracker_) {
                auto* tracker = reinterpret_cast<ExecutionQualityTracker*>(quality_tracker_);
                tracker->record(book.mid, price, position_.qty);  // intended, actual, size
            }
            
            // Release exposure
            if (portfolio_) {
                auto* port = reinterpret_cast<PortfolioEnvelope*>(portfolio_);
                double notional_exposure = position_.qty * position_.entry;
                port->release_exposure(notional_exposure);
                port->update_equity(equity_);
            }
            
            // Record R-value for trade results
            if (trade_results_) {
                auto* results = reinterpret_cast<std::vector<double>*>(trade_results_);
                double r_value = pnl / (position_.entry * position_.qty);  // R = pnl / risk
                results->push_back(r_value);
            }
            
            printf("[FILL] EXIT %s @ %.2f pnl=%.2f slip=%.2f bps equity=%.2f\n",
                   position_.symbol.c_str(), price, pnl, slip_bps, equity_);
            
            position_.open = false;
        }
    }
    
    // Getters for metrics
    int fills() const { return fills_; }
    double avg_slippage() const { return fills_ > 0 ? total_slippage_bps_ / fills_ : 0.0; }
    double worst_slippage() const { return worst_slippage_bps_; }
    double equity() const { return equity_; }
    double realized_pnl() const { return realized_pnl_; }
    int wins() const { return wins_; }
    int losses() const { return losses_; }
    bool has_open_position() const { return position_.open; }
    
    // Wire to quality tracker
    void set_quality_tracker(void* tracker) {
        quality_tracker_ = tracker;
    }
    
    void set_portfolio(void* portfolio) {
        portfolio_ = portfolio;
    }
    
    void set_trade_results(void* results) {
        trade_results_ = results;
    }

private:
    ExecPosition position_;
    double equity_;
    double realized_pnl_ = 0.0;
    int fills_ = 0;
    int wins_ = 0;
    int losses_ = 0;
    double total_slippage_bps_ = 0.0;
    double worst_slippage_bps_ = 0.0;
    void* quality_tracker_ = nullptr;  // ExecutionQualityTracker*
    void* portfolio_ = nullptr;        // PortfolioEnvelope*
    void* trade_results_ = nullptr;    // std::vector<double>*
};

} // namespace chimera
