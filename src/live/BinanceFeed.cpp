#include "live/BinanceFeed.hpp"
#include <iostream>
#include <curl/curl.h>
#include <cstring>
#include <algorithm>
#include <chrono>

namespace chimera {

BinanceFeed::BinanceFeed() {}
BinanceFeed::~BinanceFeed() { stop(); }

void BinanceFeed::add_symbol(const std::string& symbol) {
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    symbols_.push_back(lower);
}

void BinanceFeed::set_callback(TickCallback cb) { callback_ = cb; }
void BinanceFeed::start() {
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}

void BinanceFeed::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

static size_t curl_write(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

void BinanceFeed::run() {
    CURL* curl = curl_easy_init();
    if (!curl) return;
    
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
    
    int cycle = 0;
    
    while (running_) {
        for (const auto& symbol : symbols_) {
            std::string url = "https://api.binance.com/api/v3/ticker/bookTicker?symbol=";
            std::string upper = symbol;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            url += upper;
            
            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            
            auto req_start = std::chrono::high_resolution_clock::now();
            CURLcode res = curl_easy_perform(curl);
            auto req_end = std::chrono::high_resolution_clock::now();
            
            double req_ms = std::chrono::duration<double, std::milli>(req_end - req_start).count();
            
            if (res != CURLE_OK || response.empty()) continue;
            
            MarketTick tick;
            tick.symbol = symbol;
            tick.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            tick.rtt_ms = req_ms;
            
            size_t pos;
            if ((pos = response.find("\"bidPrice\":\"")) != std::string::npos) {
                pos += 12;
                tick.bid = std::stod(response.substr(pos, response.find("\"", pos) - pos));
            }
            if ((pos = response.find("\"askPrice\":\"")) != std::string::npos) {
                pos += 12;
                tick.ask = std::stod(response.substr(pos, response.find("\"", pos) - pos));
            }
            if ((pos = response.find("\"bidQty\":\"")) != std::string::npos) {
                pos += 10;
                tick.bid_size = std::stod(response.substr(pos, response.find("\"", pos) - pos));
            }
            if ((pos = response.find("\"askQty\":\"")) != std::string::npos) {
                pos += 10;
                tick.ask_size = std::stod(response.substr(pos, response.find("\"", pos) - pos));
            }
            
            tick.last_price = (tick.bid + tick.ask) / 2.0;
            
            if (callback_) callback_(tick);
            
            if (cycle % 10 == 0) {
                std::cout << "[" << symbol << "] $" << tick.last_price 
                         << " RTT=" << req_ms << "ms\n" << std::flush;
            }
        }
        
        cycle++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    curl_easy_cleanup(curl);
}

}
