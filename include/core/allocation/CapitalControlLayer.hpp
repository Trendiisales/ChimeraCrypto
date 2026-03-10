#pragma once
#include <cmath>
#include <algorithm>
#include <chrono>

namespace chimera {

/**
 * CapitalControlLayer - Risk-Aware Position Sizing
 *
 * Applies multiple dampening layers on top of allocator weights:
 * - Volatility normalization (reduce size in hyper expansion)
 * - Heat governor (dampen during unrealized swings)
 * - Queue bias (favor book imbalance direction)
 * - Funding bias (reduce exposure in extreme funding)
 * - Exposure cap (drawdown-based automatic reduction)
 * - Win-rate boost (scale up to 4x during hot streaks, per engine)
 *
 * Win-rate boost logic:
 *   winrate >= WIN_BOOST_THRESHOLD (70%) AND min trades >= WIN_BOOST_MIN_TRADES (10)
 *   -> size multiplied up to WIN_BOOST_MAX (4.0x), scaling linearly from 1x at 70% to 4x at 90%+
 *   Any loss resets consecutive win streak; boost decays on losses.
 */
class CapitalControlLayer {
public:
    struct MarketEnv {
        double short_range;
        double long_range;
        double spread_bps;
        double book_imbalance;     // -1 to +1
        double queue_density;      // normalized 0-1
        double funding_rate;       // per 8h basis
        double latency_ms;
        bool net_clean;
    };

    // Win-rate boost parameters
    static constexpr double WIN_BOOST_THRESHOLD  = 0.70;  // min win rate to start boosting
    static constexpr double WIN_BOOST_PEAK       = 0.90;  // win rate at which max boost is applied
    static constexpr double WIN_BOOST_MAX        = 4.0;   // maximum size multiplier
    static constexpr int    WIN_BOOST_MIN_TRADES = 10;    // minimum trades before boost activates
    static constexpr double WIN_BOOST_DECAY      = 0.85;  // boost decay factor on a loss

private:
    double heat_level_    = 0.0;
    double exposure_cap_  = 1.0;
    double base_capital_  = 1.0;
    double smoothed_vol_  = 1.0;
    double last_vol_ratio_= 1.0;
    double funding_bias_  = 1.0;

    // Win-rate tracking (rolling window per instance)
    int    total_trades_  = 0;
    int    total_wins_    = 0;
    double win_boost_     = 1.0;   // current active boost multiplier

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

public:
    void set_base_capital(double v) {
        base_capital_ = v;
    }

    // Call after every closed trade
    void record_trade_result(bool win) {
        total_trades_++;
        if (win) {
            total_wins_++;
            // Recompute boost
            if (total_trades_ >= WIN_BOOST_MIN_TRADES) {
                double wr = (double)total_wins_ / (double)total_trades_;
                if (wr >= WIN_BOOST_THRESHOLD) {
                    // Linear scale from 1x at threshold to WIN_BOOST_MAX at peak
                    double t = clamp((wr - WIN_BOOST_THRESHOLD) /
                                     (WIN_BOOST_PEAK - WIN_BOOST_THRESHOLD), 0.0, 1.0);
                    double target = 1.0 + t * (WIN_BOOST_MAX - 1.0);
                    // Smooth toward target (avoid sudden jumps)
                    win_boost_ = win_boost_ * 0.9 + target * 0.1;
                } else {
                    win_boost_ = clamp(win_boost_ * 0.95, 1.0, WIN_BOOST_MAX);
                }
            }
        } else {
            // Decay boost on loss
            win_boost_ = clamp(win_boost_ * WIN_BOOST_DECAY, 1.0, WIN_BOOST_MAX);
        }
        win_boost_ = clamp(win_boost_, 1.0, WIN_BOOST_MAX);
    }

    double win_boost_multiplier() const {
        return win_boost_;
    }

    double current_win_rate() const {
        return total_trades_ > 0 ? (double)total_wins_ / (double)total_trades_ : 0.0;
    }

    int total_trades() const { return total_trades_; }

    void update_volatility(const MarketEnv& env) {
        double ratio = env.short_range / std::max(env.long_range, 1e-6);
        smoothed_vol_ = smoothed_vol_ * 0.95 + ratio * 0.05;
        last_vol_ratio_ = ratio;
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
        if (std::abs(env.funding_rate) > 0.0005)
            funding_bias_ = 0.7;
        else
            funding_bias_ = 1.0;
    }

    double funding_multiplier() const {
        return funding_bias_;
    }

    void update_exposure_cap(double drawdown_bp) {
        if (drawdown_bp > 20.0)
            exposure_cap_ = 0.6;
        else if (drawdown_bp > 10.0)
            exposure_cap_ = 0.8;
        else
            exposure_cap_ = 1.0;
    }

    double compute_final_size(double allocator_weight,
                              const MarketEnv& env,
                              double unrealized_bp,
                              double drawdown_bp)
    {
        update_volatility(env);
        update_heat(unrealized_bp);
        update_funding(env);
        update_exposure_cap(drawdown_bp);

        double size =
            base_capital_
          * allocator_weight
          * volatility_normalizer()
          * heat_multiplier()
          * queue_bias(env)
          * funding_multiplier()
          * exposure_cap_
          * win_boost_;   // <-- win-rate boost applied last

        // Cap is 4x base capital (WIN_BOOST_MAX)
        return clamp(size, 0.0, base_capital_ * WIN_BOOST_MAX);
    }
};

} // namespace chimera
