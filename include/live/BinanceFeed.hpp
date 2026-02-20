#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace chimera {

struct MarketTick {
    std::string symbol;
    double bid;
    double ask;
    double bid_size;
    double ask_size;
    double last_price;
    uint64_t timestamp;
    double rtt_ms;  // ADD THIS
};

class BinanceFeed {
public:
    using TickCallback = std::function<void(const MarketTick&)>;

    BinanceFeed();
    ~BinanceFeed();

    void add_symbol(const std::string& symbol);
    void set_callback(TickCallback cb);
    void start();
    void stop();

private:
    void run();

    std::vector<std::string> symbols_;
    TickCallback callback_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::unordered_map<std::string, MarketTick> last_ticks_;
};

}
