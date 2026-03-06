#ifndef DEEPSTRIKE_BINANCECONNECTOR_HPP
#define DEEPSTRIKE_BINANCECONNECTOR_HPP

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include "../market/WebSocketTLS.hpp"

namespace DeepStrike {

struct UnifiedTick {
    std::string symbol;
    double bid{0};
    double ask{0};
    double last{0};
    double volume{0};
    double imbalance{0};
    double depth_ratio{0};
    long long timestamp{0};
};

class BinanceConnector {
public:
    BinanceConnector();

    // Start streaming all symbols
    bool start();

private:
    WebSocketTLS ws_;
    std::unordered_map<std::string,int> symbolIndex_;

    std::string build_stream_path();
    void on_message(const std::string& msg);

    void parse_depth(const std::string& sym, const std::string& payload);
    void parse_trade(const std::string& sym, const std::string& payload);
    void parse_bookticker(const std::string& sym, const std::string& payload);

    void emit_tick(const UnifiedTick& t);
};

} // namespace DeepStrike

#endif
