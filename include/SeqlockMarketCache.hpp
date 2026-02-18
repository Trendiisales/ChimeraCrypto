#pragma once
#include <atomic>
#include <cstdint>

namespace chimera {

struct MarketSnapshot {
    double bestBid = 0.0;
    double bestAsk = 0.0;
    double mid = 0.0;
    uint64_t lastUpdateId = 0;
    uint64_t ts = 0;
    bool valid = false;
};

class SeqlockMarketCache {
public:
    void publish(const MarketSnapshot& snap) {
        seq_.fetch_add(1, std::memory_order_acquire);
        data_ = snap;
        seq_.fetch_add(1, std::memory_order_release);
    }

    MarketSnapshot read() const {
        MarketSnapshot copy;
        while (true) {
            uint64_t s1 = seq_.load(std::memory_order_acquire);
            copy = data_;
            uint64_t s2 = seq_.load(std::memory_order_acquire);
            if (s1 == s2 && !(s1 & 1))
                return copy;
        }
    }

private:
    mutable std::atomic<uint64_t> seq_{0};
    MarketSnapshot data_;
};

} // namespace chimera
