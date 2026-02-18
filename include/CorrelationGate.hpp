#pragma once

namespace chimera {

class CorrelationGate {
public:
    bool allow(double primaryRet, double secondaryRet) {
        return (primaryRet * secondaryRet) >= -0.0000005;
    }
};

} // namespace chimera
