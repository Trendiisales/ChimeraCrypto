#ifndef BINANCE_FEED_HPP
#define BINANCE_FEED_HPP

#include "../market/UnifiedTick.hpp"
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>

namespace DeepStrike {

class BinanceFeed {
public:
    BinanceFeed();
    ~BinanceFeed();
    
    // Initialize with multiple symbols - uses combined stream
    bool init(const std::vector<std::string>& symbols, std::function<void(const UnifiedTick&)> cb);
    
    // Legacy single-symbol init (for compatibility)
    bool init(const std::string& symbol, std::function<void(const UnifiedTick&)> cb);
    
    void start();
    void stop();
    
private:
    void run_loop();
    std::string build_stream_path();
    
    std::vector<std::string> symbols_;
    std::function<void(const UnifiedTick&)> callback_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace DeepStrike

#endif // BINANCE_FEED_HPP
