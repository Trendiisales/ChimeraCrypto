#pragma once
// ============================================================================
// PerpFeed.hpp
// Chimera -- Binance Perpetual Futures real-time feed
//
// Connects to: wss://fstream.binance.com/stream?streams=
//   btcusdt@markPrice/btcusdt@aggTrade/
//   ethusdt@markPrice/ethusdt@aggTrade/...
//
// Provides per symbol (thread-safe atomic reads):
//   mark_price(id)     — perp mark price
//   basis_bp(id)       — (mark - spot) / spot * 10000 bp
//   funding_rate(id)   — current funding rate (fractional, e.g. 0.0001)
//   perp_flow_ratio(id)— buy-sell flow EMA ratio, range -1..+1
//   ready(id)          — true once data has arrived
//
// BASIS INTERPRETATION:
//   basis > 0  (contango):  perp trading premium to spot -> longs crowded
//   basis < 0  (backwardo): perp trading discount to spot -> shorts crowded
//   |basis| > 5bp signals strong directional positioning
//
// PERP FLOW INTERPRETATION:
//   perp_flow_ratio > +0.3: aggressive buying on perp -> spot likely follows
//   perp_flow_ratio < -0.3: aggressive selling on perp -> spot likely follows
//
// Architecture: single lws connection, same pattern as BinanceWSFeed.
// Reconnects automatically on disconnect.
// ============================================================================

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <libwebsockets.h>

#include "core/SymbolIndex.hpp"

namespace chimera {

class PerpFeed {
public:
    PerpFeed();
    ~PerpFeed();

    void start();
    void stop();

    // Per-symbol accessors (lock-free atomic reads, safe from any thread)
    double mark_price(int id)      const;
    double basis_bp(int id, double spot_price) const;
    double funding_rate(int id)    const;
    double perp_flow_ratio(int id) const;
    bool   ready(int id)           const;

private:
    void run();

    static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len);

    void handle_message(const std::string& msg, int64_t recv_ms);
    void handle_mark_price(const std::string& msg, int id);
    void handle_agg_trade(const std::string& msg, int id);

    static double  extract_dbl(const std::string& msg, const std::string& key);
    static int64_t extract_i64(const std::string& msg, const std::string& key);
    static bool    extract_bool(const std::string& msg, const std::string& key);

    // Per-symbol atomic state (double stored as uint64 bit-cast, lock-free)
    struct SymState {
        std::atomic<uint64_t> mark_price_bits{0};
        std::atomic<uint64_t> funding_rate_bits{0};
        std::atomic<uint64_t> buy_ema_bits{0};
        std::atomic<uint64_t> sell_ema_bits{0};
        std::atomic<bool>     ready{false};
    };

    static double load_dbl(const std::atomic<uint64_t>& a) {
        uint64_t bits = a.load(std::memory_order_relaxed);
        double v; __builtin_memcpy(&v, &bits, 8); return v;
    }
    static void store_dbl(std::atomic<uint64_t>& a, double v) {
        uint64_t bits; __builtin_memcpy(&bits, &v, 8);
        a.store(bits, std::memory_order_relaxed);
    }

    SymState             state_[MAX_SYMBOLS];

    std::thread          thread_;
    std::atomic<bool>    running_{false};
    struct lws_context*  context_{nullptr};
    struct lws*          wsi_{nullptr};
    std::string          stream_path_;
    std::string          recv_buf_;

    // Liveness watchdog — last wall-clock millisecond we received any WS frame
    // from fstream. Updated in handle_message(). Inspected in run()'s inner
    // service loop; if no message for >60s we force-reconnect (fixes the
    // 2026-05-03 silent-death incident where lws never delivered CLIENT_CLOSED
    // after the perp socket was dropped).
    std::atomic<int64_t> last_msg_ms_{0};
};

} // namespace chimera
