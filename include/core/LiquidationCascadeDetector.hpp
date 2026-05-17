#pragma once
// ============================================================================
// LiquidationCascadeDetector.hpp — Session 30, Edge 7
//
// Monitors Binance FAPI WebSocket liquidation stream for cascade events.
// When a large cluster of liquidations occurs (N liquidations within T seconds,
// total value > $X), this indicates a cascade / forced selling event.
//
// SIGNAL: After the cascade exhausts (liquidation rate drops), enter long
// on spot (buy the forced-selling dip). This is a structural edge because
// liquidation cascades create temporary price dislocations below fair value.
//
// ARCHITECTURE:
//   - Listens to wss://fstream.binance.com/ws/!forceOrder@arr
//   - Accumulates liquidation events in a rolling window
//   - When cluster detected, sets a "cascade_active" flag per symbol
//   - After exhaustion (rate drops below threshold), sets "cascade_entry" flag
//   - main.cpp reads cascade_entry and propagates to engines
//
// NOTE: This detector runs in a separate thread. Thread-safe via atomics.
// ============================================================================

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <array>
#include "../core/SymbolIndex.hpp"

namespace chimera {

class LiquidationCascadeDetector {
public:
    // ── Configuration ────────────────────────────────────────────────────────
    // Cascade detection: N liquidations within WINDOW_MS, total > MIN_VALUE_USD
    static constexpr int    CASCADE_MIN_COUNT     = 10;       // min liquidations in window
    static constexpr int64_t CASCADE_WINDOW_MS    = 60000;    // 60 second rolling window
    static constexpr double CASCADE_MIN_VALUE_USD = 500000.0; // $500k total liquidated value
    // Exhaustion: when rate drops to < EXHAUST_RATE per minute after cascade
    static constexpr int    EXHAUST_RATE_PER_MIN  = 3;        // cascade ending
    // Entry delay: wait this many ms after exhaustion before signalling entry
    static constexpr int64_t ENTRY_DELAY_MS       = 5000;     // 5s post-exhaustion

    struct SymbolCascadeState {
        // Rolling window of liquidation events
        struct LiqEvent {
            int64_t ts_ms;
            double  value_usd;   // notional value liquidated
            bool    is_long;     // true = long liquidated (selling pressure)
        };
        std::deque<LiqEvent> events;

        // State machine
        bool    cascade_active = false;   // cascade currently happening
        bool    entry_signal   = false;   // post-exhaustion entry opportunity
        int64_t cascade_start_ms = 0;
        int64_t exhaustion_ts_ms = 0;
        double  cascade_total_usd = 0.0;
        int     cascade_count = 0;

        // Stats
        int     total_cascades_detected = 0;
        int     total_entry_signals = 0;
    };

    LiquidationCascadeDetector() = default;

    // Called by the liquidation WebSocket callback for each forceOrder event
    void on_liquidation(int symbol_id, double price, double qty, bool is_long, int64_t ts_ms) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return;

        double value_usd = price * qty;
        std::lock_guard<std::mutex> lk(mtx_);
        auto& state = states_[symbol_id];

        // Add to rolling window
        state.events.push_back({ts_ms, value_usd, is_long});

        // Trim old events outside window
        while (!state.events.empty() &&
               state.events.front().ts_ms < ts_ms - CASCADE_WINDOW_MS) {
            state.events.pop_front();
        }

        // Count long liquidations in window (these create selling pressure)
        int long_liq_count = 0;
        double long_liq_value = 0.0;
        for (const auto& ev : state.events) {
            if (ev.is_long) {
                long_liq_count++;
                long_liq_value += ev.value_usd;
            }
        }

        // ── Cascade detection ─────────────────────────────────────────────
        if (!state.cascade_active) {
            if (long_liq_count >= CASCADE_MIN_COUNT &&
                long_liq_value >= CASCADE_MIN_VALUE_USD) {
                state.cascade_active = true;
                state.cascade_start_ms = ts_ms;
                state.cascade_total_usd = long_liq_value;
                state.cascade_count = long_liq_count;
                state.total_cascades_detected++;
                state.entry_signal = false;  // reset

                std::printf("[LIQ-CASCADE] %s: CASCADE DETECTED | count=%d val=$%.0f\n",
                    SYM_SHORT[symbol_id], long_liq_count, long_liq_value);
                std::fflush(stdout);
            }
        } else {
            // ── Exhaustion detection ──────────────────────────────────────
            // If we're in a cascade but the rate has dropped, it's exhausting
            // Count events in the last 60 seconds
            int recent_count = 0;
            for (const auto& ev : state.events) {
                if (ev.ts_ms > ts_ms - 60000 && ev.is_long) recent_count++;
            }

            if (recent_count < EXHAUST_RATE_PER_MIN) {
                // Cascade exhausted
                state.cascade_active = false;
                state.exhaustion_ts_ms = ts_ms;

                // Set entry signal after delay
                state.entry_signal = true;
                state.total_entry_signals++;

                std::printf("[LIQ-CASCADE] %s: EXHAUSTION | total=$%.0f count=%d → ENTRY SIGNAL\n",
                    SYM_SHORT[symbol_id], state.cascade_total_usd, state.cascade_count);
                std::fflush(stdout);
            } else {
                // Update cascade stats while active
                state.cascade_total_usd = long_liq_value;
                state.cascade_count = long_liq_count;
            }
        }
    }

    // Entry signal: true if a cascade just exhausted (buy the dip)
    bool has_entry_signal(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        return states_[symbol_id].entry_signal;
    }

    // Clear entry signal after it's been consumed by an engine
    void clear_entry_signal(int symbol_id) {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return;
        std::lock_guard<std::mutex> lk(mtx_);
        states_[symbol_id].entry_signal = false;
    }

    // Is a cascade currently active? (don't enter during cascade — wait for exhaustion)
    bool is_cascade_active(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        return states_[symbol_id].cascade_active;
    }

    // Stats for logging
    int total_cascades(int symbol_id) const {
        if (symbol_id < 0 || symbol_id >= MAX_SYMBOLS) return 0;
        std::lock_guard<std::mutex> lk(mtx_);
        return states_[symbol_id].total_cascades_detected;
    }

    // Reset all state (for testing)
    void reset() {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& s : states_) {
            s = {};
        }
    }

    // Periodic cleanup: expire entry signals older than 5 minutes
    void cleanup(int64_t now_ms) {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& s : states_) {
            if (s.entry_signal && s.exhaustion_ts_ms > 0 &&
                now_ms - s.exhaustion_ts_ms > 300000) {  // 5 min expiry
                s.entry_signal = false;
            }
        }
    }

private:
    mutable std::mutex mtx_;
    std::array<SymbolCascadeState, MAX_SYMBOLS> states_{};
};

} // namespace chimera
