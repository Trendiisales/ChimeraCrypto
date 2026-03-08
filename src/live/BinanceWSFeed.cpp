#include "live/BinanceWSFeed.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <stdexcept>

extern chimera::ExchangeLatencyEngine g_exchange_latency;

namespace chimera {

// ============================================================================
// Global feed pointer for libwebsockets static callback
// ============================================================================
static BinanceWSFeed* g_feed = nullptr;

BinanceWSFeed::BinanceWSFeed()  { g_feed = this; }
BinanceWSFeed::~BinanceWSFeed() { stop(); g_feed = nullptr; }

void BinanceWSFeed::add_symbol(const std::string& symbol) {
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    symbols_.push_back(lower);
}

void BinanceWSFeed::set_callback(TickCallback cb) { callback_ = cb; }

void BinanceWSFeed::start() {
    running_ = true;
    thread_  = std::thread([this]() { run(); });
}

void BinanceWSFeed::stop() {
    running_ = false;
    // Wake lws_service() immediately so the WS thread sees running_=false
    // without waiting for the next 5ms poll cycle to complete.
    // Without this, thread_.join() can hang for up to 5ms per iteration
    // plus whatever libwebsockets is doing internally mid-poll.
    if (context_) {
        lws_cancel_service(context_);
    }
    if (thread_.joinable()) thread_.join();
}

// ============================================================================
// get_or_create - fetch or initialise per-symbol state
// ============================================================================
BinanceWSFeed::SymbolState& BinanceWSFeed::get_or_create(const std::string& symbol) {
    // emplace is a single operation — inserts only if key absent, returns
    // iterator to existing-or-new element with no intermediate rehash.
    // Previously: 3 separate operator[] calls, each could trigger rehash
    // and invalidate references — caused SEGV with 7 symbols under rapid ticks.
    auto [it, inserted] = states_.emplace(symbol, SymbolState{});
    if (inserted) it->second.tick.symbol = symbol;
    return it->second;
}

// ============================================================================
// Manual JSON field extractors
// No third-party JSON lib, no regex. Minimal allocations.
// ============================================================================

// Extract a JSON string value: "key":"value"  -> returns "value"
std::string BinanceWSFeed::extract_str(const std::string& msg, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = msg.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    size_t end = msg.find('"', pos);
    if (end == std::string::npos) return "";
    return msg.substr(pos, end - pos);
}

// Extract a JSON numeric value: "key":123.45  -> returns 123.45
double BinanceWSFeed::extract_dbl(const std::string& msg, const std::string& key) {
    // Handles quoted numeric strings ("key":"123.45") AND bare numbers ("key":123.45)
    // Try quoted first (Binance sends prices as strings)
    std::string s = extract_str(msg, key);
    if (!s.empty()) {
        try { return std::stod(s); } catch (...) {}
    }
    // Fall back to bare number
    std::string needle = "\"" + key + "\":";
    size_t pos = msg.find(needle);
    if (pos == std::string::npos) return 0.0;
    pos += needle.size();
    if (pos < msg.size() && msg[pos] == '"') ++pos; // skip opening quote if present
    size_t end = msg.find_first_of(",}", pos);
    if (end == std::string::npos) return 0.0;
    try { return std::stod(msg.substr(pos, end - pos)); } catch (...) { return 0.0; }
}

// Extract a JSON int64 value: "key":1234567890
int64_t BinanceWSFeed::extract_i64(const std::string& msg, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = msg.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    size_t end = msg.find_first_of(",}", pos);
    if (end == std::string::npos) return 0;
    try { return std::stoll(msg.substr(pos, end - pos)); } catch (...) { return 0; }
}

// Extract a JSON bool value: "key":true / "key":false
bool BinanceWSFeed::extract_bool(const std::string& msg, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = msg.find(needle);
    if (pos == std::string::npos) return false;
    pos += needle.size();
    return (msg.compare(pos, 4, "true") == 0);
}

// ============================================================================
// handle_book_ticker
//
// Binance @bookTicker message format (inside "data" wrapper):
// {
//   "e": "bookTicker",
//   "s": "BTCUSDT",
//   "b": "83123.45",   <- best bid price
//   "B": "0.521",      <- best bid qty
//   "a": "83124.10",   <- best ask price
//   "A": "0.318",      <- best ask qty
//   "T": 1234567890123 <- transaction time ms
// }
// ============================================================================
void BinanceWSFeed::handle_book_ticker(const std::string& msg, int64_t recv_ms) {
    std::string sym = extract_str(msg, "s");
    if (sym.empty()) return;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    double bid      = extract_dbl(msg, "b");
    double bid_sz   = extract_dbl(msg, "B");
    double ask      = extract_dbl(msg, "a");
    double ask_sz   = extract_dbl(msg, "A");
    int64_t trade_t = extract_i64(msg, "T");

    if (bid <= 0.0 || ask <= 0.0 || ask <= bid) return;

    auto& st = get_or_create(sym);
    MarketTick& t = st.tick;

    t.bid      = bid;
    t.ask      = ask;
    t.bid_size = bid_sz;
    t.ask_size = ask_sz;

    // Derived fields - computed from real data
    t.mid_price  = (bid + ask) * 0.5;
    t.spread_bps = ((ask - bid) / t.mid_price) * 10000.0;

    double total_sz = bid_sz + ask_sz;
    t.book_imbalance = (total_sz > 1e-9)
                       ? (bid_sz - ask_sz) / total_sz
                       : 0.0;

    t.trade_time = trade_t;
    t.timestamp  = recv_ms;
    t.rtt_ms     = g_exchange_latency.p95();
    st.book_ready = true;

    // Fire callback immediately - engine wants latest book state
    if (callback_ && st.book_ready) {
        callback_(t);
    }
}

// ============================================================================
// handle_agg_trade
//
// Binance @aggTrade message format (inside "data" wrapper):
// {
//   "e": "aggTrade",
//   "s": "BTCUSDT",
//   "p": "83123.45",   <- trade price
//   "q": "0.012",      <- trade quantity
//   "T": 1234567890123,<- trade time ms
//   "m": true          <- buyer is maker? true = seller was aggressor (SELL trade)
//                                         false = buyer was aggressor (BUY trade)
// }
// ============================================================================
void BinanceWSFeed::handle_agg_trade(const std::string& msg, int64_t recv_ms) {
    std::string sym = extract_str(msg, "s");
    if (sym.empty()) return;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::tolower);

    double  price      = extract_dbl(msg, "p");
    double  qty        = extract_dbl(msg, "q");
    int64_t trade_t    = extract_i64(msg, "T");
    bool    buyer_maker = extract_bool(msg, "m");

    if (price <= 0.0 || qty <= 0.0) return;

    // Update exchange latency with trade timestamp
    g_exchange_latency.record(recv_ms, trade_t);

    auto& st = get_or_create(sym);
    MarketTick& t = st.tick;

    t.last_price      = price;
    t.trade_qty       = qty;
    t.is_buyer_maker  = buyer_maker;

    // buyer_maker = true  -> buyer was passive (limit order on book)
    //                     -> seller crossed the spread -> SELL aggression
    // buyer_maker = false -> buyer crossed the spread  -> BUY aggression
    if (buyer_maker) {
        t.agg_sell_volume = qty;
        t.agg_buy_volume  = 0.0;
    } else {
        t.agg_buy_volume  = qty;
        t.agg_sell_volume = 0.0;
    }

    t.trade_time = trade_t;
    t.timestamp  = recv_ms;
    t.rtt_ms     = g_exchange_latency.p95();
    st.trade_ready = true;

    // Fire callback - trade data has arrived
    if (callback_) {
        callback_(t);
    }
}

// ============================================================================
// ws_callback - libwebsockets event handler
// ============================================================================
int BinanceWSFeed::ws_callback(struct lws *wsi,
                               enum lws_callback_reasons reason,
                               void *user, void *in, size_t len)
{
    if (!g_feed) return 0;

    try {
        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                std::printf("[WS] Connected to stream.binance.com\n");
                std::fflush(stdout);
                break;

            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                std::printf("[WS] Connection error - will retry\n");
                std::fflush(stdout);
                break;

            case LWS_CALLBACK_CLIENT_CLOSED:
                std::printf("[WS] Connection closed\n");
                std::fflush(stdout);
                break;

            case LWS_CALLBACK_CLIENT_RECEIVE: {
                if (!in || len == 0) break;

                auto now_sys  = std::chrono::system_clock::now();
                int64_t recv_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      now_sys.time_since_epoch()).count();

                std::string msg(static_cast<char*>(in), len);

                // Combined stream wraps each event in: {"stream":"btcusdt@bookTicker","data":{...}}
                // Find the "data" object
                size_t data_pos = msg.find("\"data\":");
                if (data_pos == std::string::npos) break;
                std::string data = msg.substr(data_pos + 7); // skip "data":

                // Identify event type from "e" field
                size_t e_pos = data.find("\"e\":\"");
                if (e_pos == std::string::npos) break;
                e_pos += 5;
                size_t e_end = data.find('"', e_pos);
                if (e_end == std::string::npos) break;
                std::string event_type = data.substr(e_pos, e_end - e_pos);

                if (event_type == "bookTicker") {
                    g_feed->handle_book_ticker(data, recv_ms);
                } else if (event_type == "aggTrade") {
                    g_feed->handle_agg_trade(data, recv_ms);
                }
                break;
            }

            default:
                break;
        }
    } catch (const std::exception& ex) {
        std::printf("[WS] Exception in callback: %s\n", ex.what());
        std::fflush(stdout);
    } catch (...) {
        std::printf("[WS] Unknown exception in callback\n");
        std::fflush(stdout);
    }

    return 0;
}

// ============================================================================
// run - connect and service the websocket loop
//
// Stream path combines bookTicker + aggTrade for all symbols:
//   /stream?streams=btcusdt@bookTicker/btcusdt@aggTrade/ethusdt@bookTicker/...
// ============================================================================
void BinanceWSFeed::run() {
    // Pre-populate all symbol states before connecting — ensures the map
    // never rehashes during live tick processing, eliminating any rehash
    // race between the WS callback thread and the map's internal state.
    states_.reserve(symbols_.size() * 2);
    for (const auto& sym : symbols_) {
        auto& st = states_[sym];
        st.tick.symbol = sym;
    }

    // Build combined stream path into member so c_str() pointer stays valid
    // for the entire lifetime of the lws connection
    stream_path_ = "/stream?streams=";
    for (size_t i = 0; i < symbols_.size(); ++i) {
        if (i > 0) stream_path_ += "/";
        stream_path_ += symbols_[i] + "@bookTicker/" + symbols_[i] + "@aggTrade";
    }

    std::printf("[WS] Subscribing: %s\n", stream_path_.c_str());
    std::fflush(stdout);

    static struct lws_protocols protocols[] = {
        { "default", ws_callback, 0, 65536, 0, nullptr, 0 },
        { nullptr,   nullptr,     0,     0, 0, nullptr, 0 }
    };

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port      = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid       = -1;
    info.uid       = -1;
    info.options   = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    context_ = lws_create_context(&info);
    if (!context_) {
        std::printf("[WS] Failed to create context\n");
        std::fflush(stdout);
        return;
    }

    struct lws_client_connect_info conn;
    memset(&conn, 0, sizeof(conn));
    conn.context      = context_;
    conn.address      = "stream.binance.com";
    conn.port         = 9443;
    conn.path         = stream_path_.c_str();
    conn.host         = conn.address;
    conn.origin       = conn.address;
    conn.ssl_connection = LCCSCF_USE_SSL;
    conn.protocol     = protocols[0].name;
    conn.pwsi         = &wsi_;

    wsi_ = lws_client_connect_via_info(&conn);
    if (!wsi_) {
        std::printf("[WS] Failed to connect\n");
        std::fflush(stdout);
        lws_context_destroy(context_);
        return;
    }

    while (running_) {
        lws_service(context_, 5);
    }

    lws_context_destroy(context_);
    context_ = nullptr;
}

}  // namespace chimera
