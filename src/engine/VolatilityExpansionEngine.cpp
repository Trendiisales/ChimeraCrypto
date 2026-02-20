#include "engine/VolatilityExpansionEngine.hpp"
#include <cmath>

namespace chimera {

VolatilityExpansionEngine::VolatilityExpansionEngine(const VEConfig& cfg)
    : cfg_(cfg)
{
    reset();
}

void VolatilityExpansionEngine::reset()
{
    short_idx_ = 0;
    long_idx_ = 0;
    short_count_ = 0;
    long_count_ = 0;
    short_sum_ = 0.0;
    long_sum_ = 0.0;
    last_price_ = 0.0;
    in_position_ = false;
    signal_ready_ = false;
    exit_signal_ = false;
}

void VolatilityExpansionEngine::record_short(double v)
{
    short_sum_ -= short_buffer_[short_idx_];
    short_buffer_[short_idx_] = v;
    short_sum_ += v;
    short_idx_ = (short_idx_ + 1) % SHORT_CAP;
    if (short_count_ < SHORT_CAP) short_count_++;
}

void VolatilityExpansionEngine::record_long(double v)
{
    long_sum_ -= long_buffer_[long_idx_];
    long_buffer_[long_idx_] = v;
    long_sum_ += v;
    long_idx_ = (long_idx_ + 1) % LONG_CAP;
    if (long_count_ < LONG_CAP) long_count_++;
}

void VolatilityExpansionEngine::on_tick(double price, int64_t now_ms, double latency_p95_ms)
{
    signal_ready_ = false;
    exit_signal_ = false;

    if (last_price_ > 0.0) {
        double move_bps = ((price - last_price_) / last_price_) * 10000.0;
        record_short(std::fabs(move_bps));
        record_long(std::fabs(move_bps));
    }

    last_price_ = price;

    if (!in_position_)
        try_enter(price, now_ms, latency_p95_ms);
    else
        manage_position(price, now_ms);
}

void VolatilityExpansionEngine::try_enter(double price, int64_t now_ms, double latency_p95_ms)
{
    if (short_count_ < SHORT_CAP || long_count_ < LONG_CAP)
        return;

    double short_vol = short_sum_ / short_count_;
    double long_vol = long_sum_ / long_count_;

    if (long_vol <= 0.0)
        return;

    if (short_vol > long_vol * cfg_.vol_multiplier) {
        double dynamic_required = cfg_.min_required_edge_bps + latency_p95_ms * 0.8;

        if (short_vol > dynamic_required) {
            in_position_ = true;
            long_side_ = true;
            entry_price_ = price;
            entry_time_ = now_ms;
            signal_ready_ = true;
        }
    }
}

void VolatilityExpansionEngine::manage_position(double price, int64_t now_ms)
{
    double move_bps = ((price - entry_price_) / entry_price_) * 10000.0;

    if (!long_side_)
        move_bps = -move_bps;

    if (move_bps >= cfg_.target_bps || move_bps <= -cfg_.stop_bps || now_ms - entry_time_ > 4000) {
        in_position_ = false;
        exit_signal_ = true;
        exit_price_ = price;
    }
}

bool VolatilityExpansionEngine::has_signal() const { return signal_ready_; }
bool VolatilityExpansionEngine::exit_ready() const { return exit_signal_; }
bool VolatilityExpansionEngine::is_long() const { return long_side_; }
double VolatilityExpansionEngine::entry_price() const { return entry_price_; }
double VolatilityExpansionEngine::exit_price() const { return exit_price_; }

}
