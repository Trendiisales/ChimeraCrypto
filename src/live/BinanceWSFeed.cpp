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

    // DIAGNOSTIC: log first bookTicker per symbol to confirm parsing
    {
        static bool _bt_logged_[16] = {};
        static int _bt_idx_ = 0;
        // simple index lookup
        int _sym_idx_ = -1;
        const char* syms[] = {"btcusdt","ethusdt","solusdt","bnbusdt","avaxusdt","linkusdt","xrpusdt"};
        for(int _i_=0;_i_<7;_i_++) if(sym==syms[_i_]){_sym_idx_=_i_;break;}
        if(_sym_idx_>=0 && !_bt_logged_[_sym_idx_]){
            std::printf("[BT-DIAG] %s | bid=%.4f ask=%.4f bid_sz=%.4f ask_sz=%.4f guard=%s\n",
                sym.c_str(),bid,ask,bid_sz,ask_sz,
                (bid<=0.0||ask<=0.0||ask<bid)?"BLOCKED":"PASS");
            std::fflush(stdout);
            _bt_logged_[_sym_idx_]=true;
        }
    }

    if (bid <= 0.0 || ask <= 0.0 || ask < bid) return;  // allow ask==bid (tight symbols like XRP at low price)

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

                // Check for "e" field to identify event type.
                // NOTE: @depth5@100ms snapshot frames have NO "e" field —
                // they arrive as {"lastUpdateId":...,"bids":[...],"asks":[...]}
                // These are handled via stream name detection below.
                size_t e_pos = data.find("\"e\":\"");
                if (e_pos != std::string::npos) {
                    e_pos += 5;
                    size_t e_end = data.find('"', e_pos);
                    if (e_end == std::string::npos) break;
                    std::string event_type = data.substr(e_pos, e_end - e_pos);

                    if (event_type == "bookTicker") {
                        g_feed->handle_book_ticker(data, recv_ms);
                    } else if (event_type == "aggTrade") {
                        g_feed->handle_agg_trade(data, recv_ms);
                    }
                    // depthUpdate is the DIFF stream (@depth), not the snapshot
                    // stream (@depth5@100ms). We subscribe to @depth5@100ms which
                    // sends snapshots with NO "e" field. If somehow a depthUpdate
                    // arrives, ignore it — we don't subscribe to @depth.
                } else {
                    // No "e" field — could be @bookTicker or @depth5@100ms snapshot.
                    // Binance combined stream bookTicker does NOT include "e" field.
                    // depth5 snapshot also has no "e" field.
                    // Distinguish by stream name.
                    size_t stream_pos = msg.find("\"stream\":\"");
                    if (stream_pos != std::string::npos) {
                        size_t depth_pos  = msg.find("@depth5",      stream_pos);
                        size_t bticker_pos = msg.find("@bookTicker",  stream_pos);
                        if (depth_pos != std::string::npos) {
                            // @depth5@100ms snapshot
                            stream_pos += 10; // skip past "stream":"
                            size_t stream_end = msg.find('@', stream_pos);
                            if (stream_end != std::string::npos) {
                                std::string depth_sym = msg.substr(stream_pos, stream_end - stream_pos);
                                g_feed->handle_depth5(depth_sym + "|" + data, recv_ms);
                            }
                        } else if (bticker_pos != std::string::npos) {
                            // @bookTicker — no "e" field in combined stream format
                            g_feed->handle_book_ticker(data, recv_ms);
                        }
                    }
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
// Stream path combines bookTicker + aggTrade + depth5 for all symbols:
//   /stream?streams=btcusdt@bookTicker/btcusdt@aggTrade/btcusdt@depth5@100ms/...
// depth5@100ms gives 5 levels of bid/ask every 100ms — real order book depth
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
        stream_path_ += symbols_[i] + "@bookTicker/" + symbols_[i] + "@aggTrade/" + symbols_[i] + "@depth5@100ms";
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

// ============================================================================
// handle_depth5 — parses @depth5@100ms partial book snapshot
//
// Binance @depth5@100ms snapshot format (inside "data" wrapper):
//   {"lastUpdateId":N,"bids":[["67000.00","0.5"],...],"asks":[["67001.00","0.3"],...]}
//
// IMPORTANT: Uses "bids"/"asks" (full words) NOT "b"/"a".
// "b"/"a" is used by the @depth DIFF stream which we do NOT subscribe to.
// @depth5@100ms is a SNAPSHOT stream — different format entirely.
//
// msg format passed in: "symbolname|{json_data}"
// Populates tick.bid_prices[0..4], ask_prices[0..4], sizes, depth_imbalance
// ============================================================================
void BinanceWSFeed::handle_depth5(const std::string& msg, int64_t /*recv_ms*/) {
    // Extract symbol prefix (before '|')
    size_t sep = msg.find('|');
    if (sep == std::string::npos) return;
    std::string sym = msg.substr(0, sep);
    std::string data = msg.substr(sep + 1);

    auto& st = get_or_create(sym);
    MarketTick& t = st.tick;

    // @depth5@100ms snapshot uses "bids" and "asks" (full field names)
    // Each entry: ["price_string", "qty_string"]
    auto parse_levels = [&](const std::string& key, double* prices, double* sizes) {
        std::string needle = "\"" + key + "\":[";
        size_t pos = data.find(needle);
        if (pos == std::string::npos) return;
        pos += needle.size();  // skip past "bids":[ or "asks":[
        int level = 0;
        while (level < 5) {
            size_t bracket = data.find('[', pos);
            if (bracket == std::string::npos) break;
            // Check we haven't walked past the closing ] of the outer array
            size_t outer_close = data.find(']', pos);
            if (outer_close < bracket) break; // hit end of outer array before next inner [
            size_t comma = data.find(',', bracket);
            if (comma == std::string::npos) break;
            size_t close = data.find(']', comma);
            if (close == std::string::npos) break;
            // price is between bracket+1 and comma, strip quotes and spaces
            size_t p_start = bracket + 1;
            while (p_start < data.size() && (data[p_start] == '"' || data[p_start] == ' ')) p_start++;
            size_t p_end = comma;
            while (p_end > p_start && (data[p_end-1] == '"' || data[p_end-1] == ' ')) p_end--;
            // qty is between comma+1 and close, strip quotes and spaces
            size_t q_start = comma + 1;
            while (q_start < data.size() && (data[q_start] == '"' || data[q_start] == ' ')) q_start++;
            size_t q_end = close;
            while (q_end > q_start && (data[q_end-1] == '"' || data[q_end-1] == ' ')) q_end--;

            prices[level] = std::atof(data.substr(p_start, p_end - p_start).c_str());
            sizes[level]  = std::atof(data.substr(q_start, q_end - q_start).c_str());
            level++;
            pos = close + 1;
        }
    };

    parse_levels("bids", t.bid_prices, t.bid_sizes);
    parse_levels("asks", t.ask_prices, t.ask_sizes);

    // Compute 5-level totals and imbalance
    t.total_bid_depth = 0.0;
    t.total_ask_depth = 0.0;
    for (int i = 0; i < 5; ++i) {
        t.total_bid_depth += t.bid_sizes[i];
        t.total_ask_depth += t.ask_sizes[i];
    }
    double total = t.total_bid_depth + t.total_ask_depth;
    t.depth_imbalance = (total > 1e-9)
        ? (t.total_bid_depth - t.total_ask_depth) / total
        : 0.0;
    t.depth_ready = (t.total_bid_depth > 0.0 || t.total_ask_depth > 0.0);

    // Log first successful parse per symbol for confirmation
    static bool depth_logged[16] = {};
    int sym_idx = -1;
    if (sym == "btcusdt") sym_idx = 0;
    else if (sym == "ethusdt") sym_idx = 1;
    else if (sym == "solusdt") sym_idx = 2;
    else if (sym == "bnbusdt") sym_idx = 3;
    else if (sym == "avaxusdt") sym_idx = 4;
    else if (sym == "linkusdt") sym_idx = 5;
    else if (sym == "xrpusdt") sym_idx = 6;
    if (sym_idx >= 0 && sym_idx < 16 && !depth_logged[sym_idx] && t.depth_ready) {
        std::printf("[DEPTH5-READY] %s | bid[0]=%.2f sz=%.4f | ask[0]=%.2f sz=%.4f | imbal=%.3f\n",
            sym.c_str(),
            t.bid_prices[0], t.bid_sizes[0],
            t.ask_prices[0], t.ask_sizes[0],
            t.depth_imbalance);
        std::fflush(stdout);
        depth_logged[sym_idx] = true;
    }

    // DO NOT fire callback_ here. depth5 has no price — firing callback_ would
    // call on_tick with mid_price=0, last_price=0, poisoning leadlag buffers
    // with zero prices and making btc_move_bp() return 0 forever.
    // Depth fields are read from symbols_[id].last_tick on the next bookTicker/aggTrade tick.
}

}  // namespace chimera
