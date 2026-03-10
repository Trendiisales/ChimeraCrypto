#pragma once
#include <cmath>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <string>

namespace chimera {

/**
 * CapitalControlLayer - Risk-Aware Position Sizing with Per-Engine Win-Rate Boost
 *
 * Win-rate boost is tracked PER ENGINE independently.
 * Engines are pre-seeded with historical win rates so boost is active from trade 1.
 *
 * Pre-seeded from trade log (2026-03-10):
 *   LEADLAG     : 86.2% WR, 29 trades  -> starts at ~3.5x
 *   LL-ETH-SOL  : 100%  WR,  6 trades  -> starts at 4.0x (capped, min trades override)
 *   IMPULSE     : 54.5% WR, 99 trades  -> starts at 1.0x (below threshold)
 *   EXPAND      : 45.0% WR,100 trades  -> starts at 1.0x (below threshold)
 *   FUND/LIQ/NGAS: no data             -> starts at 1.0x
 *
 * Boost range: 1.0x (<=70% WR) -> 4.0x (>=90% WR), linear interpolation.
 * Decay on loss: 0.90x per loss. Grows on win: smooth EMA toward target.
 */
class CapitalControlLayer {
public:
    struct MarketEnv {
        double short_range;
        double long_range;
        double spread_bps;
        double book_imbalance;
        double queue_density;
        double funding_rate;
        double latency_ms;
        bool   net_clean;
    };

    static constexpr double WIN_BOOST_THRESHOLD  = 0.70;
    static constexpr double WIN_BOOST_PEAK       = 0.90;
    static constexpr double WIN_BOOST_MAX        = 4.0;
    static constexpr int    WIN_BOOST_MIN_TRADES = 6;    // lowered: LL-ETH-SOL only has 6
    static constexpr double WIN_BOOST_DECAY      = 0.90; // per loss

private:
    // Per-engine state
    struct EngineBoost {
        int    wins   = 0;
        int    total  = 0;
        double boost  = 1.0;
    };

    // Engine name -> boost state
    // Keys match layer labels used in BalancedEngine: LEADLAG, LL-ETH-SOL, IMPULSE, EXPAND, FUND, LIQ, NGAS
    std::unordered_map<std::string, EngineBoost> engine_boosts_;

    double heat_level_   = 0.0;
    double exposure_cap_ = 1.0;
    double base_capital_ = 1.0;
    double smoothed_vol_ = 1.0;
    double funding_bias_ = 1.0;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    static double compute_boost_target(int wins, int total) {
        if (total < WIN_BOOST_MIN_TRADES) return 1.0;
        double wr = (double)wins / (double)total;
        if (wr < WIN_BOOST_THRESHOLD) return 1.0;
        double t = clamp((wr - WIN_BOOST_THRESHOLD) /
                         (WIN_BOOST_PEAK - WIN_BOOST_THRESHOLD), 0.0, 1.0);
        return 1.0 + t * (WIN_BOOST_MAX - 1.0);
    }

    void seed_engine(const std::string& name, int wins, int total) {
        EngineBoost& e = engine_boosts_[name];
        e.wins  = wins;
        e.total = total;
        e.boost = clamp(compute_boost_target(wins, total), 1.0, WIN_BOOST_MAX);
    }

public:
    CapitalControlLayer() {
        // Pre-seed from historical trade log data (2026-03-10)
        // LEADLAG: 25/29 wins = 86.2% -> linear(86.2%, 70%, 90%) = 0.81 -> boost = 1 + 0.81*3 = 3.43x
        seed_engine("LEADLAG",    25, 29);
        // LL-ETH-SOL: 6/6 = 100% -> capped at 4.0x
        seed_engine("LL-ETH-SOL", 6,  6);
        // IMPULSE: 54/99 = 54.5% -> below threshold -> 1.0x
        seed_engine("IMPULSE",    54, 99);
        // EXPAND: 45/100 = 45% -> below threshold -> 1.0x
        seed_engine("EXPAND",     45, 100);
        // Others: no historical data -> 1.0x
        seed_engine("LIQ",   0, 0);
        seed_engine("FUND",  0, 0);
        seed_engine("NGAS",  0, 0);
    }

    void set_base_capital(double v) { base_capital_ = v; }

    // Call after every closed trade with the engine label string
    void record_trade_result(const std::string& engine, bool win) {
        EngineBoost& e = engine_boosts_[engine];
        e.total++;
        if (win) {
            e.wins++;
            double target = compute_boost_target(e.wins, e.total);
            // Smooth EMA toward target (no sudden jumps upward)
            e.boost = e.boost * 0.92 + target * 0.08;
        } else {
            // Decay on loss
            e.boost *= WIN_BOOST_DECAY;
        }
        e.boost = clamp(e.boost, 1.0, WIN_BOOST_MAX);
    }

    // Legacy single-arg version (global, uses "UNKNOWN")
    void record_trade_result(bool win) {
        record_trade_result("UNKNOWN", win);
    }

    double win_boost_for(const std::string& engine) const {
        auto it = engine_boosts_.find(engine);
        return (it != engine_boosts_.end()) ? it->second.boost : 1.0;
    }

    double win_boost_multiplier() const {
        return win_boost_for("UNKNOWN");
    }

    void update_volatility(const MarketEnv& env) {
        double ratio = env.short_range / std::max(env.long_range, 1e-6);
        smoothed_vol_ = smoothed_vol_ * 0.95 + ratio * 0.05;
    }

    double volatility_normalizer() const {
        return clamp(1.0 / std::max(smoothed_vol_, 0.5), 0.5, 2.0);
    }

    void update_heat(double unrealized_bp) {
        double heat_increase = std::abs(unrealized_bp) * 0.02;
        heat_level_ = clamp(heat_level_ * 0.97 + heat_increase, 0.0, 1.0);
    }

    double heat_multiplier() const {
        return 1.0 - heat_level_ * 0.6;
    }

    double queue_bias(const MarketEnv& env) const {
        double pressure = env.book_imbalance * env.queue_density;
        return clamp(1.0 + pressure * 0.5, 0.5, 1.5);
    }

    void update_funding(const MarketEnv& env) {
        funding_bias_ = (std::abs(env.funding_rate) > 0.0005) ? 0.7 : 1.0;
    }

    double funding_multiplier() const { return funding_bias_; }

    void update_exposure_cap(double drawdown_bp) {
        if      (drawdown_bp > 20.0) exposure_cap_ = 0.6;
        else if (drawdown_bp > 10.0) exposure_cap_ = 0.8;
        else                         exposure_cap_ = 1.0;
    }

    // Engine-aware final size computation
    double compute_final_size(double allocator_weight,
                              const MarketEnv& env,
                              double unrealized_bp,
                              double drawdown_bp,
                              const std::string& engine = "UNKNOWN")
    {
        update_volatility(env);
        update_heat(unrealized_bp);
        update_funding(env);
        update_exposure_cap(drawdown_bp);

        double boost = win_boost_for(engine);

        double size =
            base_capital_
          * allocator_weight
          * volatility_normalizer()
          * heat_multiplier()
          * queue_bias(env)
          * funding_multiplier()
          * exposure_cap_
          * boost;

        return clamp(size, 0.0, base_capital_ * WIN_BOOST_MAX);
    }
};

} // namespace chimera
