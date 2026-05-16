// ============================================================================
// backtest_harness.cpp — C++ backtester using the REAL EdgeEngine.hpp
//
// Reads Binance JSON kline files, feeds them through EdgeEngine::on_tick()
// with the exact same configs as main.cpp. No reimplementation — this is
// the production signal/exit logic running on historical data.
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include backtest_harness.cpp -o backtest
//
// Usage:
//   ./backtest          (runs all 7 engines, prints stats)
//   ./backtest --csv    (machine-readable CSV output)
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <numeric>

#include "core/EdgeEngine.hpp"

namespace fs = std::filesystem;

// ── Minimal JSON kline parser ────────────────────────────────────────────────
// Binance klines: [[open_ts, "o", "h", "l", "c", "vol", close_ts, ...], ...]
// We parse just enough to extract: open_ts_ms, o, h, l, c
struct Kline {
    int64_t open_ts_ms;
    double  o, h, l, c;
};

// Strip whitespace / brackets, parse one kline array element
static bool parse_kline_element(const char* start, const char* end, Kline& out) {
    // find the opening '['
    const char* p = start;
    while (p < end && *p != '[') ++p;
    if (p >= end) return false;
    ++p; // skip '['

    // field 0: open_ts_ms (integer)
    out.open_ts_ms = 0;
    while (p < end && (*p == ' ' || *p == '\t')) ++p;
    while (p < end && *p >= '0' && *p <= '9') {
        out.open_ts_ms = out.open_ts_ms * 10 + (*p - '0');
        ++p;
    }

    // skip comma, then fields 1-4 are quoted doubles: "o","h","l","c"
    auto read_quoted_double = [&](double& val) -> bool {
        while (p < end && *p != '"') ++p;
        if (p >= end) return false;
        ++p; // skip opening quote
        const char* num_start = p;
        while (p < end && *p != '"') ++p;
        if (p >= end) return false;
        std::string s(num_start, p);
        val = std::stod(s);
        ++p; // skip closing quote
        return true;
    };

    if (!read_quoted_double(out.o)) return false;
    if (!read_quoted_double(out.h)) return false;
    if (!read_quoted_double(out.l)) return false;
    if (!read_quoted_double(out.c)) return false;

    return (out.o > 0.0 && out.h > 0.0 && out.l > 0.0 && out.c > 0.0);
}

static std::vector<Kline> load_klines_from_json(const std::string& path) {
    std::vector<Kline> out;
    std::ifstream f(path);
    if (!f.is_open()) return out;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    f.close();

    // Split by "],["  — each kline is a sub-array
    const char* data = content.c_str();
    const char* end  = data + content.size();
    const char* p = data;

    // Find first '['
    while (p < end && *p != '[') ++p;
    if (p >= end) return out;
    ++p; // skip outer '['

    // Now parse each sub-array
    while (p < end) {
        // find next '['
        while (p < end && *p != '[') ++p;
        if (p >= end) break;

        // find matching ']'
        const char* sub_start = p;
        int depth = 0;
        while (p < end) {
            if (*p == '[') ++depth;
            if (*p == ']') { --depth; if (depth == 0) { ++p; break; } }
            ++p;
        }

        Kline k{};
        if (parse_kline_element(sub_start, p, k)) {
            out.push_back(k);
        }
    }

    return out;
}

// Load all part files matching a glob pattern, merge and sort by timestamp
static std::vector<Kline> load_all_parts(const std::string& dir,
                                          const std::string& prefix) {
    std::vector<Kline> all;
    std::vector<std::string> files;

    for (auto& entry : fs::directory_iterator(dir)) {
        std::string fname = entry.path().filename().string();
        if (fname.find(prefix) == 0 && fname.find(".json") != std::string::npos) {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());

    for (auto& f : files) {
        auto klines = load_klines_from_json(f);
        all.insert(all.end(), klines.begin(), klines.end());
    }

    // Sort by timestamp and deduplicate
    std::sort(all.begin(), all.end(), [](const Kline& a, const Kline& b) {
        return a.open_ts_ms < b.open_ts_ms;
    });

    // Remove duplicates (same open_ts_ms)
    auto last = std::unique(all.begin(), all.end(), [](const Kline& a, const Kline& b) {
        return a.open_ts_ms == b.open_ts_ms;
    });
    all.erase(last, all.end());

    return all;
}

// ── Synthesize H6 bars from H1 klines ────────────────────────────────────────
// Group H1 bars into 6-hour blocks aligned to 00:00, 06:00, 12:00, 18:00 UTC
static std::vector<Kline> synthesize_h6(const std::vector<Kline>& h1) {
    std::vector<Kline> h6;
    if (h1.empty()) return h6;

    const int64_t h6_secs = 21600;

    Kline cur{};
    int64_t cur_block = -1;

    for (auto& bar : h1) {
        int64_t block = (bar.open_ts_ms / 1000) / h6_secs;
        if (block != cur_block) {
            if (cur_block >= 0 && cur.o > 0.0) {
                h6.push_back(cur);
            }
            cur_block = block;
            cur.open_ts_ms = block * h6_secs * 1000;
            cur.o = bar.o;
            cur.h = bar.h;
            cur.l = bar.l;
            cur.c = bar.c;
        } else {
            if (bar.h > cur.h) cur.h = bar.h;
            if (bar.l < cur.l) cur.l = bar.l;
            cur.c = bar.c;
        }
    }
    if (cur_block >= 0 && cur.o > 0.0) {
        h6.push_back(cur);
    }

    return h6;
}

// ── Feed klines through an EdgeEngine ────────────────────────────────────────
// For each bar we simulate 4 ticks in OHLC order appropriate to bar direction:
//   Bullish bar (c >= o): O → L → H → C
//   Bearish bar (c <  o): O → H → L → C
// This ensures intra-bar SL hits are detected at the correct price.
struct BacktestResult {
    std::string tag;
    std::string symbol;
    std::string strategy;
    int         tf_secs;
    int         total_bars;
    int         trades;
    int         wins;
    double      total_bp;
    double      win_rate;
    double      pf;           // profit factor
    double      avg_bp;
    double      max_dd_bp;    // max drawdown in bp
    double      sharpe;       // annualized Sharpe (crude)
    double      cost_bp;
};

static BacktestResult run_backtest(chimera::EdgeEngine& engine,
                                    const chimera::EdgeEngine::Config& cfg,
                                    const std::vector<Kline>& klines) {
    BacktestResult r{};
    r.tag      = cfg.tag;
    r.symbol   = cfg.symbol;
    r.strategy = chimera::strategy_name(cfg.kind);
    r.tf_secs  = (int)cfg.tf_secs;
    r.cost_bp  = cfg.round_trip_bp;

    if (klines.empty()) {
        r.total_bars = 0;
        return r;
    }

    // Seed with first 80% of bars, backtest on remaining 20% (OOS)
    int total = (int)klines.size();
    int seed_count = (int)(total * 0.8);
    int oos_start  = seed_count;

    // Seed bars
    std::vector<chimera::EdgeEngine::SeedBar> seeds;
    seeds.reserve(seed_count);
    for (int i = 0; i < seed_count; ++i) {
        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = klines[i].open_ts_ms;
        sb.o = klines[i].o;
        sb.h = klines[i].h;
        sb.l = klines[i].l;
        sb.c = klines[i].c;
        seeds.push_back(sb);
    }
    engine.seed_bars(seeds);

    // Track equity curve for drawdown / Sharpe
    std::vector<double> trade_returns;
    double equity_bp     = 0.0;
    double peak_bp       = 0.0;
    double max_dd        = 0.0;
    int    prev_trades   = engine.trades();
    double prev_total_bp = engine.total_bp();

    // Feed OOS bars as ticks
    r.total_bars = total - oos_start;
    for (int i = oos_start; i < total; ++i) {
        const Kline& k = klines[i];
        int64_t bar_start_ms = k.open_ts_ms;
        int64_t tick_step    = (cfg.tf_secs * 1000) / 4;

        bool bullish = (k.c >= k.o);

        // Tick 1: Open
        engine.on_tick(k.o, bar_start_ms);

        if (bullish) {
            // Tick 2: Low (test SL first)
            engine.on_tick(k.l, bar_start_ms + tick_step);
            // Tick 3: High
            engine.on_tick(k.h, bar_start_ms + tick_step * 2);
        } else {
            // Tick 2: High (test trail arm first)
            engine.on_tick(k.h, bar_start_ms + tick_step);
            // Tick 3: Low (test SL)
            engine.on_tick(k.l, bar_start_ms + tick_step * 2);
        }

        // Tick 4: Close (triggers bar close via next bar boundary)
        // We don't send the close as the last tick of this bar — instead
        // the next iteration's open tick will close THIS bar via bar_id change.
        // But for the final bar, we need to force it:
        if (i == total - 1) {
            // Send a tick 1 second into the next bar to force close
            engine.on_tick(k.c, bar_start_ms + cfg.tf_secs * 1000 + 1000);
        }

        // Check if a trade completed this bar
        if (engine.trades() > prev_trades) {
            double trade_bp = engine.total_bp() - prev_total_bp;
            trade_returns.push_back(trade_bp);
            equity_bp = engine.total_bp();
            if (equity_bp > peak_bp) peak_bp = equity_bp;
            double dd = peak_bp - equity_bp;
            if (dd > max_dd) max_dd = dd;
            prev_trades   = engine.trades();
            prev_total_bp = engine.total_bp();
        }
    }

    r.trades   = engine.trades();
    r.wins     = engine.wins();
    r.total_bp = engine.total_bp();
    r.win_rate = r.trades > 0 ? (double)r.wins / r.trades * 100.0 : 0.0;
    r.avg_bp   = r.trades > 0 ? r.total_bp / r.trades : 0.0;
    r.max_dd_bp = max_dd;

    // Profit factor = gross_wins / gross_losses
    double sum_wins = 0.0, sum_losses = 0.0;
    for (double ret : trade_returns) {
        if (ret > 0) sum_wins   += ret;
        else         sum_losses += std::fabs(ret);
    }
    r.pf = (sum_losses > 0.0) ? sum_wins / sum_losses : (sum_wins > 0.0 ? 99.9 : 0.0);

    // Crude annualized Sharpe: mean(trade_returns) / stdev * sqrt(trades_per_year)
    if (trade_returns.size() >= 2) {
        double mean = r.total_bp / (double)trade_returns.size();
        double var = 0.0;
        for (double ret : trade_returns) {
            double d = ret - mean;
            var += d * d;
        }
        var /= (double)(trade_returns.size() - 1);
        double sd = std::sqrt(var);

        // Estimate trades per year from OOS period
        double oos_days = (double)r.total_bars * cfg.tf_secs / 86400.0;
        double trades_per_year = (oos_days > 0) ? r.trades / oos_days * 365.0 : 0.0;

        r.sharpe = (sd > 0.0) ? (mean / sd) * std::sqrt(trades_per_year) : 0.0;
    } else {
        r.sharpe = 0.0;
    }

    return r;
}

// ── Pretty print ─────────────────────────────────────────────────────────────
static void print_header() {
    std::printf("\n");
    std::printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  CHIMERA BACKTEST RESULTS — Out-of-Sample (last 20%% of data, ~2025-2026)                          ║\n");
    std::printf("╠═══════════════════╦════════╦══════╦══════╦═════════╦════════╦══════╦══════════╦════════╦═══════════╣\n");
    std::printf("║ Engine            ║ Trades ║ Wins ║ WR%%  ║ Net(bp) ║ PF     ║ Avg  ║ MaxDD bp ║ Sharpe ║ Cost(bp)  ║\n");
    std::printf("╠═══════════════════╬════════╬══════╬══════╬═════════╬════════╬══════╬══════════╬════════╬═══════════╣\n");
}

static void print_row(const BacktestResult& r) {
    std::printf("║ %-17s ║ %6d ║ %4d ║ %4.1f ║ %+7.1f ║ %6.2f ║ %+4.0f ║ %8.1f ║ %6.2f ║ %7.1f   ║\n",
        r.tag.c_str(), r.trades, r.wins, r.win_rate,
        r.total_bp, r.pf, r.avg_bp, r.max_dd_bp, r.sharpe, r.cost_bp);
}

static void print_footer() {
    std::printf("╚═══════════════════╩════════╩══════╩══════╩═════════╩════════╩══════╩══════════╩════════╩═══════════╝\n");
}

static void print_verdict(const std::vector<BacktestResult>& results) {
    std::printf("\n── VERDICT ──────────────────────────────────────────────────────\n");
    for (auto& r : results) {
        const char* status;
        if (r.trades < 5)              status = "INSUFFICIENT DATA";
        else if (r.pf >= 1.15 && r.sharpe > 0.3) status = "DEPLOY ✓";
        else if (r.pf >= 1.0)          status = "MARGINAL — needs tuning";
        else                           status = "NO EDGE ✗";

        std::printf("  %-18s  PF=%.2f  Sharpe=%.2f  →  %s\n",
            r.tag.c_str(), r.pf, r.sharpe, status);
    }
    std::printf("────────────────────────────────────────────────────────────────\n\n");
}

// ══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    bool csv_mode = (argc > 1 && std::string(argv[1]) == "--csv");

    std::string data_dir = "data";

    // Detect if running from backtest/ or project root
    if (fs::exists("data/btc_h1_part0.json")) {
        data_dir = "data";
    } else if (fs::exists("backtest/data/btc_h1_part0.json")) {
        data_dir = "backtest/data";
    } else {
        std::fprintf(stderr, "ERROR: cannot find kline data. Run from backtest/ or project root.\n");
        return 1;
    }

    std::printf("[LOAD] Loading kline data from %s/ ...\n", data_dir.c_str());

    // ── Load all kline data ──────────────────────────────────────────────
    auto btc_h1  = load_all_parts(data_dir, "btc_h1_part");
    auto btc_d1  = load_all_parts(data_dir, "btc_d1_part");
    auto eth_h1  = load_all_parts(data_dir, "ethusdt_h1_part");
    auto sol_h1  = load_all_parts(data_dir, "solusdt_h1_part");
    auto xrp_h1  = load_all_parts(data_dir, "xrpusdt_h1_part");
    auto link_h1 = load_all_parts(data_dir, "linkusdt_h1_part");

    // Synthesize H6 from H1
    auto eth_h6  = synthesize_h6(eth_h1);
    auto sol_h6  = synthesize_h6(sol_h1);
    auto link_h6 = synthesize_h6(link_h1);

    std::printf("[LOAD] BTC  H1=%zu  D1=%zu\n", btc_h1.size(), btc_d1.size());
    std::printf("[LOAD] ETH  H1=%zu  H6=%zu\n", eth_h1.size(), eth_h6.size());
    std::printf("[LOAD] SOL  H1=%zu  H6=%zu\n", sol_h1.size(), sol_h6.size());
    std::printf("[LOAD] XRP  H1=%zu\n",         xrp_h1.size());
    std::printf("[LOAD] LINK H1=%zu  H6=%zu\n", link_h1.size(), link_h6.size());

    // ── Engine configs — EXACT match to main.cpp ─────────────────────────

    // 1. BTC-TSMOM-D1
    chimera::EdgeEngine::Config btc_tsmom_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 10.0,
    };

    // 2. ETH-BB-H6
    chimera::EdgeEngine::Config eth_bb_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BB-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 10.0,
    };

    // 3. SOL-DONCH-H6
    chimera::EdgeEngine::Config sol_donch_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 10.0,
    };

    // 4. XRP-DONCH-H1
    chimera::EdgeEngine::Config xrp_donch_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H1",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 3600,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 10.0,
    };

    // 5. LINK-RSI-H6
    chimera::EdgeEngine::Config link_rsi_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 20,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 10.0,
    };

    // 6. BTC-OVERNIGHT-H1 (new)
    chimera::EdgeEngine::Config overnight_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-OVERNIGHT-H1",
        .kind           = chimera::StrategyKind::OVERNIGHT,
        .tf_secs        = 3600,
        .lookback       = 20,
        .hold_bars      = 2,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
        .entry_hour_utc = 21,
    };

    // 7. BTC-WEEKDAY-D1 (new)
    chimera::EdgeEngine::Config weekday_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-WEEKDAY-D1",
        .kind           = chimera::StrategyKind::WEEKDAY,
        .tf_secs        = 86400,
        .lookback       = 20,
        .hold_bars      = 3,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .entry_hour_utc = 21,
        .entry_dow      = 1,
        .sma_len        = 5,
    };

    // ── Instantiate engines ──────────────────────────────────────────────
    chimera::EdgeEngine e1(btc_tsmom_cfg);
    chimera::EdgeEngine e2(eth_bb_cfg);
    chimera::EdgeEngine e3(sol_donch_cfg);
    chimera::EdgeEngine e4(xrp_donch_cfg);
    chimera::EdgeEngine e5(link_rsi_cfg);
    chimera::EdgeEngine e6(overnight_cfg);
    chimera::EdgeEngine e7(weekday_cfg);

    // ── Run backtests ────────────────────────────────────────────────────
    std::printf("\n[RUN] Running OOS backtests (seed 80%%, test 20%%) ...\n\n");

    std::vector<BacktestResult> results;
    results.push_back(run_backtest(e1, btc_tsmom_cfg,  btc_d1));
    results.push_back(run_backtest(e2, eth_bb_cfg,     eth_h6));
    results.push_back(run_backtest(e3, sol_donch_cfg,  sol_h6));
    results.push_back(run_backtest(e4, xrp_donch_cfg,  xrp_h1));
    results.push_back(run_backtest(e5, link_rsi_cfg,   link_h6));
    results.push_back(run_backtest(e6, overnight_cfg,  btc_h1));
    results.push_back(run_backtest(e7, weekday_cfg,    btc_d1));

    // ── Output ───────────────────────────────────────────────────────────
    if (csv_mode) {
        std::printf("tag,symbol,strategy,tf,bars,trades,wins,wr,net_bp,pf,avg_bp,maxdd_bp,sharpe,cost_bp\n");
        for (auto& r : results) {
            std::printf("%s,%s,%s,%d,%d,%d,%d,%.1f,%.1f,%.2f,%.1f,%.1f,%.2f,%.1f\n",
                r.tag.c_str(), r.symbol.c_str(), r.strategy.c_str(),
                r.tf_secs, r.total_bars, r.trades, r.wins, r.win_rate,
                r.total_bp, r.pf, r.avg_bp, r.max_dd_bp, r.sharpe, r.cost_bp);
        }
    } else {
        print_header();
        for (auto& r : results) print_row(r);
        print_footer();
        print_verdict(results);
    }

    return 0;
}
