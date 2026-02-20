#include "recon/ExchangeTruth.hpp"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <cstdlib>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace chimera {

static size_t write_cb(void* contents,
                       size_t size,
                       size_t nmemb,
                       void* userp)
{
    ((std::string*)userp)->append(
        (char*)contents,
        size * nmemb);
    return size * nmemb;
}

ExchangeTruth::ExchangeTruth()
{
    const char* k = std::getenv("BINANCE_API_KEY");
    const char* s = std::getenv("BINANCE_SECRET_KEY");

    if (k) api_key_ = k;
    if (s) secret_key_ = s;
}

std::string ExchangeTruth::sign(
    const std::string& query) const
{
    unsigned char* digest;
    digest = HMAC(EVP_sha256(),
                  secret_key_.c_str(),
                  secret_key_.length(),
                  (unsigned char*)query.c_str(),
                  query.length(),
                  NULL,
                  NULL);

    std::ostringstream oss;
    for (int i = 0; i < 32; i++)
        oss << std::hex << std::setw(2)
            << std::setfill('0')
            << (int)digest[i];

    return oss.str();
}

std::string ExchangeTruth::http_get(
    const std::string& url,
    const std::string& header) const
{
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl)
        return "";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers,
        header.c_str());

    curl_easy_setopt(curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(curl,
        CURLOPT_URL,
        url.c_str());

    curl_easy_setopt(curl,
        CURLOPT_WRITEFUNCTION,
        write_cb);

    curl_easy_setopt(curl,
        CURLOPT_WRITEDATA,
        &response);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return response;
}

bool ExchangeTruth::refresh_account()
{
    balances_.clear();

    long ts =
        std::chrono::duration_cast<
        std::chrono::milliseconds>(
        std::chrono::system_clock::now()
        .time_since_epoch()).count();

    std::string query =
        "timestamp=" + std::to_string(ts);

    std::string signature =
        sign(query);

    std::string url =
        "https://api.binance.com/api/v3/account?"
        + query + "&signature=" + signature;

    std::string header =
        "X-MBX-APIKEY: " + api_key_;

    std::string res =
        http_get(url, header);

    if (res.empty())
        return false;

    size_t pos = 0;
    while ((pos = res.find("\"asset\":\"", pos))
           != std::string::npos)
    {
        pos += 9;
        size_t end = res.find("\"", pos);
        std::string asset =
            res.substr(pos, end - pos);

        size_t free_pos =
            res.find("\"free\":\"", end);
        free_pos += 8;
        size_t free_end =
            res.find("\"", free_pos);

        double free_balance =
            std::stod(
            res.substr(free_pos,
            free_end - free_pos));

        balances_[asset] =
            free_balance;

        pos = free_end;
    }

    return true;
}

bool ExchangeTruth::refresh_open_orders()
{
    open_orders_.clear();

    long ts =
        std::chrono::duration_cast<
        std::chrono::milliseconds>(
        std::chrono::system_clock::now()
        .time_since_epoch()).count();

    std::string query =
        "timestamp=" + std::to_string(ts);

    std::string signature =
        sign(query);

    std::string url =
        "https://api.binance.com/api/v3/openOrders?"
        + query + "&signature=" + signature;

    std::string header =
        "X-MBX-APIKEY: " + api_key_;

    std::string res =
        http_get(url, header);

    if (res.empty())
        return false;

    size_t pos = 0;
    while ((pos = res.find("\"symbol\":\"", pos))
           != std::string::npos)
    {
        OpenOrder ord;

        pos += 10;
        size_t end = res.find("\"", pos);
        ord.symbol = res.substr(pos, end - pos);

        size_t side_pos =
            res.find("\"side\":\"", end);
        side_pos += 8;
        size_t side_end =
            res.find("\"", side_pos);
        ord.side =
            res.substr(side_pos,
            side_end - side_pos);

        size_t price_pos =
            res.find("\"price\":\"", side_end);
        price_pos += 9;
        size_t price_end =
            res.find("\"", price_pos);
        ord.price =
            std::stod(
            res.substr(price_pos,
            price_end - price_pos));

        size_t qty_pos =
            res.find("\"origQty\":\"", price_end);
        qty_pos += 11;
        size_t qty_end =
            res.find("\"", qty_pos);
        ord.orig_qty =
            std::stod(
            res.substr(qty_pos,
            qty_end - qty_pos));

        size_t exec_pos =
            res.find("\"executedQty\":\"", qty_end);
        exec_pos += 15;
        size_t exec_end =
            res.find("\"", exec_pos);
        ord.executed_qty =
            std::stod(
            res.substr(exec_pos,
            exec_end - exec_pos));

        open_orders_.push_back(ord);
        pos = exec_end;
    }

    return true;
}

double ExchangeTruth::asset_balance(
    const std::string& asset) const
{
    auto it = balances_.find(asset);
    if (it == balances_.end())
        return 0.0;

    return it->second;
}

bool ExchangeTruth::balance_aligned(
    const std::string& asset,
    double engine_balance,
    double tolerance) const
{
    auto it = balances_.find(asset);
    if (it == balances_.end())
        return true;

    return std::fabs(it->second -
                     engine_balance)
           <= tolerance;
}

const std::vector<OpenOrder>&
ExchangeTruth::open_orders() const
{
    return open_orders_;
}

}
