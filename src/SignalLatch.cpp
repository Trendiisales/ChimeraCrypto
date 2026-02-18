#include "SignalLatch.hpp"

namespace chimera {

bool SignalLatch::shouldEnter(
    const std::string& symbol,
    double currentConf,
    double threshold,
    bool hasPosition)
{
    auto& s = states_[symbol];

    auto now = std::chrono::steady_clock::now();

    bool cooldownExpired =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - s.lastEntry).count() > COOLDOWN_MS;

    // CRITICAL: Rising edge detection
    bool risingEdge =
        (s.prevConf < threshold) &&
        (currentConf >= threshold);

    bool allow =
        risingEdge &&
        cooldownExpired &&
        !hasPosition;

    if (allow) {
        s.latched = true;
        s.lastEntry = now;
    }

    s.prevConf = currentConf;

    return allow;
}

void SignalLatch::reset(const std::string& symbol) {
    states_.erase(symbol);
}

} // namespace chimera
