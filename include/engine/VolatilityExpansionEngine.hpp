#pragma once
#include <cstdint>

namespace chimera {

struct VEConfig {
    double base_cost_bps = 7.0;
    double min_required_edge_bps = 15.0;
    double stop_bps = 9.0;
    double target_bps = 28.0;
    double vol_multiplier = 1.8;
    double impulse_trigger_bps = 12.0;
    int short_window_ms = 500;
    int long_window_ms = 5000;
};

class VolatilityExpansionEngine {
public:
    explicit VolatilityExpansionEngine(const VEConfig& cfg);
    void reset();
    void on_tick(double price, int64_t now_ms, double latency_p95_ms);
    bool has_signal() const;
    bool exit_ready() const;
    bool is_long() const;
    double entry_price() const;
    double exit_price() const;

private:
    void record_short(double v);
    void record_long(double v);
    void try_enter(double price, int64_t now_ms, double latency_p95_ms);
    void manage_position(double price, int64_t now_ms);

    static constexpr int SHORT_CAP = 64;
    static constexpr int LONG_CAP = 512;

    VEConfig cfg_;
    double short_buffer_[SHORT_CAP]{};
    double long_buffer_[LONG_CAP]{};
    int short_idx_ = 0;
    int long_idx_ = 0;
    int short_count_ = 0;
    int long_count_ = 0;
    double short_sum_ = 0.0;
    double long_sum_ = 0.0;
    double last_price_ = 0.0;
    bool in_position_ = false;
    bool long_side_ = false;
    double entry_price_ = 0.0;
    int64_t entry_time_ = 0;
    bool signal_ready_ = false;
    bool exit_signal_ = false;
    double exit_price_ = 0.0;
};

}
