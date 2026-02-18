#pragma once

namespace chimera {

class OrderFlowAcceleration {
public:
    void update(double imbalance) {
        double delta = imbalance - lastImb_;
        accel_ = 0.8 * accel_ + 0.2 * delta;
        lastImb_ = imbalance;
    }

    double acceleration() const {
        return accel_;
    }

private:
    double lastImb_ = 0.0;
    double accel_ = 0.0;
};

} // namespace chimera
