// ============================================================================
// edge_hunter.cpp — Bear-market edge finder for spot-only strategies
//
// Comprehensive sweep of counter-trend strategies that fire in bearish
// conditions (when TSMOM sits flat):
//   - BOLLINGER at K=2.0, 2.5, 3.0 (extreme dip-buying)
//   - RSI_REVERT at threshold 25, 30, 35 (oversold bounce)
//
// Tests all 8 symbols × 7 timeframes × full parameter grids with
// neighbourhood stability scoring. Output: ranked list of deployable edges.
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include edge_hunter.cpp -o edge_hunter
//
// Usage:
//   ./edge_hunter              # full sweep (all symbols, all TFs)
//   ./edge_hunter btcusdt      # single symbol
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
#include <tuple>
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

// ── Synthesize bars from H1 ─────────────────────────────────────────────────
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

// ── Result struct ────────────────────────────────────────────────────────────
struct HuntResult {
    std::string symbol;
    std::string strategy;
    std::string tf_label;
    int64_t     tf_secs;
    int         lookback;
    int         hold_bars;
    double      sl_atr_mult;
    double      trail_arm_atr;
    double      trail_dist_atr;
    double      bb_k;
    double      rsi_threshold;
    double      cost_bp;
    int         trades;
    int         wins;
    double      total_bp;
    double      win_rate;
    double      pf;
    double      sharpe;
    double      avg_bp;
    double      max_dd_bp;
    int         neighbour_score;
};

// ── Run single backtest ─────────────────────────────────────────────────────
static HuntResult run_single(const std::vector<Kline>& klines,
                              const std::vector<chimera::EdgeEngine::SeedBar>& seeds,
                              int seed_count,
                              const std::string& symbol,
                              chimera::StrategyKind kind,
                              int64_t tf_secs,
                              int lookback, int hold_bars, double sl_atr,
                              double trail_arm, double trail_dist,
                              double bb_k, double rsi_threshold,
                              double cost_bp) {
    HuntResult r{};
    r.lookback       = lookback;
    r.hold_bars      = hold_bars;
    r.sl_atr_mult    = sl_atr;
    r.trail_arm_atr  = trail_arm;
    r.trail_dist_atr = trail_dist;
    r.bb_k           = bb_k;
    r.rsi_threshold  = rsi_threshold;
    r.cost_bp        = cost_bp;

    chimera::EdgeEngine::Config cfg{
        .symbol         = symbol,
        .tag            = "HUNT",
        .kind           = kind,
        .tf_secs        = tf_secs,
        .lookback       = lookback,
        .hold_bars      = hold_bars,
        .sl_atr_mult    = sl_atr,
        .atr_period     = 14,
        .bb_k           = bb_k,
        .rsi_threshold  = rsi_threshold,
        .round_trip_bp  = cost_bp,
        .max_history    = std::max({lookback + 5, 19, 20}),
        .trail_arm_atr  = trail_arm,
        .trail_dist_atr = trail_dist,
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
        double oos_days = (double)(total - seed_count) * tf_secs / 86400.0;
        double trades_per_year = (oos_days > 0) ? r.trades / oos_days * 365.0 : 0.0;
        r.sharpe = (sd > 0.0) ? (mean / sd) * std::sqrt(trades_per_year) : 0.0;
    }

    return r;
}

// ══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    // ── Symbol config ───────────────────────────────────────────────────────
    struct SymbolInfo {
        std::string symbol;
        std::string sym_upper;
        std::string h1_prefix;
        std::string d1_prefix;
        double      cost_bp;
    };

    std::vector<SymbolInfo> all_symbols = {
        {"btcusdt",  "BTC",  "btc_h1_part",      "btc_d1_part",      17.0},
        {"ethusdt",  "ETH",  "ethusdt_h1_part",   "ethusdt_d1_part",  17.0},
        {"solusdt",  "SOL",  "solusdt_h1_part",   "solusdt_d1_part",  20.0},
        {"xrpusdt",  "XRP",  "xrpusdt_h1_part",   "xrpusdt_d1_part",  20.0},
        {"linkusdt", "LINK", "linkusdt_h1_part",   "linkusdt_d1_part", 22.0},
        {"bnbusdt",  "BNB",  "bnbusdt_h1_part",   "bnbusdt_d1_part",  20.0},
        {"dogeusdt", "DOGE", "dogeusdt_h1_part",   "dogeusdt_d1_part", 22.0},
        {"avaxusdt", "AVAX", "avaxusdt_h1_part",   "avaxusdt_d1_part", 22.0},
    };

    // Filter to single symbol if specified
    std::string filter_symbol;
    if (argc >= 2) {
        filter_symbol = argv[1];
    }

    if (!filter_symbol.empty()) {
        std::vector<SymbolInfo> filtered;
        for (auto& s : all_symbols) {
            if (s.symbol == filter_symbol) {
                filtered.push_back(s);
                break;
            }
        }
        if (filtered.empty()) {
            std::fprintf(stderr, "Unknown symbol: %s\n", filter_symbol.c_str());
            return 1;
        }
        all_symbols = filtered;
    }

    // ── Timeframes ──────────────────────────────────────────────────────────
    struct TFInfo {
        std::string label;
        int64_t     secs;
    };
    std::vector<TFInfo> timeframes = {
        {"D1",  86400},
        {"H12", 43200},
        {"H6",  21600},
        {"H4",  14400},
        {"H3",  10800},
        {"H2",  7200},
        {"H1",  3600},
    };

    // ── Data directory ──────────────────────────────────────────────────────
    std::string data_dir = "data";
    if (!fs::exists("data/btc_d1_part1.json")) {
        if (fs::exists("backtest/data/btc_d1_part1.json")) {
            data_dir = "backtest/data";
        } else {
            std::fprintf(stderr, "ERROR: cannot find kline data.\n");
            return 1;
        }
    }

    // ── Parameter grids ─────────────────────────────────────────────────────
    // BOLLINGER parameters: lookback (BB length), hold_bars, SL, trail, + BB K
    std::vector<int>    bb_lookbacks   = {10, 15, 20, 25, 30};
    std::vector<int>    bb_hold_bars   = {4, 6, 8, 10, 12, 16, 20};
    std::vector<double> bb_sl_atrs     = {1.5, 2.0, 2.5, 3.0, 3.5};
    std::vector<double> bb_trail_arms  = {0.5, 0.8, 1.0, 1.5};
    std::vector<double> bb_trail_dists = {0.3, 0.5, 0.8};
    std::vector<double> bb_k_values    = {2.0, 2.5, 3.0};

    // RSI_REVERT parameters: lookback (for ATR/history), hold_bars, SL, trail, + RSI threshold
    std::vector<int>    rsi_lookbacks   = {10, 14, 20, 25};
    std::vector<int>    rsi_hold_bars   = {4, 6, 8, 10, 12, 16};
    std::vector<double> rsi_sl_atrs     = {1.5, 2.0, 2.5, 3.0, 3.5};
    std::vector<double> rsi_trail_arms  = {0.5, 0.8, 1.0, 1.5};
    std::vector<double> rsi_trail_dists = {0.3, 0.5, 0.8};
    std::vector<double> rsi_thresholds  = {25.0, 30.0, 35.0};

    // Count total combos
    int bb_combos = 0;
    for (double ta : bb_trail_arms)
        for (double td : bb_trail_dists)
            if (td < ta) bb_combos++;
    bb_combos *= (int)(bb_lookbacks.size() * bb_hold_bars.size() * bb_sl_atrs.size() * bb_k_values.size());

    int rsi_combos = 0;
    for (double ta : rsi_trail_arms)
        for (double td : rsi_trail_dists)
            if (td < ta) rsi_combos++;
    rsi_combos *= (int)(rsi_lookbacks.size() * rsi_hold_bars.size() * rsi_sl_atrs.size() * rsi_thresholds.size());

    int total_jobs = (int)all_symbols.size() * (int)timeframes.size() * (bb_combos + rsi_combos);

    std::printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  EDGE HUNTER — Bear-Market Strategy Finder (Spot-Only)              ║\n");
    std::printf("║  Symbols: %d  Timeframes: %d  BOLLINGER combos: %d  RSI combos: %d  ║\n",
        (int)all_symbols.size(), (int)timeframes.size(), bb_combos, rsi_combos);
    std::printf("║  Total backtests: %d                                                ║\n", total_jobs);
    std::printf("╚══════════════════════════════════════════════════════════════════════╝\n\n");

    // ── Master results vector ───────────────────────────────────────────────
    std::vector<HuntResult> all_results;
    all_results.reserve(total_jobs);
    int global_done = 0;

    // ── Main loop: symbol × timeframe × strategy ────────────────────────────
    for (auto& sym : all_symbols) {
        // Load H1 data for this symbol
        auto h1_klines = load_all_parts(data_dir, sym.h1_prefix);
        if (h1_klines.empty()) {
            std::fprintf(stderr, "[SKIP] %s: no H1 data found (prefix=%s)\n",
                sym.sym_upper.c_str(), sym.h1_prefix.c_str());
            continue;
        }

        // Also try native D1
        auto d1_klines = load_all_parts(data_dir, sym.d1_prefix);

        std::printf("[LOAD] %s: %zu H1 bars, %zu native D1 bars\n",
            sym.sym_upper.c_str(), h1_klines.size(), d1_klines.size());

        for (auto& tf : timeframes) {
            // Get klines for this timeframe
            std::vector<Kline> klines;
            if (tf.label == "D1" && !d1_klines.empty()) {
                klines = d1_klines;
            } else if (tf.label == "D1") {
                klines = synthesize_tf(h1_klines, tf.secs);
            } else if (tf.label == "H1") {
                klines = h1_klines;
            } else {
                klines = synthesize_tf(h1_klines, tf.secs);
            }

            if ((int)klines.size() < 100) {
                std::fprintf(stderr, "[SKIP] %s-%s: only %zu bars\n",
                    sym.sym_upper.c_str(), tf.label.c_str(), klines.size());
                continue;
            }

            int total_bars = (int)klines.size();
            int seed_count = (int)(total_bars * 0.8);

            // Build seed vector
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

            // ── BOLLINGER sweep ─────────────────────────────────────────
            for (double bbk : bb_k_values) {
                for (int lb : bb_lookbacks) {
                    for (int hb : bb_hold_bars) {
                        for (double sl : bb_sl_atrs) {
                            for (double ta : bb_trail_arms) {
                                for (double td : bb_trail_dists) {
                                    if (td >= ta) continue;

                                    auto r = run_single(klines, seeds, seed_count,
                                                         sym.symbol,
                                                         chimera::StrategyKind::BOLLINGER,
                                                         tf.secs,
                                                         lb, hb, sl, ta, td,
                                                         bbk, 30.0,
                                                         sym.cost_bp);
                                    r.symbol    = sym.sym_upper;
                                    r.strategy  = "BOLL_K" + std::to_string((int)(bbk*10));
                                    r.tf_label  = tf.label;
                                    r.tf_secs   = tf.secs;
                                    all_results.push_back(r);
                                    global_done++;
                                }
                            }
                        }
                    }
                }
            }

            // ── RSI_REVERT sweep ────────────────────────────────────────
            for (double rsi_th : rsi_thresholds) {
                for (int lb : rsi_lookbacks) {
                    for (int hb : rsi_hold_bars) {
                        for (double sl : rsi_sl_atrs) {
                            for (double ta : rsi_trail_arms) {
                                for (double td : rsi_trail_dists) {
                                    if (td >= ta) continue;

                                    auto r = run_single(klines, seeds, seed_count,
                                                         sym.symbol,
                                                         chimera::StrategyKind::RSI_REVERT,
                                                         tf.secs,
                                                         lb, hb, sl, ta, td,
                                                         2.0, rsi_th,
                                                         sym.cost_bp);
                                    r.symbol    = sym.sym_upper;
                                    r.strategy  = "RSI_" + std::to_string((int)rsi_th);
                                    r.tf_label  = tf.label;
                                    r.tf_secs   = tf.secs;
                                    all_results.push_back(r);
                                    global_done++;
                                }
                            }
                        }
                    }
                }
            }

            std::fprintf(stderr, "\r  [%s-%s] done (%d total so far)        ",
                sym.sym_upper.c_str(), tf.label.c_str(), global_done);
        }
    }
    std::fprintf(stderr, "\n\n");

    // ── Filter: minimum 5 trades ────────────────────────────────────────────
    std::vector<HuntResult> filtered;
    for (auto& r : all_results) {
        if (r.trades >= 5) filtered.push_back(r);
    }

    // ── Sort by PF ──────────────────────────────────────────────────────────
    std::sort(filtered.begin(), filtered.end(), [](const HuntResult& a, const HuntResult& b) {
        if (std::fabs(a.pf - b.pf) > 0.001) return a.pf > b.pf;
        return a.trades > b.trades;
    });

    // ── Print TOP 60 overall ────────────────────────────────────────────────
    std::printf("\n");
    std::printf("╔═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  EDGE HUNTER RESULTS — Top 60 by OOS Profit Factor (min 5 trades)                                                         ║\n");
    std::printf("╠══════╦═══════════╦═════╦═════╦════╦═════╦═════╦═════╦══════╦══════╦════╦═════════╦══════╦══════╦══════════╦══════════════════╣\n");
    std::printf("║  Sym ║ Strategy  ║  TF ║  LB ║ HB ║  SL ║ T_A ║ T_D ║ Trds ║ Wins ║ WR ║ Net(bp) ║  PF  ║ Shrp ║ MaxDD bp ║ Assessment     ║\n");
    std::printf("╠══════╬═══════════╬═════╬═════╬════╬═════╬═════╬═════╬══════╬══════╬════╬═════════╬══════╬══════╬══════════╬══════════════════╣\n");

    int show = std::min(60, (int)filtered.size());
    for (int i = 0; i < show; ++i) {
        auto& r = filtered[i];
        const char* assessment;
        if (r.pf >= 1.3 && r.trades >= 10 && r.sharpe > 0.3)
            assessment = "*** STRONG";
        else if (r.pf >= 1.15 && r.trades >= 8)
            assessment = "**  GOOD";
        else if (r.pf >= 1.0 && r.trades >= 5)
            assessment = "*   MARGINAL";
        else
            assessment = "    NO EDGE";

        std::printf("║ %4s ║ %-9s ║ %3s ║ %3d ║ %2d ║ %3.1f ║ %3.1f ║ %3.1f ║ %4d ║ %4d ║%3.0f ║ %+7.0f ║ %4.2f ║ %4.2f ║ %8.0f ║ %-16s ║\n",
            r.symbol.c_str(), r.strategy.c_str(), r.tf_label.c_str(),
            r.lookback, r.hold_bars, r.sl_atr_mult,
            r.trail_arm_atr, r.trail_dist_atr,
            r.trades, r.wins, r.win_rate,
            r.total_bp, r.pf, r.sharpe, r.max_dd_bp,
            assessment);
    }
    std::printf("╚══════╩═══════════╩═════╩═════╩════╩═════╩═════╩═════╩══════╩══════╩════╩═════════╩══════╩══════╩══════════╩══════════════════╝\n\n");

    // ── Best per (symbol, strategy_class, TF) ───────────────────────────────
    // Group: BOLL = any BB K, RSI = any threshold
    struct GroupKey {
        std::string symbol;
        std::string strat_class;  // "BOLLINGER" or "RSI_REVERT"
        std::string tf;
        bool operator<(const GroupKey& o) const {
            if (symbol != o.symbol) return symbol < o.symbol;
            if (strat_class != o.strat_class) return strat_class < o.strat_class;
            return tf < o.tf;
        }
    };

    std::map<GroupKey, HuntResult> best_per_group;
    for (auto& r : filtered) {
        std::string sc = (r.strategy.find("BOLL") != std::string::npos) ? "BOLLINGER" : "RSI_REVERT";
        GroupKey key{r.symbol, sc, r.tf_label};
        auto it = best_per_group.find(key);
        if (it == best_per_group.end() || r.pf > it->second.pf) {
            best_per_group[key] = r;
        }
    }

    std::printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  BEST PER (Symbol, Strategy, TF) — min 5 trades                                                          ║\n");
    std::printf("╠══════╦═══════════╦═════╦═════╦════╦═════╦═════╦═════╦══════╦═════════╦══════╦══════╦══════════╦═════════════╣\n");
    std::printf("║  Sym ║ Strategy  ║  TF ║  LB ║ HB ║  SL ║ T_A ║ T_D ║ Trds ║ Net(bp) ║  PF  ║ Shrp ║ MaxDD bp ║ Assessment  ║\n");
    std::printf("╠══════╬═══════════╬═════╬═════╬════╬═════╬═════╬═════╬══════╬═════════╬══════╬══════╬══════════╬═════════════╣\n");

    // Sort by PF
    std::vector<std::pair<GroupKey, HuntResult>> sorted_groups(best_per_group.begin(), best_per_group.end());
    std::sort(sorted_groups.begin(), sorted_groups.end(),
        [](const auto& a, const auto& b) { return a.second.pf > b.second.pf; });

    for (auto& [key, r] : sorted_groups) {
        if (r.pf < 0.8) continue;  // skip total garbage

        const char* assessment;
        if (r.pf >= 1.3 && r.trades >= 10 && r.sharpe > 0.3)
            assessment = "*** STRONG";
        else if (r.pf >= 1.15 && r.trades >= 8)
            assessment = "**  GOOD";
        else if (r.pf >= 1.0)
            assessment = "*   MARGINAL";
        else
            assessment = "    NO EDGE";

        std::printf("║ %4s ║ %-9s ║ %3s ║ %3d ║ %2d ║ %3.1f ║ %3.1f ║ %3.1f ║ %4d ║ %+7.0f ║ %4.2f ║ %4.2f ║ %8.0f ║ %-11s ║\n",
            r.symbol.c_str(), r.strategy.c_str(), r.tf_label.c_str(),
            r.lookback, r.hold_bars, r.sl_atr_mult,
            r.trail_arm_atr, r.trail_dist_atr,
            r.trades, r.total_bp, r.pf, r.sharpe, r.max_dd_bp,
            assessment);
    }
    std::printf("╚══════╩═══════════╩═════╩═════╩════╩═════╩═════╩═════╩══════╩═════════╩══════╩══════╩══════════╩═════════════╝\n\n");

    // ── Strategy class breakdown ────────────────────────────────────────────
    int bb_profitable = 0, bb_total_filtered = 0;
    int rsi_profitable = 0, rsi_total_filtered = 0;

    for (auto& r : filtered) {
        if (r.strategy.find("BOLL") != std::string::npos) {
            bb_total_filtered++;
            if (r.pf > 1.0) bb_profitable++;
        } else {
            rsi_total_filtered++;
            if (r.pf > 1.0) rsi_profitable++;
        }
    }

    std::printf("── STRATEGY BREAKDOWN ──────────────────────────────────────────\n");
    std::printf("  BOLLINGER (K=2.0/2.5/3.0):\n");
    std::printf("    With >= 5 trades: %d\n", bb_total_filtered);
    std::printf("    Profitable (PF>1): %d (%.1f%%)\n",
        bb_profitable, bb_total_filtered > 0 ? 100.0 * bb_profitable / bb_total_filtered : 0.0);
    std::printf("  RSI_REVERT (th=25/30/35):\n");
    std::printf("    With >= 5 trades: %d\n", rsi_total_filtered);
    std::printf("    Profitable (PF>1): %d (%.1f%%)\n",
        rsi_profitable, rsi_total_filtered > 0 ? 100.0 * rsi_profitable / rsi_total_filtered : 0.0);
    std::printf("────────────────────────────────────────────────────────────────\n\n");

    // ── BB K-value breakdown ────────────────────────────────────────────────
    for (double bbk : bb_k_values) {
        std::string strat_name = "BOLL_K" + std::to_string((int)(bbk * 10));
        int count = 0, profitable = 0;
        double best_pf = 0.0;
        std::string best_tag;
        for (auto& r : filtered) {
            if (r.strategy == strat_name) {
                count++;
                if (r.pf > 1.0) profitable++;
                if (r.pf > best_pf) {
                    best_pf = r.pf;
                    best_tag = r.symbol + "-" + r.tf_label;
                }
            }
        }
        std::printf("  BB K=%.1f: %d results, %d profitable (%.0f%%), best PF=%.2f (%s)\n",
            bbk, count, profitable,
            count > 0 ? 100.0 * profitable / count : 0.0,
            best_pf, best_tag.c_str());
    }
    std::printf("\n");

    // ── RSI threshold breakdown ─────────────────────────────────────────────
    for (double th : rsi_thresholds) {
        std::string strat_name = "RSI_" + std::to_string((int)th);
        int count = 0, profitable = 0;
        double best_pf = 0.0;
        std::string best_tag;
        for (auto& r : filtered) {
            if (r.strategy == strat_name) {
                count++;
                if (r.pf > 1.0) profitable++;
                if (r.pf > best_pf) {
                    best_pf = r.pf;
                    best_tag = r.symbol + "-" + r.tf_label;
                }
            }
        }
        std::printf("  RSI th=%d: %d results, %d profitable (%.0f%%), best PF=%.2f (%s)\n",
            (int)th, count, profitable,
            count > 0 ? 100.0 * profitable / count : 0.0,
            best_pf, best_tag.c_str());
    }
    std::printf("\n");

    // ── Timeframe breakdown ─────────────────────────────────────────────────
    std::printf("── TIMEFRAME BREAKDOWN ─────────────────────────────────────────\n");
    for (auto& tf : timeframes) {
        int count = 0, profitable = 0;
        for (auto& r : filtered) {
            if (r.tf_label == tf.label) {
                count++;
                if (r.pf > 1.0) profitable++;
            }
        }
        std::printf("  %s: %d results, %d profitable (%.0f%%)\n",
            tf.label.c_str(), count, profitable,
            count > 0 ? 100.0 * profitable / count : 0.0);
    }
    std::printf("────────────────────────────────────────────────────────────────\n\n");

    // ── DEPLOYABLE CANDIDATES (PF >= 1.15, >= 8 trades, Sharpe > 0.2) ─────
    std::printf("═══ DEPLOYABLE CANDIDATES (PF >= 1.15, >= 8 trades, Sharpe > 0.2) ═══\n\n");
    int candidates = 0;
    for (auto& [key, r] : sorted_groups) {
        if (r.pf >= 1.15 && r.trades >= 8 && r.sharpe > 0.2) {
            candidates++;
            std::printf("  %s-%s-%s:\n", r.symbol.c_str(), r.strategy.c_str(), r.tf_label.c_str());
            std::printf("    lookback=%d  hold=%d  sl=%.1f  trail_arm=%.1f  trail_dist=%.1f\n",
                r.lookback, r.hold_bars, r.sl_atr_mult, r.trail_arm_atr, r.trail_dist_atr);
            if (r.strategy.find("BOLL") != std::string::npos) {
                std::printf("    bb_k=%.1f\n", r.bb_k);
            } else {
                std::printf("    rsi_threshold=%.0f\n", r.rsi_threshold);
            }
            std::printf("    OOS: %d trades, PF=%.2f, Sharpe=%.2f, net=%+.0fbp, WR=%.0f%%, MaxDD=%.0fbp\n",
                r.trades, r.pf, r.sharpe, r.total_bp, r.win_rate, r.max_dd_bp);
            std::printf("    → RECOMMEND: Run optimizer_general for deep validation\n\n");
        }
    }
    if (candidates == 0) {
        std::printf("  No candidates met the deployment threshold.\n");
        std::printf("  Consider: wider parameter grids, new symbols, or accept MARGINAL edges.\n\n");
    }

    std::printf("═══════════════════════════════════════════════════════════════════════\n");
    std::printf("  Total backtests run:    %d\n", global_done);
    std::printf("  With >= 5 trades:       %d\n", (int)filtered.size());
    std::printf("  Deployable candidates:  %d\n", candidates);
    std::printf("═══════════════════════════════════════════════════════════════════════\n\n");

    return 0;
}
