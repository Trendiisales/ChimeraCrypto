// ============================================================================
// backtest_harness.cpp — C++ backtester using the REAL EdgeEngine.hpp
//
// Reads Binance JSON kline files, feeds them through EdgeEngine::on_tick()
// with the exact same configs as main.cpp. No reimplementation — this is
// the production signal/exit logic running on historical data.
//
// Updated 2026-05-16: 18 active TSMOM engines matching main.cpp deployment
//
//   SECTION A — D1 engines (5):
//    A1. BTC-TSMOM-D1   LB=10  HB=12  SL=3.0  TA=1.0  TD=0.4  cost=17bp  PF=1.92
//    A2. ETH-TSMOM-D1   LB=25  HB=8   SL=2.5  TA=0.8  TD=0.4  cost=17bp  PF=3.15
//    A3. SOL-TSMOM-D1   LB=10  HB=20  SL=2.0  TA=0.5  TD=0.3  cost=20bp  PF=2.25
//    A4. LINK-TSMOM-D1  LB=40  HB=20  SL=2.0  TA=1.0  TD=0.8  cost=22bp  PF=2.18
//    A5. BNB-TSMOM-D1   LB=10  HB=20  SL=2.5  TA=1.2  TD=0.4  cost=20bp  PF=3.16
//
//   SECTION B — H4 engines (7):
//    B1. XRP-TSMOM-H4   LB=30  HB=20  SL=1.5  TA=0.5  TD=0.4  cost=20bp  PF=2.43
//    B2. BNB-TSMOM-H4   LB=40  HB=16  SL=3.0  TA=0.5  TD=0.3  cost=20bp  PF=1.91
//    B3. LINK-TSMOM-H4  LB=30  HB=24  SL=2.5  TA=0.5  TD=0.3  cost=22bp  PF=1.91
//    B4. SOL-TSMOM-H4   LB=40  HB=12  SL=3.0  TA=0.5  TD=0.3  cost=20bp  PF=1.89
//    B5. BTC-TSMOM-H4   LB=25  HB=16  SL=4.0  TA=0.8  TD=0.3  cost=17bp  PF=1.82
//    B6. ETH-TSMOM-H4   LB=40  HB=24  SL=3.0  TA=0.5  TD=0.4  cost=17bp  PF=1.76
//    B7. AVAX-TSMOM-H4  LB=40  HB=24  SL=2.5  TA=0.5  TD=0.4  cost=22bp  PF=1.47
//
//   SECTION C — H12 engines (3):
//    C1. BTC-TSMOM-H12  LB=15  HB=24  SL=4.0  TA=1.2  TD=0.8  cost=17bp  PF=3.63
//    C2. DOGE-TSMOM-H12 LB=35  HB=24  SL=2.0  TA=0.5  TD=0.4  cost=22bp  PF=2.78
//    C3. AVAX-TSMOM-H12 LB=30  HB=12  SL=4.0  TA=0.5  TD=0.4  cost=22bp  PF=2.61
//
// 7 disabled engines preserved as comments for reference.
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include backtest_harness.cpp -o backtest
//
// Usage:
//   ./backtest          (runs all 18 engines, prints stats)
//   ./backtest --csv    (machine-readable CSV output)
//   ./backtest --all    (runs all 25 engines including disabled ones)
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

// ── Synthesize higher-TF bars from H1 klines ────────────────────────────────
// Generic synthesizer: group H1 bars into blocks of `block_secs` seconds,
// aligned to UTC midnight. Works for H4 (14400), H6 (21600), H12 (43200).
static std::vector<Kline> synthesize_bars(const std::vector<Kline>& h1, int64_t block_secs) {
    std::vector<Kline> out;
    if (h1.empty()) return out;

    Kline cur{};
    int64_t cur_block = -1;

    for (auto& bar : h1) {
        int64_t block = (bar.open_ts_ms / 1000) / block_secs;
        if (block != cur_block) {
            if (cur_block >= 0 && cur.o > 0.0) {
                out.push_back(cur);
            }
            cur_block = block;
            cur.open_ts_ms = block * block_secs * 1000;
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
        out.push_back(cur);
    }

    return out;
}

// ── Feed klines through an EdgeEngine ────────────────────────────────────────
// For each bar we simulate 4 ticks in OHLC order appropriate to bar direction:
//   Bullish bar (c >= o): O -> L -> H -> C
//   Bearish bar (c <  o): O -> H -> L -> C
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
static void print_section_header(const char* title) {
    std::printf("╠═══════════════════╬════════╬══════╬══════╬═════════╬════════╬══════╬══════════╬════════╬═══════════╣\n");
    std::printf("║ %-91s ║\n", title);
    std::printf("╠═══════════════════╬════════╬══════╬══════╬═════════╬════════╬══════╬══════════╬════════╬═══════════╣\n");
}

static void print_header() {
    std::printf("\n");
    std::printf("╔══════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    std::printf("║  CHIMERA BACKTEST RESULTS — Out-of-Sample (last 20%% of data)    18 engines, all TSMOM             ║\n");
    std::printf("╠═══════════════════╦════════╦══════╦══════╦═════════╦════════╦══════╦══════════╦════════╦═══════════╣\n");
    std::printf("║ Engine            ║ Trades ║ Wins ║ WR%%  ║ Net(bp) ║ PF     ║ Avg  ║ MaxDD bp ║ Sharpe ║ Cost(bp)  ║\n");
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
    int deploy_count = 0;
    for (auto& r : results) {
        const char* status;
        if (r.trades < 5)              status = "INSUFFICIENT DATA";
        else if (r.pf >= 1.15 && r.sharpe > 0.3) { status = "DEPLOY"; deploy_count++; }
        else if (r.pf >= 1.0)          status = "MARGINAL — needs tuning";
        else                           status = "NO EDGE";

        std::printf("  %-18s  PF=%.2f  Sharpe=%.2f  →  %s\n",
            r.tag.c_str(), r.pf, r.sharpe, status);
    }
    std::printf("────────────────────────────────────────────────────────────────\n");
    std::printf("  SUMMARY: %d / %d engines pass deploy criteria (PF>1.15, Sharpe>0.3)\n",
        deploy_count, (int)results.size());
    std::printf("────────────────────────────────────────────────────────────────\n\n");
}

// ══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    bool csv_mode = (argc > 1 && std::string(argv[1]) == "--csv");
    bool run_all  = (argc > 1 && std::string(argv[1]) == "--all");

    std::string data_dir = "data";

    // Detect if running from backtest/ or project root
    if (fs::exists("data/btc_h1_part0.json") || fs::exists("data/btc_h1_part1.json")) {
        data_dir = "data";
    } else if (fs::exists("backtest/data/btc_h1_part0.json") || fs::exists("backtest/data/btc_h1_part1.json")) {
        data_dir = "backtest/data";
    } else {
        std::fprintf(stderr, "ERROR: cannot find kline data. Run from backtest/ or project root.\n");
        return 1;
    }

    std::printf("[LOAD] Loading kline data from %s/ ...\n", data_dir.c_str());

    // ── Load H1 data for all symbols (needed for H4/H12 synthesis) ──────
    auto btc_h1  = load_all_parts(data_dir, "btc_h1_part");
    auto eth_h1  = load_all_parts(data_dir, "ethusdt_h1_part");
    auto sol_h1  = load_all_parts(data_dir, "solusdt_h1_part");
    auto link_h1 = load_all_parts(data_dir, "linkusdt_h1_part");
    auto xrp_h1  = load_all_parts(data_dir, "xrpusdt_h1_part");
    auto bnb_h1  = load_all_parts(data_dir, "bnbusdt_h1_part");
    auto avax_h1 = load_all_parts(data_dir, "avaxusdt_h1_part");
    auto doge_h1 = load_all_parts(data_dir, "dogeusdt_h1_part");

    // ── Load D1 data ────────────────────────────────────────────────────
    auto btc_d1  = load_all_parts(data_dir, "btc_d1_part");
    auto eth_d1  = load_all_parts(data_dir, "ethusdt_d1_part");
    auto sol_d1  = load_all_parts(data_dir, "solusdt_d1_part");
    auto link_d1 = load_all_parts(data_dir, "linkusdt_d1_part");
    auto bnb_d1  = load_all_parts(data_dir, "bnbusdt_d1_part");

    // ── Synthesize H4 and H12 bars from H1 ──────────────────────────────
    auto xrp_h4   = synthesize_bars(xrp_h1,  14400);
    auto bnb_h4   = synthesize_bars(bnb_h1,  14400);
    auto link_h4  = synthesize_bars(link_h1, 14400);
    auto sol_h4   = synthesize_bars(sol_h1,  14400);
    auto btc_h4   = synthesize_bars(btc_h1,  14400);
    auto eth_h4   = synthesize_bars(eth_h1,  14400);
    auto avax_h4  = synthesize_bars(avax_h1, 14400);

    auto btc_h12  = synthesize_bars(btc_h1,  43200);
    auto doge_h12 = synthesize_bars(doge_h1, 43200);
    auto avax_h12 = synthesize_bars(avax_h1, 43200);

    std::printf("[LOAD] BTC   H1=%zu  D1=%zu  H4=%zu  H12=%zu\n", btc_h1.size(), btc_d1.size(), btc_h4.size(), btc_h12.size());
    std::printf("[LOAD] ETH   H1=%zu  D1=%zu  H4=%zu\n",          eth_h1.size(), eth_d1.size(), eth_h4.size());
    std::printf("[LOAD] SOL   H1=%zu  D1=%zu  H4=%zu\n",          sol_h1.size(), sol_d1.size(), sol_h4.size());
    std::printf("[LOAD] LINK  H1=%zu  D1=%zu  H4=%zu\n",          link_h1.size(), link_d1.size(), link_h4.size());
    std::printf("[LOAD] BNB   H1=%zu  D1=%zu  H4=%zu\n",          bnb_h1.size(), bnb_d1.size(), bnb_h4.size());
    std::printf("[LOAD] XRP   H1=%zu  H4=%zu\n",                  xrp_h1.size(), xrp_h4.size());
    std::printf("[LOAD] AVAX  H1=%zu  H4=%zu  H12=%zu\n",         avax_h1.size(), avax_h4.size(), avax_h12.size());
    std::printf("[LOAD] DOGE  H1=%zu  H12=%zu\n",                 doge_h1.size(), doge_h12.size());

    // ══════════════════════════════════════════════════════════════════════
    // SECTION A: D1 ENGINE CONFIGS — EXACT match to main.cpp (2026-05-16)
    // ══════════════════════════════════════════════════════════════════════

    // A1. BTC-TSMOM-D1 (OOS PF=1.92, Sharpe=1.67, Nbr=85%)
    chimera::EdgeEngine::Config btc_d1_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
    };

    // A2. ETH-TSMOM-D1 (OOS PF=3.15, Sharpe=3.17, Nbr=91%)
    chimera::EdgeEngine::Config eth_d1_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
    };

    // A3. SOL-TSMOM-D1 (OOS PF=2.25, Sharpe=2.41, Nbr=89%)
    chimera::EdgeEngine::Config sol_d1_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };

    // A4. LINK-TSMOM-D1 (OOS PF=2.18, Sharpe=1.92, Nbr=100%)
    chimera::EdgeEngine::Config link_d1_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
    };

    // A5. BNB-TSMOM-D1 — NEW Session 14 (OOS PF=3.16, Sharpe=2.91, Nbr=90%)
    chimera::EdgeEngine::Config bnb_d1_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.4,
    };

    // ══════════════════════════════════════════════════════════════════════
    // SECTION B: H4 ENGINE CONFIGS — ALL NEW Session 14
    // ══════════════════════════════════════════════════════════════════════

    // B1. XRP-TSMOM-H4 (OOS PF=2.43, Sharpe=5.80, 267 trades, Nbr=100%)
    chimera::EdgeEngine::Config xrp_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };

    // B2. BNB-TSMOM-H4 (OOS PF=1.91, Sharpe=3.79, 291 trades, Nbr=100%)
    chimera::EdgeEngine::Config bnb_h4_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };

    // B3. LINK-TSMOM-H4 (OOS PF=1.91, Sharpe=4.07, 205 trades, Nbr=95%)
    chimera::EdgeEngine::Config link_h4_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };

    // B4. SOL-TSMOM-H4 (OOS PF=1.89, Sharpe=3.82, 208 trades, Nbr=100%)
    chimera::EdgeEngine::Config sol_h4_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };

    // B5. BTC-TSMOM-H4 (OOS PF=1.82, Sharpe=3.54, 167 trades, Nbr=100%)
    chimera::EdgeEngine::Config btc_h4_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 25,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
    };

    // B6. ETH-TSMOM-H4 (OOS PF=1.76, Sharpe=3.26, 196 trades, Nbr=100%)
    chimera::EdgeEngine::Config eth_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };

    // B7. AVAX-TSMOM-H4 (OOS PF=1.47, Sharpe=2.17, 231 trades, Nbr=83%)
    chimera::EdgeEngine::Config avax_h4_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };

    // ══════════════════════════════════════════════════════════════════════
    // SECTION C: H12 ENGINE CONFIGS — ALL NEW Session 14
    // ══════════════════════════════════════════════════════════════════════

    // C1. BTC-TSMOM-H12 (OOS PF=3.63, Sharpe=3.40, 31 trades, Nbr=96%)
    chimera::EdgeEngine::Config btc_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 15,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.8,
    };

    // C2. DOGE-TSMOM-H12 (OOS PF=2.78, Sharpe=3.66, 82 trades, Nbr=100%)
    chimera::EdgeEngine::Config doge_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };

    // C3. AVAX-TSMOM-H12 (OOS PF=2.61, Sharpe=2.98, 76 trades, Nbr=87%)
    chimera::EdgeEngine::Config avax_h12_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
    };

    // ── Instantiate all 18 active engines ──────────────────────────────────
    chimera::EdgeEngine e_btc_d1(btc_d1_cfg);
    chimera::EdgeEngine e_eth_d1(eth_d1_cfg);
    chimera::EdgeEngine e_sol_d1(sol_d1_cfg);
    chimera::EdgeEngine e_link_d1(link_d1_cfg);
    chimera::EdgeEngine e_bnb_d1(bnb_d1_cfg);

    chimera::EdgeEngine e_xrp_h4(xrp_h4_cfg);
    chimera::EdgeEngine e_bnb_h4(bnb_h4_cfg);
    chimera::EdgeEngine e_link_h4(link_h4_cfg);
    chimera::EdgeEngine e_sol_h4(sol_h4_cfg);
    chimera::EdgeEngine e_btc_h4(btc_h4_cfg);
    chimera::EdgeEngine e_eth_h4(eth_h4_cfg);
    chimera::EdgeEngine e_avax_h4(avax_h4_cfg);

    chimera::EdgeEngine e_btc_h12(btc_h12_cfg);
    chimera::EdgeEngine e_doge_h12(doge_h12_cfg);
    chimera::EdgeEngine e_avax_h12(avax_h12_cfg);

    // ── Run active backtests ────────────────────────────────────────────
    std::printf("\n[RUN] Running OOS backtests (seed 80%%, test 20%%) for 18 engines ...\n\n");

    std::vector<BacktestResult> d1_results;
    d1_results.push_back(run_backtest(e_btc_d1,   btc_d1_cfg,   btc_d1));
    d1_results.push_back(run_backtest(e_eth_d1,   eth_d1_cfg,   eth_d1));
    d1_results.push_back(run_backtest(e_sol_d1,   sol_d1_cfg,   sol_d1));
    d1_results.push_back(run_backtest(e_link_d1,  link_d1_cfg,  link_d1));
    d1_results.push_back(run_backtest(e_bnb_d1,   bnb_d1_cfg,   bnb_d1));

    std::vector<BacktestResult> h4_results;
    h4_results.push_back(run_backtest(e_xrp_h4,   xrp_h4_cfg,   xrp_h4));
    h4_results.push_back(run_backtest(e_bnb_h4,   bnb_h4_cfg,   bnb_h4));
    h4_results.push_back(run_backtest(e_link_h4,  link_h4_cfg,  link_h4));
    h4_results.push_back(run_backtest(e_sol_h4,   sol_h4_cfg,   sol_h4));
    h4_results.push_back(run_backtest(e_btc_h4,   btc_h4_cfg,   btc_h4));
    h4_results.push_back(run_backtest(e_eth_h4,   eth_h4_cfg,   eth_h4));
    h4_results.push_back(run_backtest(e_avax_h4,  avax_h4_cfg,  avax_h4));

    std::vector<BacktestResult> h12_results;
    h12_results.push_back(run_backtest(e_btc_h12,  btc_h12_cfg,  btc_h12));
    h12_results.push_back(run_backtest(e_doge_h12, doge_h12_cfg, doge_h12));
    h12_results.push_back(run_backtest(e_avax_h12, avax_h12_cfg, avax_h12));

    // Combine all for CSV and verdict
    std::vector<BacktestResult> all_results;
    all_results.insert(all_results.end(), d1_results.begin(),  d1_results.end());
    all_results.insert(all_results.end(), h4_results.begin(),  h4_results.end());
    all_results.insert(all_results.end(), h12_results.begin(), h12_results.end());

    // ══════════════════════════════════════════════════════════════════════
    // DISABLED ENGINES — run only with --all flag for comparison
    // ══════════════════════════════════════════════════════════════════════
    std::vector<BacktestResult> disabled_results;
    if (run_all) {
        std::printf("[RUN] --all flag: also running 7 disabled engines ...\n\n");

        // Synthesize H6 for disabled engines that need it
        auto eth_h6  = synthesize_bars(eth_h1,  21600);
        auto sol_h6  = synthesize_bars(sol_h1,  21600);
        auto link_h6 = synthesize_bars(link_h1, 21600);
        auto doge_h4 = synthesize_bars(doge_h1, 14400);

        // D5. ETH-BB-H6 — DISABLED (OOS PF=0.72, Sharpe=-0.59)
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
            .max_history    = 64,
        };

        // D6. SOL-DONCH-H6 — DISABLED (OOS PF=0.83, Sharpe=-0.57)
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
            .max_history    = 64,
        };

        // D7. XRP-DONCH-H1 — DISABLED (OOS PF=0.82, Sharpe=-1.19)
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
            .max_history    = 64,
        };

        // D8. LINK-RSI-H6 — DISABLED (OOS PF=1.17 but only 4 trades)
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
            .max_history    = 64,
        };

        // D9. BTC-OVERNIGHT-H1 — DISABLED (OOS PF=0.31, Sharpe=-4.91)
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

        // D10. BTC-WEEKDAY-D1 — DISABLED (OOS PF=0.44, Sharpe=-1.86)
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

        // D11. DOGE-TSMOM-H4 — DISABLED (PF=1.28 but Nbr=49% — isolated peak)
        chimera::EdgeEngine::Config doge_h4_cfg{
            .symbol         = "dogeusdt",
            .tag            = "DOGE-TSMOM-H4",
            .kind           = chimera::StrategyKind::TSMOM,
            .tf_secs        = 14400,
            .lookback       = 25,
            .hold_bars      = 16,
            .sl_atr_mult    = 2.0,
            .atr_period     = 14,
            .bb_k           = 2.0,
            .rsi_threshold  = 30.0,
            .round_trip_bp  = 22.0,
            .max_history    = 64,
            .trail_arm_atr  = 0.5,
            .trail_dist_atr = 0.4,
        };

        chimera::EdgeEngine ed5(eth_bb_cfg);
        chimera::EdgeEngine ed6(sol_donch_cfg);
        chimera::EdgeEngine ed7(xrp_donch_cfg);
        chimera::EdgeEngine ed8(link_rsi_cfg);
        chimera::EdgeEngine ed9(overnight_cfg);
        chimera::EdgeEngine ed10(weekday_cfg);
        chimera::EdgeEngine ed11(doge_h4_cfg);

        disabled_results.push_back(run_backtest(ed5,  eth_bb_cfg,    eth_h6));
        disabled_results.push_back(run_backtest(ed6,  sol_donch_cfg, sol_h6));
        disabled_results.push_back(run_backtest(ed7,  xrp_donch_cfg, xrp_h1));
        disabled_results.push_back(run_backtest(ed8,  link_rsi_cfg,  link_h6));
        disabled_results.push_back(run_backtest(ed9,  overnight_cfg, btc_h1));
        disabled_results.push_back(run_backtest(ed10, weekday_cfg,   btc_d1));
        disabled_results.push_back(run_backtest(ed11, doge_h4_cfg,   doge_h4));

        all_results.insert(all_results.end(), disabled_results.begin(), disabled_results.end());
    }

    // ── Output ───────────────────────────────────────────────────────────
    if (csv_mode) {
        std::printf("tag,symbol,strategy,tf,bars,trades,wins,wr,net_bp,pf,avg_bp,maxdd_bp,sharpe,cost_bp\n");
        for (auto& r : all_results) {
            std::printf("%s,%s,%s,%d,%d,%d,%d,%.1f,%.1f,%.2f,%.1f,%.1f,%.2f,%.1f\n",
                r.tag.c_str(), r.symbol.c_str(), r.strategy.c_str(),
                r.tf_secs, r.total_bars, r.trades, r.wins, r.win_rate,
                r.total_bp, r.pf, r.avg_bp, r.max_dd_bp, r.sharpe, r.cost_bp);
        }
    } else {
        print_header();
        print_section_header("D1 ENGINES (5) — Session 13 + BNB NEW");
        for (auto& r : d1_results) print_row(r);
        print_section_header("H4 ENGINES (7) — ALL NEW Session 14");
        for (auto& r : h4_results) print_row(r);
        print_section_header("H12 ENGINES (3) — ALL NEW Session 14");
        for (auto& r : h12_results) print_row(r);

        if (!disabled_results.empty()) {
            print_section_header("DISABLED ENGINES (7) — No OOS edge");
            for (auto& r : disabled_results) print_row(r);
        }

        print_footer();
        print_verdict(all_results);
    }

    return 0;
}
