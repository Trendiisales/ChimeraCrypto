#pragma once
// MarketTick is defined in BinanceWSFeed.hpp - include that as the single source of truth
#include "live/BinanceWSFeed.hpp"
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace chimera {

// BinanceFeed is a legacy REST-based feed, kept for compilation.
// Active feed is BinanceWSFeed. MarketTick struct lives in BinanceWSFeed.hpp.
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
