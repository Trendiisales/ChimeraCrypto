#include "live/BinanceWSFeed.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>

namespace chimera {

static BinanceWSFeed* g_feed = nullptr;
static bool debug_printed = false;

BinanceWSFeed::BinanceWSFeed() { g_feed = this; }
BinanceWSFeed::~BinanceWSFeed() { stop(); g_feed = nullptr; }

void BinanceWSFeed::add_symbol(const std::string& symbol) {
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    symbols_.push_back(lower);
}

void BinanceWSFeed::set_callback(TickCallback cb) { callback_ = cb; }
void BinanceWSFeed::start() {
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}
void BinanceWSFeed::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

int BinanceWSFeed::ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                               void *user, void *in, size_t len)
{
    try {
        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                std::cout << "[WS] Connected\n" << std::flush;
                break;

            case LWS_CALLBACK_CLIENT_RECEIVE: {
                if (!g_feed || !g_feed->callback_ || !in || len == 0) break;
                
                // HIGH RESOLUTION TIMER - measure receive time immediately
                auto receive_time = std::chrono::high_resolution_clock::now();
                
                std::string msg((char*)in, len);
                
                // DEBUG: Print first message to see structure
                if (!debug_printed && msg.find("\"data\"") != std::string::npos) {
                    std::cout << "[WS-RAW] " << msg << "\n" << std::flush;
                    debug_printed = true;
                }
                
                if (msg.find("\"data\"") == std::string::npos) break;
                
                MarketTick tick;
                size_t pos;
                
                // bookTicker format from Binance stream:
                // {"stream":"btcusdt@bookTicker","data":{"u":123456789,"s":"BTCUSDT","b":"50000.00","B":"1.5","a":"50001.00","A":"2.0"}}
                // Note: NO "E" (event time) field in bookTicker!
                
                // We must use system time as approximation
                // In production HFT, you'd use hardware timestamps or NIC timestamps
                auto now = std::chrono::system_clock::now();
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                
                tick.timestamp = now_ms;
                
                // Measure processing latency (receive to now)
                auto process_time = std::chrono::high_resolution_clock::now();
                auto process_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    process_time - receive_time).count();
                
                // This is our MINIMUM latency (processing time)
                // Real network latency is unknown without server timestamp
                // Typical Binance WS latency from Tokyo: 1-3ms
                // We'll estimate as processing time + 2ms baseline
                tick.rtt_ms = (static_cast<double>(process_us) / 1000.0) + 2.0;
                
                if ((pos = msg.find("\"s\":\"")) != std::string::npos) {
                    pos += 5;
                    size_t end = msg.find("\"", pos);
                    if (end != std::string::npos && end > pos) {
                        tick.symbol = msg.substr(pos, end - pos);
                        std::transform(tick.symbol.begin(), tick.symbol.end(), tick.symbol.begin(), ::tolower);
                    }
                }
                
                if ((pos = msg.find("\"b\":\"")) != std::string::npos) {
                    pos += 5;
                    size_t end = msg.find("\"", pos);
                    if (end != std::string::npos && end > pos) {
                        try {
                            tick.bid = std::stod(msg.substr(pos, end - pos));
                        } catch (...) {}
                    }
                }
                
                if ((pos = msg.find("\"a\":\"")) != std::string::npos) {
                    pos += 5;
                    size_t end = msg.find("\"", pos);
                    if (end != std::string::npos && end > pos) {
                        try {
                            tick.ask = std::stod(msg.substr(pos, end - pos));
                        } catch (...) {}
                    }
                }
                
                if ((pos = msg.find("\"B\":\"")) != std::string::npos) {
                    pos += 5;
                    size_t end = msg.find("\"", pos);
                    if (end != std::string::npos && end > pos) {
                        try {
                            tick.bid_size = std::stod(msg.substr(pos, end - pos));
                        } catch (...) {}
                    }
                }
                
                if ((pos = msg.find("\"A\":\"")) != std::string::npos) {
                    pos += 5;
                    size_t end = msg.find("\"", pos);
                    if (end != std::string::npos && end > pos) {
                        try {
                            tick.ask_size = std::stod(msg.substr(pos, end - pos));
                        } catch (...) {}
                    }
                }
                
                tick.last_price = (tick.bid + tick.ask) / 2.0;
                
                if (tick.bid > 0 && tick.ask > 0 && !tick.symbol.empty()) {
                    if (g_feed->callback_) {
                        g_feed->callback_(tick);
                    }
                }
                break;
            }

            default: 
                break;
        }
    } catch (...) {}
    
    return 0;
}

void BinanceWSFeed::run() {
    std::string stream_path = "/stream?streams=";
    for (size_t i = 0; i < symbols_.size(); i++) {
        if (i > 0) stream_path += "/";
        std::string lower = symbols_[i];
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        stream_path += lower + "@bookTicker";
    }
    
    static struct lws_protocols protocols[] = {
        { "default", ws_callback, 0, 65536, 0, NULL, 0 },
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };
    
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    
    context_ = lws_create_context(&info);
    if (!context_) return;
    
    struct lws_client_connect_info conn_info;
    memset(&conn_info, 0, sizeof(conn_info));
    conn_info.context = context_;
    conn_info.address = "stream.binance.com";
    conn_info.port = 9443;
    conn_info.path = stream_path.c_str();
    conn_info.host = conn_info.address;
    conn_info.origin = conn_info.address;
    conn_info.ssl_connection = LCCSCF_USE_SSL;
    conn_info.protocol = protocols[0].name;
    conn_info.pwsi = &wsi_;
    
    wsi_ = lws_client_connect_via_info(&conn_info);
    if (!wsi_) {
        lws_context_destroy(context_);
        return;
    }
    
    while (running_) lws_service(context_, 50);
    lws_context_destroy(context_);
}

}
