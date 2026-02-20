#include "net/SnapshotFetcher.hpp"
#include <curl/curl.h>
#include <string>
#include <sstream>

namespace chimera {

size_t SnapshotFetcher::write_callback(void* contents,
                                       size_t size,
                                       size_t nmemb,
                                       void* userp)
{
    size_t total = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total);
    return total;
}

bool SnapshotFetcher::fetch(const std::string& symbol,
                            Snapshot& out_snapshot)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    std::string response;

    std::string url =
        "https://api.binance.com/api/v3/depth?symbol=" +
        symbol + "&limit=10";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);

    // Very minimal naive parser (placeholder for real JSON parsing)
    // We just simulate snapshot for now to avoid full JSON dep

    out_snapshot.bids = { {50000.0, 1.0}, {49990.0, 2.0} };
    out_snapshot.asks = { {50010.0, 1.5}, {50020.0, 1.2} };
    out_snapshot.last_update_id = 1;

    return true;
}

}
