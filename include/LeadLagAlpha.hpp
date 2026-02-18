#pragma once
#include <cmath>

namespace chimera {

class LeadLagAlpha {
public:
    void update(double leaderReturn, double followerReturn) {
        double signal = leaderReturn - followerReturn;
        alpha_ = 0.85 * alpha_ + 0.15 * signal;
    }

    double edge() const {
        return alpha_;
    }

private:
    double alpha_ = 0.0;
};

} // namespace chimera
