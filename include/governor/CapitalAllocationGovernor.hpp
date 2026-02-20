#pragma once

#include <string>
#include <unordered_map>
#include <deque>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <array>

namespace chimera {

// Latency bands for performance tracking
enum LatencyBand {
    BAND_FAST = 0,      // <15ms
    BAND_MEDIUM = 1,    // 15-25ms
    BAND_SLOW = 2,      // 25-50ms
    BAND_COUNT = 3
};

struct TradeSample {
    double pnl_bps;
    double slippage_bps;
    double latency_ms;
    int64_t timestamp_us;
};

struct LatencyHistogram {
    std::array<double, 256> samples;
    size_t count = 0;
    size_t write_idx = 0;

    void add(double latency_ms) {
        samples[write_idx] = latency_ms;
        write_idx = (write_idx + 1) % samples.size();
        if (count < samples.size()) count++;
    }

    double percentile(double p) const {
        if (count == 0) return 0.0;
        
        std::vector<double> sorted;
        sorted.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            sorted.push_back(samples[i]);
        }
        std::sort(sorted.begin(), sorted.end());
        
        size_t idx = static_cast<size_t>(p * count);
        if (idx >= count) idx = count - 1;
        return sorted[idx];
    }

    double p50() const { return percentile(0.50); }
    double p75() const { return percentile(0.75); }
    double p95() const { return percentile(0.95); }
    double p99() const { return percentile(0.99); }
    double max() const {
        if (count == 0) return 0.0;
        double m = samples[0];
        for (size_t i = 1; i < count; ++i) {
            if (samples[i] > m) m = samples[i];
        }
        return m;
    }
};

struct LatencyBandStats {
    int trades = 0;
    double total_pnl_bps = 0.0;
    double avg_slippage_bps = 0.0;

    void record(double pnl_bps, double slippage_bps) {
        total_pnl_bps += pnl_bps;
        avg_slippage_bps = (avg_slippage_bps * trades + slippage_bps) / (trades + 1);
        trades++;
    }

    double avg_pnl_bps() const {
        return trades > 0 ? total_pnl_bps / trades : 0.0;
    }

    bool is_profitable() const {
        return trades >= 10 && avg_pnl_bps() > 0.0;
    }
};

struct InfrastructureShock {
    bool active = false;
    int64_t end_time_us = 0;
    double suppress_factor = 1.0;
};

struct SymbolAllocationState {
    std::deque<TradeSample> samples;
    double smoothed_weight = 0.33;
    int total_trades = 0;

    // Enhanced latency tracking
    LatencyHistogram latency_hist;
    
    // Performance by latency band
    std::array<LatencyBandStats, BAND_COUNT> band_stats;
    
    // Infrastructure shock tracking
    InfrastructureShock shock;
    double last_p95_latency = 0.0;
    int64_t last_shock_check_us = 0;
};

class CapitalAllocationGovernor {
public:
    CapitalAllocationGovernor(
        double total_capital,
        int rolling_window = 100,
        double min_weight = 0.10,
        double max_weight = 0.70,
        double shock_threshold = 0.40,          // 40% p95 jump triggers shock
        int shock_duration_us = 10'000'000      // 10 second suppression
    );

    // Core trade recording with enhanced metrics
    void record_trade(
        const std::string& symbol,
        double pnl_bps,
        double slippage_bps,
        double latency_ms,
        int64_t timestamp_us
    );

    // Record latency even without trade (for histogram accuracy)
    void record_latency(
        const std::string& symbol,
        double latency_ms,
        int64_t timestamp_us
    );

    // Periodic rebalancing
    void update();

    // Capital allocation queries
    double weight(const std::string& symbol) const;
    double capital_for(const std::string& symbol) const;

    // Latency histogram queries
    struct LatencyStats {
        double p50 = 0.0;
        double p75 = 0.0;
        double p95 = 0.0;
        double p99 = 0.0;
        double max = 0.0;
    };
    LatencyStats get_latency_stats(const std::string& symbol) const;

    // Performance by latency band
    struct BandPerformance {
        double avg_pnl_bps = 0.0;
        double avg_slippage_bps = 0.0;
        int trades = 0;
        bool profitable = false;
    };
    std::array<BandPerformance, BAND_COUNT> get_band_performance(const std::string& symbol) const;

    // Infrastructure shock detection
    bool is_under_shock(const std::string& symbol) const;
    double shock_factor(const std::string& symbol) const;

    // JSON export for telemetry
    std::string build_json_snapshot() const;

private:
    LatencyBand classify_latency(double latency_ms) const;
    
    double compute_edge_score(const SymbolAllocationState& state) const;
    double compute_win_factor(const SymbolAllocationState& state) const;
    double compute_exec_factor(const SymbolAllocationState& state) const;
    double compute_latency_factor(const SymbolAllocationState& state) const;
    
    void detect_infrastructure_shock(
        const std::string& symbol,
        SymbolAllocationState& state,
        int64_t timestamp_us
    );

    double total_capital_;
    int rolling_window_;
    double min_weight_;
    double max_weight_;
    double shock_threshold_;
    int shock_duration_us_;

    mutable std::unordered_map<std::string, SymbolAllocationState> symbols_;
};

} // namespace chimera
