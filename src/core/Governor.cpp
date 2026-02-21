#include "core/Governor.hpp"

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
                                           LayerType layer,
                                           double pnl)
{
    symbol_rolling_pnl_[symbol] += pnl;

    if (pnl < 0.0) {
        layer_loss_streak_[layer]++;
        if (layer_loss_streak_[layer] >= config_.loss_streak_limit) {
            layer_cooldown_until_[layer] =
                std::chrono::steady_clock::now() +
                std::chrono::seconds(config_.layer_cooldown_sec);
        }
    } else {
        layer_loss_streak_[layer] = 0;
    }
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

void StatefulGovernor::update_symbol_ranking()
{
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(
        now - last_rank_update_).count()
        < config_.symbol_rank_interval_sec)
        return;

    last_rank_update_ = now;

    std::vector<std::pair<std::string,double>> ranking(
        symbol_rolling_pnl_.begin(),
        symbol_rolling_pnl_.end());

    std::sort(ranking.begin(), ranking.end(),
              [](auto& a, auto& b) {
                  return a.second > b.second;
              });

    if (!ranking.empty()) {
        top_ranked_symbol_ = ranking.front().first;
        std::cout << "[GOV] New Top Symbol: "
                  << top_ranked_symbol_
                  << " pnl=" << ranking.front().second
                  << std::endl;
    }
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

bool StatefulGovernor::approve(const Signal& signal)
{
    // CRITICAL: MICRO layer is PARKED - reject all MICRO signals
    if (signal.layer == LayerType::MICRO) {
        log_reject(signal, "MICRO_PARKED");
        return false;
    }

    update_symbol_ranking();

    if (current_latency_ > config_.latency_limit_ms) {
        log_reject(signal, "LATENCY");
        return false;
    }

    if (layer_in_cooldown(signal.layer)) {
        log_reject(signal, "LAYER_COOLDOWN");
        return false;
    }

    double min_bps = dynamic_min_bps();

    if (signal.expected_bps < min_bps) {
        log_reject(signal, "LOW_EXPECTED_BPS");
        return false;
    }

    if (!top_ranked_symbol_.empty() &&
        signal.symbol != top_ranked_symbol_) {
        log_reject(signal, "SYMBOL_NOT_TOP");
        return false;
    }

    return true;
}

}
