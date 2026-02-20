#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>

namespace chimera {

enum class LayerType {
    IMPULSE = 0,
    EXPAND  = 1,
    MICRO   = 2,
    LEADLAG = 3
};

struct Signal {
    std::string symbol;
    LayerType layer;
    double expected_bps;
    double confidence;
};

struct GovernorConfig {
    double base_min_bps        = 9.0;
    double low_vol_bps         = 12.0;
    double high_vol_bps        = 7.0;
    double latency_limit_ms    = 25.0;
    int    loss_streak_limit   = 2;
    int    layer_cooldown_sec  = 30;
    int    symbol_rank_interval_sec = 60;
};

class StatefulGovernor {
public:
    StatefulGovernor(const GovernorConfig& cfg);

    void update_volatility(double vol_score);
    void update_latency(double p95_latency_ms);

    void record_trade_result(const std::string& symbol,
                             LayerType layer,
                             double pnl);

    bool approve(const Signal& signal);

private:
    GovernorConfig config_;

    double current_vol_score_;
    double current_latency_;

    std::unordered_map<std::string,double> symbol_rolling_pnl_;
    std::unordered_map<LayerType,int> layer_loss_streak_;
    std::unordered_map<LayerType,std::chrono::steady_clock::time_point> layer_cooldown_until_;

    std::string top_ranked_symbol_;
    std::chrono::steady_clock::time_point last_rank_update_;

    double dynamic_min_bps() const;
    bool layer_in_cooldown(LayerType layer) const;
    void update_symbol_ranking();
    void log_reject(const Signal& signal, const std::string& reason) const;
};

}
