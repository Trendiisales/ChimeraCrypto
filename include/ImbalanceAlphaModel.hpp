#pragma once
#include <cmath>

namespace chimera {

class ImbalanceAlphaModel {
public:
    void update(double imbalance) {
        alpha_ = 0.85 * alpha_ + 0.15 * imbalance;
        decay_ *= 0.97;
    }

    double edgeScore() const {
        return alpha_ * decay_;
    }

    void onTrade() {
        decay_ = 1.0;
    }

private:
    double alpha_ = 0.0;
    double decay_ = 1.0;
};

} // namespace chimera
