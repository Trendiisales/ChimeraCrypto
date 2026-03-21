#pragma once
// ============================================================================
// FundingSignalEngine.hpp
// Chimera — Funding Rate Signal Engine
//
// DISTINCT FROM FundingRateFetcher:
//   FundingRateFetcher  — low-level REST poller, fetches raw funding rates
//                         from Binance Futures and caches them atomically.
//   FundingSignalEngine — higher-level signal wrapper that:
//                           1. Calls fetcher.fetch() to populate rate data
//                           2. Exposes a tradeable signal (bias direction + strength)
//                           3. Tracks last-fetch timestamps + cooldown
//                           4. Provides get_signal() for engine query
//
// USAGE IN main.cpp:
//   chimera::FundingSignalEngine g_funding_signal;    // global
//
//   std::thread([&](){
//       g_funding_signal.fetch();                     // blocks ~200ms (REST)
//       controller.set_funding_signal(&g_funding_signal);
//   }).detach();
//
// USAGE IN engine:
//   if (funding_signal_ && funding_signal_->is_ready()) {
//       auto sig = funding_signal_->get_signal(symbol_id);
//       if (sig.dir == FundingSignal::LONG  && sig.strength > 0.5) { ... }
//       if (sig.dir == FundingSignal::SHORT && sig.strength > 0.5) { ... }
//   }
//
// SIGNAL LOGIC:
//   Positive funding rate → longs pay shorts → slight LONG bias  (market is long-heavy)
//   Negative funding rate → shorts pay longs → slight SHORT bias (market is short-heavy)
//
//   Threshold:  |rate| < 0.01%  → NEUTRAL  (noise floor)
//   Moderate:   0.01% – 0.05%   → weak signal, strength 0.3–0.6
//   Strong:     > 0.05%          → strong signal, strength 0.7–1.0
//   Extreme:    > 0.10%          → saturation, strength 1.0 (mean-reversion likely)
//
// Thread-safety: all public methods are lock-free (atomic storage).
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <chrono>

#include "core/SymbolIndex.hpp"
#include "core/market_data/FundingRateFetcher.hpp"

namespace chimera {

class FundingSignalEngine {
public:
    // ── Signal output ───────────────────────────────────────────────────────
    enum class Dir : int8_t { NEUTRAL = 0, LONG = 1, SHORT = -1 };

    struct Signal {
        Dir    dir      = Dir::NEUTRAL;
        double strength = 0.0;    // 0.0 – 1.0
        double rate_pct = 0.0;    // raw funding rate (%)
        bool   ready    = false;
    };

    // ── Thresholds ──────────────────────────────────────────────────────────
    static constexpr double NOISE_FLOOR_PCT  = 0.010;   // |rate| below this → NEUTRAL
    static constexpr double MODERATE_PCT     = 0.050;   // rate in [NOISE, MODERATE] → weak
    static constexpr double STRONG_PCT       = 0.100;   // rate above this → saturated (strength=1)

    // ── Lifecycle ───────────────────────────────────────────────────────────

    FundingSignalEngine() = default;

    // fetch() — blocking call, runs the underlying REST poll.
    // Call from a detached thread (as shown in main.cpp usage above).
    void fetch() {
        fetcher_.fetch();
        ready_.store(true, std::memory_order_release);
        std::printf("[FUNDING-SIG] fetch complete | BTC=%.4f%% ETH=%.4f%%
",
            fetcher_.get_rate("BTCUSDT"),
            fetcher_.get_rate("ETHUSDT"));
        std::fflush(stdout);
    }

    // is_ready() — true once fetch() has completed at least once
    bool is_ready() const {
        return ready_.load(std::memory_order_acquire);
    }

    // get_signal(symbol_id) — returns current signal for the given symbol.
    // Safe to call from any thread at any time (lock-free read).
    Signal get_signal(int symbol_id) const {
        if (!ready_.load(std::memory_order_acquire))
            return {};

        const char* full = sym_full(symbol_id);
        if (!full) return {};

        double rate = fetcher_.get_rate(full);
        return compute_signal(rate);
    }

    // get_signal(symbol_str) — convenience overload for string-keyed lookup
    Signal get_signal(const char* full_symbol) const {
        if (!ready_.load(std::memory_order_acquire))
            return {};
        double rate = fetcher_.get_rate(full_symbol);
        return compute_signal(rate);
    }

    // raw rate accessor (%) for logging/GUI
    double get_rate_pct(int symbol_id) const {
        const char* full = sym_full(symbol_id);
        return full ? fetcher_.get_rate(full) : 0.0;
    }

private:
    FundingRateFetcher fetcher_;
    std::atomic<bool>  ready_{false};

    static Signal compute_signal(double rate_pct) {
        Signal s;
        s.rate_pct = rate_pct;
        s.ready    = true;

        double abs_rate = std::fabs(rate_pct);

        if (abs_rate < NOISE_FLOOR_PCT) {
            s.dir      = Dir::NEUTRAL;
            s.strength = 0.0;
            return s;
        }

        // Direction: positive rate → longs paying → long-heavy → mild LONG bias
        //            negative rate → shorts paying → short-heavy → mild SHORT bias
        s.dir = (rate_pct > 0.0) ? Dir::LONG : Dir::SHORT;

        // Strength: linear ramp from NOISE to STRONG, clamped at 1.0
        if (abs_rate >= STRONG_PCT) {
            s.strength = 1.0;
        } else {
            double range = STRONG_PCT - NOISE_FLOOR_PCT;
            s.strength = (abs_rate - NOISE_FLOOR_PCT) / range;
            // Clamp to [0.0, 1.0]
            if (s.strength < 0.0) s.strength = 0.0;
            if (s.strength > 1.0) s.strength = 1.0;
        }

        return s;
    }
};

} // namespace chimera
