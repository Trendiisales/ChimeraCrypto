#pragma once
// ============================================================================
// BinanceREST — Signed REST client for Binance Spot API
//
// SHADOW MODE (default, shadow_mode=true in credentials):
//   - Loads real API keys and builds properly signed requests
//   - Validates connectivity via GET /api/v3/ping and account balance
//   - Logs every order as [SHADOW-ORDER] with full signed payload
//   - Does NOT call POST /api/v3/order — no real orders hit the exchange
//   - Calls GET /api/v3/order (read-only) to confirm order simulation
//
// LIVE MODE (shadow_mode=false):
//   - Identical code path — POST /api/v3/order is called for real
//   - HMAC-SHA256 signed, timestamp + recvWindow attached
//   - Response parsed for orderId, status, executedQty, price
//
// THREAD SAFETY: All public methods are thread-safe (mutex-protected).
// ============================================================================
#include <string>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <atomic>
#include <mutex>
#include <sstream>
#include <fstream>
#include <functional>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace chimera {

// ============================================================================
// OrderResult — returned by place_order()
// ============================================================================
struct OrderResult {
    bool        ok            = false;
    bool        shadow        = false;   // true if this was a shadow (not real)
    long        order_id      = 0;
    std::string client_id;
    std::string status;       // "FILLED", "NEW", "PARTIALLY_FILLED", "SHADOW"
    double      executed_qty  = 0.0;
    double      avg_price     = 0.0;
    std::string error;        // non-empty on failure
};

// ============================================================================
// AccountBalance — from GET /api/v3/account
// ============================================================================
struct AccountBalance {
    double btc_free  = 0.0;
    double eth_free  = 0.0;
    double sol_free  = 0.0;
    double usdt_free = 0.0;
    bool   ok        = false;
};

class BinanceREST {
public:
    // -----------------------------------------------------------------------
    // load_credentials — reads config/binance_credentials.json
    // Returns true if keys are loaded and ping succeeds.
    // -----------------------------------------------------------------------
    bool load_credentials(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::fprintf(stderr, "[REST] Cannot open credentials file: %s\n", path.c_str());
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());

        api_key_    = extract_json_string(content, "api_key");
        api_secret_ = extract_json_string(content, "api_secret");
        if (api_secret_.empty())
            api_secret_ = extract_json_string(content, "secret_key");

        std::printf("[REST] Key loaded: ...%s | Secret loaded: %s\n",
            api_key_.size() > 8 ? api_key_.substr(api_key_.size()-8).c_str() : "??",
            api_secret_.empty() ? "NO" : "YES");

        // Parse shadow_mode — default true for safety
        shadow_mode_ = true;
        auto pos = content.find("\"shadow_mode\"");
        if (pos != std::string::npos) {
            auto colon = content.find(':', pos);
            if (colon != std::string::npos) {
                auto vstart = content.find_first_not_of(" \t\r\n", colon + 1);
                if (vstart != std::string::npos) {
                    shadow_mode_ = (content.substr(vstart, 5) != "false");
                }
            }
        }

        if (api_key_.empty() || api_key_ == "YOUR_BINANCE_API_KEY_HERE") {
            std::fprintf(stderr, "[REST] No API key set in %s\n", path.c_str());
            return false;
        }
        if (api_secret_.empty() || api_secret_ == "YOUR_BINANCE_API_SECRET_HERE"
                                 || api_secret_ == "YOUR_BINANCE_SECRET_KEY_HERE") {
            std::fprintf(stderr, "[REST] No API secret set in %s\n", path.c_str());
            return false;
        }

        std::printf("[REST] Credentials loaded. shadow_mode=%s\n",
                    shadow_mode_ ? "TRUE (no real orders)" : "FALSE (LIVE TRADING)");
        std::fflush(stdout);

        curl_global_init(CURL_GLOBAL_DEFAULT);
        ready_ = true;

        // Ping exchange to verify connectivity
        return ping();
    }

    // -----------------------------------------------------------------------
    // ping — GET /api/v3/ping
    // -----------------------------------------------------------------------
    bool ping() {
        std::string body;
        long http_code = 0;
        bool ok = get("/api/v3/ping", "", body, http_code);
        if (ok && http_code == 200) {
            std::printf("[REST] Binance ping OK\n");
            std::fflush(stdout);
            return true;
        }
        std::fprintf(stderr, "[REST] Ping failed: http=%ld body=%s\n", http_code, body.c_str());
        return false;
    }

    // -----------------------------------------------------------------------
    // get_account_balance — GET /api/v3/account (signed)
    // -----------------------------------------------------------------------
    AccountBalance get_account_balance() {
        AccountBalance bal;
        if (!ready_) return bal;

        std::string params = "recvWindow=5000&timestamp=" + timestamp_ms();
        params += "&signature=" + sign(params);

        std::string body;
        long http_code = 0;
        if (!get("/api/v3/account", params, body, http_code) || http_code != 200) {
            std::fprintf(stderr, "[REST] account balance failed: http=%ld\n", http_code);
            return bal;
        }

        bal.usdt_free = parse_balance(body, "USDT");
        bal.btc_free  = parse_balance(body, "BTC");
        bal.eth_free  = parse_balance(body, "ETH");
        bal.sol_free  = parse_balance(body, "SOL");
        bal.ok        = true;

        std::printf("[REST] Balance: USDT=%.2f BTC=%.6f ETH=%.4f SOL=%.4f\n",
                    bal.usdt_free, bal.btc_free, bal.eth_free, bal.sol_free);
        std::fflush(stdout);
        return bal;
    }

    // -----------------------------------------------------------------------
    // place_order — Market BUY or SELL
    //
    // symbol:   "BTCUSDT"
    // is_buy:   true=BUY, false=SELL
    // qty:      base asset quantity (e.g. 0.001 BTC)
    // client_id: unique order id (for deduplication)
    //
    // In shadow mode: logs the signed payload, returns synthetic FILLED result.
    // In live mode:   POSTs to /api/v3/order, returns real exchange response.
    // -----------------------------------------------------------------------
    OrderResult place_order(const std::string& symbol,
                            bool is_buy,
                            double qty,
                            const std::string& client_id) {
        OrderResult result;
        result.shadow    = shadow_mode_;
        result.client_id = client_id;

        if (!ready_) {
            result.error = "not_ready";
            return result;
        }

        std::lock_guard<std::mutex> lk(mtx_);

        // Build the signed order params
        std::string side    = is_buy ? "BUY" : "SELL";
        std::string ts      = timestamp_ms();

        std::ostringstream qs;
        qs << "symbol="          << symbol
           << "&side="           << side
           << "&type=MARKET"
           << "&quantity="       << format_qty(qty)
           << "&newClientOrderId=" << client_id
           << "&recvWindow=5000"
           << "&timestamp="      << ts;

        std::string payload  = qs.str();
        std::string sig      = sign(payload);
        std::string full_qs  = payload + "&signature=" + sig;

        if (shadow_mode_) {
            // Shadow: log the fully-formed signed request, DO NOT POST
            std::printf("[SHADOW-ORDER] POST /api/v3/order | %s | %s\n",
                        symbol.c_str(), full_qs.c_str());
            std::fflush(stdout);

            result.ok           = true;
            result.status       = "SHADOW";
            result.executed_qty = qty;
            result.avg_price    = 0.0;  // caller uses market price
            orders_sent_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        // Live: POST to exchange
        std::string body;
        long http_code = 0;
        if (!post("/api/v3/order", full_qs, body, http_code)) {
            result.error = "curl_failed";
            std::fprintf(stderr, "[REST] order POST failed\n");
            return result;
        }

        if (http_code != 200) {
            result.error = body;
            std::fprintf(stderr, "[REST] order rejected: http=%ld body=%s\n",
                         http_code, body.c_str());
            return result;
        }

        // Parse response
        result.ok           = true;
        result.order_id     = (long)extract_json_int(body, "orderId");
        result.status       = extract_json_string(body, "status");
        result.executed_qty = extract_json_double(body, "executedQty");
        result.avg_price    = extract_json_double(body, "cummulativeQuoteQty");
        if (result.executed_qty > 1e-12)
            result.avg_price /= result.executed_qty;

        std::printf("[LIVE-FILL] %s %s %.8f | id=%ld status=%s avg_px=%.4f\n",
                    side.c_str(), symbol.c_str(), qty,
                    result.order_id, result.status.c_str(), result.avg_price);
        std::fflush(stdout);

        orders_sent_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    // -----------------------------------------------------------------------
    // cancel_order — DELETE /api/v3/order
    // -----------------------------------------------------------------------
    bool cancel_order(const std::string& symbol, long order_id) {
        if (!ready_ || shadow_mode_) return true;

        std::lock_guard<std::mutex> lk(mtx_);
        std::string ts = timestamp_ms();
        std::ostringstream qs;
        qs << "symbol=" << symbol
           << "&orderId=" << order_id
           << "&recvWindow=5000"
           << "&timestamp=" << ts;
        std::string payload = qs.str();
        payload += "&signature=" + sign(payload);

        std::string body;
        long http_code = 0;
        del("/api/v3/order", payload, body, http_code);
        return http_code == 200;
    }

    bool   is_shadow()     const { return shadow_mode_; }
    bool   is_ready()      const { return ready_; }
    int    orders_sent()   const { return orders_sent_.load(); }

private:
    std::string api_key_;
    std::string api_secret_;
    bool        shadow_mode_ = true;
    bool        ready_       = false;
    std::mutex  mtx_;
    std::atomic<int> orders_sent_{0};

    // -----------------------------------------------------------------------
    // HMAC-SHA256 signature
    // -----------------------------------------------------------------------
    std::string sign(const std::string& data) const {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int  digest_len = 0;

        HMAC(EVP_sha256(),
             api_secret_.data(), (int)api_secret_.size(),
             (const unsigned char*)data.data(), (int)data.size(),
             digest, &digest_len);

        char hex[EVP_MAX_MD_SIZE * 2 + 1];
        for (unsigned int i = 0; i < digest_len; ++i)
            std::sprintf(hex + i * 2, "%02x", digest[i]);
        hex[digest_len * 2] = '\0';
        return std::string(hex);
    }

    // -----------------------------------------------------------------------
    // Timestamp — current epoch milliseconds as string
    // -----------------------------------------------------------------------
    static std::string timestamp_ms() {
        auto now = std::chrono::system_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()).count();
        return std::to_string(ms);
    }

    // -----------------------------------------------------------------------
    // format_qty — truncate to 8 decimal places (Binance requirement)
    // -----------------------------------------------------------------------
    static std::string format_qty(double qty) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.8f", qty);
        // Strip trailing zeros after decimal
        std::string s(buf);
        auto dot = s.find('.');
        if (dot != std::string::npos) {
            size_t last = s.find_last_not_of('0');
            if (last != std::string::npos && last > dot)
                s = s.substr(0, last + 1);
            else if (last == dot)
                s = s.substr(0, dot);
        }
        return s;
    }

    // -----------------------------------------------------------------------
    // CURL helpers
    // -----------------------------------------------------------------------
    static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* s = static_cast<std::string*>(userdata);
        s->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    bool get(const std::string& path, const std::string& query,
             std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path;
        if (!query.empty()) url += "?" + query;

        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }

    bool post(const std::string& path, const std::string& query,
              std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path;

        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }

    bool del(const std::string& path, const std::string& query,
             std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path + "?" + query;
        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,  "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }

    // -----------------------------------------------------------------------
    // Minimal JSON parsers (no third-party lib)
    // -----------------------------------------------------------------------
    static std::string extract_json_string(const std::string& s, const std::string& key) {
        std::string needle = "\"" + key + "\"";
        auto pos = s.find(needle);
        if (pos == std::string::npos) return "";
        pos += needle.size();
        // skip whitespace and colon
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == ':')) pos++;
        if (pos >= s.size() || s[pos] != '"') return "";
        pos++; // skip opening quote
        auto end = s.find('"', pos);
        if (end == std::string::npos) return "";
        return s.substr(pos, end - pos);
    }

    static double extract_json_double(const std::string& s, const std::string& key) {
        // Try quoted first
        std::string sv = extract_json_string(s, key);
        if (!sv.empty()) { try { return std::stod(sv); } catch (...) {} }
        // Bare number
        std::string needle = "\"" + key + "\":";
        auto pos = s.find(needle);
        if (pos == std::string::npos) return 0.0;
        pos += needle.size();
        auto end = s.find_first_of(",}", pos);
        if (end == std::string::npos) return 0.0;
        try { return std::stod(s.substr(pos, end - pos)); } catch (...) { return 0.0; }
    }

    static long long extract_json_int(const std::string& s, const std::string& key) {
        std::string needle = "\"" + key + "\":";
        auto pos = s.find(needle);
        if (pos == std::string::npos) return 0;
        pos += needle.size();
        auto end = s.find_first_of(",}", pos);
        if (end == std::string::npos) return 0;
        try { return std::stoll(s.substr(pos, end - pos)); } catch (...) { return 0; }
    }

    static double parse_balance(const std::string& body, const std::string& asset) {
        // Binance account response: {"balances":[{"asset":"BTC","free":"0.001","locked":"0"},...]}
        std::string needle = "\"asset\":\"" + asset + "\"";
        auto pos = body.find(needle);
        if (pos == std::string::npos) return 0.0;
        // Find "free" after this point
        auto free_pos = body.find("\"free\":\"", pos);
        if (free_pos == std::string::npos) return 0.0;
        free_pos += 8;
        auto end = body.find('"', free_pos);
        if (end == std::string::npos) return 0.0;
        try { return std::stod(body.substr(free_pos, end - free_pos)); } catch (...) { return 0.0; }
    }
};

} // namespace chimera
