#pragma once
#include <cmath>

namespace chimera {

class UltraExpansionEngine {
public:
    UltraExpansionEngine() { reset(); }

    inline void reset() {
        last_price_ = 0.0;
        short_sum_ = 0.0;
        long_sum_ = 0.0;
        short_count_ = 0;
        long_count_ = 0;
        signal_ = false;
    }

    inline void on_tick(double price, int64_t, double latency_ms) {
        signal_ = false;

        if (last_price_ > 0.0) {
            double move = (price - last_price_) / last_price_ * 10000.0;
            double abs_move = std::fabs(move);

            short_sum_ += abs_move;
            long_sum_  += abs_move;

            if (++short_count_ > 32) short_count_ = 32;
            if (++long_count_ > 256) long_count_ = 256;

            double short_avg = short_sum_ / short_count_;
            double long_avg  = long_sum_  / long_count_;

            double dynamic = 7.0 + latency_ms * 0.8;

            if (short_avg > long_avg * 1.8 && short_avg > dynamic) {
                signal_ = true;
            }
        }

        last_price_ = price;
    }

    inline bool signal() const { return signal_; }

private:
    double last_price_;
    double short_sum_;
    double long_sum_;
    int short_count_;
    int long_count_;
    bool signal_;
};

}
