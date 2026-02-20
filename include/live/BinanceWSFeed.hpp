#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <libwebsockets.h>

namespace chimera {

struct MarketTick {
    std::string symbol;
    double bid;
    double ask;
    double bid_size;
    double ask_size;
    double last_price;
    uint64_t timestamp;
    double rtt_ms;
};

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

    std::vector<std::string> symbols_;
    TickCallback callback_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    
    struct lws_context *context_{nullptr};
    struct lws *wsi_{nullptr};
};

}
