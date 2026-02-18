#pragma once
#include <chrono>
#include <cstdint>

namespace chimera {

class LatencyShockDetector {
public:
    bool update(double rttMs) {
        uint64_t now = nowMs();

        if (rttMs > thresholdMs_) {
            cooldownUntil_ = now + cooldownMs_;
        }

        return now < cooldownUntil_;
    }

    bool inShock() const {
        return nowMs() < cooldownUntil_;
    }

private:
    uint64_t nowMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    double thresholdMs_ = 25.0;
    uint64_t cooldownMs_ = 5000;
    uint64_t cooldownUntil_ = 0;
};

} // namespace chimera
