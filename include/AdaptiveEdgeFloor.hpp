#pragma once

namespace chimera {

class AdaptiveEdgeFloor {
public:
    void update(double realizedNetBps) {
        ema_ = 0.9 * ema_ + 0.1 * realizedNetBps;

        if (ema_ < 0) {
            floorBps_ += 1.0;
        } else {
            floorBps_ *= 0.995;
        }
    }

    double floor() const {
        return floorBps_;
    }

private:
    double ema_ = 0.0;
    double floorBps_ = 10.0;
};

} // namespace chimera
