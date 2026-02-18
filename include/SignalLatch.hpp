#pragma once
#include <unordered_map>
#include <chrono>
#include <string>

namespace chimera {

class SignalLatch {
public:
    bool shouldEnter(
        const std::string& symbol,
        double currentConf,
        double threshold,
        bool hasPosition);

    void reset(const std::string& symbol);

private:
    struct State {
        double prevConf = 0.0;
        bool latched = false;
        std::chrono::steady_clock::time_point lastEntry;
    };

    std::unordered_map<std::string, State> states_;
    static constexpr int COOLDOWN_MS = 1500;  // 1.5 second cooldown
};

} // namespace chimera
