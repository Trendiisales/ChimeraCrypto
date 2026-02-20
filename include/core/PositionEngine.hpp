#pragma once

namespace chimera {

class PositionEngine {
public:
    PositionEngine(double cost_bps);

    bool in_position() const;

    void enter(bool is_buy,
               double price,
               double size,
               double expected_edge_bps);

    bool should_exit(double current_price,
                     bool refill_detected,
                     double expected_edge_bps);

    void exit();

private:
    bool active_ = false;
    bool is_buy_ = true;

    double entry_price_ = 0.0;
    double size_ = 0.0;

    double target_bps_ = 0.0;
    double stop_bps_ = 6.0;

    double cost_bps_;
};

}
