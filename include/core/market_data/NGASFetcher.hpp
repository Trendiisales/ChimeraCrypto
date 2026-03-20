#pragma once
// ============================================================================
// NGASFetcher.hpp — Macro Sentiment via Fear & Greed Index (alternative.me)
// ============================================================================
//
// ORIGINAL: NatGas futures via stooq.com — broken (returns N/D from Tokyo VPS)
// REPLACEMENT: Crypto Fear & Greed Index via alternative.me (free, no key,
//   works from all regions, updates daily)
//
// SIGNAL LOGIC (same interface as original NGAS engine):
//   Fear & Greed 0-20  (Extreme Fear)  → risk-on signal (-1): longs favoured
//   Fear & Greed 21-40 (Fear)          → mild risk-on (-1) if trending up
//   Fear & Greed 61-80 (Greed)         → mild risk-off (+1)
//   Fear & Greed 81-100 (Extreme Greed) → risk-off (+1): longs disfavoured
//   40-60 (Neutral)                    → no signal (0)
//
// RATIONALE:
//   Extreme Fear = oversold market = mean reversion longs have edge
//   Extreme Greed = crowded longs = fade the rally, reduce long size
//   This is a daily signal — not a scalp signal — consistent with original NGAS intent.
//
// DATA SOURCE: https://api.alternative.me/fng/?limit=2
//   Returns current and previous day index value (0-100)
//   Free, no API key, no rate limit for reasonable polling (every 5min is fine)
//
// THREAD SAFETY: atomic reads, background poll thread
// ============================================================================

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <thread>

namespace chimera {

class NGASFetcher {
public:
    NGASFetcher() = default;

    // ── Public API (same interface as original NGAS fetcher) ─────────────────

    // Returns latest Fear & Greed value (0-100), stored in price_ for compatibility
    double price()        const { return price_.load(std::memory_order_relaxed); }

    // Returns change from yesterday's value (positive = more greedy, negative = more fearful)
    double change_pct()   const { return change_pct_.load(std::memory_order_relaxed); }

    // True once at least one successful fetch has completed
    bool   ready()        const { return ready_.load(std::memory_order_relaxed); }

    // Signal direction: +1 = risk-off (Extreme Greed — fade longs)
    //                   -1 = risk-on  (Extreme Fear — longs favoured)
    //                    0 = neutral
    int    signal_dir()   const { return signal_dir_.load(std::memory_order_relaxed); }

    // ── Background fetch ────────────────────────────────────────────────────
    void start() {
        std::thread([this]() {
            fetch_once();
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SECS));
                fetch_once();
            }
        }).detach();

        std::printf("[NGAS-FETCHER] Background poll started (every %ds)\\n",
                    POLL_INTERVAL_SECS);
        std::fflush(stdout);
    }

    void fetch_once() {
        bool ok = fetch_fear_greed();
        if (!ok) {
            std::printf("[NGAS-FETCHER] Fear & Greed fetch failed — retaining last value=%.0f\\n",
                        price_.load(std::memory_order_relaxed));
            std::fflush(stdout);
        }
    }

private:
    static constexpr int    POLL_INTERVAL_SECS  = 300;  // 5 min
    static constexpr double EXTREME_FEAR_THRESH = 15.0; // AUDIT 2026-03-21: tightened 25->15. At 25 the signal fired throughout entire sustained bear markets. 15 = genuine capitulation spike, not prolonged fear. Also requires delta > +3 (fear INCREASING from yesterday = capitulation event, not drift).
    static constexpr double EXTREME_GREED_THRESH= 75.0; // above = risk-off signal

    std::atomic<double> price_{0.0};        // current F&G value (0-100)
    std::atomic<double> change_pct_{0.0};   // delta vs yesterday
    std::atomic<bool>   ready_{false};
    std::atomic<int>    signal_dir_{0};

    bool fetch_fear_greed() {
        int ret = ::system(
            "curl -s --max-time 8 "
            "'https://api.alternative.me/fng/?limit=2' "
            "| python3 -c \""
            "import json,sys; d=json.load(sys.stdin)['data']; "
            "curr=float(d[0]['value']); prev=float(d[1]['value']); "
            "print(f'{curr:.1f} {prev:.1f}')"
            "\" > /tmp/chimera_ngas.txt 2>/dev/null"
        );
        (void)ret;

        FILE* f = std::fopen("/tmp/chimera_ngas.txt", "r");
        if (!f) return false;
        double curr = 0.0, prev = 0.0;
        int parsed = std::fscanf(f, "%lf %lf", &curr, &prev);
        std::fclose(f);
        if (parsed < 1 || curr <= 0.0) return false;

        price_.store(curr, std::memory_order_relaxed);

        double delta = (parsed == 2 && prev > 0.0) ? (curr - prev) : 0.0;
        change_pct_.store(delta, std::memory_order_relaxed);

        // Compute signal
        int sig = 0;
        // AUDIT 2026-03-21: added delta filter for risk-on signal.
        // Extreme Fear (score <= 15) is necessary but NOT sufficient.
        // Also require delta <= -3 (sentiment getting MORE fearful today vs yesterday)
        // = genuine capitulation spike. A flat or improving delta means the market
        // has been fearful for days — that's a sustained downtrend, not a buy signal.
        // Without delta filter: fired throughout entire bear markets (F&G 11 for weeks).
        // With delta filter: only fires when fear SPIKES sharply in a single day.
        if (curr <= EXTREME_FEAR_THRESH && delta <= -3.0) sig = -1;  // risk-on: acute capitulation
        if (curr >= EXTREME_GREED_THRESH) sig =  1;  // risk-off: extreme greed = fade longs
        signal_dir_.store(sig, std::memory_order_relaxed);

        if (parsed == 2) {
            ready_.store(true, std::memory_order_relaxed);
        }

        const char* classification =
            curr <= 20  ? "Extreme Fear" :
            curr <= 40  ? "Fear" :
            curr <= 60  ? "Neutral" :
            curr <= 80  ? "Greed" : "Extreme Greed";

        std::printf("[NGAS-FETCHER] Fear & Greed: %.0f (%s) | delta=%.1f | signal=%s\\n",
            curr, classification, delta,
            sig == -1 ? "RISK-ON (longs favoured)" :
            sig ==  1 ? "RISK-OFF (reduce longs)"  : "NEUTRAL");
        std::fflush(stdout);
        return true;
    }
};

} // namespace chimera
