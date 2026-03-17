#pragma once
#include <string>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <atomic>
#include <mutex>
#include <sstream>
#include <fstream>
#include <functional>
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <cmath>
#include <iomanip>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace chimera {

struct OrderResult {
    bool        ok            = false;
    bool        shadow        = false;
    long        order_id      = 0;
    std::string client_id;
    std::string status;
    double      executed_qty  = 0.0;
    double      avg_price     = 0.0;
    double      limit_price   = 0.0;
    std::string order_type;
    std::string error;
};

struct AccountBalance {
    double btc_free  = 0.0;
    double eth_free  = 0.0;
    double sol_free  = 0.0;
    double usdt_free = 0.0;
    bool   ok        = false;
};

class BinanceREST {
public:
    struct SymbolFilters {
        bool loaded = false;
        double tick_size = 0.0;
        double min_price = 0.0;
        double max_price = 0.0;
        double step_size = 0.0;
        double min_qty = 0.0;
        double max_qty = 0.0;
        double min_notional = 0.0;
        int tick_decimals = 8;
        int qty_decimals = 8;
    };

    bool load_credentials(const std::string& path,
                          std::optional<bool> shadow_override = std::nullopt,
                          bool shadow_validate_on_exchange = false) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::fprintf(stderr, "[REST] Cannot open credentials file: %s\n", path.c_str());
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());

        api_key_ = extract_json_string(content, "api_key");
        api_secret_ = extract_json_string(content, "api_secret");
        if (api_secret_.empty()) {
            api_secret_ = extract_json_string(content, "secret_key");
        }

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
        if (shadow_override.has_value()) {
            shadow_mode_ = *shadow_override;
        }
        shadow_validate_on_exchange_ = shadow_mode_ && shadow_validate_on_exchange;

        if (api_key_.empty() || api_key_ == "YOUR_BINANCE_API_KEY_HERE") {
            std::fprintf(stderr, "[REST] No API key set in %s\n", path.c_str());
            return false;
        }
        if (api_secret_.empty() ||
            api_secret_ == "YOUR_BINANCE_API_SECRET_HERE" ||
            api_secret_ == "YOUR_BINANCE_SECRET_KEY_HERE") {
            std::fprintf(stderr, "[REST] No API secret set in %s\n", path.c_str());
            return false;
        }

        std::printf("[REST] Key loaded: ...%s | Secret loaded: %s\n",
                    api_key_.size() > 8 ? api_key_.substr(api_key_.size() - 8).c_str() : "??",
                    api_secret_.empty() ? "NO" : "YES");
        std::printf("[REST] Credentials loaded. shadow_mode=%s | shadow_transport=%s\n",
                    shadow_mode_ ? "TRUE (no real orders)" : "FALSE (LIVE TRADING)",
                    shadow_validate_on_exchange_ ? "BINANCE_ORDER_TEST" : "LOCAL_SIM");
        std::fflush(stdout);

        curl_global_init(CURL_GLOBAL_DEFAULT);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            shadow_orders_.clear();
        }
        ready_ = true;
        return ping();
    }

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

    OrderResult place_order(const std::string& symbol,
                            bool is_buy,
                            double qty,
                            const std::string& client_id,
                            const std::string& order_type = "MARKET",
                            double limit_price = 0.0) {
        OrderResult result;
        result.shadow = shadow_mode_;
        result.client_id = client_id;
        result.limit_price = limit_price;
        result.order_type = order_type;

        if (!ready_) {
            result.error = "not_ready";
            return result;
        }

        std::lock_guard<std::mutex> lk(mtx_);

        const std::string side = is_buy ? "BUY" : "SELL";
        const std::string ts = timestamp_ms();
        const SymbolFilters filters = get_symbol_filters(symbol);
        const bool requires_exchange_filters = !shadow_mode_ || shadow_validate_on_exchange_;

        if (requires_exchange_filters && !filters.loaded) {
            result.error = "exchange_filters_unavailable";
            std::fprintf(stderr,
                         "[REST] exchangeInfo unavailable | %s | refusing %s %s until filters load\n",
                         symbol.c_str(), side.c_str(), order_type.c_str());
            return result;
        }

        const double normalized_qty = normalize_qty(qty, filters);
        if (normalized_qty <= 0.0) {
            result.error = "qty_below_exchange_min";
            return result;
        }

        double normalized_limit_price = limit_price;
        if (order_type == "LIMIT_MAKER") {
            normalized_limit_price = normalize_price(limit_price, filters, is_buy);
            if (normalized_limit_price <= 0.0) {
                result.error = "price_below_exchange_min";
                return result;
            }
        }

        if (filters.min_notional > 0.0 && normalized_limit_price > 0.0) {
            if (normalized_qty * normalized_limit_price + 1e-12 < filters.min_notional) {
                result.error = "notional_below_exchange_min";
                return result;
            }
        }

        const std::string formatted_qty = format_qty(normalized_qty, filters);
        const std::string formatted_price = (order_type == "LIMIT_MAKER")
            ? format_price(normalized_limit_price, filters)
            : std::string();

        std::ostringstream qs;
        qs << "symbol=" << symbol
           << "&side=" << side
           << "&type=" << order_type
           << "&quantity=" << formatted_qty
           << "&newClientOrderId=" << client_id;

        if (order_type == "LIMIT_MAKER") {
            qs << "&price=" << formatted_price;
        }

        qs << "&recvWindow=5000"
           << "&timestamp=" << ts;

        const std::string payload = qs.str();
        const std::string full_qs = payload + "&signature=" + sign(payload);

        if (shadow_mode_) {
            if (shadow_validate_on_exchange_) {
                std::string body;
                long http_code = 0;
                if (!post("/api/v3/order/test", full_qs, body, http_code)) {
                    result.error = "shadow_order_test_curl_failed";
                    std::fprintf(stderr,
                                 "[SHADOW-ORDER-TEST] request failed | %s | type=%s\n",
                                 symbol.c_str(), order_type.c_str());
                    return result;
                }
                if (http_code != 200) {
                    result.error = body.empty()
                        ? ("shadow_order_test_http_" + std::to_string(http_code))
                        : body;
                    std::fprintf(stderr,
                                 "[SHADOW-ORDER-TEST] rejected | %s | type=%s | qty=%s | price=%s | tick=%.*f | step=%.*f | http=%ld body=%s\n",
                                 symbol.c_str(), order_type.c_str(),
                                 formatted_qty.c_str(),
                                 order_type == "LIMIT_MAKER" ? formatted_price.c_str() : "-",
                                 filters.tick_decimals, filters.tick_size,
                                 filters.qty_decimals, filters.step_size,
                                 http_code, body.c_str());
                    return result;
                }
                std::printf("[SHADOW-ORDER-TEST] POST /api/v3/order/test | %s | type=%s | %s\n",
                            symbol.c_str(), order_type.c_str(), full_qs.c_str());
            } else {
                std::printf("[SHADOW-ORDER] POST /api/v3/order | %s | type=%s | %s\n",
                            symbol.c_str(), order_type.c_str(), full_qs.c_str());
            }
            std::fflush(stdout);

            result.ok = true;
            result.status = (order_type == "LIMIT_MAKER") ? "NEW" : "FILLED";
            result.executed_qty = (order_type == "LIMIT_MAKER") ? 0.0 : normalized_qty;
            result.avg_price = (order_type == "LIMIT_MAKER") ? normalized_limit_price : 0.0;
            result.limit_price = normalized_limit_price;
            result.order_id = synth_shadow_order_id(client_id);
            shadow_orders_[client_id] = result;
            orders_sent_.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        std::string body;
        long http_code = 0;
        if (!post("/api/v3/order", full_qs, body, http_code)) {
            result.error = "curl_failed";
            std::fprintf(stderr, "[REST] order POST failed\n");
            return result;
        }

        if (http_code != 200) {
            result.error = body;
            std::fprintf(stderr, "[REST] order rejected: http=%ld body=%s\n", http_code, body.c_str());
            return result;
        }

        result.ok = true;
        result.order_id = static_cast<long>(extract_json_int(body, "orderId"));
        result.status = extract_json_string(body, "status");
        result.executed_qty = extract_json_double(body, "executedQty");
        result.avg_price = extract_json_double(body, "cummulativeQuoteQty");
        if (result.executed_qty > 1e-12) {
            result.avg_price /= result.executed_qty;
        }
        if (result.avg_price <= 0.0) {
            result.avg_price = (order_type == "LIMIT_MAKER")
                ? normalized_limit_price
                : extract_json_double(body, "price");
        }
        if (result.order_type.empty()) {
            result.order_type = extract_json_string(body, "type");
        }

        std::printf("[LIVE-ORDER] %s %s %.8f | type=%s | id=%ld status=%s avg_px=%.8f limit_px=%.8f\n",
                    side.c_str(), symbol.c_str(), normalized_qty, order_type.c_str(),
                    result.order_id, result.status.c_str(), result.avg_price, normalized_limit_price);
        std::fflush(stdout);

        orders_sent_.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    OrderResult place_limit_maker(const std::string& symbol,
                                  bool is_buy,
                                  double qty,
                                  double limit_price,
                                  const std::string& client_id) {
        return place_order(symbol, is_buy, qty, client_id, "LIMIT_MAKER", limit_price);
    }

    OrderResult get_order(const std::string& symbol,
                          const std::string& client_id) {
        OrderResult result;
        result.shadow = shadow_mode_;
        result.client_id = client_id;

        if (!ready_) {
            result.error = "not_ready";
            return result;
        }
        if (shadow_mode_) {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = shadow_orders_.find(client_id);
            if (it == shadow_orders_.end()) {
                result.error = "shadow_order_not_found";
                return result;
            }
            result = it->second;
            return result;
        }

        std::lock_guard<std::mutex> lk(mtx_);
        std::ostringstream qs;
        qs << "symbol=" << symbol
           << "&origClientOrderId=" << client_id
           << "&recvWindow=5000"
           << "&timestamp=" << timestamp_ms();
        const std::string payload = qs.str();
        const std::string full_qs = payload + "&signature=" + sign(payload);

        std::string body;
        long http_code = 0;
        if (!get("/api/v3/order", full_qs, body, http_code)) {
            result.error = "curl_failed";
            return result;
        }
        if (http_code != 200) {
            result.error = body;
            return result;
        }

        result.ok = true;
        result.order_id = static_cast<long>(extract_json_int(body, "orderId"));
        result.status = extract_json_string(body, "status");
        result.order_type = extract_json_string(body, "type");
        result.limit_price = extract_json_double(body, "price");
        result.executed_qty = extract_json_double(body, "executedQty");
        result.avg_price = extract_json_double(body, "cummulativeQuoteQty");
        if (result.executed_qty > 1e-12) {
            result.avg_price /= result.executed_qty;
        }
        if (result.avg_price <= 0.0) {
            result.avg_price = result.limit_price;
        }
        return result;
    }

    bool shadow_record_fill(const std::string& symbol,
                            bool is_buy,
                            double qty,
                            double fill_price,
                            const std::string& client_id) {
        if (!shadow_mode_) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        OrderResult& result = shadow_orders_[client_id];
        result.ok = true;
        result.shadow = true;
        result.order_id = result.order_id > 0 ? result.order_id : synth_shadow_order_id(client_id);
        result.client_id = client_id;
        result.order_type = result.order_type.empty() ? "LIMIT_MAKER" : result.order_type;
        result.status = "FILLED";
        result.executed_qty = qty;
        result.avg_price = fill_price;
        if (result.limit_price <= 0.0) {
            result.limit_price = fill_price;
        }
        result.error.clear();
        std::printf("[SHADOW-FILL] %s %s %.8f | client_id=%s | fill_px=%.8f\n",
                    is_buy ? "BUY" : "SELL", symbol.c_str(), qty, client_id.c_str(), fill_price);
        std::fflush(stdout);
        return true;
    }

    bool shadow_cancel(const std::string& symbol,
                       const std::string& client_id,
                       double limit_price,
                       const char* reason) {
        if (!shadow_mode_) return false;
        std::lock_guard<std::mutex> lk(mtx_);
        OrderResult& result = shadow_orders_[client_id];
        result.ok = true;
        result.shadow = true;
        result.order_id = result.order_id > 0 ? result.order_id : synth_shadow_order_id(client_id);
        result.client_id = client_id;
        result.order_type = result.order_type.empty() ? "LIMIT_MAKER" : result.order_type;
        result.limit_price = limit_price;
        result.status = "CANCELED";
        result.executed_qty = 0.0;
        result.avg_price = 0.0;
        result.error.clear();
        std::printf("[SHADOW-CANCEL] %s | client_id=%s | limit_px=%.8f | reason=%s\n",
                    symbol.c_str(), client_id.c_str(), limit_price, reason ? reason : "cancelled");
        std::fflush(stdout);
        return true;
    }

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
        if (!del("/api/v3/order", payload, body, http_code) || http_code != 200) {
            std::fprintf(stderr, "[REST] cancel failed: http=%ld body=%s\n", http_code, body.c_str());
            return false;
        }
        return true;
    }

    bool cancel_order(const std::string& symbol,
                      const std::string& client_id) {
        if (!ready_) return true;
        if (shadow_mode_) {
            return shadow_cancel(symbol, client_id, 0.0, "cancel_order");
        }

        std::lock_guard<std::mutex> lk(mtx_);
        std::ostringstream qs;
        qs << "symbol=" << symbol
           << "&origClientOrderId=" << client_id
           << "&recvWindow=5000"
           << "&timestamp=" << timestamp_ms();
        std::string payload = qs.str();
        payload += "&signature=" + sign(payload);

        std::string body;
        long http_code = 0;
        if (!del("/api/v3/order", payload, body, http_code) || http_code != 200) {
            std::fprintf(stderr, "[REST] cancel failed: http=%ld body=%s\n", http_code, body.c_str());
            return false;
        }
        return true;
    }

    bool is_shadow() const { return shadow_mode_; }
    bool is_ready() const { return ready_; }
    bool validates_shadow_orders_on_exchange() const { return shadow_validate_on_exchange_; }
    int orders_sent() const { return orders_sent_.load(); }

private:
    std::string api_key_;
    std::string api_secret_;
    bool shadow_mode_ = true;
    bool shadow_validate_on_exchange_ = false;
    bool ready_ = false;
    std::mutex mtx_;
    std::atomic<int> orders_sent_{0};
    std::unordered_map<std::string, OrderResult> shadow_orders_;
    std::unordered_map<std::string, SymbolFilters> symbol_filters_;

    static long synth_shadow_order_id(const std::string& client_id) {
        return static_cast<long>(
            std::llabs(static_cast<long long>(std::hash<std::string>{}(client_id))) % 2000000000LL);
    }

    std::string sign(const std::string& data) const {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digest_len = 0;
        HMAC(EVP_sha256(),
             api_secret_.data(), static_cast<int>(api_secret_.size()),
             reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()),
             digest, &digest_len);

        char hex[EVP_MAX_MD_SIZE * 2 + 1];
        for (unsigned int i = 0; i < digest_len; ++i) {
            std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
        }
        hex[digest_len * 2] = '\0';
        return std::string(hex);
    }

    static std::string timestamp_ms() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return std::to_string(ms);
    }

    SymbolFilters get_symbol_filters(const std::string& symbol) {
        auto it = symbol_filters_.find(symbol);
        if (it != symbol_filters_.end() && it->second.loaded) {
            return it->second;
        }

        SymbolFilters filters = fetch_symbol_filters(symbol);
        if (filters.loaded) {
            symbol_filters_[symbol] = filters;
        } else {
            symbol_filters_.erase(symbol);
        }
        return filters;
    }

    SymbolFilters fetch_symbol_filters(const std::string& symbol) {
        SymbolFilters filters;
        std::string body;
        long http_code = 0;
        if (!get("/api/v3/exchangeInfo", "symbol=" + symbol, body, http_code)) {
            std::fprintf(stderr, "[REST] exchangeInfo request failed | %s\n", symbol.c_str());
            return filters;
        }
        if (http_code != 200) {
            std::fprintf(stderr, "[REST] exchangeInfo rejected | %s | http=%ld body=%s\n",
                         symbol.c_str(), http_code, body.c_str());
            return filters;
        }

        const std::string tick_size_str = extract_filter_string(body, "PRICE_FILTER", "tickSize");
        const std::string min_price_str = extract_filter_string(body, "PRICE_FILTER", "minPrice");
        const std::string max_price_str = extract_filter_string(body, "PRICE_FILTER", "maxPrice");
        const std::string step_size_str = extract_filter_string(body, "LOT_SIZE", "stepSize");
        const std::string min_qty_str = extract_filter_string(body, "LOT_SIZE", "minQty");
        const std::string max_qty_str = extract_filter_string(body, "LOT_SIZE", "maxQty");
        std::string min_notional_str = extract_filter_string(body, "MIN_NOTIONAL", "minNotional");
        if (min_notional_str.empty()) {
            min_notional_str = extract_filter_string(body, "NOTIONAL", "minNotional");
        }

        filters.tick_size = parse_double(tick_size_str);
        filters.min_price = parse_double(min_price_str);
        filters.max_price = parse_double(max_price_str);
        filters.step_size = parse_double(step_size_str);
        filters.min_qty = parse_double(min_qty_str);
        filters.max_qty = parse_double(max_qty_str);
        filters.min_notional = parse_double(min_notional_str);
        filters.tick_decimals = decimals_from_filter_string(tick_size_str);
        filters.qty_decimals = decimals_from_filter_string(step_size_str);
        filters.loaded = filters.tick_size > 0.0 && filters.step_size > 0.0;

        if (!filters.loaded) {
            std::fprintf(stderr,
                         "[REST] exchangeInfo incomplete | %s | tick=%s step=%s\n",
                         symbol.c_str(), tick_size_str.c_str(), step_size_str.c_str());
            return filters;
        }

        std::printf("[REST] Filters loaded | %s | tick=%s | step=%s | min_qty=%s | min_notional=%s\n",
                    symbol.c_str(),
                    tick_size_str.c_str(),
                    step_size_str.c_str(),
                    min_qty_str.c_str(),
                    min_notional_str.empty() ? "0" : min_notional_str.c_str());
        std::fflush(stdout);
        return filters;
    }

    static std::string extract_filter_string(const std::string& body,
                                             const std::string& filter_type,
                                             const std::string& field) {
        const std::string filter_needle = "\"filterType\":\"" + filter_type + "\"";
        auto pos = body.find(filter_needle);
        if (pos == std::string::npos) return {};

        const std::string field_needle = "\"" + field + "\":\"";
        pos = body.find(field_needle, pos);
        if (pos == std::string::npos) return {};
        pos += field_needle.size();
        auto end = body.find('"', pos);
        if (end == std::string::npos) return {};
        return body.substr(pos, end - pos);
    }

    static double extract_filter_double(const std::string& body,
                                        const std::string& filter_type,
                                        const std::string& field) {
        return parse_double(extract_filter_string(body, filter_type, field));
    }

    static double parse_double(const std::string& value) {
        if (value.empty()) return 0.0;
        try {
            return std::stod(value);
        } catch (...) {
            return 0.0;
        }
    }

    static int decimals_from_filter_string(const std::string& value) {
        auto dot = value.find('.');
        if (dot == std::string::npos) return 0;

        size_t last = value.find_last_not_of('0');
        if (last == std::string::npos || last <= dot) {
            return 0;
        }
        return static_cast<int>(last - dot);
    }

    static double round_to_precision(double value, int decimals) {
        if (decimals <= 0) {
            return std::round(value);
        }
        const double scale = std::pow(10.0, decimals);
        return std::round(value * scale) / scale;
    }

    static double floor_to_step(double value, double step) {
        if (step <= 0.0) return value;
        const double scaled = std::floor((value / step) + 1e-12);
        return scaled * step;
    }

    static double ceil_to_step(double value, double step) {
        if (step <= 0.0) return value;
        const double scaled = std::ceil((value / step) - 1e-12);
        return scaled * step;
    }

    static double normalize_qty(double qty, const SymbolFilters& filters) {
        double out = qty;
        if (filters.step_size > 0.0) {
            out = floor_to_step(out, filters.step_size);
            out = round_to_precision(out, filters.qty_decimals);
        }
        if (filters.min_qty > 0.0 && out + 1e-12 < filters.min_qty) {
            return 0.0;
        }
        if (filters.max_qty > 0.0 && out > filters.max_qty) {
            out = filters.max_qty;
            if (filters.step_size > 0.0) {
                out = floor_to_step(out, filters.step_size);
                out = round_to_precision(out, filters.qty_decimals);
            }
        }
        return out;
    }

    static double normalize_price(double px, const SymbolFilters& filters, bool is_buy) {
        double out = px;
        if (filters.tick_size > 0.0) {
            out = is_buy ? floor_to_step(out, filters.tick_size)
                         : ceil_to_step(out, filters.tick_size);
            out = round_to_precision(out, filters.tick_decimals);
        }
        if (filters.min_price > 0.0 && out + 1e-12 < filters.min_price) {
            return 0.0;
        }
        if (filters.max_price > 0.0 && out > filters.max_price) {
            return filters.max_price;
        }
        return out;
    }

    static std::string format_decimal(double value, int decimals) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(std::max(0, decimals)) << value;
        return out.str();
    }

    static std::string format_qty(double qty, const SymbolFilters& filters) {
        return format_decimal(qty, filters.qty_decimals);
    }

    static std::string format_price(double px, const SymbolFilters& filters) {
        return format_decimal(px, filters.tick_decimals);
    }

    static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* s = static_cast<std::string*>(userdata);
        s->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb;
    }

    bool get(const std::string& path, const std::string& query, std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path;
        if (!query.empty()) url += "?" + query;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode rc = curl_easy_perform(curl);
        if (rc == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return rc == CURLE_OK;
    }

    bool post(const std::string& path, const std::string& query, std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode rc = curl_easy_perform(curl);
        if (rc == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return rc == CURLE_OK;
    }

    bool del(const std::string& path, const std::string& query, std::string& body, long& http_code) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "https://api.binance.com" + path + "?" + query;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        struct curl_slist* headers = nullptr;
        std::string key_header = "X-MBX-APIKEY: " + api_key_;
        headers = curl_slist_append(headers, key_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode rc = curl_easy_perform(curl);
        if (rc == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return rc == CURLE_OK;
    }

    static std::string extract_json_string(const std::string& s, const std::string& key) {
        std::string needle = "\"" + key + "\"";
        auto pos = s.find(needle);
        if (pos == std::string::npos) return std::string();
        pos = s.find(':', pos);
        if (pos == std::string::npos) return std::string();
        pos = s.find_first_of("\"-0123456789", pos + 1);
        if (pos == std::string::npos) return std::string();
        if (s[pos] == '"') {
            auto end = s.find('"', pos + 1);
            if (end == std::string::npos) return std::string();
            return s.substr(pos + 1, end - pos - 1);
        }
        auto end = s.find_first_of(",}", pos);
        return s.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    }

    static double extract_json_double(const std::string& s, const std::string& key) {
        std::string sv = extract_json_string(s, key);
        if (sv.empty()) return 0.0;
        try {
            return std::stod(sv);
        } catch (...) {
            return 0.0;
        }
    }

    static long long extract_json_int(const std::string& s, const std::string& key) {
        std::string sv = extract_json_string(s, key);
        if (sv.empty()) return 0;
        try {
            return std::stoll(sv);
        } catch (...) {
            return 0;
        }
    }

    static double parse_balance(const std::string& body, const std::string& asset) {
        std::string needle = "\"asset\":\"" + asset + "\"";
        auto pos = body.find(needle);
        if (pos == std::string::npos) return 0.0;
        pos = body.find("\"free\":\"", pos);
        if (pos == std::string::npos) return 0.0;
        pos += 8;
        auto end = body.find('"', pos);
        if (end == std::string::npos) return 0.0;
        try {
            return std::stod(body.substr(pos, end - pos));
        } catch (...) {
            return 0.0;
        }
    }
};

} // namespace chimera
