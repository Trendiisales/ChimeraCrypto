#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <libwebsockets.h>

namespace chimera {

// ============================================================================
// MarketTick - Real market data from Binance combined stream
//
// Populated from TWO streams per symbol:
//   @bookTicker  -> best bid/ask + sizes (real order book top)
//   @aggTrade    -> trade price, qty, direction (buyer market maker flag)
//
// ZERO fake/hardcoded values. Fields are 0.0 until the stream delivers them.
// ============================================================================
struct MarketTick {
    std::string symbol;

    // From @bookTicker - real best bid/ask
    double bid       = 0.0;   // Best bid price
    double ask       = 0.0;   // Best ask price
    double bid_size  = 0.0;   // Best bid quantity
    double ask_size  = 0.0;   // Best ask quantity

    // Derived from bid/ask (computed each time bookTicker arrives)
    double mid_price      = 0.0;   // (bid + ask) / 2
    double spread_bps     = 0.0;   // ((ask - bid) / mid) * 10000
    double book_imbalance = 0.0;   // (bid_size - ask_size) / (bid_size + ask_size)
                                    // +1 = strong buy pressure, -1 = strong sell pressure

    // From @aggTrade - directional flow
    double last_price      = 0.0;   // Trade execution price
    double trade_qty       = 0.0;   // Trade quantity
    double agg_buy_volume  = 0.0;   // If buyer was aggressor: trade_qty, else 0
    double agg_sell_volume = 0.0;   // If seller was aggressor: trade_qty, else 0
    bool   is_buyer_maker  = false; // true = sell trade (buyer was maker/passive)
                                    // false = buy trade (buyer was aggressor/taker)

    // Timing
    int64_t timestamp  = 0;     // Local receive time (epoch ms)
    double  rtt_ms     = 0.0;   // Exchange latency p95
    int64_t trade_time = 0;     // Exchange event timestamp (ms)
};

// ============================================================================
// BinanceWSFeed
//
// Subscribes to Binance combined stream:
//   wss://stream.binance.com:9443/stream?streams=
//     btcusdt@bookTicker/btcusdt@aggTrade/
//     ethusdt@bookTicker/ethusdt@aggTrade/
//     solusdt@bookTicker/solusdt@aggTrade
//
// bookTicker: fires on every top-of-book change
// aggTrade:   fires on every matched trade
//
// Both streams update the shared SymbolState for that symbol.
// The callback is fired on every event with the latest merged state,
// so the engine always has current bid/ask AND current trade flow.
// ============================================================================
class BinanceWSFeed {
public:
    using TickCallback = std::function<void(const MarketTick&)>;

    BinanceWSFeed();
    ~BinanceWSFeed();

    void add_symbol(const std::string& symbol);
    void set_callback(TickCallback cb);
    void start();
    void stop();

private:
    void run();

    static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len);

    // Per-symbol merged state
    struct SymbolState {
        MarketTick tick;
        bool book_ready  = false;
        bool trade_ready = false;
    };

    SymbolState& get_or_create(const std::string& symbol);

    // Parse helpers (no regex, no json lib - manual for latency)
    static std::string extract_str(const std::string& msg, const std::string& key);
    static double      extract_dbl(const std::string& msg, const std::string& key);
    static int64_t     extract_i64(const std::string& msg, const std::string& key);
    static bool        extract_bool(const std::string& msg, const std::string& key);

    void handle_book_ticker(const std::string& msg, int64_t recv_ms);
    void handle_agg_trade(const std::string& msg, int64_t recv_ms);

    std::vector<std::string>                     symbols_;
    std::unordered_map<std::string, SymbolState> states_;
    TickCallback                                 callback_;
    std::thread                                  thread_;
    std::atomic<bool>                            running_{false};

    struct lws_context *context_{nullptr};
    struct lws         *wsi_{nullptr};
};

}  // namespace chimera
