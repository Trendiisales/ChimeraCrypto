#pragma once
#include <array>
#include <cstdint>
#include <atomic>
#include "config.hpp"

namespace chimera {

// Cache-aligned level
struct alignas(16) Level {
    double price{0.0};
    double size{0.0};
    
    constexpr Level() noexcept = default;
    constexpr Level(double p, double s) noexcept : price(p), size(s) {}
};

// Order book - cache-line aligned
struct alignas(128) OrderBook {
    std::array<Level, Config::BOOK_DEPTH> bids;
    std::array<Level, Config::BOOK_DEPTH> asks;
    double mid{0.0};
    double spread{0.0};
    uint64_t last_update_us{0};
    
    constexpr OrderBook() noexcept = default;
};

// Market trade
struct MarketTrade {
    double price;
    double size;
    uint64_t timestamp_us;
    bool is_buy;
};

// Position
struct Position {
    double entry{0.0};
    double stop{0.0};
    double target{0.0};
    double size{0.0};
    uint64_t entry_time{0};
    bool is_long{true};
    bool open{false};
    
    constexpr Position() noexcept = default;
};

// Execution trace
struct ExecutionTrace {
    uint64_t signal_ts;
    uint64_t send_ts;
    uint64_t ack_ts;
    uint64_t fill_ts;
    double intended_price;
    double fill_price;
    double slippage_bps;
};

// Portfolio risk state
struct PortfolioRisk {
    std::atomic<double> equity{10000.0};
    std::atomic<double> peak_equity{10000.0};
    std::atomic<double> daily_pnl{0.0};
    double daily_start_equity{10000.0};
    double open_risk{0.0};
    
    constexpr PortfolioRisk() noexcept = default;
    explicit PortfolioRisk(double starting) noexcept 
        : equity(starting), peak_equity(starting), daily_start_equity(starting) {}
};

// Regime types
enum class Regime : uint8_t {
    COMPRESSION = 0,
    EXPANSION = 1,
    TREND = 2,
    CHAOS = 3
};

} // namespace chimera
