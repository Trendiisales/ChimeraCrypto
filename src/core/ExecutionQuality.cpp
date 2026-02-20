#include "core/ExecutionQuality.hpp"

namespace chimera {

void ExecutionQuality::record_fill(double slippage) {
    total_slippage_ += slippage;
    fill_count_++;
}

double ExecutionQuality::avg_slippage() const {
    if (fill_count_ == 0) return 0.0;
    return total_slippage_ / fill_count_;
}

}
