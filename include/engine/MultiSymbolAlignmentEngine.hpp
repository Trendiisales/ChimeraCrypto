#pragma once
#include <cstdint>

namespace chimera {

struct MSAConfig {
    double base_cost_bps = 7.0;
    double min_required_edge_bps = 16.0;
    double stop_bps = 10.0;
    double target_bps = 34.0;
    double scale_out_bps = 18.0;
    double alignment_trigger_bps = 10.0;
    int alignment_window_ticks = 64;
    int cooldown_ms = 1200;
};

class MultiSymbolAlignmentEngine {
public:
    explicit MultiSymbolAlignmentEngine(const MSAConfig& cfg);
    void reset();
    void on_tick(int symbol_index, double price, int64_t now_ms, double latency_p95_ms);
    bool enter_signal() const;
    bool exit_signal() const;
    bool is_long() const;
    int symbol() const;
    double entry_price() const;
    double exit_price() const;

private:
    void update_impulse(int s, double price);
    void evaluate_alignment(int64_t now_ms, double latency_p95_ms);
    void manage_position(int symbol_index, double price, int64_t now_ms);
    void exit_trade(double price, int64_t now_ms);

    static constexpr int SYMBOLS = 3;
    static constexpr int BUF = 64;

    MSAConfig cfg_;
    double buffer_[SYMBOLS][BUF]{};
    double sum_[SYMBOLS]{};
    double last_price_[SYMBOLS]{};
    int idx_[SYMBOLS]{};
    int count_[SYMBOLS]{};
    bool active_ = false;
    bool long_side_ = false;
    int symbol_index_ = -1;
    double entry_price_ = 0.0;
    int64_t entry_time_ = 0;
    bool partial_taken_ = false;
    bool signal_enter_ = false;
    bool signal_exit_ = false;
    int64_t cooldown_until_ = 0;
    double exit_price_ = 0.0;
};

}
