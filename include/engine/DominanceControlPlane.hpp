#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Chimera {

struct AlphaSignal {
    std::string name;
    double value;
    double weight;
    double decay_rate;
};

struct MarketState {
    double volatility_bps;
    double spread_bps;
    double slippage_bps;
    double funding_bps;
    bool trending;
    bool choppy;
};

struct PortfolioState {
    double equity;
    double exposure_usd;
    std::vector<double> symbol_exposures;
};

struct ExecutionState {
    double latency_ms;
    double fill_probability;
    double reject_rate;
};

class AlphaFusionEngine {
public:
    double compute_conviction(std::vector<AlphaSignal>& signals, double dt_seconds) {
        double total_weight = 0.0;
        double weighted_sum = 0.0;
        for (auto& s : signals) {
            s.value *= std::exp(-s.decay_rate * dt_seconds);
            weighted_sum += s.value * s.weight;
            total_weight += s.weight;
        }
        if (total_weight == 0.0) return 0.0;
        return weighted_sum / total_weight;
    }

    bool cooldown_required(double conviction, double previous_conviction) {
        if (std::abs(conviction) < 0.5 && std::abs(previous_conviction) > 1.2)
            return true;
        return false;
    }
};

class EdgeEngine {
public:
    double compute_cost_floor(const MarketState& m) {
        return (m.spread_bps + m.slippage_bps + 6.5) * 1.5;
    }

    double compute_edge(double conviction, const MarketState& m) {
        return conviction - compute_cost_floor(m);
    }

    bool regime_allows(const MarketState& m) {
        if (m.spread_bps > 10.0) return false;
        if (m.volatility_bps > 80.0) return false;
        return true;
    }
};

class ExecutionDominance {
public:
    bool should_take(double edge, const ExecutionState& e) {
        if (edge > 15.0) return true;
        if (e.fill_probability < 0.3) return true;
        if (e.reject_rate > 0.15) return true;
        return false;
    }

    double aggression_multiplier(double edge, double latency_ms) {
        double edge_factor = std::clamp(edge / 20.0, 0.5, 2.5);
        double latency_penalty = std::clamp(10.0 / (latency_ms + 1.0), 0.5, 1.5);
        return edge_factor * latency_penalty;
    }
};

class CapitalDominance {
public:
    double compute_position_size(double base_usd, double edge, double volatility_bps) {
        double edge_factor = std::clamp(edge / 20.0, 0.3, 3.0);
        double vol_adjust = std::clamp(30.0 / volatility_bps, 0.5, 1.5);
        return base_usd * edge_factor * vol_adjust;
    }

    bool correlation_block(int symbol_index,
                           const std::vector<std::vector<double>>& corr,
                           const std::vector<double>& exposures,
                           double threshold = 0.8) {
        for (size_t i = 0; i < exposures.size(); ++i) {
            if (i == static_cast<size_t>(symbol_index)) continue;
            if (std::abs(corr[symbol_index][i]) > threshold && exposures[i] > 0.0)
                return true;
        }
        return false;
    }
};

class CascadeEngine {
public:
    bool allow_entry(double price_burst_bps, double depth_imbalance, const MarketState& m) {
        if (!m.trending) return false;
        if (price_burst_bps > 25.0 && depth_imbalance > 0.6 && m.volatility_bps > 40.0)
            return true;
        return false;
    }
};

class DominanceControlPlane {
public:
    double process(std::vector<AlphaSignal>& signals,
                   const MarketState& market,
                   const PortfolioState& portfolio,
                   const ExecutionState& exec,
                   const std::vector<std::vector<double>>& corr_matrix,
                   int symbol_index,
                   double base_usd,
                   double dt_seconds) {
        double conviction = alpha_.compute_conviction(signals, dt_seconds);
        if (!edge_.regime_allows(market)) return 0.0;
        double edge = edge_.compute_edge(conviction, market);
        if (edge < 0.0) return 0.0;
        if (capital_.correlation_block(symbol_index, corr_matrix, portfolio.symbol_exposures))
            return 0.0;
        double aggression = execution_.aggression_multiplier(edge, exec.latency_ms);
        double allocation = capital_.compute_position_size(base_usd, edge, market.volatility_bps);
        return allocation * aggression;
    }

    bool should_take_liquidity(double edge, const ExecutionState& exec) {
        return execution_.should_take(edge, exec);
    }

private:
    AlphaFusionEngine alpha_;
    EdgeEngine edge_;
    ExecutionDominance execution_;
    CapitalDominance capital_;
    CascadeEngine cascade_;
};

} // namespace Chimera
