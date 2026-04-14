// ============================================================================
// Chimera — Trend Following Engine
// Strategy: H1 EMA9/EMA50 crossover on BTC/ETH/SOL spot
// Feed: Binance WebSocket (bookTicker + aggTrade)
// Execution: SpotExecutor (shadow mode default)
// GUI: HTTP :8080
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <mutex>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "live/BinanceWSFeed.hpp"
#include "live/CoinbaseWSFeed.hpp"
#include "live/PerpFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "core/TrendEngine.hpp"
#include "core/SymbolIndex.hpp"

#include "version_generated.hpp"
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

// ── Single-instance lock ─────────────────────────────────────────────────────
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) { std::fprintf(stderr, "[FATAL] Cannot open lock file\n"); std::exit(1); }
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        char buf[32] = {}; (void)::read(g_lock_fd, buf, sizeof(buf)-1); ::close(g_lock_fd);
        std::fprintf(stderr, "[FATAL] Chimera already running (PID %s)\n", buf); std::exit(1);
    }
    (void)::ftruncate(g_lock_fd, 0);
    char pidbuf[32]; int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    (void)::write(g_lock_fd, pidbuf, len); ::fsync(g_lock_fd);
    std::printf("[STARTUP] Instance lock acquired PID=%d\n", (int)::getpid());
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN); ::close(g_lock_fd); ::unlink(PID_LOCK_FILE); g_lock_fd = -1;
    }
}

// ── Minimal HTTP server for GUI ───────────────────────────────────────────────
static chimera::TrendEngine* g_engine_ptr = nullptr;
static std::mutex             g_engine_mtx;

static void http_server_thread(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
    listen(server_fd, 16);
    std::printf("[HTTP] GUI server on port %d\n", port);
    std::fflush(stdout);

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) break;
        char req[512] = {}; read(client, req, sizeof(req)-1);

        std::string body;
        const char* ct = "application/json";

        if (strstr(req, "GET /api/state")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            body = g_engine_ptr ? g_engine_ptr->state_json() : "{}";
        } else if (strstr(req, "GET /")) {
            ct = "text/html";
            body = R"(<!DOCTYPE html><html><head><title>Chimera Trend</title>
<meta http-equiv="refresh" content="5">
<style>body{background:#0d1117;color:#e6edf3;font-family:monospace;padding:20px;}
table{border-collapse:collapse;width:100%;}
td,th{border:1px solid #30363d;padding:8px 12px;text-align:left;}
th{background:#161b22;}.pos{color:#3fb950;}.neg{color:#f85149;}
h1{color:#58a6ff;}</style></head><body>
<h1>Chimera Trend Engine</h1>
<div id="data">Loading...</div>
<script>
function load(){fetch('/api/state').then(r=>r.json()).then(d=>{
  let h='<p>Trades: '+d.trades+' | Shadow: '+d.shadow+'</p>';
  h+='<table><tr><th>Symbol</th><th>Status</th><th>Entry</th><th>SL</th><th>MFE</th><th>EMA9</th><th>EMA50</th><th>ATR</th><th>Bars</th></tr>';
  (d.positions||[]).forEach(p=>{
    h+='<tr><td>'+p.sym+'</td><td>'+(p.active?'<span class="pos">'+p.side+'</span>':p.side)+'</td>';
    h+='<td>'+(p.entry>0?p.entry.toFixed(2):'--')+'</td>';
    h+='<td>'+(p.sl>0?p.sl.toFixed(2):'--')+'</td>';
    h+='<td>'+(p.mfe>0?p.mfe.toFixed(4):'--')+'</td>';
    h+='<td>'+p.ema9.toFixed(2)+'</td><td>'+p.ema50.toFixed(2)+'</td>';
    h+='<td>'+p.atr.toFixed(2)+'</td><td>'+p.bars+'</td></tr>';
  });
  h+='</table>';
  document.getElementById('data').innerHTML=h;
}).catch(e=>console.error(e));}
load();setInterval(load,5000);
</script></body></html>)";
        } else {
            body = "{}";
        }

        std::ostringstream resp;
        resp << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << ct << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Connection: close\r\n\r\n"
             << body;
        auto s = resp.str();
        write(client, s.c_str(), s.size());
        close(client);
    }
    close(server_fd);
}

// ── Signal handler ────────────────────────────────────────────────────────────
static std::atomic<bool> g_running{true};
void signal_handler(int) { g_running = false; }

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    std::printf("[STARTUP] Chimera Trend Engine | build=%s\n", BUILD_VERSION);
    std::fflush(stdout);

    acquire_instance_lock();
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Executor
    chimera::SpotExecutor executor;
    bool exec_ok = executor.init("config/binance_credentials.json");
    if (!exec_ok) {
        std::fprintf(stderr, "[STARTUP] WARNING: executor init failed — shadow mode only\n");
    }

    // Trend engine
    chimera::TrendEngine engine;
    engine.shadow_mode = true;  // always shadow until explicitly authorized
    if (exec_ok) engine.set_executor(&executor);
    g_engine_ptr = &engine;

    // HTTP GUI thread
    std::thread http_thread(http_server_thread, 8080);
    http_thread.detach();

    // WebSocket feed
    chimera::BinanceWSFeed feed;
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (mid <= 0.0) return;

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        double tick_age_ms = 0.0;
        if (tick.trade_time > 0) {
            tick_age_ms = static_cast<double>(now_ms - tick.trade_time);
            if (tick_age_ms < 0.0) tick_age_ms = 0.0;
        }
        if (tick_age_ms > 150.0) return;

        {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            engine.on_tick(id, tick, now_ms);
        }

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 5000 == 0) {
            std::printf("[TICK] n=%d | %s px=%.2f | age=%.1fms | trades=%d\n",
                n, tick.symbol.c_str(), mid, tick_age_ms, engine.total_trades());
            std::fflush(stdout);
        }
    });

    feed.start();
    std::printf("[STARTUP] Feed live. Waiting for H1 bars to warm up (~50 bars = ~50 hours)...\n");
    std::printf("[STARTUP] GUI: http://localhost:8080\n");
    std::fflush(stdout);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);

    std::atomic<bool> shutdown_done{false};
    std::thread watchdog([&](){
        for (int i = 0; i < 30; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (shutdown_done) return;
        }
        release_instance_lock(); std::_Exit(0);
    });

    feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();

    std::printf("[SHUTDOWN] trades=%d pnl=%.3f%%\n",
                engine.total_trades(), engine.total_pnl_pct());
    std::fflush(stdout);
    release_instance_lock();
    return 0;
}
