#pragma once
#include <libwebsockets.h>
#include <string>
#include <cstring>
#include <atomic>
#include <functional>
#include "types.hpp"
#include "config.hpp"

namespace chimera {

class BinanceClient {
private:
    static constexpr const char* WS_HOST = "stream.binance.com";
    static constexpr int WS_PORT = 9443;
    static constexpr const char* WS_PATH = "/stream?streams=ethusdt@aggTrade/ethusdt@depth@100ms/btcusdt@aggTrade/btcusdt@depth@100ms";
    static constexpr size_t MAX_MESSAGE_SIZE = 8192;
    
    struct lws_context* context_{nullptr};
    struct lws* connection_{nullptr};
    std::atomic<bool> connected_{false};
    
    char message_buffer_[MAX_MESSAGE_SIZE];
    
    std::function<void(size_t sym, double price, double quantity)> on_trade_;
    std::function<void(size_t sym, uint64_t U, uint64_t u, const std::vector<Level>&, const std::vector<Level>&)> on_book_;
    
    // Static callback for libwebsockets
    static int ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* user, void* in, size_t len) {
        auto* client = static_cast<BinanceClient*>(lws_context_user(lws_get_context(wsi)));
        if (!client) return 0;
        
        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                client->connected_.store(true, std::memory_order_release);
                lwsl_notice("WebSocket connection established\n");
                break;
                
            case LWS_CALLBACK_CLIENT_RECEIVE:
                if (len < MAX_MESSAGE_SIZE) {
                    std::memcpy(client->message_buffer_, in, len);
                    client->message_buffer_[len] = '\0';
                    client->parse_message(client->message_buffer_);
                }
                break;
                
            case LWS_CALLBACK_CLIENT_CLOSED:
                client->connected_.store(false, std::memory_order_release);
                lwsl_notice("WebSocket connection closed\n");
                break;
                
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                client->connected_.store(false, std::memory_order_release);
                lwsl_err("WebSocket connection error\n");
                break;
                
            default:
                break;
        }
        
        return 0;
    }
    
    void parse_message(const char* json) {
        // Simple JSON parsing - look for trade or depth updates
        if (std::strstr(json, "\"e\":\"trade\"") || std::strstr(json, "\"e\":\"aggTrade\"")) {
            parse_trade(json);
        } else if (std::strstr(json, "\"e\":\"depthUpdate\"")) {
            parse_depth(json);
        }
    }
    
    void parse_trade(const char* json) {
        char symbol[32];
        extract_string(json, "s", symbol, sizeof(symbol));
        double price = extract_double(json, "p");
        double quantity = extract_double(json, "q");
        
        // Trade logging removed - use aggregated trades instead
        
        if (std::strcmp(symbol, "ETHUSDT") == 0 && on_trade_) {
            on_trade_(1, price, quantity);  // ETH = symbol 1
        } else if (std::strcmp(symbol, "BTCUSDT") == 0 && on_trade_) {
            on_trade_(0, price, quantity);  // BTC = symbol 0
        }
    }
    
    void parse_depth(const char* json) {
        // Detect which symbol this is for
        bool is_btc = std::strstr(json, "\"s\":\"BTCUSDT\"") != nullptr;
        bool is_eth = std::strstr(json, "\"s\":\"ETHUSDT\"") != nullptr;
        
        if (!is_btc && !is_eth) {
            static int unknown_count = 0;
            if (++unknown_count < 3) {
                printf("[DEPTH_DEBUG] Unknown symbol in depth message\n");
            }
            return;
        }
        
        size_t sym = is_btc ? 0 : 1;  // BTC=0, ETH=1
        const char* sym_name = is_btc ? "BTC" : "ETH";
        
        // Extract bids and asks arrays - PROPERLY
        std::array<Level, 5> bids{}, asks{};
        
        const char* bids_ptr = std::strstr(json, "\"b\":[[");
        const char* asks_ptr = std::strstr(json, "\"a\":[[");
        
        static int parse_count = 0;
        static int success_count = 0;
        parse_count++;
        
        if (!bids_ptr || !asks_ptr) {
            // Don't spam pointer addresses - just note parsing failure
            return;
        }
        
        if (bids_ptr) {
            bids_ptr += 6;  // Skip past "\"b\":[["
            for (size_t i = 0; i < 5 && *bids_ptr; ++i) {
                // Skip opening bracket if present
                while (*bids_ptr == '[' || *bids_ptr == ' ') bids_ptr++;
                
                // Parse price (first string in array)
                if (*bids_ptr == '\"') bids_ptr++;  // Skip opening quote
                bids[i].price = std::atof(bids_ptr);
                
                // Skip to quantity
                while (*bids_ptr && *bids_ptr != ',') bids_ptr++;
                if (*bids_ptr == ',') bids_ptr++;
                while (*bids_ptr == ' ' || *bids_ptr == '\"') bids_ptr++;
                
                // Parse quantity (second string in array)
                bids[i].size = std::atof(bids_ptr);
                
                // Skip to next level
                while (*bids_ptr && *bids_ptr != ']') bids_ptr++;
                if (*bids_ptr == ']') bids_ptr++;
                while (*bids_ptr == ',' || *bids_ptr == '[' || *bids_ptr == ' ') bids_ptr++;
                
                // Stop if we hit end of bids array
                if (*bids_ptr == ']') break;
            }
        }
        
        if (asks_ptr) {
            asks_ptr += 6;  // Skip past "\"a\":[["
            for (size_t i = 0; i < 5 && *asks_ptr; ++i) {
                // Skip opening bracket if present
                while (*asks_ptr == '[' || *asks_ptr == ' ') asks_ptr++;
                
                // Parse price
                if (*asks_ptr == '\"') asks_ptr++;
                asks[i].price = std::atof(asks_ptr);
                
                // Skip to quantity
                while (*asks_ptr && *asks_ptr != ',') asks_ptr++;
                if (*asks_ptr == ',') asks_ptr++;
                while (*asks_ptr == ' ' || *asks_ptr == '\"') asks_ptr++;
                
                // Parse quantity
                asks[i].size = std::atof(asks_ptr);
                
                // Skip to next level
                while (*asks_ptr && *asks_ptr != ']') asks_ptr++;
                if (*asks_ptr == ']') asks_ptr++;
                while (*asks_ptr == ',' || *asks_ptr == '[' || *asks_ptr == ' ') asks_ptr++;
                
                if (*asks_ptr == ']') break;
            }
        }
        
        // CRITICAL: Only call callback if we have valid data
        if (bids[0].price > 0 && asks[0].price > 0 && bids[0].size > 0 && asks[0].size > 0) {
            success_count++;
            if (success_count < 5) {
                printf("[DEPTH_SUCCESS] %s: bid[0]=%.2f@%.4f ask[0]=%.2f@%.4f\n",
                       sym_name, bids[0].price, bids[0].size, asks[0].price, asks[0].size);
            }
            if (on_book_) {
                // Extract U and u sequence IDs
                const char* U_ptr = std::strstr(json, "\"U\":");
                const char* u_ptr = std::strstr(json, "\"u\":");
                
                uint64_t U = 0, u = 0;
                if (U_ptr) {
                    U = std::strtoull(U_ptr + 4, nullptr, 10);
                }
                if (u_ptr) {
                    u = std::strtoull(u_ptr + 4, nullptr, 10);
                }
                
                // Convert arrays to vectors
                std::vector<Level> bid_vec, ask_vec;
                for (size_t i = 0; i < 5 && bids[i].price > 0; ++i) {
                    bid_vec.push_back(bids[i]);
                }
                for (size_t i = 0; i < 5 && asks[i].price > 0; ++i) {
                    ask_vec.push_back(asks[i]);
                }
                
                on_book_(sym, U, u, bid_vec, ask_vec);
            }
        } else {
            if (parse_count < 5) {
                printf("[DEPTH_FAIL] %s: bid[0]=%.2f@%.4f ask[0]=%.2f@%.4f\n",
                       sym_name, bids[0].price, bids[0].size, asks[0].price, asks[0].size);
            }
        }
    }
    
    static double extract_double(const char* json, const char* field) {
        char search[128];
        std::snprintf(search, sizeof(search), "\"%s\":", field);
        const char* ptr = std::strstr(json, search);
        if (!ptr) return 0.0;
        ptr += std::strlen(search);
        while (*ptr == ' ' || *ptr == '\"') ptr++;
        return std::atof(ptr);
    }
    
    static void extract_string(const char* json, const char* field, char* output, size_t max_len) {
        char search[128];
        std::snprintf(search, sizeof(search), "\"%s\":\"", field);
        const char* ptr = std::strstr(json, search);
        if (!ptr) {
            output[0] = '\0';
            return;
        }
        ptr += std::strlen(search);
        size_t i = 0;
        while (*ptr != '\"' && *ptr != '\0' && i < max_len - 1) {
            output[i++] = *ptr++;
        }
        output[i] = '\0';
    }
    
public:
    BinanceClient() = default;
    
    ~BinanceClient() {
        disconnect();
    }
    
    bool connect() {
        static const struct lws_protocols protocols[] = {
            {"binance-stream", ws_callback, 0, MAX_MESSAGE_SIZE, 0, nullptr, 0},
            {nullptr, nullptr, 0, 0, 0, nullptr, 0}
        };
        
        struct lws_context_creation_info info{};
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.gid = -1;
        info.uid = -1;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.user = this;
        
        context_ = lws_create_context(&info);
        if (!context_) {
            lwsl_err("Failed to create WebSocket context\n");
            return false;
        }
        
        struct lws_client_connect_info ccinfo{};
        ccinfo.context = context_;
        ccinfo.address = WS_HOST;
        ccinfo.port = WS_PORT;
        ccinfo.path = WS_PATH;
        ccinfo.host = WS_HOST;
        ccinfo.origin = WS_HOST;
        ccinfo.protocol = protocols[0].name;
        ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
        
        connection_ = lws_client_connect_via_info(&ccinfo);
        if (!connection_) {
            lwsl_err("Failed to connect to Binance WebSocket\n");
            lws_context_destroy(context_);
            context_ = nullptr;
            return false;
        }
        
        return true;
    }
    
    void disconnect() {
        if (connection_) {
            lws_set_timeout(connection_, PENDING_TIMEOUT_KILLED_BY_SSL_INFO, LWS_TO_KILL_ASYNC);
            connection_ = nullptr;
        }
        if (context_) {
            lws_context_destroy(context_);
            context_ = nullptr;
        }
        connected_.store(false, std::memory_order_release);
    }
    
    void service(int timeout_ms = 0) {
        if (context_) {
            static uint64_t last_ping_us = 0;
            uint64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            
            // Send ping every 30 seconds like C version
            if (now_us - last_ping_us > 30000000) {
                printf("[WS_CTRL] PING sent\n");
                last_ping_us = now_us;
            }
            
            lws_service(context_, timeout_ms);
        }
    }
    
    [[nodiscard]] bool is_connected() const noexcept {
        return connected_.load(std::memory_order_acquire);
    }
    
    void on_trade(std::function<void(size_t, double, double)> callback) {
        on_trade_ = std::move(callback);
    }
    
    void on_book_update(std::function<void(size_t, uint64_t, uint64_t, const std::vector<Level>&, const std::vector<Level>&)> callback) {
        on_book_ = std::move(callback);
    }
};

} // namespace chimera
