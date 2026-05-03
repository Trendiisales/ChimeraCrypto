// ============================================================================
// replay.cpp — Backtest / Replay Harness for SwingEngine
//
// Pulls H4 OHLC klines from Binance REST for the 8 configured symbols, builds
// a chronologically-sorted tick stream, replays it through SwingEngine, and
// emits performance statistics + CSVs.
//
// Each H4 kline is replayed as 4 ticks within its 4-hour window:
//    open -> low -> high -> close
//
// CLI:
//   chimera_backtest [--bars N] [--symbols A,B,C] [--out DIR] [--quiet]
//
// Outputs (in --out directory):
//   trades.csv       One row per closed trade, full detail.
//   equity.csv       Equity curve (cum bp after each closed trade, by time).
//   summary.txt      Human-readable performance summary.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/stat.h>
#include <map>

#include <curl/curl.h>

#include "core/SwingEngine.hpp"
#include "execution/ExchangeLatencyEngine.hpp"

// BinanceWSFeed.cpp expects this extern symbol — define it once for the
// backtest binary so we can link the same translation units as the live build.
chimera::ExchangeLatencyEngine g_exchange_latency;

// ============================================================================
// CLI config
// ============================================================================
struct CLIConfig {
    int                       h4_bars      = 1500;
    std::vector<std::string>  symbols;
    std::string               out_dir      = "backtest_out";
    bool                      quiet        = false;
};

static void print_usage() {
    std::printf(
"chimera_backtest — replay SwingEngine over Binance H4 klines\n"
"\n"
"Usage:\n"
"  chimera_backtest [--bars N] [--symbols A,B,C] [--out DIR] [--quiet]\n"
"\n"
"Options:\n"
"  --bars N       H4 bars per symbol (default 1500). Each REST call returns\n"
"                 up to 1000; larger values paginate.\n"
"  --symbols X    Comma-separated lowercase symbols (default = all 8 from\n"
"                 SymbolIndex.hpp: btcusdt,ethusdt,solusdt,bnbusdt,avaxusdt,\n"
"                 linkusdt,xrpusdt,dogeusdt).\n"
"  --out DIR      Output directory for CSVs (default ./backtest_out).\n"
"  --quiet        Suppress per-trade stdout chatter from the engine.\n"
"  --help         Show this help.\n");
}

static bool parse_cli(int argc, char** argv, CLIConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") { print_usage(); std::exit(0); }
        if (a == "--quiet")  { cfg.quiet = true; continue; }
        if (a == "--bars" && i + 1 < argc) { cfg.h4_bars = std::atoi(argv[++i]); continue; }
        if (a == "--out"  && i + 1 < argc) { cfg.out_dir = argv[++i];           continue; }
        if (a == "--symbols" && i + 1 < argc) {
            std::string s = argv[++i];
            std::string cur;
            for (char c : s) {
                if (c == ',') { if (!cur.empty()) cfg.symbols.push_back(cur); cur.clear(); }
                else          { cur += (char)std::tolower((unsigned char)c); }
            }
            if (!cur.empty()) cfg.symbols.push_back(cur);
            continue;
        }
        std::fprintf(stderr, "[BACKTEST] Unknown arg: %s\n", a.c_str());
        print_usage();
        return false;
    }
    if (cfg.h4_bars < 200) {
        std::fprintf(stderr, "[BACKTEST] --bars must be >= 200 to warm indicators (got %d)\n",
                     cfg.h4_bars);
        return false;
    }
    return true;
}

// ============================================================================
// Klines fetching (Binance REST, paginated)
// ============================================================================
struct Kline {
    int64_t open_time_ms;
    double  open, high, low, close, volume;
};

static size_t curl_writer(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static bool parse_klines_response(const std::string& body, std::vector<Kline>& out) {
    size_t pos = body.find('[');
    if (pos == std::string::npos) return false;
    pos += 1;

    bool parsed_any = false;

    while (pos < body.size()) {
        size_t arr_start = body.find('[', pos);
        if (arr_start == std::string::npos) break;
        size_t arr_end = body.find(']', arr_start + 1);
        if (arr_end == std::string::npos) break;

        std::string inner = body.substr(arr_start + 1, arr_end - arr_start - 1);
        pos = arr_end + 1;

        if (inner.find(',') == std::string::npos) continue;

        std::vector<std::string> fields;
        size_t p = 0;
        while (p < inner.size()) {
            size_t comma = inner.find(',', p);
            if (comma == std::string::npos) comma = inner.size();
            std::string f = inner.substr(p, comma - p);
            size_t s = f.find_first_not_of(" \"\t");
            size_t e = f.find_last_not_of(" \"\t");
            if (s != std::string::npos) f = f.substr(s, e - s + 1);
            else                        f.clear();
            fields.push_back(f);
            p = comma + 1;
        }
        if (fields.size() < 6) continue;

        Kline k{};
        try {
            k.open_time_ms = static_cast<int64_t>(std::stoll(fields[0]));
            k.open    = std::stod(fields[1]);
            k.high    = std::stod(fields[2]);
            k.low     = std::stod(fields[3]);
            k.close   = std::stod(fields[4]);
            k.volume  = std::stod(fields[5]);
        } catch (...) { continue; }

        if (k.high <= 0 || k.low <= 0 || k.close <= 0) continue;
        out.push_back(k);
        parsed_any = true;
    }
    return parsed_any;
}

static bool fetch_klines_paginated(const std::string& symbol_lower,
                                   const std::string& interval,
                                   int total_bars,
                                   std::vector<Kline>& out) {
    std::string sym_upper = symbol_lower;
    for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

    out.clear();
    out.reserve(total_bars);

    int  remaining = total_bars;
    int64_t end_time_ms = 0;

    while (remaining > 0) {
        int request_limit = std::min(1000, remaining);

        std::string url = "https://api.binance.com/api/v3/klines?symbol=" + sym_upper
                        + "&interval=" + interval
                        + "&limit=" + std::to_string(request_limit);
        if (end_time_ms > 0) {
            url += "&endTime=" + std::to_string(end_time_ms);
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::fprintf(stderr, "[FETCH] curl init failed for %s %s\n",
                         sym_upper.c_str(), interval.c_str());
            return false;
        }
        std::string body;
        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::fprintf(stderr, "[FETCH] curl failed %s %s: %s\n",
                         sym_upper.c_str(), interval.c_str(),
                         curl_easy_strerror(res));
            return false;
        }

        std::vector<Kline> page;
        if (!parse_klines_response(body, page) || page.empty()) {
            std::fprintf(stderr, "[FETCH] empty/invalid response for %s %s "
                         "(first 200 chars: %.200s)\n",
                         sym_upper.c_str(), interval.c_str(), body.c_str());
            return false;
        }

        out.insert(out.begin(), page.begin(), page.end());
        remaining -= (int)page.size();

        if ((int)page.size() < request_limit) break;

        int64_t oldest = page.front().open_time_ms;
        end_time_ms = oldest - 1;

        usleep(150 * 1000);
    }

    if (out.empty()) return false;

    std::sort(out.begin(), out.end(), [](const Kline& a, const Kline& b) {
        return a.open_time_ms < b.open_time_ms;
    });

    return true;
}

// ============================================================================
// Tick stream construction
// ============================================================================
struct ReplayTick {
    int     symbol_id;
    int64_t now_ms;
    double  price;
    double  trade_qty;
};

static void build_ticks_for_symbol(int symbol_id,
                                   const std::vector<Kline>& bars,
                                   std::vector<ReplayTick>& out) {
    constexpr int64_t H4_MS = 14400000LL;
    const int64_t third = H4_MS / 3;

    out.reserve(out.size() + bars.size() * 4);
    for (const auto& k : bars) {
        const int64_t t0 = k.open_time_ms + 1;
        const int64_t t1 = k.open_time_ms + third;
        const int64_t t2 = k.open_time_ms + 2 * third;
        const int64_t t3 = k.open_time_ms + H4_MS - 1;
        out.push_back({symbol_id, t0, k.open,  0.0});
        out.push_back({symbol_id, t1, k.low,   0.0});
        out.push_back({symbol_id, t2, k.high,  0.0});
        out.push_back({symbol_id, t3, k.close, k.volume});
    }
}

static int resolve_symbol_id(const std::string& full_lower) {
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
        if (full_lower == chimera::sym_full(i)) return i;
    }
    return -1;
}

// ============================================================================
// Stats computation
// ============================================================================
struct PerGroupStats {
    int n_trades = 0;
    int n_wins   = 0;
    double sum_bp        = 0.0;
    double gross_win_bp  = 0.0;
    double gross_loss_bp = 0.0;
};

static void update_group(PerGroupStats& g, double pnl_bp) {
    g.n_trades++;
    g.sum_bp += pnl_bp;
    if (pnl_bp >= 0.0) {
        g.n_wins++;
        g.gross_win_bp += pnl_bp;
    } else {
        g.gross_loss_bp += -pnl_bp;
    }
}

static double safe_div(double a, double b) { return b == 0.0 ? 0.0 : a / b; }

static void print_group(const char* label, const PerGroupStats& g, std::FILE* f) {
    if (g.n_trades == 0) {
        std::fprintf(f, "  %-18s no trades\n", label);
        return;
    }
    const double wr   = 100.0 * (double)g.n_wins / (double)g.n_trades;
    const double avg  = g.sum_bp / g.n_trades;
    const double avg_w = g.n_wins              > 0 ? g.gross_win_bp  / g.n_wins              : 0.0;
    const double avg_l = (g.n_trades - g.n_wins) > 0 ? g.gross_loss_bp / (g.n_trades - g.n_wins) : 0.0;
    const double pf   = safe_div(g.gross_win_bp, g.gross_loss_bp);
    std::fprintf(f, "  %-18s trades=%4d  wr=%5.1f%%  total=%+9.1fbp  "
                    "avg=%+7.1fbp  avg_win=%+7.1fbp  avg_loss=%+7.1fbp  PF=%.2f\n",
                 label, g.n_trades, wr, g.sum_bp, avg, avg_w, -avg_l, pf);
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char** argv) {
    CLIConfig cfg;
    if (!parse_cli(argc, argv, cfg)) return 2;

    std::vector<int> active_ids;
    if (cfg.symbols.empty()) {
        for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) active_ids.push_back(i);
    } else {
        for (const auto& s : cfg.symbols) {
            int id = resolve_symbol_id(s);
            if (id < 0) {
                std::fprintf(stderr, "[BACKTEST] Unknown symbol '%s'\n", s.c_str());
                return 2;
            }
            active_ids.push_back(id);
        }
    }

    ::mkdir(cfg.out_dir.c_str(), 0755);

    std::printf("[BACKTEST] symbols=%zu  bars_per_symbol=%d  out=%s\n",
                active_ids.size(), cfg.h4_bars, cfg.out_dir.c_str());

    std::vector<ReplayTick> ticks;
    int64_t earliest_ms = INT64_MAX;
    int64_t latest_ms   = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    for (int id : active_ids) {
        std::string sym = chimera::sym_full(id);
        std::printf("[BACKTEST] fetching %s H4 (%d bars)...\n", sym.c_str(), cfg.h4_bars);
        std::fflush(stdout);

        std::vector<Kline> bars;
        if (!fetch_klines_paginated(sym, "4h", cfg.h4_bars, bars)) {
            std::fprintf(stderr, "[BACKTEST] FATAL: failed to fetch %s H4 history\n",
                         sym.c_str());
            curl_global_cleanup();
            return 1;
        }
        if ((int)bars.size() < 200) {
            std::fprintf(stderr, "[BACKTEST] FATAL: %s returned only %zu bars (need >=200)\n",
                         sym.c_str(), bars.size());
            curl_global_cleanup();
            return 1;
        }

        if (bars.front().open_time_ms < earliest_ms) earliest_ms = bars.front().open_time_ms;
        if (bars.back().open_time_ms  > latest_ms)   latest_ms   = bars.back().open_time_ms;

        std::printf("  %s: %zu bars, %lld -> %lld\n",
                    sym.c_str(), bars.size(),
                    (long long)bars.front().open_time_ms,
                    (long long)bars.back().open_time_ms);

        build_ticks_for_symbol(id, bars, ticks);
    }

    curl_global_cleanup();

    std::printf("[BACKTEST] sorting %zu ticks...\n", ticks.size());
    std::sort(ticks.begin(), ticks.end(), [](const ReplayTick& a, const ReplayTick& b) {
        if (a.now_ms != b.now_ms) return a.now_ms < b.now_ms;
        return a.symbol_id < b.symbol_id;
    });

    chimera::SwingEngine engine;
    engine.shadow_mode = true;
    engine.set_max_trade_log_size(0);

    if (cfg.quiet) {
        if (!std::freopen("/dev/null", "w", stdout)) { /* ignore — quiet mode best-effort */ }
    }

    chimera::MarketTick tick{};
    int processed = 0;
    const int print_every = std::max(1, (int)ticks.size() / 20);

    auto t_start = std::chrono::steady_clock::now();

    for (const auto& rt : ticks) {
        tick.symbol     = chimera::sym_full(rt.symbol_id);
        tick.bid        = rt.price * 0.99995;
        tick.ask        = rt.price * 1.00005;
        tick.mid_price  = rt.price;
        tick.last_price = rt.price;
        tick.trade_qty  = rt.trade_qty;
        tick.timestamp  = rt.now_ms;
        tick.trade_time = rt.now_ms;

        engine.update_price(rt.symbol_id, rt.price);
        engine.on_tick(rt.symbol_id, tick, rt.now_ms);

        ++processed;
        if (!cfg.quiet && (processed % print_every == 0)) {
            std::fprintf(stderr, "[BACKTEST] progress %d/%zu (%.1f%%)\r",
                         processed, ticks.size(),
                         100.0 * processed / (double)ticks.size());
            std::fflush(stderr);
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    std::fprintf(stderr, "\n[BACKTEST] replay done in %.2fs (%zu ticks)\n",
                 elapsed_ms / 1000.0, ticks.size());

    if (cfg.quiet) {
        if (!std::freopen("/dev/tty", "w", stdout)) { /* ignore — stdout restore best-effort */ }
    }

    const auto& trades = engine.get_trade_log();
    std::printf("[BACKTEST] engine reported %zu closed trades\n", trades.size());

    PerGroupStats overall;
    PerGroupStats by_strategy[5];
    std::map<std::string, PerGroupStats> by_symbol;
    std::map<std::string, PerGroupStats> by_reason;

    int max_consec_wins = 0, max_consec_losses = 0;
    int run_w = 0, run_l = 0;

    double cum_bp = 0.0;
    double peak_bp = 0.0;
    double max_dd_bp = 0.0;

    struct EquityPoint { int64_t exit_ms; double cum_bp; };
    std::vector<EquityPoint> equity_curve;
    equity_curve.reserve(trades.size());

    std::vector<double> trade_bps;
    trade_bps.reserve(trades.size());

    std::vector<chimera::SwingEngine::TradeLog> sorted_trades = trades;
    std::sort(sorted_trades.begin(), sorted_trades.end(),
              [](const chimera::SwingEngine::TradeLog& a,
                 const chimera::SwingEngine::TradeLog& b) {
                  return a.exit_ms < b.exit_ms;
              });

    for (const auto& t : sorted_trades) {
        const double pnl_bp = t.pnl_pct * 100.0;
        trade_bps.push_back(pnl_bp);

        update_group(overall, pnl_bp);
        const int s_idx = (int)t.strategy;
        if (s_idx >= 0 && s_idx <= 4) update_group(by_strategy[s_idx], pnl_bp);
        update_group(by_symbol[t.sym], pnl_bp);
        update_group(by_reason[t.why], pnl_bp);

        if (pnl_bp >= 0.0) {
            run_w++; run_l = 0;
            if (run_w > max_consec_wins) max_consec_wins = run_w;
        } else {
            run_l++; run_w = 0;
            if (run_l > max_consec_losses) max_consec_losses = run_l;
        }

        cum_bp += pnl_bp;
        if (cum_bp > peak_bp) peak_bp = cum_bp;
        const double dd = peak_bp - cum_bp;
        if (dd > max_dd_bp) max_dd_bp = dd;

        equity_curve.push_back({t.exit_ms, cum_bp});
    }

    double mean_bp = 0.0, stdev_bp = 0.0, sharpe = 0.0;
    if (!trade_bps.empty()) {
        for (double v : trade_bps) mean_bp += v;
        mean_bp /= trade_bps.size();
        double sse = 0.0;
        for (double v : trade_bps) { double d = v - mean_bp; sse += d * d; }
        if (trade_bps.size() > 1) {
            stdev_bp = std::sqrt(sse / (trade_bps.size() - 1));
            if (stdev_bp > 0.0)
                sharpe = (mean_bp / stdev_bp) * std::sqrt((double)trade_bps.size());
        }
    }

    {
        std::ofstream f(cfg.out_dir + "/trades.csv");
        f << "exit_ms,exit_iso,symbol,side,strategy,entry,exit,pnl_bp,mfe_bp,why\n";
        for (const auto& t : sorted_trades) {
            time_t s = (time_t)(t.exit_ms / 1000);
            struct tm tm_utc{};
            gmtime_r(&s, &tm_utc);
            char iso[32];
            std::strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
            const double mfe_bp = (t.entry > 0.0) ? (t.mfe / t.entry) * 10000.0 : 0.0;
            f << t.exit_ms << ',' << iso << ','
              << t.sym << ',' << t.side << ",S" << (int)t.strategy << ','
              << std::fixed << std::setprecision(8) << t.entry << ','
              << t.exit << ','
              << std::setprecision(2) << (t.pnl_pct * 100.0) << ','
              << mfe_bp << ',' << t.why << '\n';
        }
    }
    {
        std::ofstream f(cfg.out_dir + "/equity.csv");
        f << "exit_ms,cum_bp\n";
        for (const auto& p : equity_curve) {
            f << p.exit_ms << ',' << std::fixed << std::setprecision(2) << p.cum_bp << '\n';
        }
    }

    auto print_summary = [&](std::FILE* f) {
        std::fprintf(f, "================================================================\n");
        std::fprintf(f, "  Chimera SwingEngine Backtest\n");
        std::fprintf(f, "================================================================\n");

        time_t e_ms = (time_t)(earliest_ms / 1000);
        time_t l_ms = (time_t)(latest_ms   / 1000);
        struct tm tm_e{}, tm_l{};
        gmtime_r(&e_ms, &tm_e);
        gmtime_r(&l_ms, &tm_l);
        char iso_e[32], iso_l[32];
        std::strftime(iso_e, sizeof(iso_e), "%Y-%m-%dT%H:%M:%SZ", &tm_e);
        std::strftime(iso_l, sizeof(iso_l), "%Y-%m-%dT%H:%M:%SZ", &tm_l);
        std::fprintf(f, "Period:           %s -> %s\n", iso_e, iso_l);
        std::fprintf(f, "Symbols:          %zu  (", active_ids.size());
        for (size_t i = 0; i < active_ids.size(); ++i) {
            std::fprintf(f, "%s%s", chimera::sym_short(active_ids[i]),
                         i + 1 == active_ids.size() ? "" : ",");
        }
        std::fprintf(f, ")\n");
        std::fprintf(f, "H4 bars/symbol:   %d\n", cfg.h4_bars);
        std::fprintf(f, "Ticks replayed:   %zu\n", ticks.size());
        std::fprintf(f, "Replay wall time: %.2f s\n\n", elapsed_ms / 1000.0);

        std::fprintf(f, "Headline:\n");
        if (overall.n_trades == 0) {
            std::fprintf(f, "  NO TRADES FIRED. Try increasing --bars or check filter "
                            "thresholds in SwingEngine.hpp.\n\n");
        } else {
            const double wr = 100.0 * (double)overall.n_wins / (double)overall.n_trades;
            std::fprintf(f, "  Total trades:     %d\n", overall.n_trades);
            std::fprintf(f, "  Win rate:         %.1f%%\n", wr);
            std::fprintf(f, "  Total P&L:        %+.1f bp  (%.2f%%)\n",
                         overall.sum_bp, overall.sum_bp / 100.0);
            std::fprintf(f, "  Avg trade:        %+.1f bp\n", overall.sum_bp / overall.n_trades);
            std::fprintf(f, "  Avg win:          %+.1f bp\n",
                         overall.n_wins ? overall.gross_win_bp / overall.n_wins : 0.0);
            std::fprintf(f, "  Avg loss:         %+.1f bp\n",
                         (overall.n_trades - overall.n_wins) > 0
                            ? -(overall.gross_loss_bp / (overall.n_trades - overall.n_wins)) : 0.0);
            std::fprintf(f, "  Profit factor:    %.2f\n",
                         safe_div(overall.gross_win_bp, overall.gross_loss_bp));
            std::fprintf(f, "  Max drawdown:     %.1f bp from peak\n", max_dd_bp);
            std::fprintf(f, "  Max consec wins:  %d\n", max_consec_wins);
            std::fprintf(f, "  Max consec loss:  %d\n", max_consec_losses);
            std::fprintf(f, "  Sharpe-like:      %.2f\n", sharpe);
            std::fprintf(f, "\n");
        }

        std::fprintf(f, "By strategy:\n");
        const char* sn[5] = {"NONE", "S1_PULLBACK", "S2_DIVERGENCE", "S3_BREAKOUT", "S4_BRACKET"};
        for (int i = 1; i <= 4; ++i) print_group(sn[i], by_strategy[i], f);
        std::fprintf(f, "\n");

        std::fprintf(f, "By symbol:\n");
        for (const auto& kv : by_symbol) print_group(kv.first.c_str(), kv.second, f);
        std::fprintf(f, "\n");

        std::fprintf(f, "By close reason:\n");
        for (const auto& kv : by_reason) print_group(kv.first.c_str(), kv.second, f);
        std::fprintf(f, "\n");

        std::fprintf(f, "Caveats:\n");
        std::fprintf(f, "  * Tick order within each H4 bar is O->L->H->C — pessimistic\n");
        std::fprintf(f, "    for longs (potential SL touched before potential TP).\n");
        std::fprintf(f, "  * No slippage/fees applied. Subtract ~5-10 bp/trade for\n");
        std::fprintf(f, "    realistic taker execution on Binance spot.\n");
        std::fprintf(f, "================================================================\n");
    };

    print_summary(stdout);
    {
        std::FILE* f = std::fopen((cfg.out_dir + "/summary.txt").c_str(), "w");
        if (f) { print_summary(f); std::fclose(f); }
    }

    std::printf("\n[BACKTEST] outputs written to %s/\n", cfg.out_dir.c_str());
    std::printf("    %s/trades.csv\n",  cfg.out_dir.c_str());
    std::printf("    %s/equity.csv\n",  cfg.out_dir.c_str());
    std::printf("    %s/summary.txt\n", cfg.out_dir.c_str());

    return 0;
}
