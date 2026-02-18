#pragma once

namespace chimera {

class SignalFusion {
public:
    double combine(double micro, double shortTerm, double mediumTerm) {
        return 0.5 * micro + 0.3 * shortTerm + 0.2 * mediumTerm;
    }
};

} // namespace chimera
