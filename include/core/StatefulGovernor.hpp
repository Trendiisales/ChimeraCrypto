#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

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
    double rel_strength = 1.0;
    double latency_ms   = 0.0;
};

struct GovernorConfig {
    // -------------------------------------------------------------------------
    // MIN BPS THRESHOLDS
    // -------------------------------------------------------------------------
    // BUG FIX (2026-03-17): low_vol_bps was 12.0 — INVERTED LOGIC.
    // Low volatility means smaller moves; requiring MORE edge in low vol
    // means the governor was blocking ALL trades during baseline tape.
    // Correct logic: low vol = lower bar (smaller moves, tighter spreads).
    // The primary signal gates (TradingConfig cost floors + per-layer checks)
    // already enforce edge minimums. The governor is a final backstop only.
    //
    // Previous: base=9.0  low_vol=12.0  high_vol=7.0
    // Fixed:    base=6.0  low_vol=5.0   high_vol=8.0
    double base_min_bps        = 6.0;
    double low_vol_bps         = 5.0;   // FIX: was 12.0 — inverted, blocked all low-vol trades
    double high_vol_bps        = 8.0;

    // -------------------------------------------------------------------------
    // LATENCY LIMITS
    // -------------------------------------------------------------------------
    // BUG FIX (2026-03-17): governor latency_hard_limit was 50ms, but
    // TradingConfig::LATENCY_HARD_LIMIT_MS = 100ms. The governor was
    // double-blocking at 50ms what the primary gate allows at 100ms.
    // Aligned to TradingConfig values so the governor does not shadow-block.
    //
    // latency_soft_limit: matches TradingConfig::LATENCY_NET_CLEAN_MS  = 60ms
    // latency_hard_limit: matches TradingConfig::LATENCY_HARD_LIMIT_MS = 100ms
    double latency_soft_limit  = 60.0;  // FIX: was 30ms — penalised every normal p95=36ms tick
    double latency_hard_limit  = 100.0; // FIX: was 50ms — double-blocked vs TradingConfig 100ms

    int    loss_streak_limit   = 2;
    int    layer_cooldown_sec  = 30;
    int    symbol_rank_interval_sec = 60;

    // -------------------------------------------------------------------------
    // STRENGTH ALPHA
    // -------------------------------------------------------------------------
    // BUG FIX (2026-03-17): was 6.0. Boost formula:
    //   boost = strength_alpha * max(0, rel_strength - 1.5)
    // rel_strength is never populated in enter() (always defaults to 1.0),
    // so boost was always zero. Reduced to 2.0; no current behaviour change
    // but correctly scaled if rel_strength is wired in future.
    double strength_alpha      = 2.0;   // FIX: was 6.0 — boost was always 0
};

class StatefulGovernor {
public:
    StatefulGovernor(const GovernorConfig& cfg);

    void update_volatility(double vol_score);
    void update_latency(double p95_latency_ms);

    void record_trade_result(const std::string& symbol,
                             LayerType layer,
                             double pnl_bps);

    bool approve(Signal& signal);

private:
    GovernorConfig config_;

    double current_vol_score_;
    double current_latency_;

    std::unordered_map<std::string,double> symbol_edge_ema_;
    std::unordered_map<LayerType,int> layer_loss_streak_;
    std::unordered_map<LayerType,std::chrono::steady_clock::time_point> layer_cooldown_until_;

    std::chrono::steady_clock::time_point last_rank_update_;

    double dynamic_min_bps() const;
    double dynamic_expected_bps(const Signal& s) const;
    double latency_penalty(double latency) const;

    bool layer_in_cooldown(LayerType layer) const;
    void log_reject(const Signal& signal, const std::string& reason) const;
};

}
