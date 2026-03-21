#pragma once
// ============================================================================
// FundingSignalEngine.hpp
// Chimera -- Funding Rate Signal Engine
//
// Wraps FundingRateFetcher (BTC-only rate()) into a tradeable signal with
// direction + strength scoring. Called from a detached thread in main.cpp.
//
// USAGE IN main.cpp:
//   chimera::FundingSignalEngine g_funding_signal;    // global
//   std::thread([&](){
//       g_funding_signal.fetch();
//       controller.set_funding_signal(&g_funding_signal);
//   }).detach();
//
// SIGNAL LOGIC:
//   Positive funding -> longs pay shorts -> LONG bias  (market is long-heavy)
//   Negative funding -> shorts pay longs -> SHORT bias (market is short-heavy)
//   |rate| < NOISE_FLOOR -> NEUTRAL
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "core/market_data/FundingRateFetcher.hpp"
#include "core/SymbolIndex.hpp"

namespace chimera {

class FundingSignalEngine {
public:
    enum class Dir : int8_t { NEUTRAL = 0, LONG = 1, SHORT = -1 };

    struct Signal {
        Dir    dir      = Dir::NEUTRAL;
        double strength = 0.0;   // 0.0-1.0
        double rate_pct = 0.0;   // raw rate (%)
        bool   ready    = false;
    };

    static constexpr double NOISE_FLOOR_PCT = 0.010;
    static constexpr double STRONG_PCT      = 0.100;

    FundingSignalEngine() = default;

    // Blocking REST fetch -- call from detached thread
    void fetch() {
        fetcher_.fetch();
        ready_.store(true, std::memory_order_release);
        std::printf("[FUNDING-SIG] fetch complete | BTC rate=%.5f%%\n",
            fetcher_.rate() * 100.0);
        std::fflush(stdout);
    }

    bool is_ready() const {
        return ready_.load(std::memory_order_acquire);
    }

    // Single-rate fetcher covers BTC only -- return same rate for all symbols
    Signal get_signal(int /*symbol_id*/) const {
        if (!ready_.load(std::memory_order_acquire)) return {};
        return compute_signal(fetcher_.rate());
    }

    double get_rate_pct() const {
        return fetcher_.rate() * 100.0;
    }

private:
    FundingRateFetcher fetcher_;
    std::atomic<bool>  ready_{false};

    static Signal compute_signal(double rate) {
        Signal s;
        s.rate_pct = rate * 100.0;
        s.ready    = true;
        double abs_rate = std::fabs(rate * 100.0);
        if (abs_rate < NOISE_FLOOR_PCT) {
            s.dir      = Dir::NEUTRAL;
            s.strength = 0.0;
            return s;
        }
        s.dir = (rate > 0.0) ? Dir::LONG : Dir::SHORT;
        double range = STRONG_PCT - NOISE_FLOOR_PCT;
        s.strength = std::min(1.0, (abs_rate - NOISE_FLOOR_PCT) / range);
        return s;
    }
};

} // namespace chimera
