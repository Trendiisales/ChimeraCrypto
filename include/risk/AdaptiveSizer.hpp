#pragma once

namespace chimera {

class AdaptiveSizer {
public:
    AdaptiveSizer(double base_size, double max_multiplier);
    double size(double sweep_intensity, double imbalance_accel) const;

private:
    double base_;
    double max_mult_;
};

}
