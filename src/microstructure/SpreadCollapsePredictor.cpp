#include "microstructure/SpreadCollapsePredictor.hpp"

namespace chimera {

SpreadCollapsePredictor::SpreadCollapsePredictor(size_t window, double compression_threshold)
    : window_(window), threshold_(compression_threshold) {}

SpreadCollapseSignal SpreadCollapsePredictor::update(double spread, uint64_t timestamp_ns) {
    SpreadCollapseSignal sig{};
    sig.active = false;
    sig.compression = 0.0;
    sig.timestamp_ns = timestamp_ns;

    spreads_.push_back(spread);
    if (spreads_.size() > window_)
        spreads_.pop_front();

    if (spreads_.size() < window_)
        return sig;

    double max_spread = 0.0;
    for (double s : spreads_)
        if (s > max_spread)
            max_spread = s;

    if (max_spread == 0.0)
        return sig;

    double compression = (max_spread - spread) / max_spread;

    if (compression > threshold_) {
        sig.active = true;
        sig.compression = compression;
    }

    return sig;
}

}
