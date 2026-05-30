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
#include <map>
#include <numeric>

#include <unistd.h>
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

static int g_last_bars = 0;  // 0 = use 80/20 split; >0 = last N bars
static int g_last_days = 0;  // 0 = ignore; >0 = last N days (converted via tf_secs)
static int g_end_days_ago = 0; // >0: drop last N days from klines before split (holdout sim)
static double g_hard_floor_bp_override = 1.0; // 1.0 = no override; <=0 = applies after preset
static double g_ppb_be_arm_override = -1.0;    // <0 = no override
static double g_ppb_lock_pct_override = -1.0;  // <0 = no override
static double g_engine_daily_cap_bp  = 0.0;    // 0 = off; positive N = block entries when -bp in 24h >= N
static double g_symbol_daily_cap_bp  = 0.0;    // 0 = off; per-symbol cap across all engines (used in roster mode)
static double g_sl_mult_scale = 1.0;           // multiplier on sl_atr_mult (1.0 = no change)
static double g_mfe_trail_retain = 0.0;        // 0 = off
static double g_mfe_trail_min_bp = 50.0;
static int    g_swing_low_bars   = 0;          // 0 = off
static double g_low_vol_ratio    = 0.0;        // 0 = off
static int    g_low_vol_avg_bars = 20;
static int    g_signal_confirm_override = 0;   // 0 = off
// P0: baseline backtest fills stops at eff_stop (correct for continuous liquid
// pairs; the coarse O/H/L/C path-sim cannot tell a real gap from an intrabar
// trend, so filling at the bar low is an artifact). --realistic-gap-fill turns
// on price-fill, used WITH --inject-gap-bp for the explicit gap stress test.
static bool   g_realistic_gap_fill  = false;
static double g_gap_extra_slip_bp   = 0.0;     // P0: extra book-depth slippage on gap fills
// P3/S46: gap-injection stress. On every g_gap_every-th bar, inject a gap-DOWN
// open of g_inject_gap_bp (negative) so an open position fills at the gapped
// price (needs --realistic-gap-fill). Measures how often realized loss exceeds
// the -170 floor and the worst tail, per engine.
static double g_inject_gap_bp = 0.0;           // 0 = off; e.g. -400
static int    g_gap_every     = 50;            // inject on every Nth bar
static double g_worst_trade_bp   = 0.0;        // P3: worst single-trade bp this run
static int    g_n_beyond_floor   = 0;          // P3: # trades worse than -170 floor

static BacktestResult run_backtest(chimera::EdgeEngine& engine,
                                    const chimera::EdgeEngine::Config& cfg,
                                    const std::vector<Kline>& klines) {
    BacktestResult r{};
    r.tag      = cfg.tag;
    r.symbol   = cfg.symbol;
    r.strategy = chimera::strategy_name(cfg.kind);
    r.tf_secs  = (int)cfg.tf_secs;
    r.cost_bp  = cfg.round_trip_bp;

    // P0/S46: baseline fills at eff_stop; --realistic-gap-fill opts into price-
    // fill for the gap stress test.
    engine.set_realistic_gap_fill(g_realistic_gap_fill);
    engine.set_gap_extra_slip_bp(g_gap_extra_slip_bp);
    g_worst_trade_bp = 0.0; g_n_beyond_floor = 0;   // P3: reset per-engine stress counters

    if (klines.empty()) {
        r.total_bars = 0;
        return r;
    }

    // Holdout sim: drop last g_end_days_ago days so we score on data preceding
    // some recent window the optimizer "couldn't see".
    std::vector<Kline> klines_trimmed;
    const std::vector<Kline>* klines_use = &klines;
    if (g_end_days_ago > 0) {
        int trim_bars = (int)((int64_t)g_end_days_ago * 86400 / cfg.tf_secs);
        if (trim_bars > 0 && trim_bars < (int)klines.size()) {
            klines_trimmed.assign(klines.begin(), klines.end() - trim_bars);
            klines_use = &klines_trimmed;
        }
    }

    // Seed/OOS split: default 80/20. If g_last_days or g_last_bars > 0, OOS
    // = last N bars (seed = everything before). Lets us test "live era".
    int total = (int)klines_use->size();
    int oos_bars = 0;
    if (g_last_days > 0) {
        // Convert days -> bars via this engine's TF.
        oos_bars = (int)((int64_t)g_last_days * 86400 / cfg.tf_secs);
    } else if (g_last_bars > 0) {
        oos_bars = g_last_bars;
    }
    int seed_count, oos_start;
    if (oos_bars > 0 && oos_bars < total) {
        oos_start  = total - oos_bars;
        seed_count = oos_start;
    } else {
        seed_count = (int)(total * 0.8);
        oos_start  = seed_count;
    }

    // Seed bars
    std::vector<chimera::EdgeEngine::SeedBar> seeds;
    seeds.reserve(seed_count);
    for (int i = 0; i < seed_count; ++i) {
        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = (*klines_use)[i].open_ts_ms;
        sb.o = (*klines_use)[i].o;
        sb.h = (*klines_use)[i].h;
        sb.l = (*klines_use)[i].l;
        sb.c = (*klines_use)[i].c;
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
    // S44L: per-engine daily loss cap — track (exit_ts, bp), block entries
    // when cumulative net bp over rolling 24h <= -cap_bp.
    std::vector<std::pair<int64_t, double>> recent_trades_24h;
    int64_t cap_block_until_ts = 0;
    int64_t day_ms = 24LL * 3600 * 1000;

    // Feed OOS bars as ticks
    r.total_bars = total - oos_start;
    // S38: D1 trend feed for kinds that gate on it (e.g. BREAKOUT_PULLBACK).
    // D1 TSMOM lookback=10 is the production BTC-TSMOM-D1 setting; on the
    // bar TF that's `bars_per_d1 * 10` bars back.
    int bars_per_d1 = (cfg.tf_secs > 0) ? (int)(86400 / cfg.tf_secs) : 1;
    if (bars_per_d1 < 1) bars_per_d1 = 1;
    int d1_lookback_bars = bars_per_d1 * 10;
    for (int i = oos_start; i < total; ++i) {
        const Kline& k = (*klines_use)[i];
        int64_t bar_start_ms = k.open_ts_ms;
        int64_t tick_step    = (cfg.tf_secs * 1000) / 4;

        // Feed D1 trend state to the engine (no-op for kinds that ignore it).
        if (i >= d1_lookback_bars) {
            bool d1_bull = (*klines_use)[i].c > (*klines_use)[i - d1_lookback_bars].c;
            engine.set_d1_bullish(d1_bull);
        }

        bool bullish = (k.c >= k.o);

        // P3/S46: inject a synthetic gap-DOWN open on selected bars. Fed as the
        // FIRST tick so cur_open_ = gapped price; an open position below its stop
        // then fills at the gap (with --realistic-gap-fill), exercising the
        // gap-through tail that real klines rarely contain.
        if (g_inject_gap_bp < 0.0 && g_gap_every > 0 && (i % g_gap_every) == 0) {
            engine.on_tick(k.o * (1.0 + g_inject_gap_bp / 1e4), bar_start_ms);
        }

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

        // Tick 4: Close — feed the TRUE close as the last in-bar tick so the
        // engine records k.c (not the bar extreme) as cur_close_. Without this,
        // the bar was closed by the next bar's open tick, leaving cur_close_ =
        // tick3 = High on green bars / Low on red bars. Every indicator reads
        // closes_, so signals were computed on extreme-biased closes that live
        // (real websocket closes) never sees. 3*tick_step = 0.75*tf stays in-bar.
        engine.on_tick(k.c, bar_start_ms + tick_step * 3);

        // For the final bar there is no next-bar open tick to trigger close_bar_,
        // so force it with a tick 1 second into the next bar.
        if (i == total - 1) {
            engine.on_tick(k.c, bar_start_ms + cfg.tf_secs * 1000 + 1000);
        }

        // Check if a trade completed this bar
        if (engine.trades() > prev_trades) {
            double trade_bp = engine.total_bp() - prev_total_bp;
            trade_returns.push_back(trade_bp);
            if (trade_bp < g_worst_trade_bp) g_worst_trade_bp = trade_bp;  // P3 stress
            if (trade_bp < -185.0) g_n_beyond_floor++;                     // worse than -170 floor+cost
            equity_bp = engine.total_bp();
            if (equity_bp > peak_bp) peak_bp = equity_bp;
            double dd = peak_bp - equity_bp;
            if (dd > max_dd) max_dd = dd;
            prev_trades   = engine.trades();
            prev_total_bp = engine.total_bp();
            // S44L: feed daily-loss tracker
            if (g_engine_daily_cap_bp > 0.0) {
                recent_trades_24h.push_back({bar_start_ms, trade_bp});
                int64_t cutoff = bar_start_ms - day_ms;
                while (!recent_trades_24h.empty() && recent_trades_24h.front().first < cutoff)
                    recent_trades_24h.erase(recent_trades_24h.begin());
                double sum_24h = 0;
                for (auto& p : recent_trades_24h) sum_24h += p.second;
                if (sum_24h <= -g_engine_daily_cap_bp) {
                    cap_block_until_ts = bar_start_ms + day_ms;
                }
            }
        }
        // S44L: enforce daily cap — when blocked, freeze engine (no entries)
        if (g_engine_daily_cap_bp > 0.0 && bar_start_ms < cap_block_until_ts) {
            engine.set_portfolio_gate(false);
        } else if (g_engine_daily_cap_bp > 0.0) {
            engine.set_portfolio_gate(true);
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

    // P3/S46: gap-stress summary (only when injecting). Shows how a -170 floor
    // fares against injected gaps: how many fills blew past it and the worst tail.
    if (g_inject_gap_bp < 0.0) {
        std::fprintf(stderr, "[GAP-STRESS] %-20s inject=%.0fbp/every%d  trades=%d  worst_trade=%.0fbp  fills_beyond_floor=%d (%.1f%%)\n",
            cfg.tag.c_str(), g_inject_gap_bp, g_gap_every, r.trades, g_worst_trade_bp,
            g_n_beyond_floor, r.trades > 0 ? 100.0 * g_n_beyond_floor / r.trades : 0.0);
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
// Disable all post-Session32 exit protections (giveback cap, staged ratchet,
// hard floor, early kill). Pre-S32 simple ATR-trail + hold-bars baseline.
static void disable_s32_protections(chimera::EdgeEngine::Config& c) {
    c.hard_floor_bp     =  0.0;   // disabled (code: < 0 = active)
    c.ratchet_start_bp  =  0.0;   // disabled (code: > 0 = active)
    c.be_arm_bp         =  0.0;
    c.giveback_arm_bp   =  0.0;   // disabled (code: > 0 = active)
    c.early_kill_bp     =  0.0;   // disabled (code: < 0 = active)
    c.early_kill_mfe    =  0.0;
    c.trail_tighten_atr =  0.0;   // already default 0
    c.signal_confirm_bars = 1;    // back to single-bar confirm
}

// Mirror EdgeEngine::apply_protection_only_preset (EdgeEngine.hpp post-S36
// rewrite — staged-ratchet ONLY). Destructive layers disabled.
static void mirror_protection_only_preset(chimera::EdgeEngine::Config& c) {
    c.hard_floor_bp           =  0.0;
    c.early_kill_bp           =  0.0;
    c.early_kill_mfe          =  0.0;
    c.early_kill_min_hold_ms  =  0;
    c.giveback_arm_bp         =  0.0;
    c.signal_confirm_bars     =  1;
    double rt = c.round_trip_bp;
    c.ratchet_start_bp  = rt;
    c.be_arm_bp         = rt + 10.0;
    c.ratchet_lock_pct  = 0.75;
    c.prog_lock_pct_2   = 0.85;
    c.prog_lock_pct_3   = 0.90;
    c.prog_lock_pct_4   = 0.95;
    // intentionally NOT touching trail params — preserves bespoke
}

// Mirror EdgeEngine::apply_safety_preset (EdgeEngine.hpp post-S36) — same
// layer logic as protection_only + uniform S14-baseline trail override.
static void mirror_safety_preset(chimera::EdgeEngine::Config& c) {
    mirror_protection_only_preset(c);
    c.trail_arm_atr          = 1.0;
    c.trail_dist_atr         = 0.4;
    c.trail_tighten_atr      = 0.0;
    c.trail_tighten_dist_atr = 0.3;
}

// Tier per known historical PF (matches main.cpp:5251-5270 logic).
static void apply_prod_tiered(chimera::EdgeEngine::Config& c, double historical_pf) {
    if (historical_pf >= 2.0) mirror_protection_only_preset(c);
    else                       mirror_safety_preset(c);
}

// Per-engine historical PF lookup (matches main.cpp:5251-5270 tier logic).
static double pf_lookup(const std::string& tag) {
    if (tag == "BTC-TSMOM-D1")  return 1.92;
    if (tag == "ETH-TSMOM-D1")  return 3.15;
    if (tag == "SOL-TSMOM-D1")  return 2.25;
    if (tag == "LINK-TSMOM-D1") return 2.18;
    if (tag == "BNB-TSMOM-D1")  return 3.16;
    if (tag == "XRP-TSMOM-H4")  return 2.43;
    if (tag == "BNB-TSMOM-H4")  return 1.91;
    if (tag == "LINK-TSMOM-H4") return 1.91;
    if (tag == "SOL-TSMOM-H4")  return 1.89;
    if (tag == "BTC-TSMOM-H4")  return 1.82;
    if (tag == "ETH-TSMOM-H4")  return 1.76;
    if (tag == "AVAX-TSMOM-H4") return 1.47;
    if (tag == "BTC-TSMOM-H12") return 3.63;
    if (tag == "DOGE-TSMOM-H12")return 2.78;
    if (tag == "AVAX-TSMOM-H12")return 2.61;
    if (tag == "DOGE-TSMOM-D3") return 3.72;
    if (tag == "APT-TSMOM-H8")  return 1.49;
    return 1.5;
}

static void apply_preset_named(chimera::EdgeEngine::Config& c, const std::string& p);

// Loosened variant — minimal protection, lets trends breathe.
static void apply_loose_preset(chimera::EdgeEngine::Config& c) {
    double rt = c.round_trip_bp;
    c.hard_floor_bp     = -200.0;          // still caps catastrophe, not winners
    c.ratchet_start_bp  = rt;              // BE ramp start
    c.be_arm_bp         = rt + 10.0;       // BE lock
    c.ratchet_lock_pct  = 0.50;            // looser lock %
    c.prog_lock_pct_2   = 0.60;
    c.prog_lock_pct_3   = 0.70;
    c.prog_lock_pct_4   = 0.80;
    c.giveback_arm_bp   = rt + 200.0;      // arm only on big peaks
    c.giveback_pct      = 0.50;            // 50% pullback before exit
    c.early_kill_bp     = 0.0;             // disable
    c.early_kill_mfe    = 0.0;
    c.signal_confirm_bars = 1;             // single-bar confirm
}

// Apply named preset to a Config (mirrors main.cpp tiered logic).
static void apply_preset_named(chimera::EdgeEngine::Config& c, const std::string& p) {
    double hpf = pf_lookup(c.tag);
    if (p == "" || p == "defaults") return;
    if (p == "legacy")               { disable_s32_protections(c); return; }
    if (p == "staged_only") {
        disable_s32_protections(c);
        c.ratchet_start_bp = 15.0; c.be_arm_bp = 50.0; c.ratchet_lock_pct = 0.75;
        return;
    }
    if (p == "prod_safety")          { mirror_safety_preset(c); return; }
    if (p == "prod_protection_only") { mirror_protection_only_preset(c); return; }
    if (p == "prod_tiered")          { apply_prod_tiered(c, hpf); return; }
    if (p == "loose")                { apply_loose_preset(c); return; }
    if (p == "no_giveback")          { apply_prod_tiered(c, hpf); c.giveback_arm_bp = 0.0; return; }
    if (p == "no_early_kill")        { apply_prod_tiered(c, hpf); c.early_kill_bp = 0.0; c.early_kill_mfe = 0.0; return; }
    if (p == "no_signal_confirm")    { apply_prod_tiered(c, hpf); c.signal_confirm_bars = 1; return; }
    if (p == "no_hard_floor")        { apply_prod_tiered(c, hpf); c.hard_floor_bp = 0.0; return; }
    if (p == "prod_tiered_with_ek")  {
        // S36 baseline + re-enable early_kill (old behavior on dead-on-arrival)
        apply_prod_tiered(c, hpf);
        c.early_kill_bp           = -25.0;
        c.early_kill_mfe          =  15.0;
        c.early_kill_min_hold_ms  =  0;
        return;
    }
    if (p == "prod_tiered_with_ek_hf") {
        // S36 baseline + early_kill + hard_floor (closer to old prod behavior)
        apply_prod_tiered(c, hpf);
        c.early_kill_bp           = -25.0;
        c.early_kill_mfe          =  15.0;
        c.early_kill_min_hold_ms  =  0;
        c.hard_floor_bp           = -50.0;
        return;
    }
    if (p == "prod_tiered_pyramid") {
        // S38: prod_tiered + pyramid + tight trail + giveback. Only arms
        // pyramid AFTER trail-armed (BE locked), so bounded downside.
        apply_prod_tiered(c, hpf);
        c.pyramid_enabled       = true;
        c.pyramid_arm_atr       = 2.5;
        c.pyramid_step_atr      = 1.5;
        c.pyramid_size_mult     = 0.5;
        c.pyramid_max_adds      = 2;
        c.trail_tighten_atr     = 2.0;
        c.trail_tighten_dist_atr = 0.2;
        c.giveback_arm_bp       = 100.0;
        c.giveback_pct          = 0.30;
        return;
    }
    if (p == "prod_tiered_pyramid_pure") {
        // S38b: pyramid only — keep prod_tiered exits as-is, just enable adds.
        apply_prod_tiered(c, hpf);
        c.pyramid_enabled       = true;
        c.pyramid_arm_atr       = 2.5;
        c.pyramid_step_atr      = 1.5;
        c.pyramid_size_mult     = 0.5;
        c.pyramid_max_adds      = 2;
        return;
    }
    if (p == "prod_tiered_pyramid_low") {
        // S38c: pyramid_arm at +1.0 ATR (typical TSMOM winner ~1-1.5 ATR MFE).
        apply_prod_tiered(c, hpf);
        c.pyramid_enabled       = true;
        c.pyramid_arm_atr       = 1.0;
        c.pyramid_step_atr      = 0.7;
        c.pyramid_size_mult     = 0.5;
        c.pyramid_max_adds      = 2;
        return;
    }
    if (p == "prod_tiered_pyramid_xlow") {
        // S38d: pyramid_arm at +0.5 ATR — aggressive.
        // NOTE: live enable_pyramid_xlow() uses max_adds=1, NOT 2 — keep aligned.
        apply_prod_tiered(c, hpf);
        c.pyramid_enabled       = true;
        c.pyramid_arm_atr       = 0.5;
        c.pyramid_step_atr      = 0.3;
        c.pyramid_size_mult     = 0.5;
        c.pyramid_max_adds      = 1;
        return;
    }
    if (p == "prod_tiered_pyramid_elite") {
        // S44 candidate: ELITE-tier preset. arm earlier, larger adds, more adds.
        apply_prod_tiered(c, hpf);
        c.pyramid_enabled       = true;
        c.pyramid_arm_atr       = 0.5;
        c.pyramid_step_atr      = 0.3;
        c.pyramid_size_mult     = 0.75;  // up from 0.5
        c.pyramid_max_adds      = 4;     // up from 2
        return;
    }
}

int main(int argc, char* argv[]) {
    bool csv_mode      = false;
    bool run_all       = false;
    bool legacy_exits  = false;
    bool prod_presets  = false;   // legacy alias for --preset prod_tiered
    std::string preset_name = "";  // "", "legacy", "defaults", "prod_tiered",
                                    // "prod_safety", "prod_protection_only",
                                    // "loose", "no_giveback", "no_early_kill",
                                    // "no_signal_confirm", "staged_only"
    // Bisection flags (used with --legacy-exits)
    bool enable_giveback   = false;
    bool enable_staged     = false;
    bool enable_early_kill = false;
    bool enable_hard_floor = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--csv")              csv_mode          = true;
        else if (a == "--all")              run_all           = true;
        else if (a == "--legacy-exits")     legacy_exits      = true;
        else if (a == "--prod-presets")   { prod_presets = true; preset_name = "prod_tiered"; }
        else if (a == "--add-giveback")     enable_giveback   = true;
        else if (a == "--add-staged")       enable_staged     = true;
        else if (a == "--add-early-kill")   enable_early_kill = true;
        else if (a == "--add-hard-floor")   enable_hard_floor = true;
        else if (a == "--preset" && i+1 < argc) { preset_name = argv[++i]; }
        else if (a == "--last-bars" && i+1 < argc) { g_last_bars = std::atoi(argv[++i]); }
        else if (a == "--last-days" && i+1 < argc) { g_last_days = std::atoi(argv[++i]); }
        else if (a == "--end-days-ago" && i+1 < argc) { g_end_days_ago = std::atoi(argv[++i]); }
        else if (a == "--hard-floor-bp" && i+1 < argc) { g_hard_floor_bp_override = std::atof(argv[++i]); }
        else if (a == "--ppb-be-arm" && i+1 < argc) { g_ppb_be_arm_override = std::atof(argv[++i]); }
        else if (a == "--ppb-lock-pct" && i+1 < argc) { g_ppb_lock_pct_override = std::atof(argv[++i]); }
        else if (a == "--engine-daily-cap-bp" && i+1 < argc) { g_engine_daily_cap_bp = std::atof(argv[++i]); }
        else if (a == "--sl-mult-scale" && i+1 < argc) { g_sl_mult_scale = std::atof(argv[++i]); }
        else if (a == "--mfe-trail-retain" && i+1 < argc) { g_mfe_trail_retain = std::atof(argv[++i]); }
        else if (a == "--mfe-trail-min-bp" && i+1 < argc) { g_mfe_trail_min_bp = std::atof(argv[++i]); }
        else if (a == "--swing-low-bars"   && i+1 < argc) { g_swing_low_bars   = std::atoi(argv[++i]); }
        else if (a == "--low-vol-ratio"    && i+1 < argc) { g_low_vol_ratio    = std::atof(argv[++i]); }
        else if (a == "--low-vol-avg-bars" && i+1 < argc) { g_low_vol_avg_bars = std::atoi(argv[++i]); }
        else if (a == "--signal-confirm"   && i+1 < argc) { g_signal_confirm_override = std::atoi(argv[++i]); }
        else if (a == "--realistic-gap-fill")              { g_realistic_gap_fill = true; }
        else if (a == "--gap-slip-bp"       && i+1 < argc) { g_gap_extra_slip_bp = std::atof(argv[++i]); }
        else if (a == "--inject-gap-bp"     && i+1 < argc) { g_inject_gap_bp = std::atof(argv[++i]); g_realistic_gap_fill = true; }
        else if (a == "--gap-every"         && i+1 < argc) { g_gap_every = std::atoi(argv[++i]); }
    }
    if (!preset_name.empty()) {
        std::fprintf(stderr, "[MODE] --preset %s\n", preset_name.c_str());
    }
    if (g_last_bars > 0) {
        std::fprintf(stderr, "[MODE] --last-bars %d (OOS = last N bars per engine)\n", g_last_bars);
    }
    if (g_last_days > 0) {
        std::fprintf(stderr, "[MODE] --last-days %d (OOS = last N days per engine; converted to bars via tf_secs)\n", g_last_days);
    }
    if (g_end_days_ago > 0) {
        std::fprintf(stderr, "[MODE] --end-days-ago %d (holdout sim: drop last N days from klines before split)\n", g_end_days_ago);
    }

    // ── DISCOVER MODE: full matrix (symbol × tf × kind × params) ─────────
    // Usage: ./backtest_mac --discover [--preset prod_tiered] [--last-days 180]
    // Static C++ arrays; one binary run; writes one CSV row per backtest.
    bool discover_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--discover") { discover_mode = true; break; }
    }
    if (discover_mode) {
        static constexpr const char* kSymbols[] = {
            "btcusdt","ethusdt","solusdt","linkusdt","bnbusdt","xrpusdt",
            "avaxusdt","dogeusdt","aptusdt","arbusdt","fetusdt","nearusdt",
            "ondousdt","pepeusdt","suiusdt","tiausdt","wifusdt",
            "hbarusdt","injusdt","adausdt","trxusdt","seiusdt",
            "opusdt","maticusdt","atomusdt","filusdt","aaveusdt","uniusdt",
            "ldousdt","enausdt","jupusdt",
            "tonusdt","dotusdt","icpusdt","renderusdt","pythusdt","grtusdt",
            "sandusdt","manausdt","crvusdt","compusdt","mkrusdt","imxusdt",
            "stxusdt","arkmusdt","maskusdt",
            "runeusdt","jtousdt","wusdt","turbousdt","bomeusdt","flokiusdt",
            "ethfiusdt","eigenusdt","zrousdt","gmtusdt","shibusdt","bchusdt",
            "ltcusdt","etcusdt","xlmusdt","vetusdt"
        };
        // S38c WIDER NET: 10 TFs (H1, H2, H3, H4, H6, H8, H12, D1, D2, D3).
        static constexpr int kTfSecs[]   = { 3600, 7200, 10800, 14400, 21600, 28800, 43200, 86400, 172800, 259200 };
        static constexpr const char* kTfNames[] = { "H1","H2","H3","H4","H6","H8","H12","D1","D2","D3" };
        static constexpr chimera::StrategyKind kKinds[] = {
            chimera::StrategyKind::TSMOM,
            chimera::StrategyKind::DONCHIAN,
            chimera::StrategyKind::BOLLINGER,
            chimera::StrategyKind::RSI_REVERT,
            chimera::StrategyKind::OVERNIGHT,
            chimera::StrategyKind::WEEKDAY,
            chimera::StrategyKind::KELTNER_REVERT,
            chimera::StrategyKind::DUAL_THRUST,
            chimera::StrategyKind::ICHIMOKU,
            chimera::StrategyKind::SUPERTREND,
            chimera::StrategyKind::WILLIAMS_R,
            chimera::StrategyKind::STOCH_RSI,
            chimera::StrategyKind::BREAKOUT_PULLBACK,
        };
        // S38d ULTRA WIDE: finest grid (6×6×4 = 144 combos).
        static constexpr int    kLookbacks[] = { 6, 12, 18, 30, 45, 60 };
        static constexpr int    kHolds[]     = { 3, 5, 8, 12, 18, 24 };
        static constexpr double kSlAtrs[]    = { 1.0, 2.0, 3.0, 4.0 };

        std::string data_dir_local;
        if (fs::exists("data/btc_h1_part0.json"))           data_dir_local = "data";
        else if (fs::exists("backtest/data/btc_h1_part0.json")) data_dir_local = "backtest/data";
        else { std::fprintf(stderr, "ERROR: no data dir\n"); return 1; }

        std::printf("symbol,tf_name,tf_secs,kind,lookback,hold,sl_atr,trades,wins,wr,net_bp,pf,sharpe,maxdd_bp,bars\n");

        long total_runs = 0;
        for (const char* sym : kSymbols) {
            // Resolve data prefix: btcusdt -> btc, others -> <sym>
            std::string base = sym;
            if (std::string(sym) == "btcusdt") base = "btc";
            std::string h1_probe = data_dir_local + "/" + base + "_h1_part0.json";
            if (!fs::exists(h1_probe)) {
                std::fprintf(stderr, "[SKIP] %s — no H1 data\n", sym);
                continue;
            }
            auto h1 = load_all_parts(data_dir_local, base + "_h1_part");
            std::fprintf(stderr, "[DISCOVER] %s H1=%zu bars\n", sym, h1.size());

            for (size_t ti = 0; ti < sizeof(kTfSecs)/sizeof(kTfSecs[0]); ++ti) {
                int tf_secs = kTfSecs[ti];
                std::vector<Kline> bars;
                if (tf_secs == 3600) bars = h1;
                else                 bars = synthesize_bars(h1, tf_secs);
                if ((int)bars.size() < 300) {
                    std::fprintf(stderr, "  [SKIP] %s %s — only %zu bars\n", sym, kTfNames[ti], bars.size());
                    continue;
                }

                // Cost (mirror main.cpp / sweep mode)
                double rt = 22.0;
                std::string ss = sym;
                if (ss == "btcusdt" || ss == "ethusdt") rt = 17.0;
                else if (ss == "solusdt" || ss == "bnbusdt" || ss == "dogeusdt" || ss == "xrpusdt") rt = 20.0;

                for (auto kind : kKinds) {
                    // OVERNIGHT requires H1 + UTC hour gate; skip non-H1.
                    if (kind == chimera::StrategyKind::OVERNIGHT && tf_secs != 3600) continue;
                    // WEEKDAY requires D1; skip non-D1.
                    if (kind == chimera::StrategyKind::WEEKDAY && tf_secs != 86400) continue;

                    for (int lb : kLookbacks)
                    for (int hold : kHolds)
                    for (double sl : kSlAtrs) {
                        chimera::EdgeEngine::Config c{
                            .symbol = ss, .tag = "DISC", .kind = kind, .tf_secs = tf_secs,
                            .lookback = lb, .hold_bars = hold, .sl_atr_mult = sl,
                            .atr_period = 14, .bb_k = 2.0, .rsi_threshold = 30.0,
                            .round_trip_bp = rt, .max_history = 128,
                            .trail_arm_atr = 1.0, .trail_dist_atr = 0.5,
                            .trail_tighten_atr = 3.0, .trail_tighten_dist_atr = 0.25,
                        };
                        apply_preset_named(c, preset_name);
                        if (g_hard_floor_bp_override <= 0.0) c.hard_floor_bp = g_hard_floor_bp_override;
                        chimera::EdgeEngine eng(c);
                        // Suppress engine internal printf during backtest
                        std::fflush(stdout);
                        int _saved_stdout = dup(fileno(stdout));
                        FILE* _devnull = fopen("/dev/null", "w");
                        dup2(fileno(_devnull), fileno(stdout));
                        auto r = run_backtest(eng, c, bars);
                        std::fflush(stdout);
                        dup2(_saved_stdout, fileno(stdout));
                        close(_saved_stdout);
                        fclose(_devnull);
                        std::printf("%s,%s,%d,%s,%d,%d,%.1f,%d,%d,%.1f,%.1f,%.3f,%.2f,%.0f,%d\n",
                            ss.c_str(), kTfNames[ti], tf_secs,
                            chimera::strategy_name(kind),
                            lb, hold, sl,
                            r.trades, r.wins, r.win_rate, r.total_bp,
                            r.pf, r.sharpe, r.max_dd_bp, r.total_bars);
                        std::fflush(stdout);
                        ++total_runs;
                    }
                }
            }
        }
        std::fprintf(stderr, "[DISCOVER] DONE — %ld backtests\n", total_runs);
        return 0;
    }

    // ── SWEEP MODE: param search for a (symbol, tf, strategy) triple ─────
    // Usage: ./backtest_mac --sweep dogeusdt:259200:TSMOM [--preset staged_only]
    // Iterates lookback × hold × sl × trail combos, applies preset, prints
    // CSV sorted by PF descending. Use to rescue a weak engine or confirm dead.
    std::string sweep_spec;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--sweep" && i+1 < argc) {
            sweep_spec = argv[++i];
            break;
        }
    }
    if (!sweep_spec.empty()) {
        // Parse "SYMBOL:TF_SECS:STRATEGY"
        size_t p1 = sweep_spec.find(':');
        size_t p2 = sweep_spec.find(':', p1+1);
        if (p1 == std::string::npos || p2 == std::string::npos) {
            std::fprintf(stderr, "ERROR: --sweep needs SYMBOL:TF_SECS:STRATEGY (e.g. dogeusdt:259200:TSMOM)\n");
            return 1;
        }
        std::string sym  = sweep_spec.substr(0, p1);
        int tf_secs      = std::atoi(sweep_spec.substr(p1+1, p2-p1-1).c_str());
        std::string strat = sweep_spec.substr(p2+1);

        // Load + synthesize bars for this symbol/TF
        // Map symbol to base name in data dir (btcusdt -> btc_h1, ethusdt -> ethusdt_h1)
        std::string base = sym;
        if (sym == "btcusdt") base = "btc";
        std::string data_dir_local;
        if (fs::exists("data/btc_h1_part0.json"))           data_dir_local = "data";
        else if (fs::exists("backtest/data/btc_h1_part0.json")) data_dir_local = "backtest/data";
        else { std::fprintf(stderr, "ERROR: no data dir\n"); return 1; }

        // For sub-H1 timeframes, load native M15 parts (btcusdt_m15_part*) and
        // synthesize anything coarser from M15 if requested. Otherwise use H1.
        std::vector<Kline> bars;
        bool have_m15 = fs::exists(data_dir_local + "/" + sym + "_m15_part0.json");
        if (tf_secs < 3600 && have_m15) {
            auto m15 = load_all_parts(data_dir_local, sym + "_m15_part");
            if (tf_secs == 900) bars = m15;
            else                bars = synthesize_bars(m15, tf_secs);
        } else {
            auto h1 = load_all_parts(data_dir_local, base + "_h1_part");
            if (tf_secs == 3600) bars = h1;
            else                 bars = synthesize_bars(h1, tf_secs);
        }
        if (bars.size() < 100) {
            std::fprintf(stderr, "ERROR: insufficient bars (%zu) for %s tf=%d\n", bars.size(), sym.c_str(), tf_secs);
            return 1;
        }
        std::fprintf(stderr, "[SWEEP] %s tf=%d strat=%s bars=%zu preset=%s\n",
            sym.c_str(), tf_secs, strat.c_str(), bars.size(),
            preset_name.empty() ? "defaults" : preset_name.c_str());

        // Cost per symbol (mirror main.cpp)
        double rt = 22.0;
        if (sym == "btcusdt" || sym == "ethusdt") rt = 17.0;
        else if (sym == "solusdt" || sym == "bnbusdt" || sym == "dogeusdt" || sym == "xrpusdt") rt = 20.0;

        std::printf("lookback,hold,sl_atr,trail_arm,trail_dist,trades,wins,wr,net_bp,pf,sharpe,maxdd_bp\n");
        chimera::StrategyKind kind = chimera::StrategyKind::TSMOM;
        if (strat == "DONCHIAN")   kind = chimera::StrategyKind::DONCHIAN;
        else if (strat == "BOLLINGER") kind = chimera::StrategyKind::BOLLINGER;
        else if (strat == "RSI_REVERT") kind = chimera::StrategyKind::RSI_REVERT;
        else if (strat == "KELTNER_REVERT") kind = chimera::StrategyKind::KELTNER_REVERT;
        else if (strat == "SUPERTREND") kind = chimera::StrategyKind::SUPERTREND;
        else if (strat == "WILLIAMS_R") kind = chimera::StrategyKind::WILLIAMS_R;
        else if (strat == "ICHIMOKU") kind = chimera::StrategyKind::ICHIMOKU;
        else if (strat == "DUAL_THRUST") kind = chimera::StrategyKind::DUAL_THRUST;
        else if (strat == "BREAKOUT_PULLBACK") kind = chimera::StrategyKind::BREAKOUT_PULLBACK;

        for (int lookback : {8, 10, 15, 20, 25, 30, 40, 50}) {
        for (int hold : {3, 4, 6, 8, 12, 20}) {
        for (double sl : {1.5, 2.0, 2.5, 3.0, 4.0}) {
        for (double tarm : {0.5, 0.7, 1.0, 1.5}) {
        for (double tdist : {0.3, 0.4, 0.5}) {
            chimera::EdgeEngine::Config c{
                .symbol = sym, .tag = "SWEEP", .kind = kind, .tf_secs = tf_secs,
                .lookback = lookback, .hold_bars = hold, .sl_atr_mult = sl,
                .atr_period = 14, .bb_k = 2.0, .rsi_threshold = 30.0,
                .round_trip_bp = rt, .max_history = 64,
                .trail_arm_atr = tarm, .trail_dist_atr = tdist,
                .trail_tighten_atr = 3.0, .trail_tighten_dist_atr = 0.25,
            };
            apply_preset_named(c, preset_name);
            chimera::EdgeEngine eng(c);
            auto r = run_backtest(eng, c, bars);
            std::printf("%d,%d,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f,%.3f,%.2f,%.0f\n",
                lookback, hold, sl, tarm, tdist,
                r.trades, r.wins, r.win_rate, r.total_bp, r.pf, r.sharpe, r.max_dd_bp);
        }}}}}
        return 0;
    }

    // ── ROSTER MODE: bulk-validate engine list from CSV ──────────────────
    // Usage: ./backtest_mac --roster engine_roster.csv [--preset prod_tiered]
    std::string roster_path;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--roster" && i+1 < argc) {
            roster_path = argv[++i];
            break;
        }
    }
    if (!roster_path.empty()) {
        std::ifstream rf(roster_path);
        if (!rf.is_open()) {
            std::fprintf(stderr, "ERROR: cannot open roster CSV %s\n", roster_path.c_str());
            return 1;
        }
        std::string data_dir_local;
        if      (fs::exists("data/btc_h1_part0.json"))           data_dir_local = "data";
        else if (fs::exists("backtest/data/btc_h1_part0.json"))  data_dir_local = "backtest/data";
        else { std::fprintf(stderr, "ERROR: no data dir\n"); return 1; }

        std::map<std::string, std::vector<Kline>> h1_cache;
        auto get_h1 = [&](const std::string& sym) -> std::vector<Kline>& {
            auto it = h1_cache.find(sym);
            if (it != h1_cache.end()) return it->second;
            std::string base = (sym == "btcusdt") ? "btc" : sym;
            auto v = load_all_parts(data_dir_local, base + "_h1_part");
            return h1_cache.emplace(sym, std::move(v)).first->second;
        };

        std::string line;
        std::getline(rf, line); // header
        std::vector<std::string> headers;
        {
            std::stringstream ss(line); std::string col;
            while (std::getline(ss, col, ',')) headers.push_back(col);
        }
        auto col_idx = [&](const std::string& n) {
            for (size_t i = 0; i < headers.size(); ++i) if (headers[i] == n) return (int)i;
            return -1;
        };
        int ci_tag = col_idx("tag"), ci_sym = col_idx("symbol"), ci_kind = col_idx("kind"),
            ci_tf = col_idx("tf_secs"), ci_lb = col_idx("lookback"), ci_hb = col_idx("hold_bars"),
            ci_sl = col_idx("sl_atr_mult"), ci_rt = col_idx("round_trip_bp"),
            ci_ta = col_idx("trail_arm_atr"), ci_td = col_idx("trail_dist_atr"),
            ci_tta = col_idx("trail_tighten_atr"), ci_ttd = col_idx("trail_tighten_dist_atr"),
            ci_bb = col_idx("bb_k"), ci_rsi = col_idx("rsi_threshold"),
            // S44: ICHI + KELT strategy-specific overrides (optional)
            ci_itk = col_idx("ichi_tenkan_period"),
            ci_ikj = col_idx("ichi_kijun_period"),
            ci_isb = col_idx("ichi_senkou_b_period"),
            ci_kel = col_idx("keltner_ema_len"),
            ci_kam = col_idx("keltner_atr_mult");

        std::printf("tag,symbol,strategy,tf_secs,trades,wins,wr,total_bp,pf,sharpe,maxdd_bp,verdict\n");
        int passed = 0, failed = 0, skipped = 0;
        double sum_bp = 0;
        while (std::getline(rf, line)) {
            if (line.empty()) continue;
            std::vector<std::string> cols;
            std::stringstream ss(line); std::string c;
            while (std::getline(ss, c, ',')) cols.push_back(c);
            if ((int)cols.size() < (int)headers.size()) continue;

            std::string tag = cols[ci_tag];
            std::string sym = cols[ci_sym];
            std::string kind_s = cols[ci_kind];
            int tf_secs = std::atoi(cols[ci_tf].c_str());

            auto& h1 = get_h1(sym);
            if (h1.size() < 200) {
                std::printf("%s,%s,%s,%d,0,0,0,0,0,0,0,SKIP_NO_DATA\n",
                    tag.c_str(), sym.c_str(), kind_s.c_str(), tf_secs);
                ++skipped;
                continue;
            }
            std::vector<Kline> bars = (tf_secs == 3600) ? h1 : synthesize_bars(h1, tf_secs);
            if (bars.size() < 100) {
                std::printf("%s,%s,%s,%d,0,0,0,0,0,0,0,SKIP_NO_BARS\n",
                    tag.c_str(), sym.c_str(), kind_s.c_str(), tf_secs);
                ++skipped;
                continue;
            }

            chimera::StrategyKind kind = chimera::StrategyKind::TSMOM;
            if      (kind_s == "DONCHIAN")        kind = chimera::StrategyKind::DONCHIAN;
            else if (kind_s == "BOLLINGER")       kind = chimera::StrategyKind::BOLLINGER;
            else if (kind_s == "RSI_REVERT")      kind = chimera::StrategyKind::RSI_REVERT;
            else if (kind_s == "KELTNER_REVERT")  kind = chimera::StrategyKind::KELTNER_REVERT;
            else if (kind_s == "SUPERTREND")      kind = chimera::StrategyKind::SUPERTREND;
            else if (kind_s == "WILLIAMS_R")      kind = chimera::StrategyKind::WILLIAMS_R;
            else if (kind_s == "ICHIMOKU")        kind = chimera::StrategyKind::ICHIMOKU;
            else if (kind_s == "DUAL_THRUST")     kind = chimera::StrategyKind::DUAL_THRUST;
            else if (kind_s == "STOCH_RSI")       kind = chimera::StrategyKind::STOCH_RSI;
            else if (kind_s == "BREAKOUT_PULLBACK") kind = chimera::StrategyKind::BREAKOUT_PULLBACK;

            chimera::EdgeEngine::Config cfg{
                .symbol = sym, .tag = tag, .kind = kind, .tf_secs = tf_secs,
                .lookback = std::atoi(cols[ci_lb].c_str()),
                .hold_bars = std::atoi(cols[ci_hb].c_str()),
                .sl_atr_mult = std::atof(cols[ci_sl].c_str()),
                .atr_period = 14,
                .bb_k = std::atof(cols[ci_bb].c_str()),
                .rsi_threshold = std::atof(cols[ci_rsi].c_str()),
                .round_trip_bp = std::atof(cols[ci_rt].c_str()),
                .max_history = 64,
                .trail_arm_atr = std::atof(cols[ci_ta].c_str()),
                .trail_dist_atr = std::atof(cols[ci_td].c_str()),
                .trail_tighten_atr = std::atof(cols[ci_tta].c_str()),
                .trail_tighten_dist_atr = std::atof(cols[ci_ttd].c_str()),
            };
            // S44: apply optional strategy-specific overrides if present in CSV
            if (ci_itk >= 0 && ci_itk < (int)cols.size() && !cols[ci_itk].empty())
                cfg.ichi_tenkan_period = std::atoi(cols[ci_itk].c_str());
            if (ci_ikj >= 0 && ci_ikj < (int)cols.size() && !cols[ci_ikj].empty())
                cfg.ichi_kijun_period = std::atoi(cols[ci_ikj].c_str());
            if (ci_isb >= 0 && ci_isb < (int)cols.size() && !cols[ci_isb].empty())
                cfg.ichi_senkou_b_period = std::atoi(cols[ci_isb].c_str());
            if (ci_kel >= 0 && ci_kel < (int)cols.size() && !cols[ci_kel].empty())
                cfg.keltner_ema_len = std::atoi(cols[ci_kel].c_str());
            if (ci_kam >= 0 && ci_kam < (int)cols.size() && !cols[ci_kam].empty())
                cfg.keltner_atr_mult = std::atof(cols[ci_kam].c_str());
            apply_preset_named(cfg, preset_name);
            if (g_hard_floor_bp_override <= 0.0) cfg.hard_floor_bp = g_hard_floor_bp_override;
            if (g_ppb_be_arm_override   > 0.0) cfg.be_arm_bp        = g_ppb_be_arm_override;
            if (g_ppb_lock_pct_override > 0.0) cfg.ratchet_lock_pct = g_ppb_lock_pct_override;
            if (g_sl_mult_scale != 1.0) cfg.sl_atr_mult *= g_sl_mult_scale;
            if (g_mfe_trail_retain > 0.0) {
                cfg.mfe_trail_retain = g_mfe_trail_retain;
                cfg.mfe_trail_min_bp = g_mfe_trail_min_bp;
            }
            if (g_swing_low_bars > 0) cfg.swing_low_bars = g_swing_low_bars;
            if (g_low_vol_ratio > 0.0) {
                cfg.low_vol_skip_ratio = g_low_vol_ratio;
                cfg.low_vol_avg_bars   = g_low_vol_avg_bars;
            }
            if (g_signal_confirm_override > 0) cfg.signal_confirm_bars = g_signal_confirm_override;
            chimera::EdgeEngine eng(cfg);
            auto r = run_backtest(eng, cfg, bars);
            const char* verdict = (r.pf >= 1.3 && r.sharpe >= 0.3 && r.trades >= 5)
                ? "PASS" : "FAIL";
            if (std::string(verdict) == "PASS") ++passed; else ++failed;
            sum_bp += r.total_bp;
            std::printf("%s,%s,%s,%d,%d,%d,%.1f,%.1f,%.3f,%.2f,%.0f,%s\n",
                tag.c_str(), sym.c_str(), kind_s.c_str(), tf_secs,
                r.trades, r.wins, r.win_rate, r.total_bp, r.pf, r.sharpe, r.max_dd_bp, verdict);
        }
        std::fprintf(stderr, "\n[ROSTER] passed=%d failed=%d skipped=%d sum_bp=%.0f\n",
            passed, failed, skipped, sum_bp);
        return 0;
    }

    if (legacy_exits) {
        std::fprintf(stderr, "[MODE] --legacy-exits: disabling hard_floor/ratchet/giveback/early_kill on all configs\n");
        if (enable_giveback)   std::fprintf(stderr, "[MODE] +giveback (arm=100bp, pct=0.30)\n");
        if (enable_staged)     std::fprintf(stderr, "[MODE] +staged_ratchet (start=15bp, be_arm=50bp, lock=0.75)\n");
        if (enable_early_kill) std::fprintf(stderr, "[MODE] +early_kill (-50bp if mfe<10bp)\n");
        if (enable_hard_floor) std::fprintf(stderr, "[MODE] +hard_floor (-100bp)\n");
    }

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

    auto apply_preset = [&](chimera::EdgeEngine::Config& c) {
        apply_preset_named(c, preset_name);
    };

    // Back-compat: --legacy-exits maps to preset=legacy with bisection adds.
    if (legacy_exits && preset_name.empty()) preset_name = "legacy";

    for (auto* c : { &btc_d1_cfg, &eth_d1_cfg, &sol_d1_cfg, &link_d1_cfg, &bnb_d1_cfg,
                     &xrp_h4_cfg, &bnb_h4_cfg, &link_h4_cfg, &sol_h4_cfg, &btc_h4_cfg,
                     &eth_h4_cfg, &avax_h4_cfg,
                     &btc_h12_cfg, &doge_h12_cfg, &avax_h12_cfg }) {
        apply_preset(*c);
        // Bisection adds (only meaningful with --preset legacy)
        if (legacy_exits) {
            if (enable_giveback)    { c->giveback_arm_bp = 100.0; c->giveback_pct = 0.30; }
            if (enable_staged)      { c->ratchet_start_bp = 15.0; c->be_arm_bp = 50.0; c->ratchet_lock_pct = 0.75; }
            if (enable_early_kill)  { c->early_kill_bp = -50.0; c->early_kill_mfe = 10.0; }
            if (enable_hard_floor)  { c->hard_floor_bp = -100.0; }
        }
    }

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

    // ── OPEN-POSITION ENGINES (live as of 2026-05-28) ──────────────────────
    // Exact configs copied from main.cpp:3482-3499 and main.cpp:4719-4736.
    chimera::EdgeEngine::Config doge_d3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 50,   // S36-RETUNE
        .hold_bars      = 3,    // S36-RETUNE
        .sl_atr_mult    = 1.5,  // S36-RETUNE
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,  // S36
        .trail_dist_atr = 0.3,
        .trail_tighten_atr      = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine::Config apt_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,   // S36-RETUNE
        .hold_bars      = 3,    // S36-RETUNE
        .sl_atr_mult    = 1.5,  // S36-RETUNE
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr      = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    apply_preset(doge_d3_cfg);
    apply_preset(apt_h8_cfg);
    // Load + synthesize data for new symbols
    auto apt_h1 = load_all_parts(data_dir, "aptusdt_h1_part");
    auto apt_h8 = synthesize_bars(apt_h1, 28800);   // 8h
    auto doge_d3 = synthesize_bars(doge_h1, 259200); // 3d
    chimera::EdgeEngine e_doge_d3(doge_d3_cfg);
    chimera::EdgeEngine e_apt_h8(apt_h8_cfg);

    // [Old --prod-presets post-ctor block removed — preset dispatch now
    //  happens via apply_preset() on Configs BEFORE ctor, mirroring the
    //  exact same EdgeEngine.hpp preset values for byte-identical behavior.]
    (void)prod_presets;  // back-compat flag, mapped via preset_name

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
    h12_results.push_back(run_backtest(e_doge_d3,  doge_d3_cfg,  doge_d3));
    h12_results.push_back(run_backtest(e_apt_h8,   apt_h8_cfg,   apt_h8));

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
