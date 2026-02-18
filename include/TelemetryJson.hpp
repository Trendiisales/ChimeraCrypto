#pragma once
#include "TelemetryState.hpp"
#include <sstream>
#include <iomanip>

namespace chimera {

// ═══════════════════════════════════════════════════════════════════
// TELEMETRY JSON BUILDER
// Serializes TelemetryState to JSON for WebSocket broadcast
// ═══════════════════════════════════════════════════════════════════

class TelemetryJson {
public:
    static std::string build(TelemetryState& state) {
        std::lock_guard<std::mutex> lock(state.mtx);
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        
        ss << "{";
        
        // Portfolio
        ss << "\"total_pnl\":" << state.total_pnl << ",";
        ss << "\"total_trades\":" << state.total_trades << ",";
        ss << "\"runtime_seconds\":" << state.runtime_seconds << ",";
        
        // Execution
        ss << "\"execution\":{";
        ss << "\"orders_sent\":" << state.exec.orders_sent << ",";
        ss << "\"fills\":" << state.exec.fills << ",";
        ss << "\"partial_fills\":" << state.exec.partial_fills << ",";
        ss << "\"rejects\":" << state.exec.rejects << ",";
        ss << "\"cancels\":" << state.exec.cancels << ",";
        ss << "\"avg_entry_slippage\":" << state.exec.avg_entry_slippage << ",";
        ss << "\"avg_exit_slippage\":" << state.exec.avg_exit_slippage << ",";
        ss << "\"worst_slippage\":" << state.exec.worst_slippage << ",";
        ss << "\"maker_fill_rate\":" << state.exec.maker_fill_rate << ",";
        ss << "\"taker_fill_rate\":" << state.exec.taker_fill_rate << ",";
        ss << "\"maker_fills\":" << state.exec.maker_fills << ",";
        ss << "\"taker_fills\":" << state.exec.taker_fills;
        ss << "},";
        
        // Risk
        ss << "\"risk\":{";
        ss << "\"daily_drawdown\":" << state.risk.daily_drawdown << ",";
        ss << "\"peak_equity\":" << state.risk.peak_equity << ",";
        ss << "\"current_equity\":" << state.risk.current_equity << ",";
        ss << "\"exposure\":" << state.risk.exposure << ",";
        ss << "\"risk_used_pct\":" << state.risk.risk_used_pct << ",";
        ss << "\"exposure_pct\":" << state.risk.exposure_pct << ",";
        ss << "\"kill_active\":" << (state.risk.kill_active ? "true" : "false") << ",";
        ss << "\"kill_reason\":\"" << state.risk.kill_reason << "\",";
        ss << "\"max_daily_dd\":" << state.risk.max_daily_dd << ",";
        ss << "\"max_exposure\":" << state.risk.max_exposure << ",";
        ss << "\"rolling_100_dd\":" << state.risk.rolling_100_dd << ",";
        ss << "\"capital_at_risk\":" << state.risk.capital_at_risk;
        ss << "},";
        
        // Latency
        ss << "\"latency\":{";
        ss << "\"tick_to_signal_ms\":" << state.latency.tick_to_signal_ms << ",";
        ss << "\"signal_to_order_ms\":" << state.latency.signal_to_order_ms << ",";
        ss << "\"order_to_ack_ms\":" << state.latency.order_to_ack_ms << ",";
        ss << "\"ack_to_fill_ms\":" << state.latency.ack_to_fill_ms << ",";
        ss << "\"roundtrip_ms\":" << state.latency.roundtrip_ms << ",";
        ss << "\"p95_ms\":" << state.latency.p95_ms << ",";
        ss << "\"max_spike_ms\":" << state.latency.max_spike_ms;
        ss << "},";
        
        // Monte Carlo
        ss << "\"mc\":{";
        ss << "\"worst_dd\":" << state.mc.worst_dd << ",";
        ss << "\"avg_return\":" << state.mc.avg_return << ",";
        ss << "\"median_return\":" << state.mc.median_return << ",";
        ss << "\"risk_of_ruin\":" << state.mc.risk_of_ruin << ",";
        ss << "\"p95_worst_dd\":" << state.mc.p95_worst_dd << ",";
        ss << "\"iterations\":" << state.mc.iterations;
        ss << "},";
        
        // Operational
        ss << "\"ops\":{";
        ss << "\"reconnects\":" << state.ops.reconnects << ",";
        ss << "\"missed_ticks\":" << state.ops.missed_ticks << ",";
        ss << "\"cpu\":" << state.ops.cpu_usage << ",";
        ss << "\"memory\":" << state.ops.memory_mb << ",";
        ss << "\"book_stale_ms\":" << state.ops.book_stale_ms << ",";
        ss << "\"uptime_seconds\":" << state.ops.uptime_seconds << ",";
        ss << "\"ws_status\":\"" << state.ops.ws_status << "\",";
        ss << "\"depth_desynced\":" << (state.ops.depth_desynced ? "true" : "false");
        ss << "},";
        
        // Cost Coverage
        ss << "\"cost\":{";
        ss << "\"required_min_r\":" << state.cost.required_min_r << ",";
        ss << "\"current_target_r\":" << state.cost.current_target_r << ",";
        ss << "\"net_expected_r\":" << state.cost.net_expected_r << ",";
        ss << "\"cost_pct_of_target\":" << state.cost.cost_pct_of_target << ",";
        ss << "\"edge_after_costs\":" << state.cost.edge_after_costs << ",";
        ss << "\"avg_cost_per_trade\":" << state.cost.avg_cost_per_trade << ",";
        ss << "\"fees_paid\":" << state.cost.fees_paid << ",";
        ss << "\"slippage_cost\":" << state.cost.slippage_cost;
        ss << "},";
        
        // Engine State
        ss << "\"state\":{";
        ss << "\"current_state\":\"" << state.state.current_state << "\",";
        ss << "\"state_duration_ms\":" << state.state.state_duration_ms << ",";
        ss << "\"state_transitions\":" << state.state.state_transitions << ",";
        ss << "\"burst_confidence\":" << state.state.burst_confidence << ",";
        ss << "\"regime_stability\":" << state.state.regime_stability << ",";
        ss << "\"gating_reason\":\"" << state.state.gating_reason << "\",";
        ss << "\"armed\":" << (state.state.armed ? "true" : "false") << ",";
        ss << "\"warmup_ticks_remaining\":" << state.state.warmup_ticks_remaining;
        ss << "},";
        
        // Symbols
        ss << "\"symbols\":[";
        for (size_t i = 0; i < state.symbols.size(); ++i) {
            const auto& s = state.symbols[i];
            ss << "{";
            ss << "\"symbol\":\"" << s.symbol << "\",";
            ss << "\"price\":" << s.price << ",";
            ss << "\"position\":" << s.position << ",";
            ss << "\"pnl\":" << s.pnl << ",";
            ss << "\"win_rate\":" << s.win_rate << ",";
            ss << "\"expectancy\":" << s.expectancy << ",";
            ss << "\"profit_factor\":" << s.profit_factor << ",";
            ss << "\"trades\":" << s.trades << ",";
            ss << "\"imbalance\":" << s.imbalance << ",";
            ss << "\"spread_bps\":" << s.spread_bps << ",";
            ss << "\"volatility\":" << s.volatility << ",";
            ss << "\"accel\":" << s.accel << ",";
            ss << "\"burst\":" << (s.burst ? "true" : "false") << ",";
            ss << "\"absorption\":" << (s.absorption ? "true" : "false") << ",";
            ss << "\"bid_depth\":" << s.bid_depth << ",";
            ss << "\"ask_depth\":" << s.ask_depth << ",";
            ss << "\"top_bid\":" << s.top_bid << ",";
            ss << "\"top_ask\":" << s.top_ask << ",";
            ss << "\"regime\":\"" << s.regime << "\",";
            ss << "\"confidence\":" << s.confidence;
            ss << "}";
            if (i != state.symbols.size() - 1) ss << ",";
        }
        ss << "]";
        
        ss << "}";
        
        return ss.str();
    }
};

} // namespace chimera
