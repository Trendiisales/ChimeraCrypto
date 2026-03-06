#pragma once

#include "chimera/execution/ExchangeIO.hpp"
#include "chimera/execution/Hash.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace chimera {

struct BinanceConfig {
    std::string api_key;
    std::string api_secret;
    std::string rest_url = "https://api.binance.com";
    std::string ws_url = "wss://stream.binance.com:9443/stream";
    bool shadow_mode = true;
};

class BinanceIO : public IExchangeIO {
public:
    explicit BinanceIO(const BinanceConfig& cfg);
    ~BinanceIO();

    void connect() override;
    void disconnect() override;

    void subscribeMarketData(
        const std::vector<std::string>& symbols
    ) override;

    void sendOrder(const OrderRequest& req) override;
    void cancelOrder(const std::string& client_id) override;

    void poll() override;

    std::mutex sym_mutex;
    std::vector<std::string> symbols;

private:
    void wsThread();
    void restSendOrder(const OrderRequest& req);
    void restCancelOrder(const std::string& client_id);

    std::string signQuery(const std::string& query);
    uint64_t nowMs() const;

private:
    BinanceConfig config;

    std::atomic<bool> running;
    std::thread ws_worker;
    
    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    
    std::unique_ptr<boost::beast::websocket::stream<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
    >> ws_;
    std::mutex ws_mutex;

    std::mutex rate_mutex;
    uint64_t last_rest_ts = 0;
};

}
