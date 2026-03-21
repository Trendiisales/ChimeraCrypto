// ============================================================================
// Chimera  main entry point
// HTTP server is owned by QuadEngineBalancedEngine (port 8080)
// main.cpp only manages feed, executor, and signal handler.
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <sys/file.h>   // flock()
#include <fcntl.h>      // open()
#include <unistd.h>     // getpid(), write(), close()

#include "live/BinanceWSFeed.hpp"
#include "live/SpotExecutor.hpp"
#include "core/QuadEngineBalancedEngine.hpp"
#include "core/LiquidationFeed.hpp"
#include "core/market_data/FundingRateFetcher.hpp"
#include "core/market_data/NGASFetcher.hpp"
#include "core/NGASLeadLagEngine.hpp"
#include "core/FundingSignalEngine.hpp"
#include "live/CoinbaseWSFeed.hpp"
#include "execution/ExchangeLatencyEngine.hpp"
#include "execution/NetworkLatencySystem.hpp"
#include "core/SymbolIndex.hpp"
#include "config/TradingConfig.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

// ─────────────────────────────────────────────────────────────────────────────
// RollingLogger — tees stdout+stderr to daily log files
// Files: logs/chimera_YYYY-MM-DD.log
// Retention: LOG_KEEP_DAYS days (older files auto-deleted)
// Thread-safe: uses a mutex on write
// ─────────────────────────────────────────────────────────────────────────────
#include <mutex>
#include <ctime>
#include <cstring>
#include <cerrno>

class RollingLogger {
public:
    static constexpr int LOG_KEEP_DAYS = 7;
    static constexpr const char* LOG_DIR = "logs";

    RollingLogger() { open_today(); }

    ~RollingLogger() { flush_and_close(); }

    // Call once at startup to redirect stdout+stderr
    void install() {
        // Redirect stdout
        orig_stdout_ = stdout;
        pipe(stdout_pipe_);
        orig_stdout_fd_ = dup(STDOUT_FILENO);
        dup2(stdout_pipe_[1], STDOUT_FILENO);
        close(stdout_pipe_[1]);

        // Redirect stderr
        orig_stderr_fd_ = dup(STDERR_FILENO);
        dup2(stdout_pipe_[1], STDERR_FILENO);  // merge stderr into same pipe

        // Start drain thread
        drain_thread_ = std::thread([this]{ drain_loop(); });
    }

    // Write directly (bypass pipe — for pre-install messages)
    void write_direct(const char* msg) {
        std::lock_guard<std::mutex> lk(mtx_);
        check_rotate();
        if (file_.is_open()) {
            file_ << msg;
            file_.flush();
        }
    }

    std::string current_path() const { return current_path_; }

    void flush_and_close() {
        running_ = false;
        if (drain_thread_.joinable()) drain_thread_.join();
        std::lock_guard<std::mutex> lk(mtx_);
        if (file_.is_open()) { file_.flush(); file_.close(); }
    }

private:
    std::ofstream            file_;
    std::string              current_path_;
    int                      current_day_ = -1;
    std::mutex               mtx_;
    std::atomic<bool>        running_{true};
    std::thread              drain_thread_;
    FILE*                    orig_stdout_ = nullptr;
    int                      orig_stdout_fd_ = -1;
    int                      orig_stderr_fd_ = -1;
    int                      stdout_pipe_[2] = {-1, -1};

    static std::string utc_date_str() {
        time_t t = time(nullptr);
        struct tm ti{};
        gmtime_r(&t, &ti);
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                 ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday);
        return buf;
    }

    static int utc_day() {
        time_t t = time(nullptr);
        struct tm ti{};
        gmtime_r(&t, &ti);
        return ti.tm_yday;
    }

    void open_today() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (file_.is_open()) { file_.flush(); file_.close(); }
        mkdir(LOG_DIR, 0755);
        current_path_ = std::string(LOG_DIR) + "/chimera_" + utc_date_str() + ".log";
        file_.open(current_path_, std::ios::app);
        current_day_ = utc_day();
        if (file_.is_open()) {
            file_ << "\n=== LOG OPENED " << utc_date_str() << " ===\n";
            file_.flush();
        }
        purge_old_logs();
    }

    void check_rotate() {
        // called under mtx_
        if (utc_day() != current_day_) {
            if (file_.is_open()) { file_.flush(); file_.close(); }
            mkdir(LOG_DIR, 0755);
            current_path_ = std::string(LOG_DIR) + "/chimera_" + utc_date_str() + ".log";
            file_.open(current_path_, std::ios::app);
            current_day_ = utc_day();
            if (file_.is_open()) {
                file_ << "\n=== LOG ROTATED " << utc_date_str() << " ===\n";
                file_.flush();
            }
            purge_old_logs();
        }
    }

    void purge_old_logs() {
        // called under mtx_
        DIR* dir = opendir(LOG_DIR);
        if (!dir) return;
        std::vector<std::string> files;
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            std::string name(ent->d_name);
            if (name.rfind("chimera_", 0) == 0 && name.size() > 4 &&
                name.substr(name.size()-4) == ".log") {
                files.push_back(std::string(LOG_DIR) + "/" + name);
            }
        }
        closedir(dir);
        std::sort(files.begin(), files.end());
        while (static_cast<int>(files.size()) > LOG_KEEP_DAYS) {
            std::remove(files.front().c_str());
            files.erase(files.begin());
        }
    }

    void drain_loop() {
        // Drain the stdout pipe, tee to original stdout fd + log file
        char buf[4096];
        while (running_.load()) {
            struct timeval tv{0, 50000};  // 50ms timeout
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(stdout_pipe_[0], &fds);
            int n = select(stdout_pipe_[0] + 1, &fds, nullptr, nullptr, &tv);
            if (n <= 0) continue;
            ssize_t bytes = read(stdout_pipe_[0], buf, sizeof(buf)-1);
            if (bytes <= 0) continue;
            buf[bytes] = '\0';
            // Write to original stdout
            write(orig_stdout_fd_, buf, bytes);
            // Write to log file
            std::lock_guard<std::mutex> lk(mtx_);
            check_rotate();
            if (file_.is_open()) {
                file_.write(buf, bytes);
                file_.flush();
            }
        }
        // Drain remainder
        while (true) {
            struct timeval tv{0, 5000};
            fd_set fds; FD_ZERO(&fds); FD_SET(stdout_pipe_[0], &fds);
            if (select(stdout_pipe_[0]+1, &fds, nullptr, nullptr, &tv) <= 0) break;
            ssize_t bytes = read(stdout_pipe_[0], buf, sizeof(buf)-1);
            if (bytes <= 0) break;
            buf[bytes] = '\0';
            write(orig_stdout_fd_, buf, bytes);
            std::lock_guard<std::mutex> lk(mtx_);
            if (file_.is_open()) file_.write(buf, bytes);
        }
    }
};

static RollingLogger* g_logger = nullptr;

//  SINGLE-INSTANCE LOCK 
// Uses an advisory flock() on a PID file.
// The lock is automatically released by the OS if the process crashes or exits.
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) {
        std::fprintf(stderr, "[FATAL] Cannot open lock file %s\n", PID_LOCK_FILE);
        std::exit(1);
    }

    // Non-blocking exclusive lock  fails immediately if another instance holds it
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        // Read the PID of the existing instance from the file
        char buf[32] = {};
        (void)::read(g_lock_fd, buf, sizeof(buf) - 1);
        ::close(g_lock_fd);
        std::fprintf(stderr,
            "\n\n"
            "  CHIMERA ALREADY RUNNING  SECOND INSTANCE BLOCKED  \n"
            "\n"
            "  Existing PID: %-35s\n"
            "  Lock file:    %-35s\n"
            "  To stop the running instance: kill %s       \n"
            "  Or: pkill chimera                               \n"
            "\n",
            buf, PID_LOCK_FILE, buf);
        std::exit(1);
    }

    // Write our PID into the lock file for diagnostics
    (void)::ftruncate(g_lock_fd, 0);
    char pidbuf[32];
    int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    (void)::write(g_lock_fd, pidbuf, len);
    ::fsync(g_lock_fd);
    // Note: g_lock_fd intentionally left open  closing it releases the flock
    std::printf("[STARTUP] Instance lock acquired | PID=%d | lock=%s\n",
                (int)::getpid(), PID_LOCK_FILE);
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN);
        ::close(g_lock_fd);
        ::unlink(PID_LOCK_FILE);
        g_lock_fd = -1;
    }
}
// 

chimera::ExchangeLatencyEngine g_exchange_latency;
chimera::FundingRateFetcher    g_funding;
Chimera::NetworkLatencySystem  g_network_latency;
chimera::NGASFetcher           g_ngas_fetcher;
chimera::NGASLeadLagEngine     g_ngas_engine;
chimera::FundingSignalEngine   g_funding_signal;

static std::atomic<bool> g_running{true};
static std::atomic<int>  g_sig_count{0};

struct PriceCache {
    std::atomic<uint64_t> bits[chimera::MAX_SYMBOLS] = {};
    void set(int id, double v) {
        uint64_t b; __builtin_memcpy(&b, &v, 8);
        bits[id].store(b, std::memory_order_relaxed);
    }
    double get(int id) const {
        uint64_t b = bits[id].load(std::memory_order_relaxed);
        double v; __builtin_memcpy(&v, &b, 8); return v;
    }
} price_cache;

void signal_handler(int sig) {
    int n = g_sig_count.fetch_add(1) + 1;
    g_running = false;
    if (n == 1) {
        // First Ctrl+C  request clean shutdown
        std::fprintf(stderr, "\n[SHUTDOWN] Signal %d  stopping cleanly (Ctrl+C again to force kill)\n", sig);
    } else {
        // Second Ctrl+C  something is hanging, force exit immediately
        std::fprintf(stderr, "\n[SHUTDOWN] Forced exit.\n");
        release_instance_lock();
    if (g_logger) { g_logger->flush_and_close(); }
        std::_Exit(0);
    }
}

int main() {
    //  0. Rolling log — install first so all output is captured
    g_logger = new RollingLogger();
    printf("[STARTUP] Rolling log: %s (7-day retention)\n", g_logger->current_path().c_str());
    // Read git hash dynamically at runtime -- never stale
    {
        char git_hash[64] = "unknown";
        FILE* fp = popen("git -C \"$(dirname \"$(readlink -f /proc/self/exe)\")\" rev-parse --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null", "r");
        if (fp) { if (fgets(git_hash, sizeof(git_hash), fp)) {
            size_t len = strlen(git_hash);
            if (len > 0 && git_hash[len-1] == '\n') git_hash[len-1] = '\0';
        } pclose(fp); }
        printf("[STARTUP] Build version: %s\n", git_hash);
    }
    fflush(stdout);

    //  1. Single-instance lock  must be first 
    acquire_instance_lock();

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    //  1. Executor / API keys 
    chimera::SpotExecutor executor;
    bool executor_ok = executor.init("config/binance_credentials.json");
    if (!executor_ok) {
        std::fprintf(stderr,
            "[STARTUP] WARNING: executor init failed  orders will be skipped.\n"
            "[STARTUP] Edit config/binance_credentials.json with real api_key/api_secret.\n");
    }

    //  2. Engine (owns HTTP server on port 8080) 
    chimera::QuadEngineBalancedEngine controller;

    if (executor_ok) {
        controller.set_executor(&executor);
        std::printf("[STARTUP] Executor wired | shadow=%s\n",
                    executor.is_shadow() ? "YES (paper)" : "NO  LIVE");
        std::fflush(stdout);
    }

    // Funding rate background fetch (FundingRateFetcher -> engine)
    std::thread([&]() {
        g_funding.fetch();
        controller.set_funding_fetcher(&g_funding);
    }).detach();

    // FundingSignalEngine -- independent signal wrapper, separate fetch thread
    std::thread([&]() {
        g_funding_signal.fetch();
        controller.set_funding_signal(&g_funding_signal);
    }).detach();

    //  2c. NGAS fetcher  Natural Gas macro lead-lag signal 
    g_ngas_engine.set_fetcher(&g_ngas_fetcher);
    controller.set_ngas_engine(&g_ngas_engine);
    g_ngas_fetcher.start();
    std::printf("[STARTUP] NGAS macro fetcher started (5-min poll, NGO.F via stooq)\n");
    std::fflush(stdout);

    //  2b. Liquidation feed  Binance futures forceOrder stream 
    chimera::LiquidationFeed liq_feed;
    liq_feed.set_callback([&](const chimera::LiquidationEvent& ev) {
        // Route to engine  thread safe (atomic flag in LiquidationEngine)
        controller.liq_engine().on_liquidation(ev);
        if (ev.symbol_id >= 0) {
            std::printf("[LIQ] SHORT LIQ %s | $%.0f | price=%.2f\n",
                chimera::sym_short(ev.symbol_id), ev.notional_usd, ev.price);
            std::fflush(stdout);
        }
    });
    liq_feed.start();
    std::printf("[STARTUP] Liquidation feed started (fstream.binance.com)\n");
    std::fflush(stdout);

    //  3. WebSocket feed 
    chimera::BinanceWSFeed feed;
    // Add all symbols  order must match SymbolIndex.hpp (BTC=0, ETH=1, SOL=2, ...)
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        price_cache.set(id, mid);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Per-tick data age: how old is this price when we receive it.
        // Used as the latency_ms signal gate — stale ticks are skipped.
        // p95 is kept for GUI/monitoring only, NOT for signal gating.
        // BUG WAS: passing p95 (~36ms constant) as latency_ms caused ALL
        // signal gates (LEADLAG=35ms, IMBAL=25ms) to permanently block
        // after the first 2048 ticks. Only early-session trades fired.
        double tick_age_ms = 0.0;
        if (tick.trade_time > 0) {
            tick_age_ms = static_cast<double>(now_ms - tick.trade_time);
            if (tick_age_ms < 0.0) tick_age_ms = 0.0;
            g_exchange_latency.record(now_ms, tick.trade_time); // p95 for GUI only
        }

        // Don't gate on calibration — fire from tick 1.
        // Hard ceiling: skip only truly stale ticks (>200ms = feed issue).
        if (tick_age_ms > 200.0) return;

        controller.on_tick(id, tick, now_ms, tick_age_ms);

        // EMA of raw tick_age_ms — same clock both sides (AWS NTP), no correction needed
        // alpha=0.01: ~100-tick smoothing window gives stable display without lag
        static double ema_age_ms = 0.0;
        if (ema_age_ms == 0.0) ema_age_ms = tick_age_ms;
        else ema_age_ms = 0.99 * ema_age_ms + 0.01 * tick_age_ms;

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 500 == 0) {
            // Update GUI WS Delay display every 500 ticks (~0.5s) from EMA
            controller.set_lat_p95(ema_age_ms);
            std::printf("[TICK] n=%d | %s px=%.2f | tick_age=%.1fms | ema_age=%.1fms | fills=%d\n",
                n, tick.symbol.c_str(), mid,
                tick_age_ms, ema_age_ms,
                executor_ok ? executor.fills() : 0);
            std::fflush(stdout);
        }
    });

    feed.start();

    // Coinbase BTC-USD cross-exchange price reference feed
    chimera::CoinbaseWSFeed coinbase_feed;
    coinbase_feed.set_callback([&](double price, int64_t ts) {
        controller.update_coinbase_btc(price, ts);
    });
    coinbase_feed.start();
    std::printf("[STARTUP] Coinbase BTC-USD feed started (cross-exchange reference)\n");
    std::fflush(stdout);

    std::printf("[STARTUP] Feed live. Calibrating latency...\n");
    std::printf("[CONFIG] Regime thresholds: GRINDBUILDUP=%.2f  BUILDUPBREAKOUT=%.2f\n",
        chimera::TradingConfig::REGIME_GRIND_EXIT_TO_BUILDUP,
        chimera::TradingConfig::REGIME_BUILDUP_TO_BREAKOUT);
    std::printf("[CONFIG] LeadLag BTC threshold: %.1fbp  Target max: %.1fbp\n",
        chimera::TradingConfig::LEADLAG_BTC_THRESHOLD_BP,
        chimera::TradingConfig::LEADLAG_TARGET_MAX_BP);
    std::printf("[CONFIG] IMBAL threshold: %.2f  Spread max: %.1fbp\n",
        chimera::TradingConfig::IMBALANCE_THRESHOLD,
        chimera::TradingConfig::IMBALANCE_MAX_SPREAD_BPS);
    std::fflush(stdout);

    //  4. Main loop 
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //  5. Shutdown 
    std::printf("\n[SHUTDOWN] Stopping feed...\n");
    std::fflush(stdout);

    // Watchdog: if feed.stop() hangs for >3s, force exit
    std::atomic<bool> shutdown_done{false};
    std::thread watchdog([&](){
        for (int i = 0; i < 30; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (shutdown_done) return;
        }
        std::fprintf(stderr, "[SHUTDOWN] Feed stop timeout  forcing exit\n");
        release_instance_lock();
        std::_Exit(0);
    });

    feed.stop();
    coinbase_feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();
    std::printf("[SHUTDOWN] fills=%d errors=%d trades=%d pnl=%.2fbp\n",
                executor_ok ? executor.fills()  : 0,
                executor_ok ? executor.errors() : 0,
                controller.get_total_trades(),
                controller.get_total_pnl());
    std::printf("[SHUTDOWN] Clean exit.\n");
    std::fflush(stdout);
    release_instance_lock();
    return 0;
}
