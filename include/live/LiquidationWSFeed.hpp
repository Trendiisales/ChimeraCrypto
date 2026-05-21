#pragma once
// ============================================================================
// LiquidationWSFeed.hpp — Session 30, Edge 7
//
// Subscribes to Binance Futures liquidation stream:
//   wss://fstream.binance.com/ws/!forceOrder@arr
//
// Each event looks like:
// {
//   "e": "forceOrder",
//   "E": 1734567890123,
//   "o": {
//     "s": "BTCUSDT",       // symbol
//     "S": "SELL",          // side (SELL = long liquidated, BUY = short liquidated)
//     "o": "LIMIT",         // order type
//     "f": "IOC",
//     "q": "0.010",         // quantity
//     "p": "60000.00",      // price
//     "ap": "59800.00",     // average price
//     "X": "FILLED",        // status
//     "l": "0.010",         // last filled qty
//     "z": "0.010",         // cumulative filled qty
//     "T": 1734567890100    // trade time
//   }
// }
//
// When side is "SELL", a long position was liquidated (forced selling = bearish pressure).
// When side is "BUY", a short position was liquidated (forced buying = bullish pressure).
//
// For our spot-long system, we care about SELL liquidations (long cascades)
// because they create temporary downward price dislocations we can buy.
// ============================================================================

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../core/SymbolIndex.hpp"

namespace chimera {

class LiquidationWSFeed {
public:
    struct LiqEvent {
        int     symbol_id;
        double  price;
        double  qty;
        bool    is_long;     // true = long liquidated (SELL side)
        int64_t ts_ms;
    };

    using LiqCallback = std::function<void(const LiqEvent&)>;

    LiquidationWSFeed() = default;

    void set_callback(LiqCallback cb) { callback_ = std::move(cb); }

    // Start the feed in a background thread.
    // Uses a lightweight curl-based approach (no libwebsockets dependency for this stream).
    // In production, this would share the LWS event loop with BinanceWSFeed.
    // For now, uses a simple reconnecting curl websocket or raw socket.
    void start() {
        if (running_) return;
        running_ = true;
        thread_ = std::thread([this]() { run_loop_(); });
        thread_.detach();
        std::printf("[LIQ-FEED] Liquidation stream started (wss://fstream.binance.com/ws/!forceOrder@arr)\n");
        std::fflush(stdout);
    }

    void stop() {
        running_ = false;
    }

    bool is_running() const { return running_.load(); }
    int64_t events_received() const { return events_received_.load(); }

private:
    LiqCallback callback_;
    std::atomic<bool> running_{false};
    std::atomic<int64_t> events_received_{0};
    std::thread thread_;

    void run_loop_() {
        // Check if websocat is available before attempting WS connection
        int which_ret = ::system("which websocat > /dev/null 2>&1");
        if (which_ret != 0) {
            std::printf("[LIQ-FEED] websocat not installed — using REST polling (every 10s)\n");
            std::fflush(stdout);
            rest_poll_loop_();
            return;
        }

        // Reconnecting loop using websocat
        int consecutive_failures = 0;
        while (running_) {
            std::printf("[LIQ-FEED] Connecting to liquidation stream via websocat...\n");
            std::fflush(stdout);

            FILE* pipe = popen(
                "websocat -t --ping-interval 20 "
                "'wss://fstream.binance.com/ws/!forceOrder@arr' 2>/dev/null",
                "r"
            );

            if (!pipe) {
                std::printf("[LIQ-FEED] popen failed — falling back to REST polling\n");
                std::fflush(stdout);
                rest_poll_loop_();
                return;
            }

            char buf[4096];
            bool got_data = false;
            while (running_ && fgets(buf, sizeof(buf), pipe)) {
                parse_event_(buf);
                got_data = true;
                consecutive_failures = 0;
            }

            pclose(pipe);

            if (!got_data) {
                consecutive_failures++;
                if (consecutive_failures >= 3) {
                    std::printf("[LIQ-FEED] 3 consecutive connection failures — "
                               "falling back to REST polling\n");
                    std::fflush(stdout);
                    rest_poll_loop_();
                    return;
                }
            }

            if (running_) {
                int backoff = 5 * consecutive_failures;
                if (backoff < 5) backoff = 5;
                if (backoff > 60) backoff = 60;
                std::printf("[LIQ-FEED] Disconnected, reconnecting in %ds...\n", backoff);
                std::fflush(stdout);
                std::this_thread::sleep_for(std::chrono::seconds(backoff));
            }
        }
    }

    void rest_poll_loop_() {
        // Fallback: poll /fapi/v1/allForceOrders every 5 seconds
        while (running_) {
            int ret = ::system(
                "curl -s 'https://fapi.binance.com/fapi/v1/allForceOrders?limit=20' "
                "> /tmp/chimera_liq_poll.json 2>/dev/null"
            );
            (void)ret;

            FILE* f = fopen("/tmp/chimera_liq_poll.json", "r");
            if (f) {
                char content[32768];
                size_t n = fread(content, 1, sizeof(content) - 1, f);
                content[n] = '\0';
                fclose(f);
                parse_rest_response_(content);
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void parse_event_(const char* json) {
        // Quick parse: extract symbol, side, quantity, price
        // "s":"BTCUSDT" ... "S":"SELL" ... "q":"0.010" ... "p":"60000.00"

        // Find symbol
        const char* sym_pos = strstr(json, "\"s\":\"");
        if (!sym_pos) return;
        sym_pos += 5;
        char sym_buf[20];
        int i = 0;
        while (*sym_pos && *sym_pos != '"' && i < 19) sym_buf[i++] = *sym_pos++;
        sym_buf[i] = '\0';

        // Convert to lowercase for sym_id lookup
        std::string sym_lower;
        for (int j = 0; j < i; j++) sym_lower += (char)tolower(sym_buf[j]);
        int symbol_id = sym_id(sym_lower);
        if (symbol_id < 0) return;  // not one of our tracked symbols

        // Find side
        const char* side_pos = strstr(json, "\"S\":\"");
        if (!side_pos) return;
        side_pos += 5;
        bool is_long = (side_pos[0] == 'S'); // "SELL" = long liquidated

        // Find quantity
        double qty = 0.0;
        const char* qty_pos = strstr(json, "\"q\":\"");
        if (qty_pos) {
            qty_pos += 5;
            qty = atof(qty_pos);
        }

        // Find price
        double price = 0.0;
        const char* px_pos = strstr(json, "\"p\":\"");
        if (px_pos) {
            px_pos += 5;
            price = atof(px_pos);
        }

        if (qty <= 0.0 || price <= 0.0) return;

        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        events_received_.fetch_add(1, std::memory_order_relaxed);

        if (callback_) {
            callback_({symbol_id, price, qty, is_long, now_ms});
        }
    }

    void parse_rest_response_(const char* json) {
        // REST response is an array of objects with same fields
        // Parse each order in the array
        const char* p = json;
        while ((p = strstr(p, "\"symbol\":\"")) != nullptr) {
            // Extract a chunk for this order
            const char* start = p;
            const char* end = strstr(p + 1, "\"symbol\":\"");
            if (!end) end = json + strlen(json);

            char chunk[2048];
            int len = (int)(end - start);
            if (len >= (int)sizeof(chunk)) len = sizeof(chunk) - 1;
            memcpy(chunk, start, len);
            chunk[len] = '\0';

            // Reuse stream parser on chunk
            parse_event_(chunk);
            p = start + 10; // advance past current "symbol"
        }
    }
};

} // namespace chimera
