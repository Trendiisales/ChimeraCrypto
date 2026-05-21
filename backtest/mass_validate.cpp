// ============================================================================
// mass_validate.cpp — Validate every disabled engine in disabled_engines.json
// against OOS data (≥ 2024-01-01). Reuses EdgeEngine flow from
// validate_engines.cpp.
//
// Build:
//   cd backtest && g++ -std=c++17 -O2 -I../include mass_validate.cpp \
//                       -o mass_validate -pthread
//
// Output:
//   mass_validate_results.csv
//
// Verdict:
//   PASS if (oos_trades >= 20 && oos_pf >= 1.3) else FAIL
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
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>

#include "core/EdgeEngine.hpp"

namespace fs = std::filesystem;

// ── Kline + JSON loaders (same as validate_engines.cpp) ─────────────────────
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
        try { val = std::stod(s); } catch (...) { return false; }
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
    if (!fs::exists(dir)) return all;
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

// ── CSV loader: data/klines_spot/{SYM}_1h_extended.csv ─────────────────────
static std::vector<Kline> load_klines_from_csv(const std::string& path) {
    std::vector<Kline> out;
    std::ifstream f(path);
    if (!f.is_open()) return out;
    std::string line;
    // skip header
    std::getline(f, line);
    out.reserve(50000);
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        // open_time_ms,open,high,low,close,volume,...
        Kline k{};
        const char* s = line.c_str();
        char* endp = nullptr;
        k.open_ts_ms = std::strtoll(s, &endp, 10);
        if (!endp || *endp != ',') continue;
        s = endp + 1;
        k.o = std::strtod(s, &endp); if (!endp || *endp != ',') continue; s = endp + 1;
        k.h = std::strtod(s, &endp); if (!endp || *endp != ',') continue; s = endp + 1;
        k.l = std::strtod(s, &endp); if (!endp || *endp != ',') continue; s = endp + 1;
        k.c = std::strtod(s, &endp);
        if (k.o > 0 && k.h > 0 && k.l > 0 && k.c > 0) {
            out.push_back(k);
        }
    }
    return out;
}

static std::vector<Kline> synthesize_tf(const std::vector<Kline>& h1, int64_t tf_secs) {
    std::vector<Kline> result;
    if (h1.empty()) return result;
    Kline cur{};
    int64_t cur_block = -1;
    for (auto& bar : h1) {
        int64_t block = (bar.open_ts_ms / 1000) / tf_secs;
        if (block != cur_block) {
            if (cur_block >= 0 && cur.o > 0.0) result.push_back(cur);
            cur_block = block;
            cur.open_ts_ms = block * tf_secs * 1000;
            cur.o = bar.o; cur.h = bar.h; cur.l = bar.l; cur.c = bar.c;
        } else {
            if (bar.h > cur.h) cur.h = bar.h;
            if (bar.l < cur.l) cur.l = bar.l;
            cur.c = bar.c;
        }
    }
    if (cur_block >= 0 && cur.o > 0.0) result.push_back(cur);
    return result;
}

// ── Tiny JSON parser (sufficient for our flat array of objects) ─────────────
struct JsonVal {
    enum Type { STR, NUM, BOOL_, NUL } type = NUL;
    std::string s;
    double n = 0;
    bool b = false;
};

static void skip_ws(const std::string& j, size_t& i) {
    while (i < j.size() && (j[i] == ' ' || j[i] == '\n' || j[i] == '\t' || j[i] == '\r')) ++i;
}

static std::string parse_str(const std::string& j, size_t& i) {
    // expects j[i]=='"'
    ++i;
    std::string out;
    while (i < j.size() && j[i] != '"') {
        if (j[i] == '\\' && i + 1 < j.size()) {
            ++i;
            out += j[i];
            ++i;
        } else {
            out += j[i++];
        }
    }
    if (i < j.size()) ++i; // closing "
    return out;
}

static JsonVal parse_value(const std::string& j, size_t& i) {
    skip_ws(j, i);
    JsonVal v;
    if (i >= j.size()) return v;
    if (j[i] == '"') {
        v.type = JsonVal::STR;
        v.s = parse_str(j, i);
    } else if (j[i] == 't' || j[i] == 'f') {
        v.type = JsonVal::BOOL_;
        if (j.compare(i, 4, "true") == 0) { v.b = true; i += 4; }
        else { v.b = false; i += 5; }
    } else if (j[i] == 'n') {
        v.type = JsonVal::NUL;
        i += 4;
    } else {
        // number
        size_t start = i;
        while (i < j.size() && (j[i] == '-' || j[i] == '+' || j[i] == '.' ||
                                j[i] == 'e' || j[i] == 'E' || (j[i] >= '0' && j[i] <= '9'))) ++i;
        v.type = JsonVal::NUM;
        try { v.n = std::stod(j.substr(start, i - start)); } catch (...) { v.n = 0; }
    }
    return v;
}

struct EngineCfg {
    std::string var;
    std::string tag;
    std::string symbol;
    std::string kind;
    int64_t tf_secs = 0;
    int     lookback = 20;
    int     hold_bars = 12;
    double  sl_atr_mult = 2.5;
    int     atr_period = 14;
    double  bb_k = 2.0;
    double  rsi_threshold = 30.0;
    double  round_trip_bp = 17.0;
    int     max_history = 64;
    double  trail_arm_atr = 1.0;
    double  trail_dist_atr = 0.5;
    double  trail_tighten_atr = 0.0;
    double  trail_tighten_dist_atr = 0.3;
};

static std::vector<EngineCfg> load_engine_cfgs(const std::string& path) {
    std::vector<EngineCfg> out;
    std::ifstream f(path);
    if (!f.is_open()) return out;
    std::string j((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t i = 0;
    skip_ws(j, i);
    if (i >= j.size() || j[i] != '[') return out;
    ++i;
    while (i < j.size()) {
        skip_ws(j, i);
        if (i < j.size() && j[i] == ']') { ++i; break; }
        if (i < j.size() && j[i] == ',') { ++i; continue; }
        if (i >= j.size() || j[i] != '{') { ++i; continue; }
        ++i; // {
        EngineCfg c{};
        while (i < j.size()) {
            skip_ws(j, i);
            if (i < j.size() && j[i] == '}') { ++i; break; }
            if (i < j.size() && j[i] == ',') { ++i; continue; }
            skip_ws(j, i);
            if (i >= j.size() || j[i] != '"') { ++i; continue; }
            std::string key = parse_str(j, i);
            skip_ws(j, i);
            if (i < j.size() && j[i] == ':') ++i;
            JsonVal v = parse_value(j, i);
            if      (key == "var")            c.var = v.s;
            else if (key == "tag")            c.tag = v.s;
            else if (key == "symbol")         c.symbol = v.s;
            else if (key == "kind")           c.kind = v.s;
            else if (key == "tf_secs")        c.tf_secs = (int64_t)v.n;
            else if (key == "lookback")       c.lookback = (int)v.n;
            else if (key == "hold_bars")      c.hold_bars = (int)v.n;
            else if (key == "sl_atr_mult")    c.sl_atr_mult = v.n;
            else if (key == "atr_period")     c.atr_period = (int)v.n;
            else if (key == "bb_k")           c.bb_k = v.n;
            else if (key == "rsi_threshold")  c.rsi_threshold = v.n;
            else if (key == "round_trip_bp")  c.round_trip_bp = v.n;
            else if (key == "max_history")    c.max_history = (int)v.n;
            else if (key == "trail_arm_atr")  c.trail_arm_atr = v.n;
            else if (key == "trail_dist_atr") c.trail_dist_atr = v.n;
            else if (key == "trail_tighten_atr")      c.trail_tighten_atr = v.n;
            else if (key == "trail_tighten_dist_atr") c.trail_tighten_dist_atr = v.n;
            // ignore unknown
        }
        out.push_back(c);
    }
    return out;
}

// ── Map string kind -> StrategyKind ────────────────────────────────────────
static bool parse_strategy_kind(const std::string& s, chimera::StrategyKind& out) {
    if (s == "TSMOM")          { out = chimera::StrategyKind::TSMOM; return true; }
    if (s == "DONCHIAN")       { out = chimera::StrategyKind::DONCHIAN; return true; }
    if (s == "BOLLINGER")      { out = chimera::StrategyKind::BOLLINGER; return true; }
    if (s == "RSI_REVERT")     { out = chimera::StrategyKind::RSI_REVERT; return true; }
    if (s == "OVERNIGHT")      { out = chimera::StrategyKind::OVERNIGHT; return true; }
    if (s == "WEEKDAY")        { out = chimera::StrategyKind::WEEKDAY; return true; }
    if (s == "KELTNER_REVERT") { out = chimera::StrategyKind::KELTNER_REVERT; return true; }
    if (s == "DUAL_THRUST")    { out = chimera::StrategyKind::DUAL_THRUST; return true; }
    if (s == "ICHIMOKU")       { out = chimera::StrategyKind::ICHIMOKU; return true; }
    if (s == "SUPERTREND")     { out = chimera::StrategyKind::SUPERTREND; return true; }
    if (s == "WILLIAMS_R")     { out = chimera::StrategyKind::WILLIAMS_R; return true; }
    if (s == "STOCH_RSI")      { out = chimera::StrategyKind::STOCH_RSI; return true; }
    return false;
}

// ── Per-engine result ───────────────────────────────────────────────────────
struct OosResult {
    int    trades = 0;
    int    wins   = 0;
    double total_bp = 0.0;
    double pf = 0.0;
    double sharpe = 0.0;
    double max_dd_bp = 0.0;
    bool   data_ok = true;
    std::string error;
};

// ── stdout suppression helpers ──────────────────────────────────────────────
static int g_saved_stdout = -1;
static FILE* g_devnull = nullptr;

static void mute_stdout() {
    if (g_saved_stdout != -1) return;
    fflush(stdout);
    g_saved_stdout = dup(fileno(stdout));
    g_devnull = fopen("/dev/null", "w");
    dup2(fileno(g_devnull), fileno(stdout));
}
static void unmute_stdout() {
    if (g_saved_stdout == -1) return;
    fflush(stdout);
    dup2(g_saved_stdout, fileno(stdout));
    close(g_saved_stdout);
    fclose(g_devnull);
    g_saved_stdout = -1;
    g_devnull = nullptr;
}

// ── Run OOS backtest for a single engine ────────────────────────────────────
static OosResult run_oos(const EngineCfg& ec,
                          const std::vector<Kline>& klines,
                          int64_t split_ts_ms) {
    OosResult r;

    // find split index
    int total = (int)klines.size();
    int split_idx = total;
    for (int i = 0; i < total; ++i) {
        if (klines[i].open_ts_ms >= split_ts_ms) { split_idx = i; break; }
    }
    if (split_idx >= total - 1 || split_idx < 10) {
        r.data_ok = false;
        r.error = "no_oos_window";
        return r;
    }

    chimera::StrategyKind kind;
    if (!parse_strategy_kind(ec.kind, kind)) {
        r.data_ok = false;
        r.error = "unknown_kind:" + ec.kind;
        return r;
    }

    chimera::EdgeEngine::Config cfg{
        .symbol         = ec.symbol,
        .tag            = ec.tag,
        .kind           = kind,
        .tf_secs        = ec.tf_secs,
        .lookback       = ec.lookback,
        .hold_bars      = ec.hold_bars,
        .sl_atr_mult    = ec.sl_atr_mult,
        .atr_period     = ec.atr_period,
        .bb_k           = ec.bb_k,
        .rsi_threshold  = ec.rsi_threshold,
        .round_trip_bp  = ec.round_trip_bp,
        .max_history    = std::max(ec.max_history,
                                   std::max(ec.lookback + 5,
                                            std::max(ec.atr_period + 5, 64))),
        .trail_arm_atr  = ec.trail_arm_atr,
        .trail_dist_atr = ec.trail_dist_atr,
        .trail_tighten_atr      = ec.trail_tighten_atr,
        .trail_tighten_dist_atr = ec.trail_tighten_dist_atr,
    };

    mute_stdout();
    chimera::EdgeEngine engine(cfg);

    // Seeds: everything before split_idx
    std::vector<chimera::EdgeEngine::SeedBar> seeds;
    seeds.reserve(split_idx);
    for (int i = 0; i < split_idx; ++i) {
        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = klines[i].open_ts_ms;
        sb.o = klines[i].o; sb.h = klines[i].h;
        sb.l = klines[i].l; sb.c = klines[i].c;
        seeds.push_back(sb);
    }
    engine.seed_bars(seeds);

    std::vector<double> trade_returns;
    double equity_bp = 0.0, peak_bp = 0.0, max_dd = 0.0;
    int prev_trades = 0;
    double prev_total_bp = 0.0;

    int64_t tick_step = (ec.tf_secs * 1000) / 4;
    for (int i = split_idx; i < total; ++i) {
        const Kline& k = klines[i];
        int64_t bar_start_ms = k.open_ts_ms;
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
            engine.on_tick(k.c, bar_start_ms + ec.tf_secs * 1000 + 1000);
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
    unmute_stdout();

    r.trades   = engine.trades();
    r.wins     = engine.wins();
    r.total_bp = engine.total_bp();
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
        double oos_days = (double)(total - split_idx) * ec.tf_secs / 86400.0;
        double trades_per_year = (oos_days > 0) ? r.trades / oos_days * 365.0 : 0.0;
        r.sharpe = (sd > 0.0) ? (mean / sd) * std::sqrt(trades_per_year) : 0.0;
    }
    return r;
}

// ── Symbol → klines cache (load H1 once per symbol) ─────────────────────────
static std::map<std::string, std::vector<Kline>> g_h1_cache;
static std::map<std::string, std::string>        g_load_errors;

static const std::vector<Kline>& get_h1(const std::string& symbol) {
    auto it = g_h1_cache.find(symbol);
    if (it != g_h1_cache.end()) return it->second;

    std::vector<Kline> klines;

    // Try CSV first.
    std::string upper = symbol;
    for (auto& ch : upper) ch = std::toupper((unsigned char)ch);
    std::string csv_path = "../data/klines_spot/" + upper + "_1h_extended.csv";
    if (!fs::exists(csv_path)) csv_path = "data/klines_spot/" + upper + "_1h_extended.csv";
    if (fs::exists(csv_path)) {
        klines = load_klines_from_csv(csv_path);
    }

    // Fallback to JSON parts.
    if (klines.empty()) {
        std::string prefix1 = symbol + "_h1_part";
        std::string prefix2;
        if (symbol == "btcusdt") prefix2 = "btc_h1_part";
        std::string data_dir = "data";
        if (!fs::exists(data_dir)) data_dir = "backtest/data";
        if (fs::exists(data_dir)) {
            klines = load_all_parts(data_dir, prefix1);
            if (klines.empty() && !prefix2.empty()) {
                klines = load_all_parts(data_dir, prefix2);
            }
        }
    }

    if (klines.empty()) {
        g_load_errors[symbol] = "no_data_found";
    }
    g_h1_cache[symbol] = std::move(klines);
    return g_h1_cache[symbol];
}

int main(int argc, char** argv) {
    std::string cfg_path = "disabled_engines.json";
    std::string out_csv  = "mass_validate_results.csv";
    if (argc >= 2) cfg_path = argv[1];
    if (argc >= 3) out_csv  = argv[2];

    auto engines = load_engine_cfgs(cfg_path);
    std::fprintf(stderr, "Loaded %zu engine configs from %s\n",
                 engines.size(), cfg_path.c_str());

    // Split point: 2024-01-01 UTC = 1704067200000 ms
    const int64_t SPLIT_MS = 1704067200000LL;

    std::ofstream out(out_csv);
    out << "tag,symbol,strategy,tf_secs,oos_trades,oos_wins,oos_pf,oos_sharpe,"
           "oos_total_bp,oos_max_dd_bp,verdict,note\n";

    int pass_count = 0;
    int fail_count = 0;
    int skip_count = 0;
    int n = (int)engines.size();

    for (int i = 0; i < n; ++i) {
        const auto& ec = engines[i];

        std::fprintf(stderr, "[%3d/%d] %-22s %s/%s tf=%lld...",
                     i + 1, n, ec.tag.c_str(), ec.symbol.c_str(),
                     ec.kind.c_str(), (long long)ec.tf_secs);
        std::fflush(stderr);

        const auto& h1 = get_h1(ec.symbol);
        if (h1.empty()) {
            std::fprintf(stderr, " SKIP (no data)\n");
            out << ec.tag << ',' << ec.symbol << ',' << ec.kind << ','
                << ec.tf_secs
                << ",0,0,0,0,0,0,SKIP,no_h1_data\n";
            ++skip_count;
            continue;
        }

        std::vector<Kline> klines;
        if (ec.tf_secs == 3600) klines = h1;
        else klines = synthesize_tf(h1, ec.tf_secs);

        if ((int)klines.size() < 50) {
            std::fprintf(stderr, " SKIP (only %zu bars after tf synth)\n", klines.size());
            out << ec.tag << ',' << ec.symbol << ',' << ec.kind << ','
                << ec.tf_secs
                << ",0,0,0,0,0,0,SKIP,too_few_bars\n";
            ++skip_count;
            continue;
        }

        OosResult r = run_oos(ec, klines, SPLIT_MS);
        if (!r.data_ok) {
            std::fprintf(stderr, " SKIP (%s)\n", r.error.c_str());
            out << ec.tag << ',' << ec.symbol << ',' << ec.kind << ','
                << ec.tf_secs
                << ",0,0,0,0,0,0,SKIP," << r.error << '\n';
            ++skip_count;
            continue;
        }

        bool pass = (r.trades >= 20 && r.pf >= 1.3);
        const char* verdict = pass ? "PASS" : "FAIL";
        if (pass) ++pass_count; else ++fail_count;

        std::fprintf(stderr, " trades=%d pf=%.2f sharpe=%.2f bp=%.0f %s\n",
                     r.trades, r.pf, r.sharpe, r.total_bp, verdict);

        out << ec.tag << ',' << ec.symbol << ',' << ec.kind << ','
            << ec.tf_secs << ','
            << r.trades << ',' << r.wins << ','
            << r.pf << ',' << r.sharpe << ','
            << r.total_bp << ',' << r.max_dd_bp << ','
            << verdict << ",\n";
    }

    out.close();

    std::fprintf(stderr,
        "\n=== DONE ===\n"
        "  total : %d\n"
        "  PASS  : %d\n"
        "  FAIL  : %d\n"
        "  SKIP  : %d\n"
        "  output: %s\n",
        n, pass_count, fail_count, skip_count, out_csv.c_str());

    if (!g_load_errors.empty()) {
        std::fprintf(stderr, "Symbols with no data:\n");
        for (auto& kv : g_load_errors) {
            std::fprintf(stderr, "  %s: %s\n", kv.first.c_str(), kv.second.c_str());
        }
    }
    return 0;
}
