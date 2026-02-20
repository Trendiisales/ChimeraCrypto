#include "engine/MultiSymbolAlignmentEngine.hpp"
#include <cmath>

namespace chimera {

MultiSymbolAlignmentEngine::MultiSymbolAlignmentEngine(const MSAConfig& cfg) : cfg_(cfg) { reset(); }

void MultiSymbolAlignmentEngine::reset()
{
    for (int i = 0; i < SYMBOLS; ++i) {
        idx_[i] = 0;
        count_[i] = 0;
        sum_[i] = 0.0;
        last_price_[i] = 0.0;
    }
    active_ = false;
    signal_enter_ = false;
    signal_exit_ = false;
    cooldown_until_ = 0;
}

void MultiSymbolAlignmentEngine::update_impulse(int s, double price)
{
    if (last_price_[s] > 0.0) {
        double move = ((price - last_price_[s]) / last_price_[s]) * 10000.0;
        sum_[s] -= buffer_[s][idx_[s]];
        buffer_[s][idx_[s]] = move;
        sum_[s] += move;
        idx_[s] = (idx_[s] + 1) % BUF;
        if (count_[s] < BUF) count_[s]++;
    }
    last_price_[s] = price;
}

void MultiSymbolAlignmentEngine::on_tick(int symbol_index, double price, int64_t now_ms, double latency_p95_ms)
{
    signal_enter_ = false;
    signal_exit_ = false;

    if (symbol_index < 0 || symbol_index >= SYMBOLS) return;

    update_impulse(symbol_index, price);

    if (!active_) {
        if (now_ms >= cooldown_until_)
            evaluate_alignment(now_ms, latency_p95_ms);
    } else {
        manage_position(symbol_index, price, now_ms);
    }
}

void MultiSymbolAlignmentEngine::evaluate_alignment(int64_t now_ms, double latency_p95_ms)
{
    int strong_count = 0;
    int strongest = -1;
    double strongest_val = 0.0;

    double dynamic_required = cfg_.min_required_edge_bps + latency_p95_ms * 0.9;

    for (int i = 0; i < SYMBOLS; ++i) {
        double impulse = (count_[i] == 0) ? 0.0 : sum_[i] / count_[i];

        if (std::fabs(impulse) > cfg_.alignment_trigger_bps) {
            strong_count++;
            if (std::fabs(impulse) > strongest_val) {
                strongest_val = std::fabs(impulse);
                strongest = i;
            }
        }
    }

    if (strong_count >= 2 && strongest >= 0 && strongest_val > dynamic_required) {
        active_ = true;
        symbol_index_ = strongest;
        long_side_ = (count_[strongest] == 0) ? true : (sum_[strongest] / count_[strongest]) > 0.0;
        entry_price_ = last_price_[strongest];
        entry_time_ = now_ms;
        partial_taken_ = false;
        signal_enter_ = true;
    }
}

void MultiSymbolAlignmentEngine::manage_position(int symbol_index, double price, int64_t now_ms)
{
    if (symbol_index != symbol_index_) return;

    double move = ((price - entry_price_) / entry_price_) * 10000.0;
    if (!long_side_) move = -move;

    if (!partial_taken_ && move >= cfg_.scale_out_bps) {
        partial_taken_ = true;
    }

    if (move >= cfg_.target_bps || move <= -cfg_.stop_bps || now_ms - entry_time_ > 4500) {
        exit_trade(price, now_ms);
    }
}

void MultiSymbolAlignmentEngine::exit_trade(double price, int64_t now_ms)
{
    exit_price_ = price;
    active_ = false;
    signal_exit_ = true;
    cooldown_until_ = now_ms + cfg_.cooldown_ms;
}

bool MultiSymbolAlignmentEngine::enter_signal() const { return signal_enter_; }
bool MultiSymbolAlignmentEngine::exit_signal() const { return signal_exit_; }
bool MultiSymbolAlignmentEngine::is_long() const { return long_side_; }
int MultiSymbolAlignmentEngine::symbol() const { return symbol_index_; }
double MultiSymbolAlignmentEngine::entry_price() const { return entry_price_; }
double MultiSymbolAlignmentEngine::exit_price() const { return exit_price_; }

}
