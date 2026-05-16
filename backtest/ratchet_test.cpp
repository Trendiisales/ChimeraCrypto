// ============================================================================
// ratchet_test.cpp — A/B test: current trail vs staged ratcheting trail
//
// Takes an engine's existing optimized params and compares:
//   A) Baseline (no ratchet, trail_tighten_atr = 0)
//   B) Various ratchet settings (threshold × tighter_dist grid)
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include ratchet_test.cpp -o ratchet_test
//
// Usage:
//   ./ratchet_test <symbol> <strategy> <tf> <cost_bp> <lb> <hb> <sl> <ta> <td>
//
// Example:
//   ./ratchet_test xrpusdt TSMOM H6 20 20 12 2.5 1.0 0.5
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

// ── Kline loader (same as optimizer_general) ────────────────────────────────
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
        char buf[64];
        int bi = 0;
        while (p < end && *p != '"' && bi < 63) buf[bi++] = *p++;
        buf[bi] = '\0';
        val = std::atof(buf);
        if (p < end) ++p;
        return true;
    };

    if (!read_quoted_double(out.o)) return false;
    if (!read_quoted_double(out.h)) return false;
    if (!read_quoted_double(out.l)) return false;
    if (!read_quoted_double(out.c)) return false;
    return true;
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

    // Skip outer array bracket
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

static std::vector<Kline> load_all_parts(const std::string& dir, const std::string& prefix) {
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

static std::vector<Kline> synthesize_tf(const std::vector<Kline>& h1, int64_t target_tf_secs) {
    if (h1.empty()) return {};
    std::vector<Kline> out;
    int64_t tf_ms = target_tf_secs * 1000;
    int64_t cur_start = (h1[0].open_ts_ms / tf_ms) * tf_ms;
    Kline acc{cur_start, h1[0].o, h1[0].h, h1[0].l, h1[0].c};

    for (size_t i = 1; i < h1.size(); ++i) {
        int64_t bar_start = (h1[i].open_ts_ms / tf_ms) * tf_ms;
        if (bar_start != cur_start) {
            out.push_back(acc);
            cur_start = bar_start;
            acc = {cur_start, h1[i].o, h1[i].h, h1[i].l, h1[i].c};
        } else {
            if (h1[i].h > acc.h) acc.h = h1[i].h;
            if (h1[i].l < acc.l) acc.l = h1[i].l;
            acc.c = h1[i].c;
        }
    }
    out.push_back(acc);
    return out;
}

// ── Result struct ────────────────────────────────────────────────────────────
struct TestResult {
    double tighten_atr;
    double tighten_dist;
    int    trades;
    int    wins;
    double win_rate;
    double total_bp;
    double pf;
    double sharpe;
    double max_dd_bp;
    double avg_win_bp;
    double avg_loss_bp;
};

// ── Run single backtest ─────────────────────────────────────────────────────
static TestResult run_test(const std::vector<Kline>& klines,
                            const std::vector<chimera::EdgeEngine::SeedBar>& seeds,
                            int seed_count,
                            const std::string& symbol,
                            chimera::StrategyKind kind,
                            int64_t tf_secs,
                            int lookback, int hold_bars, double sl_atr,
                            double trail_arm, double trail_dist,
                            double tighten_atr, double tighten_dist,
                            double cost_bp) {
    TestResult r{};
    r.tighten_atr  = tighten_atr;
    r.tighten_dist = tighten_dist;

    chimera::EdgeEngine::Config cfg{
        .symbol              = symbol,
        .tag                 = "TEST",
        .kind                = kind,
        .tf_secs             = tf_secs,
        .lookback            = lookback,
        .hold_bars           = hold_bars,
        .sl_atr_mult         = sl_atr,
        .atr_period          = 14,
        .bb_k                = 2.0,
        .rsi_threshold       = 30.0,
        .round_trip_bp       = cost_bp,
        .max_history         = std::max(lookback + 5, 19),
        .trail_arm_atr       = trail_arm,
        .trail_dist_atr      = trail_dist,
        .trail_tighten_atr      = tighten_atr,
        .trail_tighten_dist_atr = tighten_dist,
    };

    // Suppress printf
    fflush(stdout);
    int saved_stdout = dup(fileno(stdout));
    FILE* devnull = fopen("/dev/null", "w");
    dup2(fileno(devnull), fileno(stdout));

    chimera::EdgeEngine engine(cfg);
    engine.seed_bars(seeds);

    int total = (int)klines.size();
    std::vector<double> trade_returns;
    double equity_bp = 0.0, peak_bp = 0.0, max_dd = 0.0;
    int prev_trades = 0;
    double prev_total_bp = 0.0;
    double total_win_bp = 0.0, total_loss_bp = 0.0;
    int win_count = 0, loss_count = 0;

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
        engine.on_tick(k.c, bar_start_ms + tick_step * 3);

        int cur_trades = engine.trades();
        if (cur_trades > prev_trades) {
            double cur_total = engine.total_bp();
            double ret = cur_total - prev_total_bp;
            trade_returns.push_back(ret);
            if (ret > 0) { total_win_bp += ret; win_count++; }
            else { total_loss_bp += std::fabs(ret); loss_count++; }
            equity_bp = cur_total;
            if (equity_bp > peak_bp) peak_bp = equity_bp;
            double dd = peak_bp - equity_bp;
            if (dd > max_dd) max_dd = dd;
            prev_trades = cur_trades;
            prev_total_bp = cur_total;
        }
    }

    // Restore stdout
    fflush(stdout);
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);
    fclose(devnull);

    r.trades   = engine.trades();
    r.wins     = engine.wins();
    r.win_rate = r.trades > 0 ? 100.0 * r.wins / r.trades : 0.0;
    r.total_bp = engine.total_bp();
    r.max_dd_bp = max_dd;
    r.pf       = (total_loss_bp > 0.001) ? total_win_bp / total_loss_bp : (total_win_bp > 0 ? 99.9 : 0.0);
    r.avg_win_bp  = win_count > 0 ? total_win_bp / win_count : 0.0;
    r.avg_loss_bp = loss_count > 0 ? total_loss_bp / loss_count : 0.0;

    if (trade_returns.size() >= 2) {
        double mean = 0.0;
        for (double v : trade_returns) mean += v;
        mean /= trade_returns.size();
        double var = 0.0;
        for (double v : trade_returns) var += (v - mean) * (v - mean);
        var /= (trade_returns.size() - 1);
        double sd = std::sqrt(var);
        r.sharpe = sd > 0.001 ? mean / sd : 0.0;
    }

    return r;
}

int main(int argc, char** argv) {
    if (argc < 10) {
        std::fprintf(stderr, "Usage: ./ratchet_test <symbol> <strategy> <tf> <cost_bp> <lb> <hb> <sl> <ta> <td>\n");
        std::fprintf(stderr, "Example: ./ratchet_test xrpusdt TSMOM H6 20 20 12 2.5 1.0 0.5\n");
        return 1;
    }

    std::string symbol  = argv[1];
    std::string strat_s = argv[2];
    std::string tf_s    = argv[3];
    double cost_bp      = std::atof(argv[4]);
    int    lookback     = std::atoi(argv[5]);
    int    hold_bars    = std::atoi(argv[6]);
    double sl_atr       = std::atof(argv[7]);
    double trail_arm    = std::atof(argv[8]);
    double trail_dist   = std::atof(argv[9]);

    chimera::StrategyKind kind;
    if      (strat_s == "TSMOM")      kind = chimera::StrategyKind::TSMOM;
    else if (strat_s == "DONCHIAN")   kind = chimera::StrategyKind::DONCHIAN;
    else if (strat_s == "BOLLINGER")  kind = chimera::StrategyKind::BOLLINGER;
    else if (strat_s == "RSI_REVERT") kind = chimera::StrategyKind::RSI_REVERT;
    else { std::fprintf(stderr, "Unknown strategy: %s\n", strat_s.c_str()); return 1; }

    // Parse timeframe
    int64_t tf_secs = 0;
    if      (tf_s == "H1")  tf_secs = 3600;
    else if (tf_s == "H2")  tf_secs = 7200;
    else if (tf_s == "H3")  tf_secs = 10800;
    else if (tf_s == "H4")  tf_secs = 14400;
    else if (tf_s == "H6")  tf_secs = 21600;
    else if (tf_s == "H8")  tf_secs = 28800;
    else if (tf_s == "H12") tf_secs = 43200;
    else if (tf_s == "H16") tf_secs = 57600;
    else if (tf_s == "D1")  tf_secs = 86400;
    else if (tf_s == "D2")  tf_secs = 172800;
    else if (tf_s == "D3")  tf_secs = 259200;
    else { std::fprintf(stderr, "Unknown tf: %s\n", tf_s.c_str()); return 1; }

    // Load data
    std::string data_dir = "data";
    if (!fs::exists("data/btc_d1_part1.json")) {
        if (fs::exists("backtest/data/btc_d1_part1.json")) {
            data_dir = "backtest/data";
        } else {
            std::fprintf(stderr, "ERROR: cannot find kline data.\n");
            return 1;
        }
    }
    std::string h1_prefix;
    if (symbol == "btcusdt") {
        h1_prefix = "btc_h1_part";
    } else {
        h1_prefix = symbol + "_h1_part";
    }
    std::vector<Kline> klines;

    if (tf_s == "H1") {
        klines = load_all_parts(data_dir, h1_prefix);
    } else {
        auto h1 = load_all_parts(data_dir, h1_prefix);
        klines = synthesize_tf(h1, tf_secs);
    }

    std::printf("[LOAD] %zu bars loaded for %s %s %s\n", klines.size(), symbol.c_str(), strat_s.c_str(), tf_s.c_str());
    if (klines.size() < 100) {
        std::fprintf(stderr, "ERROR: insufficient data\n");
        return 1;
    }

    int total = (int)klines.size();
    int seed_count = (int)(total * 0.8);

    std::vector<chimera::EdgeEngine::SeedBar> seeds;
    seeds.reserve(seed_count);
    for (int i = 0; i < seed_count; ++i) {
        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = klines[i].open_ts_ms;
        sb.o = klines[i].o; sb.h = klines[i].h;
        sb.l = klines[i].l; sb.c = klines[i].c;
        seeds.push_back(sb);
    }

    // ── Run baseline (no ratchet) ──────────────────────────────────────────
    TestResult baseline = run_test(klines, seeds, seed_count, symbol, kind, tf_secs,
                                    lookback, hold_bars, sl_atr, trail_arm, trail_dist,
                                    0.0, 0.0, cost_bp);

    // ── Ratchet parameter grid ─────────────────────────────────────────────
    std::vector<double> tighten_thresholds = {1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0};
    std::vector<double> tighten_dists      = {0.1, 0.15, 0.2, 0.25, 0.3, 0.4};

    std::vector<TestResult> results;
    for (double thresh : tighten_thresholds) {
        for (double tdist : tighten_dists) {
            if (tdist >= trail_dist) continue;  // tighten must be tighter than base
            TestResult r = run_test(klines, seeds, seed_count, symbol, kind, tf_secs,
                                     lookback, hold_bars, sl_atr, trail_arm, trail_dist,
                                     thresh, tdist, cost_bp);
            results.push_back(r);
        }
    }

    // ── Print results ──────────────────────────────────────────────────────
    std::string sym_upper;
    if      (symbol == "btcusdt") sym_upper = "BTC";
    else if (symbol == "ethusdt") sym_upper = "ETH";
    else if (symbol == "solusdt") sym_upper = "SOL";
    else if (symbol == "xrpusdt") sym_upper = "XRP";
    else if (symbol == "linkusdt") sym_upper = "LINK";
    else if (symbol == "bnbusdt") sym_upper = "BNB";
    else if (symbol == "dogeusdt") sym_upper = "DOGE";
    else if (symbol == "avaxusdt") sym_upper = "AVAX";
    else if (symbol == "suiusdt") sym_upper = "SUI";
    else if (symbol == "aptusdt") sym_upper = "APT";
    else if (symbol == "nearusdt") sym_upper = "NEAR";
    else if (symbol == "arbusdt") sym_upper = "ARB";
    else sym_upper = symbol;

    std::string tag = sym_upper + "-" + strat_s + "-" + tf_s;

    std::printf("\n");
    std::printf("═══════════════════════════════════════════════════════════════════════════\n");
    std::printf("  RATCHET TRAIL A/B TEST: %s\n", tag.c_str());
    std::printf("  Base params: LB=%d HB=%d SL=%.1f TA=%.1f TD=%.1f cost=%.0fbp\n",
        lookback, hold_bars, sl_atr, trail_arm, trail_dist, cost_bp);
    std::printf("═══════════════════════════════════════════════════════════════════════════\n\n");

    std::printf("  %-12s  %5s  %5s  %5s  %4s  %8s  %6s  %6s  %8s  %s\n",
        "CONFIG", "THRES", "TDIST", "TRDS", "WR%", "NET(bp)", "PF", "SHRP", "MaxDD", "vs BASE");
    std::printf("  ────────────  ─────  ─────  ─────  ────  ────────  ──────  ──────  ────────  ───────\n");

    // Print baseline
    std::printf("  %-12s  %5s  %5s  %5d  %3.0f%%  %+8.0f  %6.2f  %6.2f  %8.0f  %s\n",
        "BASELINE", "---", "---",
        baseline.trades, baseline.win_rate, baseline.total_bp,
        baseline.pf, baseline.sharpe, baseline.max_dd_bp, "---");

    // Sort results by PF descending
    std::sort(results.begin(), results.end(), [](const TestResult& a, const TestResult& b) {
        return a.pf > b.pf;
    });

    for (auto& r : results) {
        double pf_delta = r.pf - baseline.pf;
        double bp_delta = r.total_bp - baseline.total_bp;
        double dd_delta = r.max_dd_bp - baseline.max_dd_bp;
        char verdict[64];
        if (pf_delta > 0.05 && bp_delta > 0 && dd_delta <= 0)
            std::snprintf(verdict, sizeof(verdict), "BETTER (PF+%.2f, DD%.0f)", pf_delta, dd_delta);
        else if (pf_delta > 0.02)
            std::snprintf(verdict, sizeof(verdict), "SLIGHT+ (PF+%.2f)", pf_delta);
        else if (pf_delta < -0.05)
            std::snprintf(verdict, sizeof(verdict), "WORSE (PF%.2f)", pf_delta);
        else
            std::snprintf(verdict, sizeof(verdict), "~SAME (PF%+.2f)", pf_delta);

        std::printf("  RATCHET       %5.1f  %5.2f  %5d  %3.0f%%  %+8.0f  %6.2f  %6.2f  %8.0f  %s\n",
            r.tighten_atr, r.tighten_dist,
            r.trades, r.win_rate, r.total_bp,
            r.pf, r.sharpe, r.max_dd_bp, verdict);
    }

    std::printf("\n  Avg win (base): %.1fbp   Avg loss (base): %.1fbp\n",
        baseline.avg_win_bp, baseline.avg_loss_bp);

    // Find best ratchet
    TestResult* best_ratchet = nullptr;
    for (auto& r : results) {
        if (r.pf > baseline.pf && r.total_bp > baseline.total_bp) {
            best_ratchet = &r;
            break;
        }
    }

    if (best_ratchet) {
        std::printf("\n  >>> BEST RATCHET: threshold=%.1f*ATR, tighten_dist=%.2f*ATR\n",
            best_ratchet->tighten_atr, best_ratchet->tighten_dist);
        std::printf("      PF %.2f→%.2f  Net %+.0f→%+.0fbp  MaxDD %.0f→%.0fbp\n",
            baseline.pf, best_ratchet->pf,
            baseline.total_bp, best_ratchet->total_bp,
            baseline.max_dd_bp, best_ratchet->max_dd_bp);
    } else {
        std::printf("\n  >>> NO RATCHET SETTING IMPROVES ON BASELINE\n");
    }

    std::printf("\n═══════════════════════════════════════════════════════════════════════════\n\n");

    return 0;
}
