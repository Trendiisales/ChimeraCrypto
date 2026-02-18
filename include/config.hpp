#pragma once
#include <cstddef>
#include <cstdint>

namespace chimera {

struct Config {
    // System
    static constexpr size_t MAX_SYMBOLS = 4;
    static constexpr size_t BOOK_DEPTH = 20;
    static constexpr size_t TRADE_BUFFER = 10000;
    static constexpr size_t EXEC_BUFFER = 5000;
    
    // Risk limits
    static constexpr double MAX_PORTFOLIO_RISK = 0.20;
    static constexpr double MAX_SYMBOL_RISK = 0.05;
    static constexpr double BASE_RISK_PER_TRADE = 0.002;
    static constexpr double MAX_DRAWDOWN = 0.12;
    static constexpr double DAILY_DD_LIMIT = 0.05;
    static constexpr double KILL_SWITCH_DD = 0.10;
    
    // Stops
    static constexpr double MIN_STOP_BPS = 5.0;
    static constexpr double MIN_STOP_RATIO = 0.0005;
    static constexpr double MIN_R_RATIO = 1.5;
    
    // Costs
    static constexpr double TAKER_FEE = 0.0004;
    static constexpr double MAKER_FEE = 0.0002;
    static constexpr double SLIPPAGE_BPS = 1.5;
    static constexpr double COST_COVERAGE_MULT = 1.8;
    
    // Micro engine
    static constexpr double MICRO_BASE_THRESHOLD = 0.45;  // Lowered for reasonable entry frequency
    static constexpr double HYSTERESIS = 0.10;
    static constexpr int MAX_MICRO_PER_MINUTE = 8;
    static constexpr uint64_t THROTTLE_WINDOW_US = 60'000'000;  // 60 seconds
    static constexpr uint64_t PARALYSIS_TIME_US = 30'000'000;   // 30 seconds
    static constexpr uint64_t MAX_LATENCY_US = 5000;
    
    // Loss cluster
    static constexpr int CONSEC_LOSS_LIMIT = 3;
    static constexpr int LOSS_WINDOW = 10;
    static constexpr int LOSS_COUNT_LIMIT = 4;
    static constexpr double LOSS_CLUSTER_SIZE_CUT = 0.50;
    
    // Liquidity shock
    static constexpr double MAX_SPREAD_BPS = 5.0;
    static constexpr double MIN_BOOK_DEPTH = 10.0;
    static constexpr double MAX_TRADE_VELOCITY = 2.0;
    static constexpr uint64_t SHOCK_COOLDOWN_US = 300'000'000;  // 5 minutes
    
    // Features
    static constexpr double FUNDING_THRESHOLD = 0.0005;
    static constexpr double FUNDING_BOOST = 0.15;
    static constexpr double HIDDEN_LIQ_BOOST = 0.10;
    static constexpr double SPOOF_PENALTY = 0.30;
    static constexpr double LEADLAG_BOOST = 0.20;
    
    // Monte Carlo
    static constexpr int MC_RUNS = 5000;
    static constexpr int MC_MAX_TRADES = 5000;
};

} // namespace chimera
