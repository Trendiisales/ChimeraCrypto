#include "engine/LiquidityVacuumEngine.hpp"
#include <cmath>

namespace chimera {

LiquidityVacuumEngine::LiquidityVacuumEngine(const LVConfig& cfg) : cfg_(cfg) { reset(); }

void LiquidityVacuumEngine::reset()
{
    idx_ = 0;
    count_ = 0;
    bid_sum_ = 0.0;
    ask_sum_ = 0.0;
    last_mid_ = 0.0;
    cooldown_until_ = 0;
    active_ = false;
    signal_enter_ = false;
    signal_exit_ = false;
}

void LiquidityVacuumEngine::record_depth(double bid, double ask)
{
    bid_sum_ -= bid_buffer_[idx_];
    ask_sum_ -= ask_buffer_[idx_];
    bid_buffer_[idx_] = bid;
    ask_buffer_[idx_] = ask;
    bid_sum_ += bid;
    ask_sum_ += ask;
    idx_ = (idx_ + 1) % DEPTH_CAP;
    if (count_ < DEPTH_CAP) count_++;
}

void LiquidityVacuumEngine::on_book(double bid_price, double bid_size, double ask_price, double ask_size, int64_t now_ms, double latency_p95_ms)
{
    signal_enter_ = false;
    signal_exit_ = false;

    double mid = (bid_price + ask_price) * 0.5;
    double spread_bps = ((ask_price - bid_price) / mid) * 10000.0;

    if (spread_bps > cfg_.max_spread_bps) return;

    record_depth(bid_size, ask_size);

    if (!active_) {
        if (now_ms >= cooldown_until_)
            evaluate_entry(mid, now_ms, latency_p95_ms);
    } else {
        manage_position(mid, now_ms);
    }

    last_mid_ = mid;
}

void LiquidityVacuumEngine::evaluate_entry(double mid, int64_t now_ms, double latency_p95_ms)
{
    if (count_ < DEPTH_CAP) return;

    double bid_avg = (count_ == 0) ? 0.0 : bid_sum_ / count_;
    double ask_avg = (count_ == 0) ? 0.0 : ask_sum_ / count_;

    if (bid_avg <= 0.0 || ask_avg <= 0.0) return;

    double imbalance = bid_avg / ask_avg;
    double impulse_bps = (last_mid_ > 0.0) ? std::fabs((mid - last_mid_) / last_mid_) * 10000.0 : 0.0;
    double dynamic_edge = cfg_.min_required_edge_bps + latency_p95_ms * 0.9;

    if (imbalance > cfg_.imbalance_trigger && impulse_bps > dynamic_edge) {
        active_ = true;
        long_side_ = true;
        entry_price_ = mid;
        entry_time_ = now_ms;
        partial_taken_ = false;
        signal_enter_ = true;
        return;
    }

    if ((1.0 / imbalance) > cfg_.imbalance_trigger && impulse_bps > dynamic_edge) {
        active_ = true;
        long_side_ = false;
        entry_price_ = mid;
        entry_time_ = now_ms;
        partial_taken_ = false;
        signal_enter_ = true;
    }
}

void LiquidityVacuumEngine::manage_position(double mid, int64_t now_ms)
{
    double move_bps = ((mid - entry_price_) / entry_price_) * 10000.0;
    if (!long_side_) move_bps = -move_bps;

    if (!partial_taken_ && move_bps >= cfg_.scale_out_bps) {
        partial_taken_ = true;
    }

    if (move_bps >= cfg_.target_bps || move_bps <= -cfg_.stop_bps || now_ms - entry_time_ > 3500) {
        exit_price_ = mid;
        active_ = false;
        signal_exit_ = true;
        cooldown_until_ = now_ms + cfg_.cooldown_ms;
    }
}

bool LiquidityVacuumEngine::enter_signal() const { return signal_enter_; }
bool LiquidityVacuumEngine::exit_signal() const { return signal_exit_; }
bool LiquidityVacuumEngine::is_long() const { return long_side_; }
double LiquidityVacuumEngine::entry_price() const { return entry_price_; }
double LiquidityVacuumEngine::exit_price() const { return exit_price_; }

}
