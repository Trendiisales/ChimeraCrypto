#pragma once
#include <cstdint>

namespace chimera {

class QueueEstimator {
public:
    QueueEstimator() : last_bid_vol_(0.0), last_ask_vol_(0.0), queue_position_(0.0) {}

    void update(double bid_vol, double ask_vol, bool posting_bid) {
        if (posting_bid) {
            double delta = last_bid_vol_ - bid_vol;
            queue_position_ += delta;
            last_bid_vol_ = bid_vol;
        } else {
            double delta = last_ask_vol_ - ask_vol;
            queue_position_ += delta;
            last_ask_vol_ = ask_vol;
        }

        if (queue_position_ < 0.0) queue_position_ = 0.0;
    }

    double position() const { return queue_position_; }

    bool filled_estimate(double threshold = 0.0) const {
        return queue_position_ <= threshold;
    }

private:
    double last_bid_vol_;
    double last_ask_vol_;
    double queue_position_;
};

}
