#pragma once

// Example integration showing how to wire all new components into BalancedEngine

#include "core/BalancedEngine.hpp"
#include "governor/CapitalAllocationGovernor.hpp"
#include "execution/LatencyDiagnostics.hpp"

namespace chimera {

// Enhanced BalancedEngine with capital allocation and diagnostics
class EnhancedBalancedEngine : public BalancedEngine {
public:
    EnhancedBalancedEngine()
        : BalancedEngine(),
          capital_governor_(10000.0, 100, 0.10, 0.70, 0.40, 10'000'000),
          last_capital_update_us_(0),
          last_diagnostic_dump_us_(0)
    {
    }

    // Override tick processing to add diagnostics
    void on_tick_enhanced(int sym_idx, double price, double latency_ms, int64_t timestamp_us) {
        const char* symbol_name = get_symbol_name(sym_idx);
        
        // 1. Record tick arrival for load profiling
        latency_diagnostics_.on_tick(symbol_name);
        
        // 2. Record latency sample (for histogram and shock detection)
        capital_governor_.record_latency(symbol_name, latency_ms, timestamp_us);
        latency_diagnostics_.on_latency_sample(symbol_name, latency_ms);
        
        // 3. Apply infrastructure shock suppression
        double shock_factor = capital_governor_.shock_factor(symbol_name);
        
        // 4. Process tick normally (your existing logic)
        // ... call existing tick handler ...
        
        // 5. Periodic capital rebalancing (every 30 seconds)
        if (timestamp_us - last_capital_update_us_ > 30'000'000) {
            capital_governor_.update();
            log_capital_allocation();
            last_capital_update_us_ = timestamp_us;
        }
        
        // 6. Periodic diagnostic dump (every 60 seconds)
        if (timestamp_us - last_diagnostic_dump_us_ > 60'000'000) {
            log_diagnostics();
            last_diagnostic_dump_us_ = timestamp_us;
        }
    }

    // Override trade evaluation to add diagnostics
    bool should_enter_trade_enhanced(int sym_idx, double volatility, double latency_ms) {
        const char* symbol_name = get_symbol_name(sym_idx);
        
        // Record evaluation attempt
        latency_diagnostics_.on_evaluation(symbol_name);
        
        // Check infrastructure shock - suppress if active
        if (capital_governor_.is_under_shock(symbol_name)) {
            // Could still evaluate but with reduced aggression
            double shock_factor = capital_governor_.shock_factor(symbol_name);
            volatility *= shock_factor;  // Reduce perceived volatility
        }
        
        // Your existing evaluation logic
        bool passed = evaluate_entry_conditions(sym_idx, volatility, latency_ms);
        
        // If blocked by latency, record it
        if (!passed && was_blocked_by_latency(latency_ms)) {
            latency_diagnostics_.on_latency_block(symbol_name);
        }
        
        return passed;
    }

    // Override position sizing to use capital allocation
    double compute_position_size_enhanced(int sym_idx, double volatility_mult, double base_size) {
        const char* symbol_name = get_symbol_name(sym_idx);
        
        // Get allocated capital for this symbol
        double allocated_capital = capital_governor_.capital_for(symbol_name);
        
        // Get shock suppression factor
        double shock_factor = capital_governor_.shock_factor(symbol_name);
        
        // Compute size with allocation
        double size = (allocated_capital * 0.02) * volatility_mult * shock_factor;
        
        return size;
    }

    // Override trade close to record performance
    void on_trade_close_enhanced(int sym_idx, double pnl_bps, double slippage_bps, 
                                 double trade_latency_ms, int64_t timestamp_us) {
        const char* symbol_name = get_symbol_name(sym_idx);
        
        // Record trade with full metrics
        capital_governor_.record_trade(
            symbol_name,
            pnl_bps,
            slippage_bps,
            trade_latency_ms,
            timestamp_us
        );
        
        // Your existing close logic
        // ... handle position cleanup ...
    }

    // Build enhanced telemetry JSON
    std::string build_telemetry_json_enhanced() const {
        std::ostringstream oss;
        oss << "{";
        
        // Your existing telemetry fields
        oss << "\"equity\":" << get_equity() << ",";
        oss << "\"realized_pnl\":" << get_realized_pnl() << ",";
        
        // Add capital allocation snapshot
        oss << "\"capital_allocation\":" << capital_governor_.build_json_snapshot() << ",";
        
        // Add latency diagnostics
        oss << "\"latency_diagnostics\":" << latency_diagnostics_.build_json();
        
        oss << "}";
        return oss.str();
    }

    // Get performance by latency band for analysis
    void analyze_band_performance() {
        const char* symbols[] = {"btcusdt", "ethusdt", "solusdt"};
        
        fprintf(stderr, "\n=== PnL BY LATENCY BAND ===\n");
        for (const char* symbol : symbols) {
            auto bands = capital_governor_.get_band_performance(symbol);
            
            fprintf(stderr, "%s:\n", symbol);
            fprintf(stderr, "  FAST (<15ms):   %3d trades, avg %.1f bps, slip %.1f bps - %s\n",
                   bands[BAND_FAST].trades,
                   bands[BAND_FAST].avg_pnl_bps,
                   bands[BAND_FAST].avg_slippage_bps,
                   bands[BAND_FAST].profitable ? "PROFITABLE" : "UNPROFITABLE");
            
            fprintf(stderr, "  MEDIUM (15-25ms): %3d trades, avg %.1f bps, slip %.1f bps - %s\n",
                   bands[BAND_MEDIUM].trades,
                   bands[BAND_MEDIUM].avg_pnl_bps,
                   bands[BAND_MEDIUM].avg_slippage_bps,
                   bands[BAND_MEDIUM].profitable ? "PROFITABLE" : "UNPROFITABLE");
            
            fprintf(stderr, "  SLOW (25-50ms): %3d trades, avg %.1f bps, slip %.1f bps - %s\n",
                   bands[BAND_SLOW].trades,
                   bands[BAND_SLOW].avg_pnl_bps,
                   bands[BAND_SLOW].avg_slippage_bps,
                   bands[BAND_SLOW].profitable ? "PROFITABLE" : "UNPROFITABLE");
        }
        fprintf(stderr, "===========================\n\n");
    }

    // Get latency percentiles for monitoring
    void log_latency_percentiles() {
        const char* symbols[] = {"btcusdt", "ethusdt", "solusdt"};
        
        fprintf(stderr, "\n=== LATENCY PERCENTILES ===\n");
        for (const char* symbol : symbols) {
            auto stats = capital_governor_.get_latency_stats(symbol);
            
            fprintf(stderr, "%s: p50=%.1f p75=%.1f p95=%.1f p99=%.1f max=%.1f ms\n",
                   symbol,
                   stats.p50,
                   stats.p75,
                   stats.p95,
                   stats.p99,
                   stats.max);
        }
        fprintf(stderr, "===========================\n\n");
    }

private:
    const char* get_symbol_name(int sym_idx) const {
        return (sym_idx == 0) ? "btcusdt" : 
               (sym_idx == 1) ? "ethusdt" : "solusdt";
    }

    void log_capital_allocation() {
        fprintf(stderr, "[CAPITAL-ALLOCATION] BTC=%.1f%% ETH=%.1f%% SOL=%.1f%%\n",
               capital_governor_.weight("btcusdt") * 100.0,
               capital_governor_.weight("ethusdt") * 100.0,
               capital_governor_.weight("solusdt") * 100.0);
    }

    void log_diagnostics() {
        // Dump full diagnostic report
        fprintf(stderr, "%s", latency_diagnostics_.generate_report().c_str());
        
        // Also dump band performance
        analyze_band_performance();
        
        // And latency percentiles
        log_latency_percentiles();
    }

    // Placeholder for existing logic
    bool evaluate_entry_conditions(int sym_idx, double volatility, double latency_ms) {
        // Your existing logic here
        return true;
    }

    bool was_blocked_by_latency(double latency_ms) {
        // Check if this latency would trigger block
        return latency_ms > 25.0;  // Example threshold
    }

    double get_equity() const { return 10000.0; }  // Placeholder
    double get_realized_pnl() const { return 0.0; }  // Placeholder

    // New members
    CapitalAllocationGovernor capital_governor_;
    LatencyDiagnostics latency_diagnostics_;
    int64_t last_capital_update_us_;
    int64_t last_diagnostic_dump_us_;
};

} // namespace chimera
