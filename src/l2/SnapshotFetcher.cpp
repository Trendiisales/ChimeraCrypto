#include "l2/SnapshotFetcher.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace chimera {

size_t SnapshotFetcher::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool SnapshotFetcher::fetch(const std::string& symbol, Snapshot& out) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[SNAPSHOT_ERROR] curl_easy_init failed for %s\n", symbol.c_str());
        return false;
    }

    std::string url = "https://api.binance.com/api/v3/depth?symbol=" + symbol + "&limit=1000";
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    printf("[SNAPSHOT] Fetching %s from %s\n", symbol.c_str(), url.c_str());
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        printf("[SNAPSHOT_ERROR] %s fetch failed: curl_code=%d http_code=%ld\n", 
               symbol.c_str(), res, http_code);
        return false;
    }

    if (http_code != 200) {
        printf("[SNAPSHOT_ERROR] %s HTTP error: %ld\n", symbol.c_str(), http_code);
        printf("[SNAPSHOT_ERROR] Response: %.500s\n", response.c_str());
        return false;
    }

    printf("[SNAPSHOT] Got response for %s, size=%zu bytes\n", symbol.c_str(), response.size());

    out.symbol = symbol;
    return parse_snapshot(response, out);
}

bool SnapshotFetcher::parse_snapshot(const std::string& json, Snapshot& out) {
    const char* s = json.c_str();

    // Extract lastUpdateId
    const char* id_ptr = std::strstr(s, "\"lastUpdateId\":");
    if (!id_ptr) {
        printf("[SNAPSHOT_ERROR] lastUpdateId not found\n");
        return false;
    }
    out.lastUpdateId = std::strtoull(id_ptr + 15, nullptr, 10);

    // Parse bids: format is "bids":[[67645.69000000,1.55855000],...]
    const char* bids_ptr = std::strstr(s, "\"bids\":");
    if (bids_ptr) {
        const char* p = bids_ptr + 7;
        while (*p && *p != '[') p++;  // Find array start
        if (*p == '[') p++;
        
        while (*p) {
            while (*p == ' ' || *p == '\n') p++;
            if (*p == ']') break;
            if (*p != '[') { p++; continue; }
            p++;  // Skip [
            
            // Parse price (no quotes in actual response)
            while (*p == ' ' || *p == '\"') p++;
            double price = atof(p);
            
            // Skip to comma
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
            
            // Parse quantity (no quotes)
            while (*p == ' ' || *p == '\"') p++;
            double qty = atof(p);
            
            if (price > 0 && qty > 0) {
                out.bids.push_back({price, qty});
            }
            
            // Skip to closing bracket
            while (*p && *p != ']') p++;
            if (*p == ']') p++;
        }
    }

    // Parse asks: format is "asks":[[price,qty],...]
    const char* asks_ptr = std::strstr(s, "\"asks\":");
    if (asks_ptr) {
        const char* p = asks_ptr + 7;
        while (*p && *p != '[') p++;  // Find array start
        if (*p == '[') p++;
        
        while (*p) {
            while (*p == ' ' || *p == '\n') p++;
            if (*p == ']') break;
            if (*p != '[') { p++; continue; }
            p++;  // Skip [
            
            // Parse price (no quotes)
            while (*p == ' ' || *p == '\"') p++;
            double price = atof(p);
            
            // Skip to comma
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
            
            // Parse quantity (no quotes)
            while (*p == ' ' || *p == '\"') p++;
            double qty = atof(p);
            
            if (price > 0 && qty > 0) {
                out.asks.push_back({price, qty});
            }
            
            // Skip to closing bracket
            while (*p && *p != ']') p++;
            if (*p == ']') p++;
        }
    }

    printf("[SNAPSHOT] Parsed: lastUpdateId=%lu, bids=%zu, asks=%zu\n",
           out.lastUpdateId, out.bids.size(), out.asks.size());

    if (out.bids.empty() || out.asks.empty()) {
        printf("[SNAPSHOT_ERROR] Empty after parse\n");
        printf("[SNAPSHOT_DEBUG] %.200s\n", s);
    }

    return !out.bids.empty() && !out.asks.empty();
}

}
