#include "core/StatefulGovernor.hpp"

namespace chimera {

StatefulGovernor::StatefulGovernor(const GovernorConfig& cfg)
: config_(cfg),
  current_vol_score_(1.0),
  current_latency_(0.0),
  last_rank_update_(std::chrono::steady_clock::now())
{
}

void StatefulGovernor::update_volatility(double vol_score)
{
    current_vol_score_ = vol_score;
}

void StatefulGovernor::update_latency(double p95_latency_ms)
{
    current_latency_ = p95_latency_ms;
}

void StatefulGovernor::record_trade_result(const std::string& symbol,
                                           LayerType,
                                           double pnl_bps)
{
    double alpha = 0.2;
    double& ema = symbol_edge_ema_[symbol];
    ema = alpha * pnl_bps + (1.0 - alpha) * ema;
}

bool StatefulGovernor::layer_in_cooldown(LayerType layer) const
{
    auto it = layer_cooldown_until_.find(layer);
    if (it == layer_cooldown_until_.end())
        return false;

    return std::chrono::steady_clock::now() < it->second;
}

double StatefulGovernor::dynamic_min_bps() const
{
    if (current_vol_score_ < 1.0)
        return config_.low_vol_bps;

    if (current_vol_score_ > 1.3)
        return config_.high_vol_bps;

    return config_.base_min_bps;
}

double StatefulGovernor::latency_penalty(double latency) const
{
    if (latency <= config_.latency_soft_limit)
        return 0.0;

    if (latency >= config_.latency_hard_limit)
        return 1000.0;

    double x = (latency - config_.latency_soft_limit) /
               (config_.latency_hard_limit - config_.latency_soft_limit);

    return x * 8.0;
}

double StatefulGovernor::dynamic_expected_bps(const Signal& s) const
{
    double strength_boost =
        config_.strength_alpha * std::max(0.0, s.rel_strength - 1.5);

    double adjusted =
        s.expected_bps + strength_boost;

    adjusted -= latency_penalty(s.latency_ms);

    return adjusted;
}

void StatefulGovernor::log_reject(const Signal& signal,
                                  const std::string& reason) const
{
    const char* layer_name =
        (signal.layer == LayerType::IMPULSE) ? "IMPULSE" :
        (signal.layer == LayerType::EXPAND) ? "EXPAND" :
        (signal.layer == LayerType::MICRO) ? "MICRO" : "LEADLAG";

    std::cout << "[GOV-REJECT] "
              << signal.symbol
              << " layer=" << layer_name
              << " reason=" << reason
              << std::endl;
}

bool StatefulGovernor::approve(Signal& signal)
{
    // MICRO is allowed again, but only when volatility is not dead.
    // This prevents imbalance-style entries in flat/noise tape.
    if (signal.layer == LayerType::MICRO && current_vol_score_ < 0.85) {
        log_reject(signal, "MICRO_LOW_VOL");
        return false;
    }

    if (current_latency_ >= config_.latency_hard_limit) {
        log_reject(signal, "LATENCY_HARD");
        return false;
    }

    if (layer_in_cooldown(signal.layer)) {
        log_reject(signal, "LAYER_COOLDOWN");
        return false;
    }

    double adjusted_bps = dynamic_expected_bps(signal);
    double min_bps = dynamic_min_bps();

    if (adjusted_bps < min_bps) {
        log_reject(signal, "LOW_EXPECTED_BPS");
        return false;
    }

    return true;
}

}
