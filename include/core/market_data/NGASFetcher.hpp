#pragma once
// ============================================================================
// NGASFetcher.hpp — Natural Gas (NGAS) spot price via stooq.com REST poll
// ============================================================================
//
// RATIONALE (NGAS lead-lag for crypto):
//   Natural Gas price spikes are a leading macro signal for crypto volatility.
//   Mechanism:
//     1. NGAS price jumps sharply (>2% in <15min) = energy inflation shock
//     2. Risk-off rotation occurs within 15-60 minutes
//     3. BTC/ETH sell off as macro traders de-risk energy exposure
//     4. Conversely, NGAS dropping sharply → risk-on → BTC/ETH bid
//
//   This is a MACRO signal, not a microstructure signal.
//   Entry is slow (latency insensitive), hold is 15-30min.
//   Only fires during high-vol sessions (EU open, US open).
//
// DATA SOURCE:
//   stooq.com — free, no API key, returns CSV for NGO.F (NGAS CME futures)
//   URL: https://stooq.com/q/d/l/?s=ngo.f&i=5  (5-min bar CSV)
//   Fallback: Yahoo Finance /v8/finance/chart/NG=F?interval=5m
//
// POLL INTERVAL: every 5 minutes (NGAS price changes on bar close)
//
// SIGNAL:
//   - price_change_pct > +NGAS_SPIKE_UP_PCT  → SHORT signal (risk-off crypto)
//   - price_change_pct < -NGAS_DROP_PCT       → LONG signal  (risk-on crypto)
//   - Otherwise: no signal
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

    // ── Public API ──────────────────────────────────────────────────────────

    // Returns latest NGAS price (0.0 if not yet fetched)
    double price()        const { return price_.load(std::memory_order_relaxed); }

    // Returns price change % over last LOOKBACK_BARS bars (positive = up)
    double change_pct()   const { return change_pct_.load(std::memory_order_relaxed); }

    // True once at least two successful fetches have completed (needed to compute delta)
    bool   ready()        const { return ready_.load(std::memory_order_relaxed); }

    // Signal direction: +1 = risk-off SHORT crypto, -1 = risk-on LONG crypto, 0 = neutral
    int    signal_dir()   const { return signal_dir_.load(std::memory_order_relaxed); }

    // ── Background fetch ────────────────────────────────────────────────────

    // Start background polling thread (detached, runs for lifetime of process)
    void start() {
        std::thread([this]() {
            // First fetch immediately on startup
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

    // Synchronous one-shot fetch (also called from start() loop)
    void fetch_once() {
        // Try stooq.com first (reliable, free, no auth)
        bool ok = fetch_stooq();
        if (!ok) {
            // Fallback: Yahoo Finance
            ok = fetch_yahoo();
        }
        if (!ok) {
            std::printf("[NGAS-FETCHER] All sources failed — retaining last price=%.4f\\n",
                        price_.load(std::memory_order_relaxed));
            std::fflush(stdout);
        }
    }

private:
    // ── Constants ───────────────────────────────────────────────────────────
    static constexpr int    POLL_INTERVAL_SECS  = 300;   // 5 min — matches bar size
    static constexpr int    LOOKBACK_BARS        = 3;    // compare current vs 15min ago
    static constexpr double SPIKE_UP_PCT         = 2.0;  // >2% up = risk-off SHORT
    static constexpr double DROP_PCT             = 2.0;  // <-2% down = risk-on LONG

    // ── State ────────────────────────────────────────────────────────────────
    std::atomic<double> price_{0.0};
    std::atomic<double> change_pct_{0.0};
    std::atomic<bool>   ready_{false};
    std::atomic<int>    signal_dir_{0};

    // Rolling price buffer for change calculation (not atomic — only written from fetch thread)
    static constexpr int BUF_SIZE = 8;
    double price_buf_[BUF_SIZE] = {};
    int    buf_head_ = 0;
    int    buf_count_ = 0;

    // ── Stooq.com fetch ──────────────────────────────────────────────────────
    // Returns CSV: Date,Time,Open,High,Low,Close,Volume
    // We want the Close of the most recent completed bar.
    bool fetch_stooq() {
        // Download last 10 5-minute bars of NGO.F (CME Natural Gas front month)
        int ret = ::system(
            "curl -s --max-time 8 "
            "'https://stooq.com/q/d/l/?s=ngo.f&i=5' "
            "| tail -2 | head -1 "  // second-to-last line = most recent COMPLETED bar
            "| cut -d',' -f5 "      // column 5 = Close
            "> /tmp/chimera_ngas.txt 2>/dev/null"
        );
        (void)ret;

        double val = read_price_file("/tmp/chimera_ngas.txt");
        if (val <= 0.0) return false;

        update_price(val, "stooq");
        return true;
    }

    // ── Yahoo Finance fallback ───────────────────────────────────────────────
    bool fetch_yahoo() {
        int ret = ::system(
            "curl -s --max-time 10 "
            "'https://query1.finance.yahoo.com/v8/finance/chart/NG%3DF"
            "?interval=5m&range=1h' "
            "| python3 -c \""
            "import json,sys; d=json.load(sys.stdin); "
            "c=d['chart']['result'][0]['indicators']['quote'][0]['close']; "
            "vals=[x for x in c if x is not None]; "
            "print(vals[-2] if len(vals)>=2 else vals[-1]) if vals else None"
            "\" > /tmp/chimera_ngas.txt 2>/dev/null"
        );
        (void)ret;

        double val = read_price_file("/tmp/chimera_ngas.txt");
        if (val <= 0.0) return false;

        update_price(val, "yahoo");
        return true;
    }

    // ── Helpers ──────────────────────────────────────────────────────────────
    static double read_price_file(const char* path) {
        FILE* f = std::fopen(path, "r");
        if (!f) return 0.0;
        double val = 0.0;
        bool ok = (std::fscanf(f, "%lf", &val) == 1 && val > 0.0);
        std::fclose(f);
        return ok ? val : 0.0;
    }

    void update_price(double new_price, const char* source) {
        double old_price = price_.load(std::memory_order_relaxed);

        // Push into rolling buffer
        price_buf_[buf_head_ % BUF_SIZE] = new_price;
        buf_head_++;
        if (buf_count_ < BUF_SIZE) buf_count_++;

        price_.store(new_price, std::memory_order_relaxed);

        // Compute change vs LOOKBACK_BARS ago
        double change = 0.0;
        int dir = 0;
        if (buf_count_ >= LOOKBACK_BARS + 1) {
            int lookback_idx = (buf_head_ - 1 - LOOKBACK_BARS + BUF_SIZE) % BUF_SIZE;
            double ref_price = price_buf_[lookback_idx];
            if (ref_price > 0.0) {
                change = (new_price - ref_price) / ref_price * 100.0;
                if (change >  SPIKE_UP_PCT) dir = +1;  // spike up → risk-off
                else if (change < -DROP_PCT) dir = -1;  // drop     → risk-on
            }
        }

        change_pct_.store(change, std::memory_order_relaxed);
        signal_dir_.store(dir, std::memory_order_relaxed);

        if (buf_count_ >= 2) ready_.store(true, std::memory_order_relaxed);

        std::printf("[NGAS-FETCHER] src=%s | price=%.4f (was %.4f) | chg=%.2f%% | signal=%s\\n",
            source, new_price, old_price, change,
            dir == +1 ? "SHORT(risk-off)" : dir == -1 ? "LONG(risk-on)" : "NEUTRAL");
        std::fflush(stdout);
    }
};

} // namespace chimera
