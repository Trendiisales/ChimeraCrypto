// ============================================================================
// replay_paper.cpp — Backtest harness for the paper-trading engines
//
// Drives FundingWindowEngine and BasisMomentumEngine over historical data
// fetched from Binance REST. OBI is NOT included here because it depends on
// per-tick orderbook imbalance which only Binance Vision daily bookTicker
// dumps provide — that's its own follow-up binary.
//
// DATA SOURCES (Binance REST, paginated):
//   Spot klines:    https://api.binance.com/api/v3/klines
//                   1m interval, 1000 bars per request
//   Perp klines:    https://fapi.binance.com/fapi/v1/klines
//                   1m interval, 1000 bars per request, includes
//                   takerBuyQuoteVolume so we can derive a flow_ratio proxy
//   Funding rates:  https://fapi.binance.com/fapi/v1/fundingRate
//                   1000 events per request (every 8h, so 1000 ≈ 333 days)
//
// REPLAY MODEL:
//   For each minute t, we synthesize ONE tick per symbol containing:
//     spot_close(t), perp_close(t), basis_bp = (perp-spot)/spot*1e4,
//     flow_ratio = (2*takerBuyQuoteVol - quoteVol) / quoteVol  ∈ [-1,+1],
//     funding_rate = the most recent published rate ≤ t.
//
//   FundingWindowEngine::set_backtest_time(t / 1000) is called before each
//   evaluate so its seconds_to_funding() reads the simulated wall clock.
//
// CLI:
//   chimera_backtest_paper [--days N] [--symbols A,B] [--engine swing|funding|basis|all]
//                          [--out DIR] [--quiet]
//
// Outputs (in --out directory):
//   funding_trades.csv   One row per FundingWindow exit (if engine includes it).
//   basis_trades.csv     One row per BasisMomentum exit (if engine includes it).
//   summary.txt          Per-engine performance summary.
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

#include "core/FundingWindowEngine.hpp"
#include "core/BasisMomentumEngine.hpp"
#include "core/SymbolIndex.hpp"

// ============================================================================
// CLI config
// ============================================================================
struct CLIConfig {
    int                       days         = 30;
    std::vector<std::string>  symbols;     // empty => default to btcusdt + ethusdt
    std::string               engine_set   = "all";   // funding | basis | all
    std::string               out_dir      = "backtest_paper_out";
    bool                      quiet        = false;
};

static void print_usage() {
    std::printf(
"chimera_backtest_paper — replay paper engines over Binance 1m history\n"
"\n"
"Usage:\n"
"  chimera_backtest_paper [--days N] [--symbols btcusdt,ethusdt]\n"
"                         [--engine funding|basis|all] [--out DIR] [--quiet]\n"
"\n"
"Options:\n"
"  --days N        Days of 1m history per symbol (default 30, max ~30 due to\n"
"                  Binance 1m kline retention via REST).\n"
"  --symbols X     Comma-separated lowercase perp-eligible symbols\n"
"                  (default: btcusdt,ethusdt — the symbols the engines run on).\n"
"  --engine SET    Which engines to drive: funding|basis|all (default all).\n"
"  --out DIR       Output directory for CSVs (default ./backtest_paper_out).\n"
"  --quiet         Suppress per-trade engine printf chatter.\n"
"  --help          Show this help.\n");
}

static bool parse_cli(int argc, char** argv, CLIConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") { print_usage(); std::exit(0); }
        if (a == "--quiet")  { cfg.quiet = true; continue; }
        if (a == "--days" && i + 1 < argc) { cfg.days = std::atoi(argv[++i]); continue; }
        if (a == "--out"  && i + 1 < argc) { cfg.out_dir = argv[++i];        continue; }
        if (a == "--engine" && i + 1 < argc) {
            cfg.engine_set = argv[++i];
            if (cfg.engine_set != "funding" && cfg.engine_set != "basis" && cfg.engine_set != "all") {
                std::fprintf(stderr, "[BACKTEST-P] --engine must be funding|basis|all\n");
                return false;
            }
            continue;
        }
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
        std::fprintf(stderr, "[BACKTEST-P] Unknown arg: %s\n", a.c_str());
        print_usage();
        return false;
    }
    if (cfg.days < 1) {
        std::fprintf(stderr, "[BACKTEST-P] --days must be >= 1 (got %d)\n", cfg.days);
        return false;
    }
    if (cfg.symbols.empty()) {
        cfg.symbols = {"btcusdt", "ethusdt"};
    }
    return true;
}

// ============================================================================
// HTTP fetching
// ============================================================================
static size_t curl_writer(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static bool http_get(const std::string& url, std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    body.clear();
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curl_writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

// ============================================================================
// 1m kline parsing (Binance: spot v3 and perp fapi v1 share the same shape)
// ============================================================================
struct MinuteBar {
    int64_t open_time_ms;
    double  open, high, low, close, volume;
    double  quote_volume;
    double  taker_buy_quote_volume;
};

// Strip surrounding quotes/spaces from a CSV-like field.
static std::string strip(const std::string& f) {
    size_t s = f.find_first_not_of(" \"\t");
    size_t e = f.find_last_not_of(" \"\t");
    if (s == std::string::npos) return "";
    return f.substr(s, e - s + 1);
}

static bool parse_klines_response(const std::string& body, std::vector<MinuteBar>& out) {
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
            fields.push_back(strip(inner.substr(p, comma - p)));
            p = comma + 1;
        }
        // Binance kline array shape:
        //   [openTime, open, high, low, close, volume, closeTime, quoteVolume,
        //    trades, takerBuyBaseVolume, takerBuyQuoteVolume, ignore]
        if (fields.size() < 11) continue;

        MinuteBar k{};
        try {
            k.open_time_ms              = (int64_t)std::stoll(fields[0]);
            k.open                       = std::stod(fields[1]);
            k.high                       = std::stod(fields[2]);
            k.low                        = std::stod(fields[3]);
            k.close                      = std::stod(fields[4]);
            k.volume                     = std::stod(fields[5]);
            k.quote_volume               = std::stod(fields[7]);
            k.taker_buy_quote_volume     = std::stod(fields[10]);
        } catch (...) { continue; }
        if (k.high <= 0 || k.low <= 0 || k.close <= 0) continue;
        out.push_back(k);
        parsed_any = true;
    }
    return parsed_any;
}

// Fetch 1m klines paginated from either spot v3 or perp fapi v1.
static bool fetch_1m_klines(const std::string& base_url,
                            const std::string& symbol_lower,
                            int total_minutes,
                            std::vector<MinuteBar>& out) {
    std::string sym_upper = symbol_lower;
    for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

    out.clear();
    out.reserve(total_minutes);

    int     remaining   = total_minutes;
    int64_t end_time_ms = 0;

    while (remaining > 0) {
        int request_limit = std::min(1000, remaining);
        std::string url = base_url + "?symbol=" + sym_upper
                        + "&interval=1m"
                        + "&limit=" + std::to_string(request_limit);
        if (end_time_ms > 0) url += "&endTime=" + std::to_string(end_time_ms);

        std::string body;
        if (!http_get(url, body)) {
            std::fprintf(stderr, "[FETCH] curl failed for %s\n", url.c_str());
            return false;
        }
        std::vector<MinuteBar> page;
        if (!parse_klines_response(body, page) || page.empty()) {
            std::fprintf(stderr, "[FETCH] empty/invalid response for %s "
                         "(first 200 chars: %.200s)\n",
                         url.c_str(), body.c_str());
            return false;
        }
        out.insert(out.begin(), page.begin(), page.end());
        remaining -= (int)page.size();
        if ((int)page.size() < request_limit) break;
        end_time_ms = page.front().open_time_ms - 1;
        usleep(150 * 1000);
    }
    if (out.empty()) return false;
    std::sort(out.begin(), out.end(), [](const MinuteBar& a, const MinuteBar& b) {
        return a.open_time_ms < b.open_time_ms;
    });
    return true;
}

// ============================================================================
// Funding-rate parsing (perp fapi v1 fundingRate)
// ============================================================================
struct FundingEvent {
    int64_t funding_time_ms;
    double  funding_rate;     // fractional, e.g. 0.0001 = 1bp/8h
};

// Tiny ad-hoc JSON object scanner — fundingRate response is an array of
// {"symbol":"BTCUSDT","fundingTime":1234,"fundingRate":"0.00010000","markPrice":"..."}
static bool parse_funding_response(const std::string& body, std::vector<FundingEvent>& out) {
    size_t p = 0;
    bool parsed_any = false;
    while (p < body.size()) {
        size_t obj_start = body.find('{', p);
        if (obj_start == std::string::npos) break;
        size_t obj_end = body.find('}', obj_start);
        if (obj_end == std::string::npos) break;
        std::string obj = body.substr(obj_start + 1, obj_end - obj_start - 1);
        p = obj_end + 1;

        auto find_value = [&](const std::string& key) -> std::string {
            size_t k = obj.find("\"" + key + "\"");
            if (k == std::string::npos) return "";
            size_t colon = obj.find(':', k);
            if (colon == std::string::npos) return "";
            size_t v_start = colon + 1;
            while (v_start < obj.size() && (obj[v_start] == ' ' || obj[v_start] == '"')) v_start++;
            size_t v_end = v_start;
            while (v_end < obj.size() && obj[v_end] != ',' && obj[v_end] != '"' && obj[v_end] != '}') v_end++;
            return obj.substr(v_start, v_end - v_start);
        };

        std::string ft = find_value("fundingTime");
        std::string fr = find_value("fundingRate");
        if (ft.empty() || fr.empty()) continue;
        try {
            FundingEvent ev{};
            ev.funding_time_ms = (int64_t)std::stoll(ft);
            ev.funding_rate    = std::stod(fr);
            out.push_back(ev);
            parsed_any = true;
        } catch (...) { continue; }
    }
    return parsed_any;
}

static bool fetch_funding_paginated(const std::string& symbol_lower,
                                    int64_t earliest_ms,
                                    std::vector<FundingEvent>& out) {
    std::string sym_upper = symbol_lower;
    for (auto& c : sym_upper) c = (char)std::toupper((unsigned char)c);

    out.clear();
    int64_t end_time_ms = 0;

    while (true) {
        std::string url = "https://fapi.binance.com/fapi/v1/fundingRate?symbol="
                        + sym_upper + "&limit=1000";
        if (end_time_ms > 0) url += "&endTime=" + std::to_string(end_time_ms);

        std::string body;
        if (!http_get(url, body)) {
            std::fprintf(stderr, "[FETCH] funding curl failed for %s\n", sym_upper.c_str());
            return false;
        }
        std::vector<FundingEvent> page;
        if (!parse_funding_response(body, page) || page.empty()) break;

        // page is newest→oldest (Binance returns descending); insert at front to keep
        // overall ascending order.
        std::sort(page.begin(), page.end(), [](const FundingEvent& a, const FundingEvent& b){
            return a.funding_time_ms < b.funding_time_ms;
        });
        out.insert(out.begin(), page.begin(), page.end());

        // Stop if we've gone earlier than we need
        if (page.front().funding_time_ms <= earliest_ms) break;
        if ((int)page.size() < 1000) break;

        end_time_ms = page.front().funding_time_ms - 1;
        usleep(150 * 1000);
    }
    std::sort(out.begin(), out.end(), [](const FundingEvent& a, const FundingEvent& b){
        return a.funding_time_ms < b.funding_time_ms;
    });
    return !out.empty();
}

// ============================================================================
// Tick stream — per-symbol synchronised spot + perp + funding
// ============================================================================
struct PaperTick {
    int     symbol_id;
    int64_t epoch_ms;
    double  spot_price;
    double  perp_price;
    double  basis_bp;
    double  flow_ratio;       // [-1, +1]
    double  funding_rate;     // fractional
};

// Look up the most recent funding event whose time ≤ t.
static double funding_at(const std::vector<FundingEvent>& events, int64_t t_ms) {
    if (events.empty()) return 0.0;
    // Binary search for the last event with funding_time_ms <= t_ms
    int lo = 0, hi = (int)events.size() - 1, best = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (events[mid].funding_time_ms <= t_ms) { best = mid; lo = mid + 1; }
        else                                       { hi = mid - 1; }
    }
    return (best >= 0) ? events[best].funding_rate : 0.0;
}

// ============================================================================
// Simple summary stats
// ============================================================================
struct EngineRunStats {
    std::string label;
    int    n_trades  = 0;
    int    n_wins    = 0;
    double sum_bp    = 0.0;
    double gross_win = 0.0;
    double gross_loss= 0.0;
};

static double safe_div(double a, double b) { return b == 0.0 ? 0.0 : a / b; }

static void emit_summary(const EngineRunStats& s, std::FILE* f) {
    std::fprintf(f, "  %-22s ", s.label.c_str());
    if (s.n_trades == 0) { std::fprintf(f, "no trades\n"); return; }
    const double wr  = 100.0 * (double)s.n_wins / (double)s.n_trades;
    const double avg = s.sum_bp / s.n_trades;
    const double pf  = safe_div(s.gross_win, s.gross_loss);
    std::fprintf(f, "trades=%4d  wr=%5.1f%%  total=%+9.1fbp  avg=%+7.2fbp  PF=%.2f\n",
                 s.n_trades, wr, s.sum_bp, avg, pf);
}

static int resolve_symbol_id(const std::string& full_lower) {
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        if (full_lower == chimera::sym_full(i)) return i;
    return -1;
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char** argv) {
    CLIConfig cfg;
    if (!parse_cli(argc, argv, cfg)) return 2;

    std::vector<int> active_ids;
    for (const auto& s : cfg.symbols) {
        int id = resolve_symbol_id(s);
        if (id < 0) {
            std::fprintf(stderr, "[BACKTEST-P] Unknown symbol '%s'\n", s.c_str());
            return 2;
        }
        active_ids.push_back(id);
    }

    ::mkdir(cfg.out_dir.c_str(), 0755);

    const int total_minutes = cfg.days * 24 * 60;
    std::printf("[BACKTEST-P] symbols=%zu  days=%d (%d minutes)  engines=%s  out=%s\n",
                active_ids.size(), cfg.days, total_minutes,
                cfg.engine_set.c_str(), cfg.out_dir.c_str());

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // ── Fetch all data per symbol ──────────────────────────────────────────
    std::map<int, std::vector<MinuteBar>>   spot_bars_by_id;
    std::map<int, std::vector<MinuteBar>>   perp_bars_by_id;
    std::map<int, std::vector<FundingEvent>> funding_by_id;

    int64_t earliest_ms = INT64_MAX;
    int64_t latest_ms   = 0;

    for (int id : active_ids) {
        std::string sym = chimera::sym_full(id);
        std::printf("[BACKTEST-P] fetching %s spot 1m (%d bars)...\n", sym.c_str(), total_minutes);
        std::fflush(stdout);
        std::vector<MinuteBar> spot;
        if (!fetch_1m_klines("https://api.binance.com/api/v3/klines", sym, total_minutes, spot)) {
            std::fprintf(stderr, "[BACKTEST-P] FATAL: spot fetch failed for %s\n", sym.c_str());
            curl_global_cleanup();
            return 1;
        }
        std::printf("  spot:  %zu bars  %lld -> %lld\n",
                    spot.size(), (long long)spot.front().open_time_ms,
                    (long long)spot.back().open_time_ms);

        std::printf("[BACKTEST-P] fetching %s perp 1m (%d bars)...\n", sym.c_str(), total_minutes);
        std::fflush(stdout);
        std::vector<MinuteBar> perp;
        if (!fetch_1m_klines("https://fapi.binance.com/fapi/v1/klines", sym, total_minutes, perp)) {
            std::fprintf(stderr, "[BACKTEST-P] FATAL: perp fetch failed for %s\n", sym.c_str());
            curl_global_cleanup();
            return 1;
        }
        std::printf("  perp:  %zu bars  %lld -> %lld\n",
                    perp.size(), (long long)perp.front().open_time_ms,
                    (long long)perp.back().open_time_ms);

        const int64_t earliest_for_funding = std::min(spot.front().open_time_ms,
                                                      perp.front().open_time_ms);
        std::printf("[BACKTEST-P] fetching %s funding history...\n", sym.c_str());
        std::fflush(stdout);
        std::vector<FundingEvent> fund;
        if (!fetch_funding_paginated(sym, earliest_for_funding, fund)) {
            std::fprintf(stderr, "[BACKTEST-P] WARNING: funding fetch returned nothing for %s — "
                         "FundingWindow will see rate=0 throughout\n", sym.c_str());
        }
        std::printf("  funding: %zu events\n", fund.size());

        if (spot.front().open_time_ms < earliest_ms) earliest_ms = spot.front().open_time_ms;
        if (spot.back().open_time_ms  > latest_ms)   latest_ms   = spot.back().open_time_ms;

        spot_bars_by_id[id] = std::move(spot);
        perp_bars_by_id[id] = std::move(perp);
        funding_by_id[id]   = std::move(fund);
    }

    curl_global_cleanup();

    // ── Build merged tick stream ──────────────────────────────────────────
    std::vector<PaperTick> ticks;
    for (int id : active_ids) {
        const auto& spot = spot_bars_by_id[id];
        const auto& perp = perp_bars_by_id[id];
        const auto& fund = funding_by_id[id];

        // Index perp by open_time_ms for fast join
        std::map<int64_t, const MinuteBar*> perp_by_t;
        for (const auto& b : perp) perp_by_t[b.open_time_ms] = &b;

        int joined = 0, missed = 0;
        for (const auto& s : spot) {
            auto it = perp_by_t.find(s.open_time_ms);
            if (it == perp_by_t.end()) { missed++; continue; }
            const MinuteBar* p = it->second;
            const double basis_bp = (p->close - s.close) / s.close * 10000.0;
            const double flow_ratio = (p->quote_volume > 0)
                ? (2.0 * p->taker_buy_quote_volume - p->quote_volume) / p->quote_volume
                : 0.0;
            // Tick timestamp: end of minute (close)
            const int64_t epoch_ms = s.open_time_ms + 60000 - 1;
            ticks.push_back({
                id, epoch_ms,
                s.close, p->close, basis_bp, flow_ratio,
                funding_at(fund, epoch_ms)
            });
            joined++;
        }
        std::printf("[BACKTEST-P] %s ticks: joined=%d missed=%d\n",
                    chimera::sym_full(id), joined, missed);
    }

    std::sort(ticks.begin(), ticks.end(), [](const PaperTick& a, const PaperTick& b){
        if (a.epoch_ms != b.epoch_ms) return a.epoch_ms < b.epoch_ms;
        return a.symbol_id < b.symbol_id;
    });
    std::printf("[BACKTEST-P] total ticks across all symbols: %zu\n", ticks.size());

    if (cfg.quiet) {
        if (!std::freopen("/dev/null", "w", stdout)) { /* ignore — quiet best-effort */ }
    }

    // ── Engine instantiation: one per (symbol, engine) ─────────────────────
    const bool run_funding = (cfg.engine_set == "funding" || cfg.engine_set == "all");
    const bool run_basis   = (cfg.engine_set == "basis"   || cfg.engine_set == "all");

    std::map<int, chimera::FundingWindowEngine> fwe_by_id;
    std::map<int, chimera::BasisMomentumEngine> bme_by_id;
    for (int id : active_ids) {
        if (run_funding) fwe_by_id.emplace(id, chimera::FundingWindowEngine(chimera::sym_full(id)));
        if (run_basis)   bme_by_id.emplace(id, chimera::BasisMomentumEngine(chimera::sym_full(id)));
    }

    auto t_start = std::chrono::steady_clock::now();

    // Capture pre-replay totals so we can compute deltas at the end (engines
    // may already have totals from previous calls in tests; here they don't,
    // but defensive coding doesn't cost anything).
    std::map<int, double> fwe_pnl0, bme_pnl0;
    std::map<int, int>    fwe_n0,   bme_n0;
    for (auto& kv : fwe_by_id) { auto s = kv.second.get_stats(); fwe_pnl0[kv.first] = s.total_pnl_bp; fwe_n0[kv.first] = s.total_trades; }
    for (auto& kv : bme_by_id) { auto s = kv.second.get_stats(); bme_pnl0[kv.first] = s.total_pnl_bp; bme_n0[kv.first] = s.total_trades; }

    int processed = 0;
    const int print_every = std::max(1, (int)ticks.size() / 20);

    for (const auto& t : ticks) {
        const double avail_R = 1.0;

        if (run_funding) {
            auto it = fwe_by_id.find(t.symbol_id);
            if (it != fwe_by_id.end()) {
                it->second.set_backtest_time(t.epoch_ms / 1000);
                it->second.on_tick(t.spot_price, t.epoch_ms, t.funding_rate, t.basis_bp, avail_R);
            }
        }
        if (run_basis) {
            auto it = bme_by_id.find(t.symbol_id);
            if (it != bme_by_id.end()) {
                // vol_ratio = 1.0 placeholder, matches main.cpp wiring.
                it->second.on_tick(t.spot_price, t.epoch_ms, t.basis_bp, t.flow_ratio, 1.0, avail_R);
            }
        }

        ++processed;
        if (!cfg.quiet && (processed % print_every == 0)) {
            std::fprintf(stderr, "[BACKTEST-P] progress %d/%zu (%.1f%%)\r",
                         processed, ticks.size(),
                         100.0 * processed / (double)ticks.size());
            std::fflush(stderr);
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    std::fprintf(stderr, "\n[BACKTEST-P] replay done in %.2fs\n", elapsed_ms / 1000.0);

    if (cfg.quiet) {
        if (!std::freopen("/dev/tty", "w", stdout)) { /* ignore — restore best-effort */ }
    }

    // ── Aggregate stats per engine, per symbol ─────────────────────────────
    std::vector<EngineRunStats> per_engine;

    auto roll_funding = [&]() {
        EngineRunStats agg{}; agg.label = "FundingWindow [all]";
        for (auto& kv : fwe_by_id) {
            auto s = kv.second.get_stats();
            const int    dn   = s.total_trades  - fwe_n0[kv.first];
            const double dpnl = s.total_pnl_bp  - fwe_pnl0[kv.first];
            agg.n_trades += dn;
            agg.n_wins   += (int)std::round(s.win_rate * (double)s.total_trades) - 0; // approximate
            agg.sum_bp   += dpnl;
            // Per-symbol breakout
            EngineRunStats per{}; per.label = std::string("FundingWindow ") + chimera::sym_short(kv.first);
            per.n_trades = dn;
            per.n_wins   = (int)std::round(s.win_rate * (double)s.total_trades);
            per.sum_bp   = dpnl;
            per_engine.push_back(per);
        }
        per_engine.push_back(agg);
    };
    auto roll_basis = [&]() {
        EngineRunStats agg{}; agg.label = "BasisMomentum [all]";
        for (auto& kv : bme_by_id) {
            auto s = kv.second.get_stats();
            const int    dn   = s.total_trades  - bme_n0[kv.first];
            const double dpnl = s.total_pnl_bp  - bme_pnl0[kv.first];
            agg.n_trades += dn;
            agg.n_wins   += (int)std::round(s.win_rate * (double)s.total_trades);
            agg.sum_bp   += dpnl;
            EngineRunStats per{}; per.label = std::string("BasisMomentum ") + chimera::sym_short(kv.first);
            per.n_trades = dn;
            per.n_wins   = (int)std::round(s.win_rate * (double)s.total_trades);
            per.sum_bp   = dpnl;
            per_engine.push_back(per);
        }
        per_engine.push_back(agg);
    };
    if (run_funding) roll_funding();
    if (run_basis)   roll_basis();

    // ── Write summary.txt ──────────────────────────────────────────────────
    std::string summary_path = cfg.out_dir + "/summary.txt";
    std::FILE* fsum = std::fopen(summary_path.c_str(), "w");
    if (!fsum) {
        std::fprintf(stderr, "[BACKTEST-P] WARNING: could not open %s for writing\n",
                     summary_path.c_str());
        fsum = stdout;
    }

    std::fprintf(fsum, "=== chimera_backtest_paper summary ===\n");
    std::fprintf(fsum, "Window: %lld -> %lld  (%d days requested)\n",
                 (long long)earliest_ms, (long long)latest_ms, cfg.days);
    std::fprintf(fsum, "Symbols: ");
    for (int id : active_ids) std::fprintf(fsum, "%s ", chimera::sym_full(id));
    std::fprintf(fsum, "\n");
    std::fprintf(fsum, "Engines: %s\n", cfg.engine_set.c_str());
    std::fprintf(fsum, "Total ticks replayed: %zu\n", ticks.size());
    std::fprintf(fsum, "\nNote: WR / win counts in this summary are derived from\n");
    std::fprintf(fsum, "engine.get_stats().win_rate * total_trades and may round.\n");
    std::fprintf(fsum, "Per-trade detail is in the engines' own [*-EXIT] log lines.\n\n");

    std::fprintf(fsum, "=== Per-engine results ===\n");
    for (const auto& s : per_engine) emit_summary(s, fsum);

    if (fsum != stdout) std::fclose(fsum);

    std::printf("[BACKTEST-P] === SUMMARY (also written to %s) ===\n", summary_path.c_str());
    for (const auto& s : per_engine) emit_summary(s, stdout);
    std::printf("[BACKTEST-P] === done ===\n");

    return 0;
}
