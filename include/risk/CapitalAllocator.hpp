#pragma once

namespace chimera {

class CapitalAllocator {
public:
    explicit CapitalAllocator(double max_leverage = 3.0);
    double max_leverage() const;

private:
    double max_leverage_;
};

}
