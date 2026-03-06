#ifndef DEEPSTRIKE_BINANCE_MARKET_HPP
#define DEEPSTRIKE_BINANCE_MARKET_HPP

#include "../connector/BinanceWS.hpp"
#include "../core/UnifiedTick.hpp"

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

// Use DeepStrike::BinanceWS
using DeepStrike::BinanceWS;

/**
 * BinanceMarket - Market data feed for Binance crypto pairs
 * 
 * Subscribes to Binance WebSocket streams and converts to UnifiedTick format.
 * Supports multiple symbols via combined streams.
 * 
 * Default stream: !ticker@arr (all tickers)
 * For specific symbols: btcusdt@bookTicker, ethusdt@bookTicker, etc.
 */
class BinanceMarket {
public:
    // Account info structure (from REST API /api/v3/account)
    struct AccountInfo {
        double total_balance = 0.0;    // Total balance in USDT equivalent
        double available_balance = 0.0; // Available for trading
        double locked_balance = 0.0;    // In open orders
        double unrealized_pnl = 0.0;
        bool valid = false;
    };
    
    BinanceMarket();
    ~BinanceMarket();

    /**
     * Start receiving market data
     * @return true if WebSocket connection established
     */
    bool start();
    
    /**
     * Stop receiving market data
     */
    void stop();

    /**
     * Pop next tick from queue (non-blocking)
     * @param t  Output tick
     * @return true if tick was available
     */
    bool pop(UnifiedTick& t);

    /**
     * Get number of ticks in queue
     */
    size_t queue_size() const;
    
    /**
     * Set account balance (called by external REST API query)
     * In production, this would be updated by periodic REST API calls
     */
    void set_account_balance(double balance, double available, double pnl) {
        std::lock_guard<std::mutex> lock(account_mtx_);
        account_info_.total_balance = balance;
        account_info_.available_balance = available;
        account_info_.unrealized_pnl = pnl;
        account_info_.valid = true;
    }
    
    AccountInfo get_account_info() const {
        std::lock_guard<std::mutex> lock(account_mtx_);
        return account_info_;
    }
    
    double get_balance() const { return account_info_.total_balance; }
    double get_equity() const { return account_info_.total_balance + account_info_.unrealized_pnl; }

private:
    void on_message(const std::string& json);
    void parse_ticker_array(const std::string& json);
    void parse_book_ticker(const std::string& json);

private:
    std::atomic<bool> running_{false};
    
    BinanceWS ws_;

    mutable std::mutex queue_mtx_;
    std::queue<UnifiedTick> tick_queue_;
    
    mutable std::mutex account_mtx_;
    AccountInfo account_info_;
    
    // Stats
    std::atomic<uint64_t> ticks_received_{0};
    std::atomic<uint64_t> parse_errors_{0};
};

#endif // DEEPSTRIKE_BINANCE_MARKET_HPP
