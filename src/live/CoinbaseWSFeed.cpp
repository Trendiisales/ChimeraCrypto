// ============================================================================
// CoinbaseWSFeed.cpp
// Chimera -- Coinbase Advanced Trade WebSocket feed (BTC-USD spot price)
// ============================================================================

#include "live/CoinbaseWSFeed.hpp"

#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>

namespace chimera {

const char* CoinbaseWSFeed::SUBSCRIBE_MSG =
    "{"
        "\"type\":\"subscribe\","
        "\"product_ids\":[\"BTC-USD\"],"
        "\"channel\":\"ticker\""
    "}";

static CoinbaseWSFeed* g_coinbase_feed = nullptr;

CoinbaseWSFeed::CoinbaseWSFeed() {
    g_coinbase_feed = this;
}

CoinbaseWSFeed::~CoinbaseWSFeed() {
    stop();
    g_coinbase_feed = nullptr;
}

void CoinbaseWSFeed::set_callback(PriceCallback cb) {
    callback_ = std::move(cb);
}

void CoinbaseWSFeed::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread([this]{ run(); });
}

void CoinbaseWSFeed::stop() {
    if (!running_.exchange(false)) return;
    if (context_) lws_cancel_service(context_);
    if (thread_.joinable()) thread_.join();
}

int CoinbaseWSFeed::ws_callback(struct lws* wsi,
                                enum lws_callback_reasons reason,
                                void* /*user*/, void* in, size_t len)
{
    if (!g_coinbase_feed) return 0;
    auto* self = g_coinbase_feed;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        std::printf("[COINBASE-WS] Connected to advanced-trade-ws.coinbase.com\n");
        std::fflush(stdout);
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        static bool subscribed = false;
        if (!subscribed) {
            subscribed = true;
            size_t msg_len = std::strlen(SUBSCRIBE_MSG);
            std::vector<unsigned char> buf(LWS_PRE + msg_len);
            std::memcpy(buf.data() + LWS_PRE, SUBSCRIBE_MSG, msg_len);
            lws_write(wsi, buf.data() + LWS_PRE, msg_len, LWS_WRITE_TEXT);
            std::printf("[COINBASE-WS] Subscribed to BTC-USD ticker\n");
            std::fflush(stdout);
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
        auto recv_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        self->recv_buf_.append(static_cast<const char*>(in), len);
        if (lws_is_final_fragment(wsi)) {
            self->handle_message(self->recv_buf_, recv_ms);
            self->recv_buf_.clear();
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        std::printf("[COINBASE-WS] Connection error -- will reconnect\n");
        std::fflush(stdout);
        self->wsi_ = nullptr;
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        std::printf("[COINBASE-WS] Connection closed -- will reconnect\n");
        std::fflush(stdout);
        self->wsi_ = nullptr;
        break;

    default:
        break;
    }
    return 0;
}

void CoinbaseWSFeed::handle_message(const std::string& msg, int64_t recv_ms) {
    if (msg.find("\"channel\":\"ticker\"") == std::string::npos &&
        msg.find("\"type\":\"ticker\"")    == std::string::npos) {
        return;
    }

    double price = extract_dbl(msg, "price");
    if (price <= 0.0) return;

    int64_t ts_ms = recv_ms;
    std::string time_str = extract_str(msg, "time");
    if (!time_str.empty()) {
        struct tm t{};
        int ms_part = 0;
        if (std::sscanf(time_str.c_str(),
                "%4d-%2d-%2dT%2d:%2d:%2d.%dZ",
                &t.tm_year, &t.tm_mon, &t.tm_mday,
                &t.tm_hour, &t.tm_min, &t.tm_sec, &ms_part) >= 6) {
            t.tm_year -= 1900;
            t.tm_mon  -= 1;
            t.tm_isdst = 0;
            time_t epoch = timegm(&t);
            if (epoch > 0)
                ts_ms = static_cast<int64_t>(epoch) * 1000 + (ms_part % 1000);
        }
    }

    if (callback_) callback_(price, ts_ms);
}

void CoinbaseWSFeed::run() {
    static constexpr struct lws_protocols protocols[] = {
        { "coinbase-ticker", ws_callback, 0, 65536, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    while (running_.load(std::memory_order_acquire)) {
        struct lws_context_creation_info ctx_info{};
        ctx_info.port      = CONTEXT_PORT_NO_LISTEN;
        ctx_info.protocols = protocols;
        ctx_info.gid       = -1;
        ctx_info.uid       = -1;
        ctx_info.options   = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

        context_ = lws_create_context(&ctx_info);
        if (!context_) {
            std::printf("[COINBASE-WS] Failed to create lws context -- retrying in 5s\n");
            std::fflush(stdout);
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        struct lws_client_connect_info conn{};
        conn.context        = context_;
        conn.address        = "advanced-trade-ws.coinbase.com";
        conn.port           = 443;
        conn.path           = "/";
        conn.host           = conn.address;
        conn.origin         = conn.address;
        conn.ssl_connection = LCCSCF_USE_SSL;
        conn.protocol       = protocols[0].name;
        conn.pwsi           = &wsi_;

        wsi_ = lws_client_connect_via_info(&conn);
        if (!wsi_) {
            std::printf("[COINBASE-WS] connect failed -- retrying in 5s\n");
            std::fflush(stdout);
            lws_context_destroy(context_);
            context_ = nullptr;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        while (running_.load(std::memory_order_acquire) && wsi_ != nullptr) {
            lws_service(context_, 50);
        }

        lws_context_destroy(context_);
        context_ = nullptr;
        wsi_     = nullptr;

        if (!running_.load(std::memory_order_acquire)) break;

        std::printf("[COINBASE-WS] Reconnecting in 2s...\n");
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::printf("[COINBASE-WS] Feed thread exited cleanly.\n");
    std::fflush(stdout);
}

double CoinbaseWSFeed::extract_dbl(const std::string& msg, const std::string& key) {
    std::string needle_q = "\"" + key + "\":\"";
    auto pos = msg.find(needle_q);
    if (pos != std::string::npos) {
        pos += needle_q.size();
        auto end = msg.find('"', pos);
        if (end != std::string::npos) {
            try { return std::stod(msg.substr(pos, end - pos)); } catch (...) {}
        }
    }
    std::string needle_b = "\"" + key + "\":";
    pos = msg.find(needle_b);
    if (pos != std::string::npos) {
        pos += needle_b.size();
        auto end = msg.find_first_of(",}\"]", pos);
        if (end != std::string::npos) {
            try { return std::stod(msg.substr(pos, end - pos)); } catch (...) {}
        }
    }
    return 0.0;
}

int64_t CoinbaseWSFeed::extract_i64(const std::string& msg, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    auto pos = msg.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    auto end = msg.find_first_of(",}\"]", pos);
    if (end == std::string::npos) return 0;
    try { return std::stoll(msg.substr(pos, end - pos)); } catch (...) { return 0; }
}

std::string CoinbaseWSFeed::extract_str(const std::string& msg, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    auto pos = msg.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    auto end = msg.find('"', pos);
    if (end == std::string::npos) return {};
    return msg.substr(pos, end - pos);
}

} // namespace chimera
