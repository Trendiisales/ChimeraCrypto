#pragma once
#include <cmath>
#include <chrono>
#include <algorithm>

namespace chimera {

/**
 * AdaptiveAllocator - Dynamic Capital Distribution Between Impulse and Maker
 * 
 * 5ms stable control loop that continuously adjusts capital allocation
 * based on realized performance and market conditions.
 * 
 * Features:
 * - Vol acceleration responsiveness
 * - Drawdown dampening per engine
 * - Softmax normalization
 * - Weight inertia (no oscillation)
 * - Movement limiter (no capital shocks)
 */
class AdaptiveAllocator {
public:
    struct EngineMetrics {
        double pnl_ema = 0.0;
        double mfe_ema = 0.0;
        double mae_ema = 1.0;
        double drawdown_bp = 0.0;
    };

    struct Environment {
        double short_range = 0.0;
        double long_range = 1.0;
        double spread_bps = 1.0;
        double latency_ms = 0.0;
        bool   net_clean = true;
    };

private:
    EngineMetrics impulse_;
    EngineMetrics maker_;

    double weight_impulse_ = 0.5;
    double weight_maker_   = 0.5;

    double vol_ratio_prev_ = 1.0;
    double allocator_interval_ms_ = 5.0;
    double alpha_ = 0.05;
    double softmax_k_ = 4.0;
    double max_delta_ = 0.03;

    std::chrono::steady_clock::time_point last_update_;

    static double clamp(double v, double lo, double hi) {
        return std::max(lo, std::min(v, hi));
    }

    static double ema_update(double prev, double value, double a) {
        return prev * (1.0 - a) + value * a;
    }

public:
    AdaptiveAllocator() {
        last_update_ = std::chrono::steady_clock::now();
    }

    void update_impulse_metrics(double pnl_bp, double mfe, double mae, double dd) {
        impulse_.pnl_ema = ema_update(impulse_.pnl_ema, pnl_bp, 0.05);
        impulse_.mfe_ema = ema_update(impulse_.mfe_ema, mfe, 0.05);
        impulse_.mae_ema = ema_update(impulse_.mae_ema, mae, 0.05);
        impulse_.drawdown_bp = dd;
    }

    void update_maker_metrics(double pnl_bp, double mfe, double mae, double dd) {
        maker_.pnl_ema = ema_update(maker_.pnl_ema, pnl_bp, 0.05);
        maker_.mfe_ema = ema_update(maker_.mfe_ema, mfe, 0.05);
        maker_.mae_ema = ema_update(maker_.mae_ema, mae, 0.05);
        maker_.drawdown_bp = dd;
    }

    void tick(const Environment& env) {
        auto now = std::chrono::steady_clock::now();
        double elapsed =
            std::chrono::duration<double, std::milli>(now - last_update_).count();

        if (elapsed < allocator_interval_ms_)
            return;

        last_update_ = now;

        double vol_ratio = env.short_range / std::max(env.long_range, 1e-6);
        double vol_accel = vol_ratio - vol_ratio_prev_;
        vol_ratio_prev_ = vol_ratio;

        double dd_penalty_impulse = impulse_.drawdown_bp > 10.0 ? 0.6 : 1.0;
        double dd_penalty_maker   = maker_.drawdown_bp   > 10.0 ? 0.6 : 1.0;

        double impulse_alignment =
            clamp((vol_ratio - 1.2) + vol_accel * 0.5, 0.0, 3.0);

        double compression =
            clamp(1.3 - vol_ratio, 0.0, 1.3);

        double latency_factor =
            env.net_clean ? 1.0 : 0.7;

        double spread_quality =
            clamp(1.0 / std::max(env.spread_bps, 0.1), 0.0, 5.0);

        double impulse_eff =
            clamp(impulse_.mfe_ema / std::max(impulse_.mae_ema, 1e-6), 0.5, 3.0);

        double maker_eff =
            clamp(maker_.mfe_ema / std::max(maker_.mae_ema, 1e-6), 0.5, 3.0);

        double impulse_score =
            impulse_.pnl_ema
          * impulse_eff
          * impulse_alignment
          * latency_factor
          * dd_penalty_impulse;

        double maker_score =
            maker_.pnl_ema
          * maker_eff
          * compression
          * spread_quality
          * latency_factor
          * dd_penalty_maker;

        double w_imp_raw = std::exp(softmax_k_ * impulse_score);
        double w_mkr_raw = std::exp(softmax_k_ * maker_score);

        double weight_new =
            w_imp_raw / std::max(w_imp_raw + w_mkr_raw, 1e-6);

        weight_new = clamp(weight_new, 0.1, 0.9);

        double delta = weight_new - weight_impulse_;

        if (delta >  max_delta_) delta =  max_delta_;
        if (delta < -max_delta_) delta = -max_delta_;

        weight_impulse_ =
            weight_impulse_ * (1.0 - alpha_)
          + (weight_impulse_ + delta) * alpha_;

        weight_impulse_ = clamp(weight_impulse_, 0.1, 0.9);
        weight_maker_ = 1.0 - weight_impulse_;
    }

    double impulse_weight() const { return weight_impulse_; }
    double maker_weight()   const { return weight_maker_; }
};

} // namespace chimera
