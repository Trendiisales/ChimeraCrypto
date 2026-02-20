#include "governor/CapitalAllocationGovernor.hpp"
#include <sstream>
#include <iomanip>

namespace chimera {

static double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

CapitalAllocationGovernor::CapitalAllocationGovernor(
    double total_capital,
    int rolling_window,
    double min_weight,
    double max_weight,
    double shock_threshold,
    int shock_duration_us
)
: total_capital_(total_capital),
  rolling_window_(rolling_window),
  min_weight_(min_weight),
  max_weight_(max_weight),
  shock_threshold_(shock_threshold),
  shock_duration_us_(shock_duration_us)
{
}

LatencyBand CapitalAllocationGovernor::classify_latency(double latency_ms) const {
    if (latency_ms < 15.0) return BAND_FAST;
    if (latency_ms < 25.0) return BAND_MEDIUM;
    return BAND_SLOW;
}

void CapitalAllocationGovernor::record_latency(
    const std::string& symbol,
    double latency_ms,
    int64_t timestamp_us
) {
    auto& state = symbols_[symbol];
    state.latency_hist.add(latency_ms);
    
    // Check for infrastructure shock every 2 seconds
    if (timestamp_us - state.last_shock_check_us > 2'000'000) {
        detect_infrastructure_shock(symbol, state, timestamp_us);
        state.last_shock_check_us = timestamp_us;
    }
}

void CapitalAllocationGovernor::record_trade(
    const std::string& symbol,
    double pnl_bps,
    double slippage_bps,
    double latency_ms,
    int64_t timestamp_us
) {
    auto& state = symbols_[symbol];

    // Maintain rolling window
    if ((int)state.samples.size() >= rolling_window_) {
        state.samples.pop_front();
    }

    state.samples.push_back({pnl_bps, slippage_bps, latency_ms, timestamp_us});
    state.total_trades++;

    // Update latency histogram
    state.latency_hist.add(latency_ms);

    // Record performance by latency band
    LatencyBand band = classify_latency(latency_ms);
    state.band_stats[band].record(pnl_bps, slippage_bps);

    // Check for infrastructure shock
    if (timestamp_us - state.last_shock_check_us > 2'000'000) {
        detect_infrastructure_shock(symbol, state, timestamp_us);
        state.last_shock_check_us = timestamp_us;
    }
}

void CapitalAllocationGovernor::detect_infrastructure_shock(
    const std::string& symbol,
    SymbolAllocationState& state,
    int64_t timestamp_us
) {
    // Check if current shock expired
    if (state.shock.active && timestamp_us >= state.shock.end_time_us) {
        state.shock.active = false;
        state.shock.suppress_factor = 1.0;
    }

    // Need at least some samples to detect shock
    if (state.latency_hist.count < 20) {
        state.last_p95_latency = state.latency_hist.p95();
        return;
    }

    double current_p95 = state.latency_hist.p95();
    
    // Detect shock: p95 jumped more than threshold percent
    if (state.last_p95_latency > 0.0) {
        double jump_ratio = (current_p95 - state.last_p95_latency) / state.last_p95_latency;
        
        if (jump_ratio > shock_threshold_) {
            // Infrastructure shock detected
            state.shock.active = true;
            state.shock.end_time_us = timestamp_us + shock_duration_us_;
            state.shock.suppress_factor = 0.6;  // Reduce aggression by 40%
            
            // Could log this event here if needed
            // fprintf(stderr, "[INFRA-REGIME] %s NETWORK_JITTER p95_jump=%.1f%%\n", 
            //         symbol.c_str(), jump_ratio * 100.0);
        }
    }
    
    state.last_p95_latency = current_p95;
}

double CapitalAllocationGovernor::compute_win_factor(const SymbolAllocationState& state) const {
    if (state.samples.empty()) return 0.0;

    double wins = 0.0;
    for (const auto& s : state.samples) {
        if (s.pnl_bps > 0.0) wins += 1.0;
    }

    double win_rate = wins / state.samples.size();
    return clamp(win_rate, 0.5, 1.0);
}

double CapitalAllocationGovernor::compute_exec_factor(const SymbolAllocationState& state) const {
    if (state.samples.empty()) return 0.0;

    double avg_slip = 0.0;
    for (const auto& s : state.samples) {
        avg_slip += std::abs(s.slippage_bps);
    }

    avg_slip /= state.samples.size();

    // Penalize high slippage
    double factor = 1.0 - (avg_slip / 10.0);
    return clamp(factor, 0.5, 1.0);
}

double CapitalAllocationGovernor::compute_latency_factor(const SymbolAllocationState& state) const {
    if (state.samples.empty()) return 0.0;

    double avg_latency = 0.0;
    for (const auto& s : state.samples) {
        avg_latency += s.latency_ms;
    }

    avg_latency /= state.samples.size();

    // Penalize high latency
    double factor = 1.0 - (avg_latency / 50.0);
    return clamp(factor, 0.5, 1.0);
}

double CapitalAllocationGovernor::compute_edge_score(const SymbolAllocationState& state) const {
    // Need minimum sample size
    if (state.samples.size() < 20) return 0.0;

    double avg_bps = 0.0;
    for (const auto& s : state.samples) {
        avg_bps += s.pnl_bps;
    }

    avg_bps /= state.samples.size();

    // Must be profitable
    if (avg_bps <= 0.0) return 0.0;

    // Compute quality multipliers
    double win_factor = compute_win_factor(state);
    double exec_factor = compute_exec_factor(state);
    double latency_factor = compute_latency_factor(state);

    // Apply infrastructure shock suppression
    double shock_factor = state.shock.active ? state.shock.suppress_factor : 1.0;

    return avg_bps * win_factor * exec_factor * latency_factor * shock_factor;
}

void CapitalAllocationGovernor::update() {
    std::unordered_map<std::string, double> raw_scores;
    double total_score = 0.0;

    // Compute raw scores
    for (const auto& kv : symbols_) {
        double score = compute_edge_score(kv.second);
        raw_scores[kv.first] = score;
        total_score += score;
    }

    // Fallback to equal weights if no positive scores
    if (total_score <= 0.0) return;

    // Update smoothed weights
    for (auto& kv : symbols_) {
        const std::string& sym = kv.first;
        auto& state = kv.second;

        double target_weight = raw_scores[sym] / total_score;
        target_weight = clamp(target_weight, min_weight_, max_weight_);

        // Exponential smoothing: 70% old, 30% new
        double new_weight = 0.7 * state.smoothed_weight + 0.3 * target_weight;
        state.smoothed_weight = new_weight;
    }

    // Normalize to sum to 1.0
    double norm = 0.0;
    for (const auto& kv : symbols_) {
        norm += kv.second.smoothed_weight;
    }

    if (norm > 0.0) {
        for (auto& kv : symbols_) {
            kv.second.smoothed_weight /= norm;
        }
    }
}

double CapitalAllocationGovernor::weight(const std::string& symbol) const {
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return 0.0;
    return it->second.smoothed_weight;
}

double CapitalAllocationGovernor::capital_for(const std::string& symbol) const {
    return total_capital_ * weight(symbol);
}

CapitalAllocationGovernor::LatencyStats 
CapitalAllocationGovernor::get_latency_stats(const std::string& symbol) const {
    LatencyStats stats;
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return stats;
    
    const auto& hist = it->second.latency_hist;
    if (hist.count == 0) return stats;
    
    stats.p50 = hist.p50();
    stats.p75 = hist.p75();
    stats.p95 = hist.p95();
    stats.p99 = hist.p99();
    stats.max = hist.max();
    
    return stats;
}

std::array<CapitalAllocationGovernor::BandPerformance, BAND_COUNT> 
CapitalAllocationGovernor::get_band_performance(const std::string& symbol) const {
    std::array<BandPerformance, BAND_COUNT> result;
    
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return result;
    
    const auto& state = it->second;
    for (int i = 0; i < BAND_COUNT; ++i) {
        result[i].avg_pnl_bps = state.band_stats[i].avg_pnl_bps();
        result[i].avg_slippage_bps = state.band_stats[i].avg_slippage_bps;
        result[i].trades = state.band_stats[i].trades;
        result[i].profitable = state.band_stats[i].is_profitable();
    }
    
    return result;
}

bool CapitalAllocationGovernor::is_under_shock(const std::string& symbol) const {
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return false;
    return it->second.shock.active;
}

double CapitalAllocationGovernor::shock_factor(const std::string& symbol) const {
    auto it = symbols_.find(symbol);
    if (it == symbols_.end()) return 1.0;
    return it->second.shock.active ? it->second.shock.suppress_factor : 1.0;
}

std::string CapitalAllocationGovernor::build_json_snapshot() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "{\"symbols\":{";
    
    bool first = true;
    for (const auto& kv : symbols_) {
        if (!first) oss << ",";
        first = false;
        
        const std::string& sym = kv.first;
        const auto& state = kv.second;
        
        auto lat_stats = get_latency_stats(sym);
        auto band_perf = get_band_performance(sym);
        
        oss << "\"" << sym << "\":{"
            << "\"weight\":" << state.smoothed_weight << ","
            << "\"capital\":" << (total_capital_ * state.smoothed_weight) << ","
            << "\"trades\":" << state.total_trades << ","
            << "\"samples\":" << state.samples.size() << ","
            << "\"latency\":{"
                << "\"p50\":" << lat_stats.p50 << ","
                << "\"p75\":" << lat_stats.p75 << ","
                << "\"p95\":" << lat_stats.p95 << ","
                << "\"p99\":" << lat_stats.p99 << ","
                << "\"max\":" << lat_stats.max
            << "},"
            << "\"band_performance\":{"
                << "\"fast\":{"
                    << "\"trades\":" << band_perf[BAND_FAST].trades << ","
                    << "\"avg_pnl_bps\":" << band_perf[BAND_FAST].avg_pnl_bps << ","
                    << "\"avg_slippage_bps\":" << band_perf[BAND_FAST].avg_slippage_bps << ","
                    << "\"profitable\":" << (band_perf[BAND_FAST].profitable ? "true" : "false")
                << "},"
                << "\"medium\":{"
                    << "\"trades\":" << band_perf[BAND_MEDIUM].trades << ","
                    << "\"avg_pnl_bps\":" << band_perf[BAND_MEDIUM].avg_pnl_bps << ","
                    << "\"avg_slippage_bps\":" << band_perf[BAND_MEDIUM].avg_slippage_bps << ","
                    << "\"profitable\":" << (band_perf[BAND_MEDIUM].profitable ? "true" : "false")
                << "},"
                << "\"slow\":{"
                    << "\"trades\":" << band_perf[BAND_SLOW].trades << ","
                    << "\"avg_pnl_bps\":" << band_perf[BAND_SLOW].avg_pnl_bps << ","
                    << "\"avg_slippage_bps\":" << band_perf[BAND_SLOW].avg_slippage_bps << ","
                    << "\"profitable\":" << (band_perf[BAND_SLOW].profitable ? "true" : "false")
                << "}"
            << "},"
            << "\"shock\":{"
                << "\"active\":" << (state.shock.active ? "true" : "false") << ","
                << "\"suppress_factor\":" << state.shock.suppress_factor
            << "}"
            << "}";
    }
    
    oss << "}}";
    return oss.str();
}

} // namespace chimera
