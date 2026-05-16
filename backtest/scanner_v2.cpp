// ============================================================================
// scanner_v2.cpp — Enhanced multi-symbol, multi-strategy, multi-timeframe scanner
//
// Extends scanner.cpp with:
//   1. H4 timeframe (14400s) — never tested before
//   2. Wider Bollinger bands (BB_K=2.5, 3.0) on D1
//   3. H1 TSMOM for all symbols
//   4. Expanded parameter grid
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include scanner_v2.cpp -o scanner_v2
//
// Run:
//   ./scanner_v2
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
#include <map>

#include <unistd.h>

#include "core/EdgeEngine.hpp"

namespace fs = std::filesystem;

// ── Kline loader ────────────────────────────────────────────────────────────
struct Kline {
    int64_t open_ts_ms;
    double  o, h, l, c;
};

static bool parse_kline_element(const char* start, const char* end, Kline& out) {
    const char* p = start;
    while (p < end && *p != '[') ++p;
    if (p >= end) return false;
    ++p;

    out.open_ts_ms = 0;
    while (p < end && (*p == ' ' || *p == '\t')) ++p;
    while (p < end && *p >= '0' && *p <= '9') {
        out.open_ts_ms = out.open_ts_ms * 10 + (*p - '0');
        ++p;
    }

    auto read_quoted_double = [&](double& val) -> bool {
        while (p < end && *p != '"') ++p;
        if (p >= end) return false;
        ++p;
        const char* num_start = p;
        while (p < end && *p != '"') ++p;
        if (p >= end) return false;
        std::string s(num_start, p);
        val = std::stod(s);
        ++p;
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

    const char* data = content.c_str();
    const char* end  = data + content.size();
    const char* p = data;

    while (p < end && *p != '[') ++p;
    if (p >= end) return out;
    ++p;

    while (p < end) {
        while (p < end && *p != '[') ++p;
        if (p >= end) break;

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

    std::sort(all.begin(), all.end(), [](const Kline& a, const Kline& b) {
        return a.open_ts_ms < b.open_ts_ms;
    });

    auto last = std::unique(all.begin(), all.end(), [](const Kline& a, const Kline& b) {
        return a.open_ts_ms == b.open_ts_ms;
    });
    all.erase(last, all.end());

    return all;
}

// ── Synthesize arbitrary-timeframe bars from H1 klines ──────────────────────
static std::vector<Kline> synthesize_tf(const std::vector<Kline>& h1, int64_t tf_secs) {
    std::vector<Kline> result;
    if (h1.empty()) return result;

    Kline cur{};
    int64_t cur_block = -1;

    for (auto& bar : h1) {
        int64_t block = (bar.open_ts_ms / 1000) / tf_secs;
        if (block != cur_block) {
            if (cur_block >= 0 && cur.o > 0.0) {
                result.push_back(cur);
            }
            cur_block = block;
            cur.open_ts_ms = block * tf_secs * 1000;
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
        result.push_back(cur);
    }

    return result;
}

// ── Scan result ─────────────────────────────────────────────────────────────
struct ScanResult {
    std::string tag;
    std::string symbol;
    std::string strategy;
    std::string tf_label;
    int         tf_secs;
    int         lookback;
    int         hold_bars;
    double      sl_atr_mult;
    double      trail_arm_atr;
    double      trail_dist_atr;
    double      bb_k;
    double      cost_bp;
    int         total_bars;
    int         trades;
    int         wins;
    double      total_bp;
    double      win_rate;
    double      pf;
    double      avg_bp;
    double      max_dd_bp;
    double      sharpe;
};

static ScanResult run_scan(const std::string& tag,
                            const std::string& symbol,
                            chimera::StrategyKind kind,
                            int64_t tf_secs,
                            const std::string& tf_label,
                            int lookback, int hold_bars,
                            double sl_atr, double trail_arm, double trail_dist,
                            double bb_k,
                            double cost_bp,
                            const std::vector<Kline>& klines) {
    ScanResult r{};
    r.tag            = tag;
    r.symbol         = symbol;
    r.strategy       = chimera::strategy_name(kind);
    r.tf_label       = tf_label;
    r.tf_secs        = (int)tf_secs;
    r.lookback       = lookback;
    r.hold_bars      = hold_bars;
    r.sl_atr_mult    = sl_atr;
    r.trail_arm_atr  = trail_arm;
    r.trail_dist_atr = trail_dist;
    r.bb_k           = bb_k;
    r.cost_bp        = cost_bp;

    if (klines.empty()) return r;

    chimera::EdgeEngine::Config cfg{
        .symbol         = symbol,
        .tag            = tag,
        .kind           = kind,
        .tf_secs        = tf_secs,
        .lookback       = lookback,
        .hold_bars      = hold_bars,
        .sl_atr_mult    = sl_atr,
        .atr_period     = 14,
        .bb_k           = bb_k,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = cost_bp,
        .max_history    = std::max(lookback + 5, 19),
        .trail_arm_atr  = trail_arm,
        .trail_dist_atr = trail_dist,
    };

    // Suppress EdgeEngine printf
    fflush(stdout);
    int saved_stdout = dup(fileno(stdout));
    FILE* devnull = fopen("/dev/null", "w");
    dup2(fileno(devnull), fileno(stdout));

    chimera::EdgeEngine engine(cfg);

    int total = (int)klines.size();
    int seed_count = (int)(total * 0.8);

    // Seed with IS data
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

    // Feed OOS bars
    std::vector<double> trade_returns;
    double equity_bp = 0.0, peak_bp = 0.0, max_dd = 0.0;
    int prev_trades = 0;
    double prev_total_bp = 0.0;

    r.total_bars = total - seed_count;
    for (int i = seed_count; i < total; ++i) {
        const Kline& k = klines[i];
        int64_t bar_start_ms = k.open_ts_ms;
        int64_t tick_step = (tf_secs * 1000) / 4;
        bool bullish = (k.c >= k.o);

        engine.on_tick(k.o, bar_start_ms);
        if (bullish) {
            engine.on_tick(k.l, bar_start_ms + tick_step);
            engine.on_tick(k.h, bar_start_ms + tick_step * 2);
        } else {
            engine.on_tick(k.h, bar_start_ms + tick_step);
            engine.on_tick(k.l, bar_start_ms + tick_step * 2);
        }

        if (i == total - 1) {
            engine.on_tick(k.c, bar_start_ms + tf_secs * 1000 + 1000);
        }

        if (engine.trades() > prev_trades) {
            double trade_bp = engine.total_bp() - prev_total_bp;
            trade_returns.push_back(trade_bp);
            equity_bp = engine.total_bp();
            if (equity_bp > peak_bp) peak_bp = equity_bp;
            double dd = peak_bp - equity_bp;
            if (dd > max_dd) max_dd = dd;
            prev_trades = engine.trades();
            prev_total_bp = engine.total_bp();
        }
    }

    // Restore stdout
    fflush(stdout);
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);
    fclose(devnull);

    r.trades   = engine.trades();
    r.wins     = engine.wins();
    r.total_bp = engine.total_bp();
    r.win_rate = r.trades > 0 ? (double)r.wins / r.trades * 100.0 : 0.0;
    r.avg_bp   = r.trades > 0 ? r.total_bp / r.trades : 0.0;
    r.max_dd_bp = max_dd;

    double sum_wins = 0.0, sum_losses = 0.0;
    for (double ret : trade_returns) {
        if (ret > 0) sum_wins   += ret;
        else         sum_losses += std::fabs(ret);
    }
    r.pf = (sum_losses > 0.0) ? sum_wins / sum_losses : (sum_wins > 0.0 ? 99.9 : 0.0);

    if (trade_returns.size() >= 2) {
        double mean = r.total_bp / (double)trade_returns.size();
        double var = 0.0;
        for (double ret : trade_returns) {
            double d = ret - mean;
            var += d * d;
        }
        var /= (double)(trade_returns.size() - 1);
        double sd = std::sqrt(var);
        double oos_days = (double)r.total_bars * tf_secs / 86400.0;
        double trades_per_year = (oos_days > 0) ? r.trades / oos_days * 365.0 : 0.0;
        r.sharpe = (sd > 0.0) ? (mean / sd) * std::sqrt(trades_per_year) : 0.0;
    }

    return r;
}

// ══════════════════════════════════════════════════════════════════════════════
int main() {
    std::string data_dir = "data";
    if (!fs::exists("data/btc_d1_part1.json")) {
        if (fs::exists("backtest/data/btc_d1_part1.json")) {
            data_dir = "backtest/data";
        } else {
            std::fprintf(stderr, "ERROR: cannot find kline data.\n");
            return 1;
        }
    }

    std::printf("[LOAD] Loading kline data from %s/ ...\n", data_dir.c_str());

    // ── Load all H1 kline data (base for synthesis) ─────────────────────
    auto btc_h1  = load_all_parts(data_dir, "btc_h1_part");
    auto eth_h1  = load_all_parts(data_dir, "ethusdt_h1_part");
    auto sol_h1  = load_all_parts(data_dir, "solusdt_h1_part");
    auto xrp_h1  = load_all_parts(data_dir, "xrpusdt_h1_part");
    auto link_h1 = load_all_parts(data_dir, "linkusdt_h1_part");

    // ── Load D1 data (direct files where available) ─────────────────────
    auto btc_d1  = load_all_parts(data_dir, "btc_d1_part");
    auto eth_d1  = load_all_parts(data_dir, "ethusdt_d1_part");
    auto sol_d1  = load_all_parts(data_dir, "solusdt_d1_part");
    auto xrp_d1  = load_all_parts(data_dir, "xrpusdt_d1_part");
    auto link_d1 = load_all_parts(data_dir, "linkusdt_d1_part");

    // ── Synthesize H4 (14400s) — NEW, never tested ──────────────────────
    auto btc_h4  = synthesize_tf(btc_h1,  14400);
    auto eth_h4  = synthesize_tf(eth_h1,  14400);
    auto sol_h4  = synthesize_tf(sol_h1,  14400);
    auto xrp_h4  = synthesize_tf(xrp_h1,  14400);
    auto link_h4 = synthesize_tf(link_h1, 14400);

    // ── Synthesize H6 (21600s) ──────────────────────────────────────────
    auto btc_h6  = synthesize_tf(btc_h1,  21600);
    auto eth_h6  = synthesize_tf(eth_h1,  21600);
    auto sol_h6  = synthesize_tf(sol_h1,  21600);
    auto xrp_h6  = synthesize_tf(xrp_h1,  21600);
    auto link_h6 = synthesize_tf(link_h1, 21600);

    // ── Synthesize H12 (43200s) — wider swing timeframe ─────────────────
    auto btc_h12 = synthesize_tf(btc_h1,  43200);
    auto eth_h12 = synthesize_tf(eth_h1,  43200);
    auto sol_h12 = synthesize_tf(sol_h1,  43200);
    auto xrp_h12 = synthesize_tf(xrp_h1,  43200);
    auto link_h12= synthesize_tf(link_h1, 43200);

    std::printf("[LOAD] BTC  H1=%zu  H4=%zu  H6=%zu  H12=%zu  D1=%zu\n",
        btc_h1.size(), btc_h4.size(), btc_h6.size(), btc_h12.size(), btc_d1.size());
    std::printf("[LOAD] ETH  H1=%zu  H4=%zu  H6=%zu  H12=%zu  D1=%zu\n",
        eth_h1.size(), eth_h4.size(), eth_h6.size(), eth_h12.size(), eth_d1.size());
    std::printf("[LOAD] SOL  H1=%zu  H4=%zu  H6=%zu  H12=%zu  D1=%zu\n",
        sol_h1.size(), sol_h4.size(), sol_h6.size(), sol_h12.size(), sol_d1.size());
    std::printf("[LOAD] XRP  H1=%zu  H4=%zu  H6=%zu  H12=%zu  D1=%zu\n",
        xrp_h1.size(), xrp_h4.size(), xrp_h6.size(), xrp_h12.size(), xrp_d1.size());
    std::printf("[LOAD] LINK H1=%zu  H4=%zu  H6=%zu  H12=%zu  D1=%zu\n",
        link_h1.size(), link_h4.size(), link_h6.size(), link_h12.size(), link_d1.size());

    // ── Costs by symbol ─────────────────────────────────────────────────
    double cost_btc  = 17.0;
    double cost_eth  = 17.0;
    double cost_sol  = 20.0;
    double cost_xrp  = 20.0;
    double cost_link = 22.0;

    // ── Parameter combos ────────────────────────────────────────────────
    struct ParamSet {
        int    lookback;
        int    hold_bars;
        double sl_atr;
        double trail_arm;
        double trail_dist;
        double bb_k;       // only used for BOLLINGER; ignored by others
    };

    // TSMOM params — expanded grid for H4/H1 (shorter hold makes sense)
    std::vector<ParamSet> tsmom_params = {
        // Short lookback (fast momentum)
        { 5,  4, 2.0, 0.8, 0.3, 2.0},
        { 5,  6, 2.0, 0.8, 0.4, 2.0},
        { 5,  8, 2.5, 1.0, 0.4, 2.0},
        { 5, 12, 2.5, 1.0, 0.5, 2.0},
        // Medium lookback
        { 8,  4, 2.0, 0.8, 0.3, 2.0},
        { 8,  6, 2.0, 0.8, 0.4, 2.0},
        { 8,  8, 2.5, 1.0, 0.4, 2.0},
        { 8, 12, 2.5, 1.0, 0.5, 2.0},
        { 8, 16, 3.0, 1.2, 0.5, 2.0},
        {10,  4, 2.0, 0.8, 0.3, 2.0},
        {10,  6, 2.0, 0.8, 0.4, 2.0},
        {10,  8, 2.5, 1.0, 0.4, 2.0},
        {10, 12, 3.0, 1.0, 0.4, 2.0},
        {10, 16, 3.0, 1.2, 0.5, 2.0},
        {10, 20, 3.0, 1.0, 0.4, 2.0},
        // Longer lookback
        {15,  6, 2.0, 0.8, 0.4, 2.0},
        {15,  8, 2.5, 1.0, 0.4, 2.0},
        {15, 12, 3.0, 1.0, 0.4, 2.0},
        {15, 16, 3.0, 1.2, 0.5, 2.0},
        {15, 20, 3.5, 1.2, 0.5, 2.0},
        {20,  8, 2.5, 1.0, 0.5, 2.0},
        {20, 12, 3.0, 1.0, 0.5, 2.0},
        {20, 16, 3.5, 1.2, 0.5, 2.0},
        {20, 24, 3.5, 1.5, 0.6, 2.0},
        {25,  8, 2.5, 0.8, 0.4, 2.0},
        {25, 12, 3.0, 1.0, 0.5, 2.0},
        {25, 16, 3.5, 1.2, 0.6, 2.0},
        {30, 12, 3.0, 1.0, 0.5, 2.0},
        {30, 16, 3.5, 1.2, 0.6, 2.0},
        {30, 24, 4.0, 1.5, 0.8, 2.0},
        {40, 16, 3.5, 1.2, 0.6, 2.0},
        {40, 20, 3.5, 1.0, 0.8, 2.0},
        {40, 24, 4.0, 1.5, 0.8, 2.0},
    };

    // DONCHIAN params
    std::vector<ParamSet> donch_params = {
        { 5,  4, 2.0, 0.8, 0.3, 2.0},
        { 5,  8, 2.5, 1.0, 0.4, 2.0},
        { 8,  6, 2.0, 0.8, 0.4, 2.0},
        { 8,  8, 2.5, 1.0, 0.4, 2.0},
        { 8, 12, 2.5, 1.0, 0.5, 2.0},
        {10,  8, 2.5, 1.0, 0.4, 2.0},
        {10, 12, 3.0, 1.0, 0.4, 2.0},
        {10, 16, 3.0, 1.2, 0.5, 2.0},
        {15,  8, 2.5, 1.0, 0.4, 2.0},
        {15, 12, 3.0, 1.0, 0.5, 2.0},
        {15, 16, 3.5, 1.2, 0.5, 2.0},
        {20,  8, 2.5, 1.0, 0.5, 2.0},
        {20, 12, 3.0, 1.0, 0.5, 2.0},
        {20, 16, 3.5, 1.2, 0.6, 2.0},
        {20, 24, 3.5, 1.5, 0.6, 2.0},
        {25, 12, 3.0, 1.0, 0.5, 2.0},
        {25, 16, 3.5, 1.2, 0.6, 2.0},
        {30, 16, 3.5, 1.2, 0.6, 2.0},
        {30, 24, 4.0, 1.5, 0.8, 2.0},
        {40, 16, 3.5, 1.2, 0.6, 2.0},
        {40, 24, 4.0, 1.5, 0.8, 2.0},
    };

    // BOLLINGER params — NOW WITH WIDER BANDS (K=2.5, 3.0)
    // Previous scanner only tested K=2.0
    std::vector<ParamSet> bb_params = {
        // K=2.0 (original)
        {20,  4, 2.0, 0.8, 0.3, 2.0},
        {20,  8, 2.5, 1.0, 0.4, 2.0},
        {20, 12, 2.5, 1.0, 0.5, 2.0},
        {20, 16, 3.0, 1.2, 0.5, 2.0},
        {30,  8, 2.5, 1.0, 0.4, 2.0},
        {30, 12, 2.5, 1.0, 0.5, 2.0},
        {30, 16, 3.0, 1.2, 0.5, 2.0},
        // K=2.5 (wider — fewer but higher-quality signals)
        {20,  4, 2.0, 0.8, 0.3, 2.5},
        {20,  6, 2.0, 0.8, 0.4, 2.5},
        {20,  8, 2.5, 1.0, 0.4, 2.5},
        {20, 12, 2.5, 1.0, 0.5, 2.5},
        {20, 16, 3.0, 1.2, 0.5, 2.5},
        {20, 24, 3.5, 1.5, 0.6, 2.5},
        {30,  6, 2.0, 0.8, 0.3, 2.5},
        {30,  8, 2.5, 1.0, 0.4, 2.5},
        {30, 12, 2.5, 1.0, 0.5, 2.5},
        {30, 16, 3.0, 1.2, 0.5, 2.5},
        {40,  8, 2.5, 1.0, 0.4, 2.5},
        {40, 12, 3.0, 1.0, 0.5, 2.5},
        {40, 16, 3.0, 1.2, 0.5, 2.5},
        // K=3.0 (very wide — extreme oversold only)
        {20,  4, 2.0, 0.8, 0.3, 3.0},
        {20,  6, 2.0, 0.8, 0.4, 3.0},
        {20,  8, 2.5, 1.0, 0.4, 3.0},
        {20, 12, 2.5, 1.0, 0.5, 3.0},
        {20, 16, 3.0, 1.2, 0.5, 3.0},
        {30,  6, 2.0, 0.8, 0.3, 3.0},
        {30,  8, 2.5, 1.0, 0.4, 3.0},
        {30, 12, 2.5, 1.0, 0.5, 3.0},
        {30, 16, 3.0, 1.2, 0.5, 3.0},
        {40,  8, 2.5, 1.0, 0.4, 3.0},
        {40, 12, 3.0, 1.0, 0.5, 3.0},
    };

    // RSI_REVERT params — expanded thresholds tested via lookback
    std::vector<ParamSet> rsi_params = {
        {14,  4, 2.0, 0.8, 0.3, 2.0},
        {14,  6, 2.0, 0.8, 0.4, 2.0},
        {14,  8, 2.5, 1.0, 0.4, 2.0},
        {14, 12, 2.5, 1.0, 0.5, 2.0},
        {14, 16, 3.0, 1.2, 0.5, 2.0},
        {14, 24, 3.5, 1.5, 0.6, 2.0},
        {14, 32, 4.0, 1.5, 0.8, 2.0},
        {21,  6, 2.0, 0.8, 0.4, 2.0},
        {21,  8, 2.5, 1.0, 0.4, 2.0},
        {21, 12, 2.5, 1.0, 0.5, 2.0},
        {21, 16, 3.0, 1.2, 0.5, 2.0},
    };

    // ── Define all scan jobs ────────────────────────────────────────────
    struct ScanJob {
        std::string                  symbol;
        std::string                  label;
        chimera::StrategyKind        kind;
        int64_t                      tf_secs;
        std::string                  tf_label;
        double                       cost;
        const std::vector<Kline>*    klines;
        const std::vector<ParamSet>* params;
    };

    std::vector<ScanJob> jobs = {
        // ══════════════════════════════════════════════════════════════════
        // H4 TIMEFRAME — NEVER TESTED (14400s)
        // ══════════════════════════════════════════════════════════════════

        // TSMOM H4
        {"btcusdt",  "BTC",  chimera::StrategyKind::TSMOM,    14400, "H4", cost_btc,  &btc_h4,  &tsmom_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::TSMOM,    14400, "H4", cost_eth,  &eth_h4,  &tsmom_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::TSMOM,    14400, "H4", cost_sol,  &sol_h4,  &tsmom_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::TSMOM,    14400, "H4", cost_xrp,  &xrp_h4,  &tsmom_params},
        {"linkusdt", "LINK", chimera::StrategyKind::TSMOM,    14400, "H4", cost_link, &link_h4, &tsmom_params},

        // DONCHIAN H4
        {"btcusdt",  "BTC",  chimera::StrategyKind::DONCHIAN, 14400, "H4", cost_btc,  &btc_h4,  &donch_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::DONCHIAN, 14400, "H4", cost_eth,  &eth_h4,  &donch_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::DONCHIAN, 14400, "H4", cost_sol,  &sol_h4,  &donch_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::DONCHIAN, 14400, "H4", cost_xrp,  &xrp_h4,  &donch_params},
        {"linkusdt", "LINK", chimera::StrategyKind::DONCHIAN, 14400, "H4", cost_link, &link_h4, &donch_params},

        // ══════════════════════════════════════════════════════════════════
        // H12 TIMEFRAME — NEVER TESTED (43200s)
        // ══════════════════════════════════════════════════════════════════

        // TSMOM H12
        {"btcusdt",  "BTC",  chimera::StrategyKind::TSMOM,    43200, "H12", cost_btc,  &btc_h12,  &tsmom_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::TSMOM,    43200, "H12", cost_eth,  &eth_h12,  &tsmom_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::TSMOM,    43200, "H12", cost_sol,  &sol_h12,  &tsmom_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::TSMOM,    43200, "H12", cost_xrp,  &xrp_h12,  &tsmom_params},
        {"linkusdt", "LINK", chimera::StrategyKind::TSMOM,    43200, "H12", cost_link, &link_h12, &tsmom_params},

        // DONCHIAN H12
        {"btcusdt",  "BTC",  chimera::StrategyKind::DONCHIAN, 43200, "H12", cost_btc,  &btc_h12,  &donch_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::DONCHIAN, 43200, "H12", cost_eth,  &eth_h12,  &donch_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::DONCHIAN, 43200, "H12", cost_sol,  &sol_h12,  &donch_params},

        // ══════════════════════════════════════════════════════════════════
        // H1 TSMOM — NEVER TESTED ON ALTS
        // ══════════════════════════════════════════════════════════════════
        {"btcusdt",  "BTC",  chimera::StrategyKind::TSMOM,    3600, "H1", cost_btc,  &btc_h1,  &tsmom_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::TSMOM,    3600, "H1", cost_eth,  &eth_h1,  &tsmom_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::TSMOM,    3600, "H1", cost_sol,  &sol_h1,  &tsmom_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::TSMOM,    3600, "H1", cost_xrp,  &xrp_h1,  &tsmom_params},
        {"linkusdt", "LINK", chimera::StrategyKind::TSMOM,    3600, "H1", cost_link, &link_h1, &tsmom_params},

        // ══════════════════════════════════════════════════════════════════
        // WIDER BOLLINGER BANDS on D1 (K=2.5, 3.0 — original only tested 2.0)
        // ══════════════════════════════════════════════════════════════════
        {"btcusdt",  "BTC",  chimera::StrategyKind::BOLLINGER, 86400, "D1", cost_btc,  &btc_d1,  &bb_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::BOLLINGER, 86400, "D1", cost_eth,  &eth_d1,  &bb_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::BOLLINGER, 86400, "D1", cost_sol,  &sol_d1,  &bb_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::BOLLINGER, 86400, "D1", cost_xrp,  &xrp_d1,  &bb_params},
        {"linkusdt", "LINK", chimera::StrategyKind::BOLLINGER, 86400, "D1", cost_link, &link_d1, &bb_params},

        // BOLLINGER H4 (wider bands)
        {"btcusdt",  "BTC",  chimera::StrategyKind::BOLLINGER, 14400, "H4", cost_btc,  &btc_h4,  &bb_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::BOLLINGER, 14400, "H4", cost_eth,  &eth_h4,  &bb_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::BOLLINGER, 14400, "H4", cost_sol,  &sol_h4,  &bb_params},

        // ══════════════════════════════════════════════════════════════════
        // RSI_REVERT on D1 + H4 (expanded)
        // ══════════════════════════════════════════════════════════════════
        {"btcusdt",  "BTC",  chimera::StrategyKind::RSI_REVERT, 86400, "D1", cost_btc,  &btc_d1,  &rsi_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::RSI_REVERT, 86400, "D1", cost_eth,  &eth_d1,  &rsi_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::RSI_REVERT, 86400, "D1", cost_sol,  &sol_d1,  &rsi_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::RSI_REVERT, 86400, "D1", cost_xrp,  &xrp_d1,  &rsi_params},
        {"linkusdt", "LINK", chimera::StrategyKind::RSI_REVERT, 86400, "D1", cost_link, &link_d1, &rsi_params},

        // RSI H4
        {"btcusdt",  "BTC",  chimera::StrategyKind::RSI_REVERT, 14400, "H4", cost_btc,  &btc_h4,  &rsi_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::RSI_REVERT, 14400, "H4", cost_eth,  &eth_h4,  &rsi_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::RSI_REVERT, 14400, "H4", cost_sol,  &sol_h4,  &rsi_params},

        // ══════════════════════════════════════════════════════════════════
        // DONCHIAN on D1 (re-scan with expanded params)
        // ══════════════════════════════════════════════════════════════════
        {"btcusdt",  "BTC",  chimera::StrategyKind::DONCHIAN, 86400, "D1", cost_btc,  &btc_d1,  &donch_params},
        {"ethusdt",  "ETH",  chimera::StrategyKind::DONCHIAN, 86400, "D1", cost_eth,  &eth_d1,  &donch_params},
        {"solusdt",  "SOL",  chimera::StrategyKind::DONCHIAN, 86400, "D1", cost_sol,  &sol_d1,  &donch_params},
        {"xrpusdt",  "XRP",  chimera::StrategyKind::DONCHIAN, 86400, "D1", cost_xrp,  &xrp_d1,  &donch_params},
        {"linkusdt", "LINK", chimera::StrategyKind::DONCHIAN, 86400, "D1", cost_link, &link_d1, &donch_params},
    };

    // ── Run all scans ───────────────────────────────────────────────────
    std::vector<ScanResult> all_results;

    int total_scans = 0;
    for (auto& job : jobs) total_scans += (int)job.params->size();
    std::printf("\n[SCAN] %d total scan combinations across %d jobs\n\n", total_scans, (int)jobs.size());

    int done = 0;
    for (auto& job : jobs) {
        for (auto& p : *job.params) {
            char tag[64];
            snprintf(tag, sizeof(tag), "%s-%s-%s",
                     job.label.c_str(),
                     chimera::strategy_name(job.kind),
                     job.tf_label.c_str());

            ScanResult r = run_scan(tag, job.symbol, job.kind, job.tf_secs,
                                     job.tf_label,
                                     p.lookback, p.hold_bars,
                                     p.sl_atr, p.trail_arm, p.trail_dist,
                                     p.bb_k,
                                     job.cost, *job.klines);
            all_results.push_back(r);
            done++;

            if (done % 50 == 0) {
                std::fprintf(stderr, "\r  [%d / %d] ...", done, total_scans);
                fflush(stderr);
            }
        }
    }
    std::fprintf(stderr, "\r  [%d / %d] done.    \n", done, total_scans);

    // ── Sort by PF descending ───────────────────────────────────────────
    std::sort(all_results.begin(), all_results.end(),
        [](const ScanResult& a, const ScanResult& b) {
            if (std::fabs(a.pf - b.pf) > 0.001) return a.pf > b.pf;
            return a.sharpe > b.sharpe;
        });

    // ── Print results with >= 5 trades ──────────────────────────────────
    std::printf("\n");
    std::printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  CHIMERA SCANNER v2 — All combos with >= 5 OOS trades, sorted by PF                                                                     ║\n");
    std::printf("╠═══════════════════╦════════╦══════════╦════╦════╦════╦═════╦═════╦═════╦══════╦════╦═════════╦══════╦══════╦══════════╦════════╦═══════════╣\n");
    std::printf("║ Tag               ║ Symbol ║ Strategy ║ TF ║ LB ║ HB ║ SL  ║ T_A ║ T_D ║ Trds ║ WR ║ Net(bp) ║  PF  ║ Shrp ║ MaxDD bp ║ AvgBP  ║ BBk/Cost  ║\n");
    std::printf("╠═══════════════════╬════════╬══════════╬════╬════╬════╬═════╬═════╬═════╬══════╬════╬═════════╬══════╬══════╬══════════╬════════╬═══════════╣\n");

    int printed = 0;
    for (auto& r : all_results) {
        if (r.trades < 5) continue;
        if (printed >= 100) break;

        const char* highlight = "";
        if (r.pf >= 1.3 && r.sharpe > 0.3 && r.trades >= 10) highlight = " ***";
        else if (r.pf >= 1.15 && r.trades >= 10) highlight = " **";
        else if (r.pf >= 1.0 && r.trades >= 8) highlight = " *";

        std::printf("║ %-17s ║ %-6s ║ %-8s ║ %-2s ║%3d ║ %2d ║ %3.1f ║ %3.1f ║ %3.1f ║ %4d ║%3.0f ║ %+7.0f ║ %4.2f ║ %4.2f ║ %8.0f ║ %+5.0f   ║ %.1f/%2.0f    ║%s\n",
            r.tag.c_str(), r.symbol.c_str(), r.strategy.c_str(),
            r.tf_label.c_str(),
            r.lookback, r.hold_bars, r.sl_atr_mult,
            r.trail_arm_atr, r.trail_dist_atr,
            r.trades, r.win_rate, r.total_bp, r.pf, r.sharpe,
            r.max_dd_bp, r.avg_bp, r.bb_k, r.cost_bp, highlight);
        printed++;
    }
    std::printf("╚═══════════════════╩════════╩══════════╩════╩════╩════╩═════╩═════╩═════╩══════╩════╩═════════╩══════╩══════╩══════════╩════════╩═══════════╝\n");

    // ── Summary: best per (symbol, strategy, tf) — only NEW combos ─────
    std::printf("\n── BEST PER COMBO (>= 8 trades, PF > 1.0) — focus on NEW timeframes ─\n");

    struct ComboKey {
        std::string key;
        bool operator<(const ComboKey& o) const { return key < o.key; }
    };
    std::map<ComboKey, const ScanResult*> best_per_combo;

    for (auto& r : all_results) {
        if (r.trades < 8 || r.pf <= 1.0) continue;
        std::string k = r.tag;
        // For BOLLINGER, include bb_k in key
        if (r.strategy == "BOLLINGER") {
            char buf[16];
            snprintf(buf, sizeof(buf), "-K%.1f", r.bb_k);
            k += buf;
        }
        ComboKey key{k};
        auto it = best_per_combo.find(key);
        if (it == best_per_combo.end() || r.pf > it->second->pf) {
            best_per_combo[key] = &r;
        }
    }

    if (best_per_combo.empty()) {
        std::printf("  No combos found with >= 8 trades and PF > 1.0\n");
    } else {
        for (auto& [key, r] : best_per_combo) {
            const char* verdict;
            if (r->pf >= 1.3 && r->sharpe > 0.3)
                verdict = ">>> OPTIMIZER CANDIDATE <<<";
            else if (r->pf >= 1.15 && r->sharpe > 0.2)
                verdict = "PROMISING";
            else
                verdict = "MARGINAL";

            std::printf("  %-22s  PF=%.2f  Sharpe=%.2f  Trades=%d  Net=%+.0fbp  AvgBP=%+.0f  →  %s\n",
                key.key.c_str(), r->pf, r->sharpe, r->trades,
                r->total_bp, r->avg_bp, verdict);
            std::printf("    params: LB=%d HB=%d SL=%.1f T_A=%.1f T_D=%.1f BBk=%.1f cost=%.0fbp\n",
                r->lookback, r->hold_bars, r->sl_atr_mult,
                r->trail_arm_atr, r->trail_dist_atr, r->bb_k, r->cost_bp);
        }
    }
    std::printf("────────────────────────────────────────────────────────────────\n\n");

    // ── Count profitable combos per timeframe ───────────────────────────
    std::printf("── PROFITABILITY BY TIMEFRAME ──────────────────────────────────\n");
    std::map<std::string, std::pair<int,int>> tf_stats;  // total, profitable
    for (auto& r : all_results) {
        if (r.trades < 5) continue;
        tf_stats[r.tf_label].first++;
        if (r.pf > 1.0) tf_stats[r.tf_label].second++;
    }
    for (auto& [tf, stats] : tf_stats) {
        double pct = stats.first > 0 ? 100.0 * stats.second / stats.first : 0.0;
        std::printf("  %-4s  tested=%3d  profitable=%3d  (%.1f%%)\n",
            tf.c_str(), stats.first, stats.second, pct);
    }
    std::printf("────────────────────────────────────────────────────────────────\n\n");

    return 0;
}
