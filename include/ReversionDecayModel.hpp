#pragma once
#include <cmath>

namespace chimera {

class ReversionDecayModel {
public:
    void update(double priceReturn) {
        double absRet = std::fabs(priceReturn);
        decay_ *= (1.0 - absRet * 5.0);

        if (decay_ < 0.1) {
            decay_ = 0.1;
        }
    }

    double multiplier() const {
        return decay_;
    }

    void reset() {
        decay_ = 1.0;
    }

private:
    double decay_ = 1.0;
};

} // namespace chimera
