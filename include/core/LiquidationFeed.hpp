#pragma once
// ============================================================================
// LiquidationFeed.hpp — Binance futures forced-liquidation stream
//
// Connects to: wss://fstream.binance.com/ws/!forceOrder@arr
//
// When a SHORT is forcibly liquidated, the liquidation engine BUYS on perp.
// Spot price follows the perp pump within 50-200ms.
// We buy spot BEFORE spot catches up.
//
// Signal: side=SELL liquidation (a short being closed = buy pressure)
//         symbol in our watch list
//         original_qty * price >= MIN_NOTIONAL_USD
//
// Thread-safe. Reconnects automatically on disconnect.
// ============================================================================

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#include "core/SymbolIndex.hpp"

namespace chimera {

// ============================================================================
// LiquidationEvent — one forced liquidation from Binance futures
// ============================================================================
struct LiquidationEvent {
    int     symbol_id;          // chimera symbol id (0=BTC, 1=ETH, etc) or -1 if unknown
    char    symbol[16];         // e.g. "BTCUSDT"
    bool    is_short_liq;       // true = short being liquidated = BUY signal on spot
    double  price;              // liquidation fill price
    double  qty;                // liquidation qty (base asset)
    double  notional_usd;       // price * qty
    int64_t ts_ms;              // local receive time
};

using LiquidationCallback = std::function<void(const LiquidationEvent&)>;

// ============================================================================
// LiquidationFeed
// ============================================================================
class LiquidationFeed {
public:
    static constexpr const char* HOST    = "fstream.binance.com";
    static constexpr int         PORT    = 443;
    static constexpr const char* PATH    = "/ws/!forceOrder@arr";
    static constexpr int RECONNECT_MS    = 2000;
    static constexpr int STALE_SEC       = 60;

    LiquidationFeed() : running_(false), connected_(false), ssl_ctx_(nullptr), ssl_(nullptr), sock_fd_(-1) {}

    ~LiquidationFeed() { stop(); }

    void set_callback(LiquidationCallback cb) { callback_ = std::move(cb); }

    void start() {
        running_.store(true);
        thread_ = std::thread([this]() { run_loop(); });
    }

    void stop() {
        running_.store(false);
        if (thread_.joinable()) thread_.join();
        cleanup();
    }

    bool is_connected() const { return connected_.load(); }

private:
    std::atomic<bool>  running_;
    std::atomic<bool>  connected_;
    LiquidationCallback callback_;
    std::thread        thread_;

    SSL_CTX* ssl_ctx_;
    SSL*     ssl_;
    int      sock_fd_;

    char     recv_buf_[65536];
    char     frame_buf_[65536];

    // ── Main reconnect loop ───────────────────────────────────────────────
    void run_loop() {
        while (running_.load()) {
            if (connect_and_handshake()) {
                connected_.store(true);
                std::printf("[LIQ-FEED] Connected to %s%s\n", HOST, PATH);
                std::fflush(stdout);
                recv_loop();
            }
            connected_.store(false);
            cleanup();
            if (!running_.load()) break;
            std::printf("[LIQ-FEED] Reconnecting in %dms...\n", RECONNECT_MS);
            std::fflush(stdout);
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_MS));
        }
    }

    // ── Connect + TLS + WebSocket handshake ──────────────────────────────
    bool connect_and_handshake() {
        // DNS
        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        char port_str[8];
        std::snprintf(port_str, sizeof(port_str), "%d", PORT);
        if (getaddrinfo(HOST, port_str, &hints, &res) != 0 || !res) return false;

        sock_fd_ = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock_fd_ < 0) { freeaddrinfo(res); return false; }

        if (::connect(sock_fd_, res->ai_addr, res->ai_addrlen) != 0) {
            freeaddrinfo(res); ::close(sock_fd_); sock_fd_ = -1; return false;
        }
        freeaddrinfo(res);

        // TLS
        ssl_ctx_ = SSL_CTX_new(TLS_client_method());
        if (!ssl_ctx_) { ::close(sock_fd_); sock_fd_ = -1; return false; }

        ssl_ = SSL_new(ssl_ctx_);
        SSL_set_fd(ssl_, sock_fd_);
        SSL_set_tlsext_host_name(ssl_, HOST);
        if (SSL_connect(ssl_) != 1) { cleanup(); return false; }

        // WebSocket upgrade
        // Generate a simple base64 key
        unsigned char key_bytes[16];
        for (int i = 0; i < 16; ++i) key_bytes[i] = rand() & 0xFF;
        char key_b64[32] = {};
        base64_encode(key_bytes, 16, key_b64);

        char req[1024];
        int  len = std::snprintf(req, sizeof(req),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n",
            PATH, HOST, key_b64);
        if (SSL_write(ssl_, req, len) != len) { cleanup(); return false; }

        // Read HTTP 101
        char resp[2048] = {};
        int  n = SSL_read(ssl_, resp, sizeof(resp) - 1);
        if (n <= 0 || !std::strstr(resp, "101")) { cleanup(); return false; }

        return true;
    }

    // ── Receive loop — reads WebSocket frames, parses JSON ───────────────
    void recv_loop() {
        int64_t last_msg_ms = now_ms();

        while (running_.load()) {
            // Check stale
            if (now_ms() - last_msg_ms > STALE_SEC * 1000) {
                std::printf("[LIQ-FEED] Stale — reconnecting\n");
                std::fflush(stdout);
                break;
            }

            // Poll for data
            struct pollfd pfd{ sock_fd_, POLLIN, 0 };
            int rc = ::poll(&pfd, 1, 1000);
            if (rc <= 0) continue;

            // Read WebSocket frame header
            unsigned char hdr[2];
            if (ssl_read_exact(hdr, 2) != 2) break;

            int opcode   = hdr[0] & 0x0F;
            bool masked  = (hdr[1] & 0x80) != 0;
            uint64_t plen = hdr[1] & 0x7F;

            if (plen == 126) {
                unsigned char ext[2];
                if (ssl_read_exact(ext, 2) != 2) break;
                plen = ((uint64_t)ext[0] << 8) | ext[1];
            } else if (plen == 127) {
                unsigned char ext[8];
                if (ssl_read_exact(ext, 8) != 8) break;
                plen = 0;
                for (int i = 0; i < 8; ++i) plen = (plen << 8) | ext[i];
            }

            unsigned char mask[4] = {};
            if (masked) { if (ssl_read_exact(mask, 4) != 4) break; }

            if (plen == 0 || plen >= sizeof(frame_buf_) - 1) {
                // Skip oversized or empty
                if (plen > 0) {
                    uint64_t skip = plen;
                    while (skip > 0) {
                        uint64_t chunk = std::min(skip, (uint64_t)sizeof(recv_buf_));
                        ssl_read_exact((unsigned char*)recv_buf_, (int)chunk);
                        skip -= chunk;
                    }
                }
                continue;
            }

            if (ssl_read_exact((unsigned char*)frame_buf_, (int)plen) != (int)plen) break;
            frame_buf_[plen] = '\0';

            if (masked) {
                for (uint64_t i = 0; i < plen; ++i)
                    frame_buf_[i] ^= mask[i % 4];
            }

            // Ping → send Pong
            if (opcode == 0x9) {
                unsigned char pong[2] = { 0x8A, 0x00 };
                SSL_write(ssl_, pong, 2);
                last_msg_ms = now_ms();
                continue;
            }

            // Connection close
            if (opcode == 0x8) break;

            // Text/binary frame — parse JSON
            if (opcode == 0x1 || opcode == 0x2) {
                last_msg_ms = now_ms();
                parse_liquidation(frame_buf_);
            }
        }
    }

    // ── Parse Binance forceOrder JSON ─────────────────────────────────────
    // Format: {"o":{"s":"BTCUSDT","S":"SELL","p":"68123.40","q":"0.050","T":1234567890000}}
    // S=SELL means a SHORT is being liquidated (buy side pressure on perp → spot follows up)
    // S=BUY  means a LONG is being liquidated  (sell side pressure → spot follows down — skip, spot only)
    void parse_liquidation(const char* json) {
        // Symbol
        const char* s_ptr = std::strstr(json, "\"s\":\"");
        const char* side_ptr = std::strstr(json, "\"S\":\"");
        const char* p_ptr = std::strstr(json, "\"p\":\"");
        const char* q_ptr = std::strstr(json, "\"q\":\"");
        const char* t_ptr = std::strstr(json, "\"T\":");

        if (!s_ptr || !side_ptr || !p_ptr || !q_ptr) return;

        // Parse symbol
        s_ptr += 5;
        const char* s_end = std::strchr(s_ptr, '"');
        if (!s_end) return;
        char sym[16] = {};
        size_t slen = std::min((size_t)(s_end - s_ptr), sizeof(sym) - 1);
        std::memcpy(sym, s_ptr, slen);

        // Parse side: "SELL" = short liq (good for us), "BUY" = long liq (skip)
        side_ptr += 5;
        bool is_short_liq = (side_ptr[0] == 'S'); // "SELL"
        if (!is_short_liq) return; // long liquidation → spot falls → skip (spot only)

        // Parse price
        p_ptr += 5;
        double price = std::atof(p_ptr);
        if (price <= 0.0) return;

        // Parse qty
        q_ptr += 5;
        double qty = std::atof(q_ptr);
        if (qty <= 0.0) return;

        double notional = price * qty;

        // Map to symbol id (Binance futures use BTCUSDT, ETHUSDT etc — same as spot)
        // Convert to lowercase for sym_id lookup
        char sym_lower[16] = {};
        for (int i = 0; sym[i] && i < 15; ++i)
            sym_lower[i] = (char)std::tolower((unsigned char)sym[i]);

        int sid = sym_id(std::string(sym_lower));

        LiquidationEvent ev;
        ev.symbol_id    = sid;
        ev.is_short_liq = true;
        ev.price        = price;
        ev.qty          = qty;
        ev.notional_usd = notional;
        ev.ts_ms        = t_ptr ? std::atoll(t_ptr + 4) : now_ms();
        std::memcpy(ev.symbol, sym, sizeof(ev.symbol));

        if (callback_) callback_(ev);
    }

    // ── Helpers ───────────────────────────────────────────────────────────
    int ssl_read_exact(unsigned char* buf, int n) {
        int total = 0;
        while (total < n) {
            int r = SSL_read(ssl_, buf + total, n - total);
            if (r <= 0) return total;
            total += r;
        }
        return total;
    }

    void cleanup() {
        if (ssl_)     { SSL_shutdown(ssl_); SSL_free(ssl_); ssl_ = nullptr; }
        if (ssl_ctx_) { SSL_CTX_free(ssl_ctx_); ssl_ctx_ = nullptr; }
        if (sock_fd_ >= 0) { ::close(sock_fd_); sock_fd_ = -1; }
    }

    static int64_t now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    static void base64_encode(const unsigned char* in, int len, char* out) {
        static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        int i = 0, j = 0;
        while (i < len) {
            unsigned a = i < len ? in[i++] : 0;
            unsigned b = i < len ? in[i++] : 0;
            unsigned c = i < len ? in[i++] : 0;
            unsigned n = (a << 16) | (b << 8) | c;
            out[j++] = t[(n >> 18) & 63];
            out[j++] = t[(n >> 12) & 63];
            out[j++] = t[(n >> 6)  & 63];
            out[j++] = t[ n        & 63];
        }
        for (int k = 0; k < (3 - len % 3) % 3; ++k) out[j - 1 - k] = '=';
        out[j] = '\0';
    }
};

} // namespace chimera
