#pragma once
#include <atomic>
#include <string>
#include <mutex>
#include <vector>
#include <chrono>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// TELEMETRY STATE - Institutional Operator Console Data
// ═══════════════════════════════════════════════════════════════════

struct SymbolTelemetry {
    std::string symbol;
    
    // Price & Position
    double price{0.0};
    double position{0.0};
    double pnl{0.0};
    
    // Strategy Metrics
    double win_rate{0.0};
    double expectancy{0.0};
    double profit_factor{0.0};
    int trades{0};
    
    // Microstructure
    double imbalance{0.0};
    double spread_bps{0.0};
    double volatility{0.0};
    double accel{0.0};
    bool burst{false};
    bool absorption{false};
    
    // Book Depth
    double bid_depth{0.0};
    double ask_depth{0.0};
    double top_bid{0.0};
    double top_ask{0.0};
    
    // Regime
    std::string regime{"OBSERVE"};
    double confidence{0.0};
};

struct ExecutionTelemetry {
    int orders_sent{0};
    int fills{0};
    int partial_fills{0};
    int rejects{0};
    int cancels{0};
    
    double avg_entry_slippage{0.0};
    double avg_exit_slippage{0.0};
    double worst_slippage{0.0};
    
    double maker_fill_rate{0.0};
    double taker_fill_rate{0.0};
    
    int maker_fills{0};
    int taker_fills{0};
};

struct RiskTelemetry {
    double daily_drawdown{0.0};
    double peak_equity{10000.0};
    double current_equity{10000.0};
    double exposure{0.0};
    
    double risk_used_pct{0.0};
    double exposure_pct{0.0};
    
    bool kill_active{false};
    std::string kill_reason{"NONE"};
    
    double max_daily_dd{500.0};
    double max_exposure{5000.0};
    
    // Rolling metrics
    double rolling_100_dd{0.0};
    double capital_at_risk{0.0};
};

struct LatencyTelemetry {
    double tick_to_signal_ms{0.0};
    double signal_to_order_ms{0.0};
    double order_to_ack_ms{0.0};
    double ack_to_fill_ms{0.0};
    double roundtrip_ms{0.0};
    double p95_ms{0.0};
    double max_spike_ms{0.0};
    
    uint64_t last_tick_us{0};
    uint64_t last_signal_us{0};
};

struct MonteCarloTelemetry {
    double worst_dd{0.0};
    double avg_return{0.0};
    double median_return{0.0};
    double risk_of_ruin{0.0};
    double p95_worst_dd{0.0};
    int iterations{0};
};

struct OperationalTelemetry {
    int reconnects{0};
    int missed_ticks{0};
    double cpu_usage{0.0};
    double memory_mb{0.0};
    double book_stale_ms{0.0};
    
    uint64_t uptime_seconds{0};
    uint64_t last_fill_timestamp{0};
    
    std::string ws_status{"DISCONNECTED"};
    bool depth_desynced{false};
};

struct CostCoverageTelemetry {
    double required_min_r{0.0};
    double current_target_r{0.0};
    double net_expected_r{0.0};
    double cost_pct_of_target{0.0};
    double edge_after_costs{0.0};
    
    double avg_cost_per_trade{0.0};
    double fees_paid{0.0};
    double slippage_cost{0.0};
};

struct EngineStateTelemetry {
    std::string current_state{"WARMUP"};
    uint64_t state_duration_ms{0};
    int state_transitions{0};
    
    double burst_confidence{0.0};
    double regime_stability{0.0};
    
    std::string gating_reason{"STARTING"};
    bool armed{false};
    int warmup_ticks_remaining{5000};
};

// Main telemetry state
struct TelemetryState {
    std::mutex mtx;
    
    // Portfolio
    double total_pnl{0.0};
    int total_trades{0};
    uint64_t runtime_seconds{0};
    
    // Components
    ExecutionTelemetry exec;
    RiskTelemetry risk;
    LatencyTelemetry latency;
    MonteCarloTelemetry mc;
    OperationalTelemetry ops;
    CostCoverageTelemetry cost;
    EngineStateTelemetry state;
    
    std::vector<SymbolTelemetry> symbols;
    
    // Constructor
    TelemetryState() {
        symbols.resize(3);  // BTC, ETH, SOL
        symbols[0].symbol = "BTC";
        symbols[1].symbol = "ETH";
        symbols[2].symbol = "SOL";
    }
    
    // Helper: Update timestamp
    inline uint64_t now_us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

} // namespace chimera
