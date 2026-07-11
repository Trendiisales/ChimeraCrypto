#pragma once
// ============================================================================
// ExchangeTimeSync — track the offset between local clock and Binance server
// time, and HALT signed trading when the drift exceeds a threshold.
// (Phase-2 review fix item 6, 2026-07-11.)
//
// Binance signs every order with a `timestamp` and rejects it (code -1021) if
// it falls outside recvWindow of the server clock. A drifting local clock
// therefore silently blocks (or, worse, mistimes) live orders. This tracks the
// measured offset and lets the gateway refuse to submit a SIGNED order while the
// clock is out of tolerance — fail safe, do not fire blind.
//
// The REST probe (GET /api/v3/time) is a live-activated method on BinanceREST;
// this class holds the pure offset/halt logic so it is unit-testable by
// injecting an offset directly (the clock-drift regression test).
// ============================================================================
#include <atomic>
#include <cstdint>

namespace chimera {

class ExchangeTimeSync {
public:
    // Max tolerated |offset| (ms) before signed trading halts. Binance default
    // recvWindow is 5000ms; stay well inside it.
    void set_threshold_ms(int64_t t) { threshold_ms_ = t; }
    int64_t threshold_ms() const { return threshold_ms_; }

    // Record a measured offset = server_time_ms - local_time_ms.
    void record_offset(int64_t offset_ms) {
        offset_ms_.store(offset_ms, std::memory_order_relaxed);
        synced_.store(true, std::memory_order_relaxed);
    }
    // Convenience: compute + store from a matched (server, local) pair.
    void record_pair(int64_t server_ms, int64_t local_ms) {
        record_offset(server_ms - local_ms);
    }

    int64_t offset_ms() const { return offset_ms_.load(std::memory_order_relaxed); }
    bool    synced()    const { return synced_.load(std::memory_order_relaxed); }

    // True => a SIGNED order must NOT be sent. Halts if we have never synced OR
    // the last measured drift exceeds the threshold.
    bool signed_trading_halted() const {
        if (!synced_.load(std::memory_order_relaxed)) return true;
        int64_t o = offset_ms_.load(std::memory_order_relaxed);
        if (o < 0) o = -o;
        return o > threshold_ms_;
    }

private:
    std::atomic<int64_t> offset_ms_{0};
    std::atomic<bool>    synced_{false};
    int64_t              threshold_ms_ = 1000;   // 1s: conservative inside 5s recvWindow
};

} // namespace chimera
