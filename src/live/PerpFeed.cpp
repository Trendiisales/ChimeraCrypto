// ============================================================================
// PerpFeed.cpp
// ============================================================================

#include "live/PerpFeed.hpp"

#include <chrono>
#include <algorithm>
#include <cstdlib>

// [REST FALLBACK] — added 2026-05-10. libcurl is already linked in
// CMakeLists.txt (used by BinanceREST.hpp). curl_global_init is called
// elsewhere in the binary (BinanceREST::load_credentials), so we rely on
// the implicit init done by curl_easy_init() here as a safety net.
#include <curl/curl.h>

namespace chimera {

static PerpFeed* g_perp_feed = nullptr;

PerpFeed::PerpFeed() {
    g_perp_feed = this;
}

PerpFeed::~PerpFeed() {
    stop();
    g_perp_feed = nullptr;
}

void PerpFeed::start() {
    if (running_.exchange(true)) return;

    // Build combined stream path:
    // btcusdt@markPrice/btcusdt@aggTrade/ethusdt@markPrice/...
    stream_path_ = "/stream?streams=";
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
        std::string sym = sym_full(i);
        // Convert to lowercase
        for (char& c : sym) c = (char)std::tolower((unsigned char)c);
        if (i > 0) stream_path_ += "/";
        stream_path_ += sym + "@markPrice/" + sym + "@aggTrade";
    }

    std::printf("[PERP-FEED] Starting | streams: %zu symbols x (markPrice+aggTrade)\n",
        (size_t)MAX_SYMBOLS);
    std::fflush(stdout);

    thread_ = std::thread([this]{ run(); });

    // [REST FALLBACK] launch the REST poller alongside the WS thread.
    // See rest_run() for the design notes (header has the full comment).
    rest_thread_ = std::thread([this]{ rest_run(); });
}

void PerpFeed::stop() {
    if (!running_.exchange(false)) return;
    if (context_) lws_cancel_service(context_);
    if (thread_.joinable()) thread_.join();

    // [REST FALLBACK] join. running_ already flipped to false above; the
    // REST loop polls running_ and exits cleanly within one poll cycle.
    if (rest_thread_.joinable()) rest_thread_.join();
}

// ── Accessors ────────────────────────────────────────────────────────────────

double PerpFeed::mark_price(int id) const {
    if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
    return load_dbl(state_[id].mark_price_bits);
}

double PerpFeed::basis_bp(int id, double spot_price) const {
    if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
    double mp = load_dbl(state_[id].mark_price_bits);
    if (mp <= 0.0 || spot_price <= 0.0) return 0.0;
    return (mp - spot_price) / spot_price * 10000.0;
}

double PerpFeed::funding_rate(int id) const {
    if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
    return load_dbl(state_[id].funding_rate_bits);
}

double PerpFeed::perp_flow_ratio(int id) const {
    if (id < 0 || id >= MAX_SYMBOLS) return 0.0;
    double buy  = load_dbl(state_[id].buy_ema_bits);
    double sell = load_dbl(state_[id].sell_ema_bits);
    double total = buy + sell;
    if (total < 1e-9) return 0.0;
    return (buy - sell) / total;
}

bool PerpFeed::ready(int id) const {
    if (id < 0 || id >= MAX_SYMBOLS) return false;
    return state_[id].ready.load(std::memory_order_acquire);
}

// ── WebSocket callback ───────────────────────────────────────────────────────

int PerpFeed::ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                          void* /*user*/, void* in, size_t len)
{
    if (!g_perp_feed) return 0;
    auto* self = g_perp_feed;

    // Wrap the entire callback body in try/catch — symmetry with BinanceWSFeed
    // (spot feed). Without this, an exception in handle_message would propagate
    // through lws_service into run() and terminate the thread (and via std::
    // thread destructor, the whole process). Defensive: not the cause of the
    // 2026-05-03 incident but cheap insurance.
    try {
        switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            std::printf("[PERP-FEED] Connected to fstream.binance.com\n");
            std::fflush(stdout);
            // Reset the liveness clock so the watchdog doesn't fire spuriously
            // immediately after a fresh reconnect.
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            self->last_msg_ms_.store(now_ms, std::memory_order_relaxed);
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
        case LWS_CALLBACK_CLIENT_CLOSED:
            std::printf("[PERP-FEED] Disconnected -- reconnecting\n");
            std::fflush(stdout);
            self->wsi_ = nullptr;
            break;

        default: break;
        }
    } catch (const std::exception& ex) {
        std::printf("[PERP-FEED] Exception in callback: %s\n", ex.what());
        std::fflush(stdout);
    } catch (...) {
        std::printf("[PERP-FEED] Unknown exception in callback\n");
        std::fflush(stdout);
    }
    return 0;
}

// ── Message dispatch ─────────────────────────────────────────────────────────

void PerpFeed::handle_message(const std::string& msg, int64_t recv_ms) {
    // Stamp the liveness clock on EVERY frame, before any parsing or early
    // returns — even malformed/unrecognised frames count as "the WS is alive."
    // The watchdog in run() reads this to detect a dead-but-not-closed socket.
    last_msg_ms_.store(recv_ms, std::memory_order_relaxed);

    // Combined stream wraps: {"stream":"btcusdt@markPrice","data":{...}}
    auto stream_pos = msg.find("\"stream\":\"");
    if (stream_pos == std::string::npos) return;
    stream_pos += 10;
    auto stream_end = msg.find('"', stream_pos);
    if (stream_end == std::string::npos) return;
    std::string stream_name = msg.substr(stream_pos, stream_end - stream_pos);

    // Extract data object
    auto data_pos = msg.find("\"data\":{");
    if (data_pos == std::string::npos) return;
    std::string data_str = msg.substr(data_pos + 7);  // from "{" onward

    // Find symbol id from stream name (e.g. "btcusdt@markPrice")
    std::string sym_lower = stream_name.substr(0, stream_name.find('@'));
    int id = sym_id(sym_lower);
    if (id < 0) return;

    if (stream_name.find("@markPrice") != std::string::npos) {
        handle_mark_price(data_str, id);
    } else if (stream_name.find("@aggTrade") != std::string::npos) {
        handle_agg_trade(data_str, id);
    }
}

void PerpFeed::handle_mark_price(const std::string& msg, int id) {
    // {"e":"markPriceUpdate","p":"67500.0","r":"0.0001",...}
    double mp = extract_dbl(msg, "p");   // mark price
    double fr = extract_dbl(msg, "r");   // funding rate

    if (mp > 0.0) {
        store_dbl(state_[id].mark_price_bits, mp);
        store_dbl(state_[id].funding_rate_bits, fr);
        state_[id].ready.store(true, std::memory_order_release);
    }
}

void PerpFeed::handle_agg_trade(const std::string& msg, int id) {
    // {"e":"aggTrade","q":"0.012","m":true,...}
    double qty = extract_dbl(msg, "q");
    bool   is_buyer_maker = extract_bool(msg, "m");

    if (qty <= 0.0) return;

    // Update flow EMAs (alpha=0.05, ~20-trade window)
    double cur_buy  = load_dbl(state_[id].buy_ema_bits);
    double cur_sell = load_dbl(state_[id].sell_ema_bits);
    constexpr double ALPHA = 0.05;

    if (is_buyer_maker) {
        // buyer is maker = seller was aggressor
        store_dbl(state_[id].buy_ema_bits,  cur_buy  * (1.0 - ALPHA));
        store_dbl(state_[id].sell_ema_bits, cur_sell * (1.0 - ALPHA) + qty * ALPHA);
    } else {
        // buyer is taker = buyer was aggressor
        store_dbl(state_[id].buy_ema_bits,  cur_buy  * (1.0 - ALPHA) + qty * ALPHA);
        store_dbl(state_[id].sell_ema_bits, cur_sell * (1.0 - ALPHA));
    }
}

// ── Run loop ─────────────────────────────────────────────────────────────────

void PerpFeed::run() {
    static constexpr struct lws_protocols protocols[] = {
        { "perp-feed", ws_callback, 0, 65536, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    // Staleness watchdog threshold. Binance perp markPrice fires once per
    // second per symbol → ≥8 msgs/sec across our 8-symbol subscription. 60s
    // of total silence is well past any normal pause; treat as a dead socket
    // that lws missed and force a clean reconnect.
    constexpr int64_t STALE_MS = 60'000;

    while (running_.load(std::memory_order_acquire)) {
        struct lws_context_creation_info ctx_info{};
        ctx_info.port        = CONTEXT_PORT_NO_LISTEN;
        ctx_info.protocols   = protocols;
        ctx_info.gid         = -1;
        ctx_info.uid         = -1;
        ctx_info.options     = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        // TCP keepalive — kernel surfaces dead connections to lws via recv()
        // errors instead of silently spinning on a half-closed socket. Tuned
        // for Binance's typical ≤30s idle behaviour. The watchdog below is the
        // safety net for cases the kernel keepalive misses.
        ctx_info.ka_time     = 30;   // start probing after 30s of silence
        ctx_info.ka_probes   = 3;    // 3 failed probes = dead
        ctx_info.ka_interval = 10;   // 10s between probes

        context_ = lws_create_context(&ctx_info);
        if (!context_) {
            std::printf("[PERP-FEED] Failed to create lws context -- retry in 5s\n");
            std::fflush(stdout);
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        struct lws_client_connect_info conn{};
        conn.context        = context_;
        conn.address        = "fstream.binance.com";
        conn.port           = 443;
        conn.path           = stream_path_.c_str();
        conn.host           = conn.address;
        conn.origin         = conn.address;
        conn.ssl_connection = LCCSCF_USE_SSL;
        conn.protocol       = protocols[0].name;
        conn.pwsi           = &wsi_;

        wsi_ = lws_client_connect_via_info(&conn);
        if (!wsi_) {
            std::printf("[PERP-FEED] connect failed -- retry in 5s\n");
            std::fflush(stdout);
            lws_context_destroy(context_);
            context_ = nullptr;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // Initialise the liveness clock at connect-attempt time so the
        // watchdog gives the handshake + first message at least STALE_MS
        // grace. CLIENT_ESTABLISHED resets it again on success.
        {
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            last_msg_ms_.store(now_ms, std::memory_order_relaxed);
        }

        while (running_.load(std::memory_order_acquire) && wsi_ != nullptr) {
            lws_service(context_, 50);

            // Application-layer staleness watchdog. If we go STALE_MS without
            // a single frame from fstream, assume the socket is dead-but-lws-
            // doesn't-know-it and force a reconnect by breaking out of the
            // inner loop. The outer loop will destroy the context (which
            // closes the underlying fd) and rebuild from scratch.
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int64_t last = last_msg_ms_.load(std::memory_order_relaxed);
            if (last > 0 && (now_ms - last) > STALE_MS) {
                std::printf("[PERP-FEED] Stale (no msgs in %lldms) -- forcing reconnect\n",
                    (long long)(now_ms - last));
                std::fflush(stdout);
                break;
            }
        }

        lws_context_destroy(context_);
        context_ = nullptr;
        wsi_     = nullptr;

        if (!running_.load(std::memory_order_acquire)) break;

        std::printf("[PERP-FEED] Reconnecting in 2s...\n");
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::printf("[PERP-FEED] Feed thread exited.\n");
    std::fflush(stdout);
}

// ── JSON extractors ───────────────────────────────────────────────────────────

double PerpFeed::extract_dbl(const std::string& msg, const std::string& key) {
    std::string nq = "\"" + key + "\":\"";
    auto pos = msg.find(nq);
    if (pos != std::string::npos) {
        pos += nq.size();
        auto end = msg.find('"', pos);
        if (end != std::string::npos) {
            try { return std::stod(msg.substr(pos, end-pos)); } catch(...) {}
        }
    }
    std::string nb = "\"" + key + "\":";
    pos = msg.find(nb);
    if (pos != std::string::npos) {
        pos += nb.size();
        auto end = msg.find_first_of(",}]\"", pos);
        if (end != std::string::npos) {
            try { return std::stod(msg.substr(pos, end-pos)); } catch(...) {}
        }
    }
    return 0.0;
}

int64_t PerpFeed::extract_i64(const std::string& msg, const std::string& key) {
    std::string nb = "\"" + key + "\":";
    auto pos = msg.find(nb);
    if (pos == std::string::npos) return 0;
    pos += nb.size();
    auto end = msg.find_first_of(",}]\"", pos);
    if (end == std::string::npos) return 0;
    try { return std::stoll(msg.substr(pos, end-pos)); } catch(...) { return 0; }
}

bool PerpFeed::extract_bool(const std::string& msg, const std::string& key) {
    std::string nb = "\"" + key + "\":";
    auto pos = msg.find(nb);
    if (pos == std::string::npos) return false;
    pos += nb.size();
    return msg.substr(pos, 4) == "true";
}



// ════════════════════════════════════════════════════════════════════════════
// REST fallback (added 2026-05-10)
// ────────────────────────────────────────────────────────────────────────────
// Polls https://fapi.binance.com/fapi/v1/premiumIndex?symbol=<SYM>USDT for
// each of our 8 symbols every REST_POLL_MS milliseconds. Parses markPrice
// and lastFundingRate. Writes them to the same atomics as handle_mark_price()
// — but only when the WS path is silent (last_msg_ms_ older than
// REST_WS_FRESHNESS_MS). When WS is alive, REST writes are no-ops and
// behaviour is identical to the pre-fallback build.
//
// One curl handle per loop iteration (per symbol per cycle). libcurl reuses
// the underlying TLS connection between calls to the same host within the
// same handle, but for the cleanest crash-safety we open/close per call —
// each call is ~80ms from Tokyo, 8 symbols × 80ms = ~640ms per cycle, well
// under the 5s poll interval.
//
// Bandwidth: ~250B per symbol response × 8 symbols / 5s ≈ 400 B/s ≈ 35 MB/day.
// ════════════════════════════════════════════════════════════════════════════

namespace {
    constexpr int  REST_POLL_MS         = 5000;   // 5s between full sweeps
    constexpr int  REST_WS_FRESHNESS_MS = 5000;   // WS frames within 5s = WS alive
    constexpr long REST_TIMEOUT_S       = 8;      // libcurl per-request timeout
}

size_t PerpFeed::curl_write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(static_cast<const char*>(ptr), size * nmemb);
    return size * nmemb;
}

void PerpFeed::rest_run() {
    std::printf("[PERP-REST] REST fallback thread starting (poll every %dms, "
                "WS-freshness gate %dms)\n", REST_POLL_MS, REST_WS_FRESHNESS_MS);
    std::fflush(stdout);

    // Initial brief sleep so we don't race the WS thread's first connect
    // attempt. If WS comes up cleanly we'll see a fresh last_msg_ms_ within
    // a couple of seconds and our writes will gate themselves off.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    int total_polls   = 0;
    int total_skipped = 0;   // skipped because WS was fresh
    int total_writes  = 0;   // wrote into atomics
    int last_log_polls = 0;

    while (running_.load(std::memory_order_acquire)) {
        auto cycle_start = std::chrono::steady_clock::now();

        // Gate: is WS fresh?
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int64_t last_ws = last_msg_ms_.load(std::memory_order_relaxed);
        bool ws_fresh = (last_ws > 0) && (now_ms - last_ws) < REST_WS_FRESHNESS_MS;

        for (int id = 0; id < MAX_SYMBOLS; ++id) {
            if (!running_.load(std::memory_order_acquire)) break;

            std::string sym = sym_full(id);
            for (char& c : sym) c = (char)std::toupper((unsigned char)c);

            std::string url = "https://fapi.binance.com/fapi/v1/premiumIndex?symbol=" + sym;
            std::string body;
            body.reserve(512);

            CURL* curl = curl_easy_init();
            if (!curl) {
                std::printf("[PERP-REST] curl_easy_init failed -- skipping cycle\n");
                std::fflush(stdout);
                break;
            }

            curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_write_cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT,        REST_TIMEOUT_S);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT,      "chimera-perp-rest/1.0");
            // SSL verification on by default — relies on the system CA bundle
            // shipped with the VPS (ubuntu/debian have /etc/ssl/certs).

            CURLcode rc = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            curl_easy_cleanup(curl);

            ++total_polls;
            if (rc != CURLE_OK || http_code != 200 || body.empty()) {
                // Don't log every miss — would spam if Binance has a brief
                // outage. Quiet failure; the engines will see stale data.
                continue;
            }

            // Parse markPrice + lastFundingRate from the JSON response.
            // Response is a single object: {"symbol":"BTCUSDT","markPrice":"...",
            //   "lastFundingRate":"...","nextFundingTime":...,"time":...}
            double mp = extract_dbl(body, "markPrice");
            double fr = extract_dbl(body, "lastFundingRate");

            if (mp <= 0.0) continue;

            // ── The gate. If WS has written within the freshness window,
            //    skip the write so we don't fight with WS. WS data is
            //    higher-frequency and authoritative when available.
            if (ws_fresh) { ++total_skipped; continue; }

            // WS is silent — we're the data source.
            store_dbl(state_[id].mark_price_bits,    mp);
            store_dbl(state_[id].funding_rate_bits,  fr);
            state_[id].ready.store(true, std::memory_order_release);
            ++total_writes;
            rest_last_ok_ms_.store(now_ms, std::memory_order_relaxed);
        }

        // Periodic status line: every ~60s (12 cycles). Emits a single line
        // showing polls / skipped (WS alive) / writes (WS dead) ratios so the
        // operator can tell at a glance what mode the fallback is in.
        if (total_polls - last_log_polls >= MAX_SYMBOLS * 12) {
            std::printf("[PERP-REST] polls=%d ws_fresh_skip=%d rest_wrote=%d  "
                        "(mode: %s)\n",
                total_polls, total_skipped, total_writes,
                ws_fresh ? "WS-active (REST idle)" : "REST-fallback-active");
            std::fflush(stdout);
            last_log_polls = total_polls;
        }

        // Sleep the remainder of the cycle. If the cycle took longer than
        // REST_POLL_MS (e.g. all 8 symbols timed out), skip the sleep and
        // poll again immediately.
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - cycle_start).count();
        int sleep_ms = REST_POLL_MS - (int)elapsed;
        if (sleep_ms > 0) {
            // Sleep in 200ms chunks so a stop() request gets serviced
            // promptly rather than waiting the full 5s.
            int slept = 0;
            while (slept < sleep_ms && running_.load(std::memory_order_acquire)) {
                int chunk = std::min(200, sleep_ms - slept);
                std::this_thread::sleep_for(std::chrono::milliseconds(chunk));
                slept += chunk;
            }
        }
    }

    std::printf("[PERP-REST] REST fallback thread exiting "
                "(polls=%d skipped=%d wrote=%d).\n",
                total_polls, total_skipped, total_writes);
    std::fflush(stdout);
}

} // namespace chimera
