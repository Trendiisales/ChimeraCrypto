#pragma once
// ============================================================================
// CoinbaseWSFeed.hpp
// Chimera — Coinbase Advanced Trade WebSocket feed (BTC-USD spot price)
//
// PURPOSE:
//   Cross-exchange BTC price reference. Coinbase BTC-USD provides an
//   independent spot price source to detect inter-exchange divergence
//   and validate Binance BTC prices for lead-lag signals.
//
// ENDPOINT:
//   wss://advanced-trade-ws.coinbase.com
//   Channel: "ticker" | Product: "BTC-USD"
//
// CALLBACK:
//   set_callback([](double price, int64_t timestamp_ms) { ... });
//
// USAGE IN main.cpp:
//   chimera::CoinbaseWSFeed coinbase_feed;
//   coinbase_feed.set_callback([&](double price, int64_t ts) {
//       controller.update_coinbase_btc(price, ts);
//   });
//   coinbase_feed.start();
//   // ... main loop ...
//   coinbase_feed.stop();
//
// ARCHITECTURE:
//   - Single libwebsockets connection (same pattern as BinanceWSFeed)
//   - Dedicated thread (detached from main, joins on stop())
//   - Atomic running_ flag for clean shutdown
//   - Reconnect loop: on disconnect, retries after 2s
//   - Manual JSON parse (no external json lib) for latency parity
//
// Thread-safety: callback fires from the lws service thread.
//   Ensure callback is thread-safe (controller.update_coinbase_btc is atomic).
// ============================================================================

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <libwebsockets.h>

namespace chimera {

class CoinbaseWSFeed {
public:
    // callback(price_usd, exchange_timestamp_ms)
    using PriceCallback = std::function<void(double price, int64_t ts_ms)>;

    CoinbaseWSFeed();
    ~CoinbaseWSFeed();

    void set_callback(PriceCallback cb);
    void start();
    void stop();

private:
    void run();

    static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                           void* user, void* in, size_t len);

    // Manual JSON field extractors (consistent with BinanceWSFeed approach)
    static double  extract_dbl(const std::string& msg, const std::string& key);
    static int64_t extract_i64(const std::string& msg, const std::string& key);
    static std::string extract_str(const std::string& msg, const std::string& key);

    void handle_message(const std::string& msg, int64_t recv_ms);

    // Subscribe message sent on connect
    static const char* SUBSCRIBE_MSG;

    PriceCallback        callback_;
    std::thread          thread_;
    std::atomic<bool>    running_{false};

    struct lws_context*  context_{nullptr};
    struct lws*          wsi_{nullptr};

    // Receive buffer — accumulates fragmented lws frames
    std::string          recv_buf_;
};

} // namespace chimera
