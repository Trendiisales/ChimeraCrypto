// ============================================================================
// optimizer_v2.cpp — Extended optimizer with bb_k and rsi_threshold params
//
// Same as optimizer_general.cpp but accepts bb_k and rsi_threshold as
// optional CLI args (argv[5] and argv[6]) for deep-validating edge_hunter
// discoveries at non-default BB/RSI settings.
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include optimizer_v2.cpp -o optimizer_v2
//
// Usage:
//   ./optimizer_v2 <symbol> <strategy> <tf> <cost_bp> [bb_k] [rsi_threshold]
//
// Examples:
//   ./optimizer_v2 btcusdt RSI_REVERT H2 17 2.0 25    # RSI threshold=25
//   ./optimizer_v2 ethusdt BOLLINGER H3 17 2.5 30      # BB K=2.5
//   ./optimizer_v2 btcusdt TSMOM D1 17                 # same as optimizer_general
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
struct OptResult {
    int    lookback;
    int    hold_bars;
    double sl_atr_mult;
    double trail_arm_atr;
    double trail_dist_atr;
    int    trades;
    int    wins;
    double total_bp;
    double win_rate;
    double pf;
    double sharpe;
    double avg_bp;
    double max_dd_bp;
    int    neighbour_score;
};

// ── Global strategy params (set from CLI) ───────────────────────────────────
static double g_bb_k           = 2.0;
static double g_rsi_threshold  = 30.0;

// ── Run single backtest ─────────────────────────────────────────────────────
static OptResult run_single(const std::vector<Kline>& klines,
                             const std::vector<chimera::EdgeEngine::SeedBar>& seeds,
                             int seed_count,
                             const std::string& symbol,
                             chimera::StrategyKind kind,
                             int64_t tf_secs,
                             int lookback, int hold_bars, double sl_atr,
                             double trail_arm, double trail_dist,
                             double cost_bp) {
    OptResult r{};
    r.lookback       = lookback;
    r.hold_bars      = hold_bars;
    r.sl_atr_mult    = sl_atr;
    r.trail_arm_atr  = trail_arm;
    r.trail_dist_atr = trail_dist;

    chimera::EdgeEngine::Config cfg{
        .symbol         = symbol,
        .tag            = "OPT",
        .kind           = kind,
        .tf_secs        = tf_secs,
        .lookback       = lookback,
        .hold_bars      = hold_bars,
        .sl_atr_mult    = sl_atr,
        .atr_period     = 14,
        .bb_k           = g_bb_k,
        .rsi_threshold  = g_rsi_threshold,
        .round_trip_bp  = cost_bp,
        .max_history    = std::max(lookback + 5, 19),
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
    if (argc < 5) {
        std::fprintf(stderr, "Usage: %s <symbol> <strategy> <tf> <cost_bp> [bb_k] [rsi_threshold]\n", argv[0]);
        std::fprintf(stderr, "  symbol:        btcusdt, ethusdt, solusdt, xrpusdt, linkusdt, bnbusdt, dogeusdt, avaxusdt\n");
        std::fprintf(stderr, "  strategy:      TSMOM, DONCHIAN, BOLLINGER, RSI_REVERT\n");
        std::fprintf(stderr, "  tf:            D1, H12, H6, H4, H3, H2, H1\n");
        std::fprintf(stderr, "  cost_bp:       17, 20, 22, etc.\n");
        std::fprintf(stderr, "  bb_k:          Bollinger K (default 2.0)\n");
        std::fprintf(stderr, "  rsi_threshold: RSI oversold threshold (default 30.0)\n");
        return 1;
    }

    std::string symbol   = argv[1];
    std::string strat_s  = argv[2];
    std::string tf_s     = argv[3];
    double      cost_bp  = std::stod(argv[4]);

    // Optional extended params
    if (argc >= 6) g_bb_k          = std::stod(argv[5]);
    if (argc >= 7) g_rsi_threshold = std::stod(argv[6]);

    // Parse strategy
    chimera::StrategyKind kind;
    if      (strat_s == "TSMOM")      kind = chimera::StrategyKind::TSMOM;
    else if (strat_s == "DONCHIAN")   kind = chimera::StrategyKind::DONCHIAN;
    else if (strat_s == "BOLLINGER")  kind = chimera::StrategyKind::BOLLINGER;
    else if (strat_s == "RSI_REVERT") kind = chimera::StrategyKind::RSI_REVERT;
    else {
        std::fprintf(stderr, "Unknown strategy: %s\n", strat_s.c_str());
        return 1;
    }

    // Parse timeframe
    int64_t tf_secs;
    if      (tf_s == "D1")  tf_secs = 86400;
    else if (tf_s == "H12") tf_secs = 43200;
    else if (tf_s == "H6")  tf_secs = 21600;
    else if (tf_s == "H4")  tf_secs = 14400;
    else if (tf_s == "H3")  tf_secs = 10800;
    else if (tf_s == "H2")  tf_secs = 7200;
    else if (tf_s == "H1")  tf_secs = 3600;
    else {
        std::fprintf(stderr, "Unknown timeframe: %s\n", tf_s.c_str());
        return 1;
    }

    // Build data prefix from symbol
    std::string data_dir = "data";
    if (!fs::exists("data/btc_d1_part1.json")) {
        if (fs::exists("backtest/data/btc_d1_part1.json")) {
            data_dir = "backtest/data";
        } else {
            std::fprintf(stderr, "ERROR: cannot find kline data.\n");
            return 1;
        }
    }

    // Determine file prefix based on symbol
    std::string h1_prefix, d1_prefix;
    if (symbol == "btcusdt") {
        h1_prefix = "btc_h1_part";
        d1_prefix = "btc_d1_part";
    } else {
        h1_prefix = symbol + "_h1_part";
        d1_prefix = symbol + "_d1_part";
    }

    std::printf("[LOAD] Loading %s klines for %s %s (bb_k=%.1f, rsi_th=%.0f)...\n",
        symbol.c_str(), strat_s.c_str(), tf_s.c_str(), g_bb_k, g_rsi_threshold);

    // Load appropriate data
    std::vector<Kline> klines;
    if (tf_s == "D1") {
        klines = load_all_parts(data_dir, d1_prefix);
        if (klines.empty()) {
            auto h1 = load_all_parts(data_dir, h1_prefix);
            klines = synthesize_tf(h1, tf_secs);
        }
    } else if (tf_s == "H12" || tf_s == "H6" || tf_s == "H4" || tf_s == "H3" || tf_s == "H2") {
        auto h1 = load_all_parts(data_dir, h1_prefix);
        klines = synthesize_tf(h1, tf_secs);
    } else {
        klines = load_all_parts(data_dir, h1_prefix);
    }

    std::printf("[LOAD] %zu bars loaded (tf=%llds)\n", klines.size(), (long long)tf_secs);

    if (klines.size() < 100) {
        std::fprintf(stderr, "ERROR: insufficient data (%zu bars)\n", klines.size());
        return 1;
    }

    int total = (int)klines.size();
    int seed_count = (int)(total * 0.8);

    // Pre-build seed vector
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

    // ── Parameter grid ──────────────────────────────────────────────────
    std::vector<int>    lookbacks   = {5, 8, 10, 15, 20, 25, 30, 35, 40};
    std::vector<int>    hold_bars_v = {4, 6, 8, 10, 12, 16, 20, 24};
    std::vector<double> sl_atrs     = {1.5, 2.0, 2.5, 3.0, 3.5, 4.0};
    std::vector<double> trail_arms  = {0.5, 0.8, 1.0, 1.2, 1.5, 2.0};
    std::vector<double> trail_dists = {0.3, 0.4, 0.5, 0.6, 0.8, 1.0};

    int combos = (int)(lookbacks.size() * hold_bars_v.size() * sl_atrs.size() *
                       trail_arms.size() * trail_dists.size());
    std::printf("[SWEEP] %d combinations to test (cost=%.0fbp, bb_k=%.1f, rsi_th=%.0f)\n\n",
        combos, cost_bp, g_bb_k, g_rsi_threshold);

    std::vector<OptResult> all_results;
    all_results.reserve(combos);

    int done = 0;
    for (int lb : lookbacks) {
        for (int hb : hold_bars_v) {
            for (double sl : sl_atrs) {
                for (double ta : trail_arms) {
                    for (double td : trail_dists) {
                        if (td >= ta) continue;

                        OptResult r = run_single(klines, seeds, seed_count,
                                                  symbol, kind, tf_secs,
                                                  lb, hb, sl, ta, td, cost_bp);
                        all_results.push_back(r);
                        done++;

                        if (done % 500 == 0) {
                            std::fprintf(stderr, "\r  [%d / %d] ...", done, combos);
                            fflush(stderr);
                        }
                    }
                }
            }
        }
    }
    std::fprintf(stderr, "\r  [%d / %d] done.    \n", done, combos);

    // ── Neighbourhood scoring ───────────────────────────────────────────
    using ParamKey = std::tuple<int,int,int,int,int>;
    std::map<ParamKey, double> pf_map;

    auto find_idx = [](const std::vector<int>& v, int val) -> int {
        for (int i = 0; i < (int)v.size(); ++i) if (v[i] == val) return i;
        return -1;
    };
    auto find_idx_d = [](const std::vector<double>& v, double val) -> int {
        for (int i = 0; i < (int)v.size(); ++i) if (std::fabs(v[i] - val) < 0.001) return i;
        return -1;
    };

    for (auto& r : all_results) {
        int li = find_idx(lookbacks, r.lookback);
        int hi = find_idx(hold_bars_v, r.hold_bars);
        int si = find_idx_d(sl_atrs, r.sl_atr_mult);
        int ai = find_idx_d(trail_arms, r.trail_arm_atr);
        int di = find_idx_d(trail_dists, r.trail_dist_atr);
        pf_map[{li, hi, si, ai, di}] = r.pf;
    }

    for (auto& r : all_results) {
        int li = find_idx(lookbacks, r.lookback);
        int hi = find_idx(hold_bars_v, r.hold_bars);
        int si = find_idx_d(sl_atrs, r.sl_atr_mult);
        int ai = find_idx_d(trail_arms, r.trail_arm_atr);
        int di = find_idx_d(trail_dists, r.trail_dist_atr);

        int score = 0, checked = 0;
        for (int dl = -1; dl <= 1; ++dl) {
            for (int dh = -1; dh <= 1; ++dh) {
                for (int ds = -1; ds <= 1; ++ds) {
                    for (int da = -1; da <= 1; ++da) {
                        for (int dd = -1; dd <= 1; ++dd) {
                            if (dl == 0 && dh == 0 && ds == 0 && da == 0 && dd == 0) continue;
                            int ni = li+dl, nh = hi+dh, ns = si+ds, na = ai+da, nd = di+dd;
                            if (ni < 0 || ni >= (int)lookbacks.size()) continue;
                            if (nh < 0 || nh >= (int)hold_bars_v.size()) continue;
                            if (ns < 0 || ns >= (int)sl_atrs.size()) continue;
                            if (na < 0 || na >= (int)trail_arms.size()) continue;
                            if (nd < 0 || nd >= (int)trail_dists.size()) continue;
                            auto it = pf_map.find({ni, nh, ns, na, nd});
                            if (it != pf_map.end()) {
                                checked++;
                                if (it->second > 1.0) score++;
                            }
                        }
                    }
                }
            }
        }
        r.neighbour_score = checked > 0 ? (int)(100.0 * score / checked) : 0;
    }

    // ── Filter and sort ─────────────────────────────────────────────────
    std::vector<OptResult> filtered;
    for (auto& r : all_results) {
        if (r.trades >= 8) filtered.push_back(r);
    }

    std::sort(filtered.begin(), filtered.end(), [](const OptResult& a, const OptResult& b) {
        if (std::fabs(a.pf - b.pf) > 0.001) return a.pf > b.pf;
        return a.neighbour_score > b.neighbour_score;
    });

    // ── Print top 30 ────────────────────────────────────────────────────
    std::string sym_upper;
    if (symbol == "btcusdt") sym_upper = "BTC";
    else if (symbol == "ethusdt") sym_upper = "ETH";
    else if (symbol == "solusdt") sym_upper = "SOL";
    else if (symbol == "xrpusdt") sym_upper = "XRP";
    else if (symbol == "linkusdt") sym_upper = "LINK";
    else if (symbol == "bnbusdt") sym_upper = "BNB";
    else if (symbol == "dogeusdt") sym_upper = "DOGE";
    else if (symbol == "avaxusdt") sym_upper = "AVAX";
    else sym_upper = symbol;

    std::string extra_label;
    if (strat_s == "BOLLINGER") extra_label = " (K=" + std::to_string(g_bb_k).substr(0,3) + ")";
    if (strat_s == "RSI_REVERT") extra_label = " (th=" + std::to_string((int)g_rsi_threshold) + ")";

    std::string engine_tag = sym_upper + "-" + strat_s + "-" + tf_s + extra_label;

    std::printf("\n");
    std::printf("╔═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  %s OPTIMIZER — Top 30 by OOS Profit Factor (cost = %.0fbp)                                  ║\n", engine_tag.c_str(), cost_bp);
    std::printf("╠═════╦════╦════╦═════╦══════╦══════╦══════╦════╦═════════╦══════╦══════╦══════════╦════════╦═══════════════════════════╣\n");
    std::printf("║  LB ║ HB ║ SL ║ T_A ║ T_D  ║ Trds ║ Wins ║ WR ║ Net(bp) ║  PF  ║ Shrp ║ MaxDD bp ║ Nbr%%   ║ Assessment              ║\n");
    std::printf("╠═════╬════╬════╬═════╬══════╬══════╬══════╬════╬═════════╬══════╬══════╬══════════╬════════╬═══════════════════════════╣\n");

    int show = std::min(30, (int)filtered.size());
    for (int i = 0; i < show; ++i) {
        auto& r = filtered[i];
        const char* assessment;
        if (r.pf >= 1.3 && r.neighbour_score >= 60 && r.sharpe > 0.3)
            assessment = "STRONG — robust edge";
        else if (r.pf >= 1.15 && r.neighbour_score >= 40)
            assessment = "GOOD — deployable";
        else if (r.pf >= 1.0 && r.neighbour_score >= 30)
            assessment = "MARGINAL";
        else if (r.pf >= 1.0)
            assessment = "FRAGILE — isolated peak";
        else
            assessment = "NO EDGE";

        std::printf("║ %3d ║ %2d ║%3.1f ║ %3.1f ║ %4.1f ║ %4d ║ %4d ║%3.0f ║ %+7.0f ║ %4.2f ║ %4.2f ║ %8.0f ║  %3d%%   ║ %-25s ║\n",
            r.lookback, r.hold_bars, r.sl_atr_mult,
            r.trail_arm_atr, r.trail_dist_atr,
            r.trades, r.wins, r.win_rate,
            r.total_bp, r.pf, r.sharpe, r.max_dd_bp,
            r.neighbour_score, assessment);
    }
    std::printf("╚═════╩════╩════╩═════╩══════╩══════╩══════╩════╩═════════╩══════╩══════╩══════════╩════════╩═══════════════════════════╝\n");

    // ── Summary ─────────────────────────────────────────────────────────
    int profitable = 0;
    for (auto& r : all_results) {
        if (r.trades >= 8 && r.pf > 1.0) profitable++;
    }
    std::printf("\n── SUMMARY ─────────────────────────────────────────────────────\n");
    std::printf("  Total combinations tested: %d\n", done);
    std::printf("  With >= 8 trades:          %d\n", (int)filtered.size());
    std::printf("  Profitable (PF > 1.0):     %d (%.1f%%)\n",
        profitable, filtered.empty() ? 0.0 : 100.0 * profitable / filtered.size());
    std::printf("────────────────────────────────────────────────────────────────\n\n");

    // ── Best recommendation ─────────────────────────────────────────────
    OptResult* best = nullptr;
    for (auto& r : filtered) {
        if (r.pf >= 1.15 && r.neighbour_score >= 40 && r.sharpe > 0.2 && r.trades >= 10) {
            best = &r;
            break;
        }
    }
    if (!best) {
        for (auto& r : filtered) {
            if (r.pf >= 1.1 && r.neighbour_score >= 30 && r.trades >= 10) {
                best = &r;
                break;
            }
        }
    }
    if (!best && !filtered.empty()) best = &filtered[0];

    if (best) {
        std::printf("═══ RECOMMENDED CONFIG FOR %s ════════════════════════════\n", engine_tag.c_str());
        std::printf("  lookback       = %d\n", best->lookback);
        std::printf("  hold_bars      = %d\n", best->hold_bars);
        std::printf("  sl_atr_mult    = %.1f\n", best->sl_atr_mult);
        std::printf("  trail_arm_atr  = %.1f\n", best->trail_arm_atr);
        std::printf("  trail_dist_atr = %.1f\n", best->trail_dist_atr);
        std::printf("  bb_k           = %.1f\n", g_bb_k);
        std::printf("  rsi_threshold  = %.0f\n", g_rsi_threshold);
        std::printf("  ──────────────────────────────\n");
        std::printf("  OOS trades     = %d\n", best->trades);
        std::printf("  OOS PF         = %.2f\n", best->pf);
        std::printf("  OOS Sharpe     = %.2f\n", best->sharpe);
        std::printf("  OOS net bp     = %+.0f\n", best->total_bp);
        std::printf("  OOS WR         = %.1f%%\n", best->win_rate);
        std::printf("  OOS MaxDD      = %.0fbp\n", best->max_dd_bp);
        std::printf("  Neighbour %%    = %d%%\n", best->neighbour_score);
        std::printf("════════════════════════════════════════════════════════════════\n\n");
    }

    return 0;
}
