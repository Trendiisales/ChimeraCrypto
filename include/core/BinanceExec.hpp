#ifndef DEEPSTRIKE_BINANCE_EXEC_HPP
#define DEEPSTRIKE_BINANCE_EXEC_HPP

#include <string>
#include <functional>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace DeepStrike {

class BinanceExec {
public:
    BinanceExec();
    ~BinanceExec();

    bool init(const std::string& api_key, const std::string& secret);
    void shutdown();

    // Trading operations
    bool market_buy(const std::string& symbol, double qty);
    bool market_sell(const std::string& symbol, double qty);
    
    // Balance operations
    double get_balance(const std::string& asset = "USDC");
    double get_total_balance();
    
    // Balance callback - called when balance is fetched
    void set_balance_callback(std::function<void(double)> cb) { balance_callback_ = cb; }
    void fetch_and_update_balance();

private:
    bool send_order(const std::string& symbol, const std::string& side, double qty);
    std::string http_post(const std::string& path, const std::string& body);
    std::string http_get_signed(const std::string& path);
    std::string sign_query(const std::string& query);

private:
    std::string api_key_;
    std::string secret_;
    std::function<void(double)> balance_callback_;
    std::atomic<double> last_balance_{0.0};
    
#ifdef _WIN32
    HINTERNET session_ = NULL;
    HINTERNET connect_ = NULL;
#endif
};

} // namespace DeepStrike

#endif // DEEPSTRIKE_BINANCE_EXEC_HPP
