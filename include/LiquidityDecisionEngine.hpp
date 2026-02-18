#pragma once
#include <cmath>

namespace chimera {

enum class ExecutionStyle {
    MAKER,
    TAKER
};

class LiquidityDecisionEngine {
public:
    ExecutionStyle decide(double spreadBps, double queueWaitMs, 
                         double imbalance, double volatility) {
        if (volatility > 0.0015) return ExecutionStyle::TAKER;
        if (std::fabs(imbalance) > 0.7) return ExecutionStyle::TAKER;
        if (queueWaitMs > 8.0) return ExecutionStyle::TAKER;
        if (spreadBps < 1.0) return ExecutionStyle::TAKER;
        return ExecutionStyle::MAKER;
    }
};

} // namespace chimera
