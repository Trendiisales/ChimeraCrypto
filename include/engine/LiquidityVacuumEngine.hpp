#pragma once
#include <cstdint>

namespace chimera {

struct LVConfig {
    double base_cost_bps = 7.0;
    double min_required_edge_bps = 14.0;
    double stop_bps = 8.0;
    double target_bps = 26.0;
    double scale_out_bps = 14.0;
    double imbalance_trigger = 2.4;
    double depth_drop_ratio = 0.30;
    double max_spread_bps = 3.0;
    int depth_window_ticks = 64;
    int cooldown_ms = 800;
};

class LiquidityVacuumEngine {
public:
    explicit LiquidityVacuumEngine(const LVConfig& cfg);
    void reset();
    void on_book(double bid_price, double bid_size, double ask_price, double ask_size, int64_t now_ms, double latency_p95_ms);
    bool enter_signal() const;
    bool exit_signal() const;
    bool is_long() const;
    double entry_price() const;
    double exit_price() const;

private:
    void record_depth(double bid, double ask);
    void evaluate_entry(double mid, int64_t now_ms, double latency_p95_ms);
    void manage_position(double mid, int64_t now_ms);

    static constexpr int DEPTH_CAP = 64;

    LVConfig cfg_;
    double bid_buffer_[DEPTH_CAP]{};
    double ask_buffer_[DEPTH_CAP]{};
    int idx_ = 0;
    int count_ = 0;
    double bid_sum_ = 0.0;
    double ask_sum_ = 0.0;
    double last_mid_ = 0.0;
    int64_t cooldown_until_ = 0;
    bool active_ = false;
    bool long_side_ = false;
    double entry_price_ = 0.0;
    int64_t entry_time_ = 0;
    bool partial_taken_ = false;
    bool signal_enter_ = false;
    bool signal_exit_ = false;
    double exit_price_ = 0.0;
};

}
