// ============================================================================
// Chimera — Tier-2 long-only longer-timeframe edges (rewrite 2026-05-16)
//
// After full OOS backtest + multi-symbol parameter sweep (2026-05-16/17):
//   - 5 D1 engines (Sessions 13-14)
//   - 3 H12 engines (Session 14)
//   - 8 H6 engines (Session 15 — strongest timeframe discovered)
//   - 7 H4 engines (Session 14)
//   - 7 H2 engines (Session 17+20 — new timeframe discovery)
//   - 7 H3 engines (Session 17+20 — new timeframe, no native Binance candles)
//   - 3 H1 engines (Session 15 — XRP/SOL/LINK only, rest eaten by costs)
//   - 12 counter-trend engines (Session 19 — RSI_REVERT + BOLLINGER dip-buy)
//   - 9 new-symbol TSMOM engines (Session 20 — NEAR/SUI/APT/ARB)
//   - 31 counter-trend on new symbols (Session 21a — RSI_REVERT + BOLLINGER on NEAR/SUI/APT/ARB)
//   - 100 exotic TF + extended sweep (Session 21b — H8/H16/D2/D3 TSMOM + H6/H8/H12 CT + DONCHIAN)
//   - 39 new + 18 re-optimized engines (Session 22 — RSI/BOLL H8/H16 + DONCHIAN H8/H16/D2/D3)
//   - Removed: SOL-RSI-H6 (overfit exposed by extended data)
//   - 15 new engines (Session 24 — DONCHIAN H2/H3/H4/H6/H12 gap-fill + TSMOM H12 fill)
//   - Total: 245 engines, all shadow mode, all long-only spot
//
// SESSION 15 DISCOVERY:
//   H6 timeframe was never tested before Session 15.
//   Optimizer found H6 to be the strongest timeframe overall:
//     - XRP-H6: 98.9% of combos profitable, PF=2.68
//     - BTC-H6: 73.3%, PF=2.59
//     - All 8 symbols pass deploy criteria on H6
//   H1 yielded 3 deploy-grade engines (XRP PF=1.66, SOL PF=1.40, LINK PF=1.32)
//
//   instance              symbol     strat   tf    OOS PF  OOS Shrp  Nbr%  Trades
//   ──────────────────────────────────────────────────────────────────────────────
//   D1 ENGINES (5):
//   btc_tsmom_d1          BTCUSDT    TSMOM   D1    1.92    1.67      85%     24
//   eth_tsmom_d1          ETHUSDT    TSMOM   D1    3.15    3.17      91%     26
//   sol_tsmom_d1          SOLUSDT    TSMOM   D1    2.25    2.41      89%     15
//   link_tsmom_d1         LINKUSDT   TSMOM   D1    2.18    1.92     100%     23
//   bnb_tsmom_d1          BNBUSDT    TSMOM   D1    3.16    2.91      90%     32
//
//   H12 ENGINES (3):
//   btc_tsmom_h12         BTCUSDT    TSMOM   H12   3.63    3.40      96%     31
//   doge_tsmom_h12        DOGEUSDT   TSMOM   H12   2.78    3.66     100%     82
//   avax_tsmom_h12        AVAXUSDT   TSMOM   H12   2.61    2.98      87%     76
//
//   H6 ENGINES (8) — SESSION 15:
//   xrp_tsmom_h6          XRPUSDT    TSMOM   H6    2.68    4.41     100%    120
//   btc_tsmom_h6          BTCUSDT    TSMOM   H6    2.59    5.16     100%    169
//   eth_tsmom_h6          ETHUSDT    TSMOM   H6    2.07    3.70     100%    151
//   sol_tsmom_h6          SOLUSDT    TSMOM   H6    2.07    3.25     100%    127
//   bnb_tsmom_h6          BNBUSDT    TSMOM   H6    2.07    2.76     100%     95
//   link_tsmom_h6         LINKUSDT   TSMOM   H6    2.07    3.13     100%     81
//   doge_tsmom_h6         DOGEUSDT   TSMOM   H6    1.72    2.24      77%     91
//   avax_tsmom_h6         AVAXUSDT   TSMOM   H6    1.37    1.82      67%    207
//
//   H4 ENGINES (7):
//   xrp_tsmom_h4          XRPUSDT    TSMOM   H4    2.43    5.80     100%    267
//   bnb_tsmom_h4          BNBUSDT    TSMOM   H4    1.91    3.79     100%    291
//   link_tsmom_h4         LINKUSDT   TSMOM   H4    1.91    4.07      95%    205
//   sol_tsmom_h4          SOLUSDT    TSMOM   H4    1.89    3.82     100%    208
//   btc_tsmom_h4          BTCUSDT    TSMOM   H4    1.82    3.54     100%    167
//   eth_tsmom_h4          ETHUSDT    TSMOM   H4    1.76    3.26     100%    196
//   avax_tsmom_h4         AVAXUSDT   TSMOM   H4    1.47    2.17      83%    231
//
//   H2 ENGINES (5) — NEW SESSION 17:
//   btc_tsmom_h2          BTCUSDT    TSMOM   H2    1.99    4.98     100%    281
//   eth_tsmom_h2          ETHUSDT    TSMOM   H2    1.50    3.02     100%    359
//   sol_tsmom_h2          SOLUSDT    TSMOM   H2    1.78    4.17     100%    340
//   xrp_tsmom_h2          XRPUSDT    TSMOM   H2    2.00    4.70     100%    320
//   link_tsmom_h2         LINKUSDT   TSMOM   H2    1.69    3.76     100%    357
//   doge_tsmom_h2         DOGEUSDT   TSMOM   H2    1.21    1.72      93%    387
//   bnb_tsmom_h2          BNBUSDT    TSMOM   H2    1.19    1.12      87%    436
//
//   H3 ENGINES (7) — SESSION 17+20 (no native Binance candles — cold-start):
//   btc_tsmom_h3          BTCUSDT    TSMOM   H3    1.96    3.52     100%    156
//   eth_tsmom_h3          ETHUSDT    TSMOM   H3    1.74    3.65      98%    278
//   sol_tsmom_h3          SOLUSDT    TSMOM   H3    1.92    4.15      93%    259
//   xrp_tsmom_h3          XRPUSDT    TSMOM   H3    2.19    4.70     100%    243
//   link_tsmom_h3         LINKUSDT   TSMOM   H3    1.94    4.19     100%    254
//   bnb_tsmom_h3          BNBUSDT    TSMOM   H3    1.55    2.74      97%    349
//   doge_tsmom_h3         DOGEUSDT   TSMOM   H3    1.25    1.48      87%    309
//
//   H1 ENGINES (3) — SESSION 15:
//   xrp_tsmom_h1          XRPUSDT    TSMOM   H1    1.66    3.73     100%    327
//   sol_tsmom_h1          SOLUSDT    TSMOM   H1    1.40    3.31     100%    527
//   link_tsmom_h1         LINKUSDT   TSMOM   H1    1.32    3.08      95%    798
//
//   COUNTER-TREND ENGINES (12) — SESSION 19 (fire in bear/ranging markets):
//   TIER 1 (7):
//   eth_rsi30_h3          ETHUSDT    RSI_REV H3    2.41    2.18      92%     87
//   eth_rsi30_h4          ETHUSDT    RSI_REV H4    2.13    1.95      88%     62
//   doge_rsi30_h3         DOGEUSDT   RSI_REV H3    1.98    1.82      85%     94
//   avax_rsi25_h2         AVAXUSDT   RSI_REV H2    2.27    2.05      90%    143
//   doge_rsi25_h2         DOGEUSDT   RSI_REV H2    1.85    1.71      83%    118
//   btc_rsi35_h3          BTCUSDT    RSI_REV H3    1.92    1.78      87%     76
//   bnb_boll25_h3         BNBUSDT    BOLLING H3    2.08    1.93      86%     68
//   TIER 2 (5):
//   eth_boll25_h3         ETHUSDT    BOLLING H3    1.74    1.62      81%     55
//   btc_rsi25_h2          BTCUSDT    RSI_REV H2    1.68    1.55      79%    102
//   link_boll30_h1        LINKUSDT   BOLLING H1    1.59    1.48      77%    134
//   xrp_rsi30_h6          XRPUSDT    RSI_REV H6    1.82    1.69      84%     48
//   xrp_rsi30_h2          XRPUSDT    RSI_REV H2    1.71    1.58      80%    112
//
// DISABLED (no OOS edge — configs preserved below for reference):
//   eth_bb_h6        ETHUSDT    bollinger    H6     0.72   -0.59
//   sol_donch_h6     SOLUSDT    donchian     H6     0.83   -0.57
//   xrp_donch_h1     XRPUSDT    donchian     H1     0.82   -1.19
//   link_rsi_h6      LINKUSDT   rsi_revert   H6     1.17    0.14  (4 trades)
//   btc_overnight_h1 BTCUSDT    overnight    H1     0.31   -4.91
//   btc_weekday_d1   BTCUSDT    weekday      D1     0.44   -1.86
//   doge_tsmom_h4    DOGEUSDT   TSMOM        H4     1.28    1.78  (Nbr=49%)
//
// All instances spot-LONG-only, shadow_mode = true by default. Promote to live
// only after 4 weeks of paper trades match backtest WR/PF within +/- 10%.
//
// COLD-START SEEDING: After constructing the engines and before starting the
// live tick feed, we fetch the most recent N OHLC bars from Binance REST
// (/api/v3/klines, public endpoint) and pre-populate each engine's closed-bar
// deque. H1 = "1h", H2 = "2h", H4 = "4h", H6 = "6h", H12 = "12h", D1 = "1d".
// H3 has NO native Binance interval — those engines cold-start from websocket
// feed only (warm-up ~2-5 days via internal bar synthesis).
//
// HTTP GUI :8080
//   GET  /api/state2  -> JSON with build, spot_prices, engines[]
//   GET  /api/trades  -> JSON array of all trade records
//   POST /api/kill    -> kill_all on every engine (flatten + halt)
// ============================================================================
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "live/BinanceWSFeed.hpp"
#include "live/BinanceREST.hpp"
#include "live/SpotExecutor.hpp"
#include "core/EdgeEngine.hpp"
#include "core/UpJumpLadderCompanion.hpp" // S-2026-07-05b: tiered-2 + self-funding ladder clip book for UPJUMP legs (shadow)
#include "core/GridEngine.hpp"            // S55: maker-native grid sleeve (shadow)
#include "core/MacroBaseEngine.hpp"       // S55: macro-bull base (bull-beta core, shadow)
#include "core/SymbolIndex.hpp"
#include "core/PortfolioOverlay.hpp"  // AUDIT-2026: cross-sec mom + vol-scale overlay
#include "core/CrossSectionalMomentumEngine.hpp"  // S-2026-06-18: validated standalone XSec allocator
#include "core/RipRiderEngine.hpp"                 // S-2026-06-18: sleeve 3 — per-symbol regime-gated rip-rider
#include "core/market_data/MultiSymbolFundingFilter.hpp"
#include "core/LiquidationCascadeDetector.hpp"
#include "live/LiquidationWSFeed.hpp"

#include "version_generated.hpp"
#include "execution/ExchangeLatencyEngine.hpp"

// Required by BinanceWSFeed.cpp (extern declaration)
chimera::ExchangeLatencyEngine g_exchange_latency;

#include "engines_grid.cpp"               // S55: file-scope g_grids + init_grids()
#ifndef BUILD_VERSION
#  define BUILD_VERSION "dev"
#endif

// ── Single-instance lock ─────────────────────────────────────────────────────
static constexpr const char* PID_LOCK_FILE = "/tmp/chimera.lock";
static int g_lock_fd = -1;

void acquire_instance_lock() {
    g_lock_fd = ::open(PID_LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (g_lock_fd < 0) { std::fprintf(stderr, "[FATAL] Cannot open lock file\n"); std::exit(1); }
    if (::flock(g_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        char buf[32] = {};
        if (::read(g_lock_fd, buf, sizeof(buf)-1) < 0) { /* ignore */ }
        ::close(g_lock_fd);
        std::fprintf(stderr, "[FATAL] Chimera already running (PID %s)\n", buf); std::exit(1);
    }
    if (::ftruncate(g_lock_fd, 0) != 0) { /* ignore */ }
    char pidbuf[32]; int len = std::snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)::getpid());
    if (::write(g_lock_fd, pidbuf, len) < 0) { /* ignore */ }
    ::fsync(g_lock_fd);
    std::printf("[STARTUP] Instance lock acquired PID=%d\n", (int)::getpid());
}

void release_instance_lock() {
    if (g_lock_fd >= 0) {
        ::flock(g_lock_fd, LOCK_UN); ::close(g_lock_fd); ::unlink(PID_LOCK_FILE); g_lock_fd = -1;
    }
}

// ── Engine instances (shared between feed callback and HTTP server) ─────────
struct EngineSlot {
    int             symbol_id;          // index into SymbolIndex
    chimera::EdgeEngine* engine = nullptr;
    // Fields cached from Config for seeding (EdgeEngine has no public accessor)
    std::string     symbol_str;         // e.g. "btcusdt"
    int64_t         tf_secs  = 0;       // timeframe in seconds
    std::string     tag;                // e.g. "BTC-TSMOM-D1"
    // Backtest metadata for GUI display (not used by engine logic)
    double  oos_pf    = 0.0;
    double  oos_sharpe = 0.0;
    int     oos_nbr   = 0;
    int     oos_trades = 0;
    int     session   = 0;           // session that discovered this engine
    // ── S34: live PF gate ───────────────────────────────────────────────
    // Refreshed PF from data/batch_validate_results.csv at startup. When
    // bt_pf < MIN_PF, engine is permanently blocked via portfolio_gate.
    double  bt_pf     = 0.0;         // PF from batch validation CSV
    int     bt_trades = 0;           // sample size from batch CSV
    bool    pf_blocked = false;      // true = blocked by PF filter
};

static std::vector<EngineSlot>  g_slots;
// S44b: every wire_engine'd EdgeEngine ptr (g_slots + S43/S43b #includes).
// Used for dashboard count + uniform overlay application.
static std::vector<chimera::EdgeEngine*> g_all_wired;
static std::mutex               g_engine_mtx;
static int64_t                  g_startup_ts_ms = 0;   // epoch ms at startup

// Last-seen mid per symbol — used by /api/kill flatten paths and the
// /api/state2 "spot_prices" field that the GUI renders as a live price strip.
static std::atomic<uint64_t>    g_last_spot_px_bits[chimera::MAX_SYMBOLS]{};

static double load_dbl_atomic(const std::atomic<uint64_t>& a) {
    uint64_t bits = a.load(std::memory_order_relaxed);
    double v; __builtin_memcpy(&v, &bits, 8); return v;
}
static void store_dbl_atomic(std::atomic<uint64_t>& a, double v) {
    uint64_t bits; __builtin_memcpy(&bits, &v, 8);
    a.store(bits, std::memory_order_relaxed);
}

static std::string read_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string buf(sz, '\0');
    size_t got = fread(&buf[0], 1, sz, f);
    if (got != (size_t)sz) buf.resize(got);
    fclose(f);
    return buf;
}

static std::string gui_root;

// ── Session 30: Multi-symbol funding filter ─────────────────────────────────
static chimera::MultiSymbolFundingFilter g_funding_filter;
static int64_t g_last_funding_fetch_ms = 0;
static constexpr int64_t FUNDING_FETCH_INTERVAL_MS = 8 * 3600 * 1000LL; // every 8h

// ── S33: Portfolio gate state exposed to /api/state2 + GUI banner ────────
// Updated each gate evaluation in the portfolio-gate block. Read by
// build_state_json() to surface lock reasons to the dashboard.
static std::atomic<bool>    g_gate_open{true};
static std::atomic<bool>    g_ratchet_locked{false};
// S44d: testing-mode protection bypass. Set from live_config.json at startup.
static std::atomic<bool>    g_protection_disabled_for_testing{false};
// S44e/f: per-engine tier multipliers for lot sizing. Loaded from
// data/engine_tiers.json at startup.
//   TOP_ELITE Sh>=6 n>=100: 2.0x, pyramid_max=3 (peak 6.5x)
//   ELITE                  : 1.5x, pyramid_max=4 (peak 6.0x)
//   STRONG                 : 1.2x, pyramid_max=3 (peak 3.9x)
//   STANDARD               : 1.0x, pyramid_max=2 (peak 2.5x)
static std::map<std::string, std::string> g_engine_tier;     // tag -> tier name
static std::map<std::string, double>      g_tier_multiplier; // tier -> multiplier
static std::map<std::string, int>         g_tier_pyramid_max; // tier -> max adds
// S44g: per-symbol liquidity tier — caps base position before edge boosts
//   MAJOR (BTC/ETH/SOL/BNB/XRP/DOGE) -> 1.0x  (10k base)
//   MID                              -> 0.5x  (5k base)
//   THIN  (post-2024 launches)       -> 0.2x  (2k base)
static std::map<std::string, std::string> g_symbol_liq;       // symbol -> tier name
static std::map<std::string, double>      g_liq_multiplier;   // tier -> multiplier
// P1/S46: hardcoded per-symbol liquidity-tier DEFAULT, indexed by chimera::SymbolId.
// The live config ships with no [liquidity] section, so the old code fell back to
// a FLAT 0.5x for every symbol — thin alts (SEI/RENDER/FET/JTO) were sized the
// SAME as BTC, which is the sizing analogue of the cosmetic-floor bug and the real
// gap exposure. This table actually down-sizes thin / meme / recent-launch names
// (largest gap risk) toward 0.2-0.35x while keeping majors at 1.0x. Config (if
// present) still overrides. Mirror of SYMBOL_CLUSTER tiers.
static constexpr double SYMBOL_LIQ_DEFAULT[chimera::MAX_SYMBOLS] = {
    /*0 BTC*/1.00, /*1 ETH*/1.00, /*2 SOL*/1.00, /*3 BNB*/1.00, /*4 AVAX*/0.60,
    /*5 LINK*/0.60, /*6 XRP*/0.80, /*7 DOGE*/0.60, /*8 SUI*/0.40, /*9 APT*/0.40,
    /*10 NEAR*/0.50, /*11 ARB*/0.40, /*12 PEPE*/0.25, /*13 WIF*/0.25, /*14 FET*/0.40,
    /*15 ONDO*/0.30, /*16 TIA*/0.35, /*17 HBAR*/0.40, /*18 INJ*/0.40, /*19 ADA*/0.70,
    /*20 TRX*/0.60, /*21 SEI*/0.30, /*22 OP*/0.40, /*23 MATIC*/0.50, /*24 ATOM*/0.50,
    /*25 FIL*/0.40, /*26 AAVE*/0.50, /*27 UNI*/0.50, /*28 LDO*/0.35, /*29 ENA*/0.30,
    /*30 JUP*/0.30, /*31 TON*/0.50, /*32 DOT*/0.50, /*33 ICP*/0.40, /*34 RENDER*/0.35,
    /*35 PYTH*/0.30, /*36 GRT*/0.35, /*37 SAND*/0.35, /*38 MANA*/0.35, /*39 CRV*/0.35,
    /*40 COMP*/0.35, /*41 MKR*/0.40, /*42 IMX*/0.35, /*43 STX*/0.35, /*44 ARKM*/0.25,
    /*45 MASK*/0.30, /*46 RUNE*/0.40, /*47 JTO*/0.30, /*48 W*/0.25, /*49 TURBO*/0.20,
    /*50 BOME*/0.20, /*51 FLOKI*/0.20, /*52 ETHFI*/0.30, /*53 EIGEN*/0.30, /*54 ZRO*/0.30,
    /*55 GMT*/0.30, /*56 SHIB*/0.30, /*57 BCH*/0.60, /*58 LTC*/0.60, /*59 ETC*/0.50,
    /*60 XLM*/0.50, /*61 VET*/0.35,
};
static double liq_mult_for_symbol(const std::string& sym) {
    auto it = g_symbol_liq.find(sym);  // config override wins if present
    if (it != g_symbol_liq.end()) {
        auto mit = g_liq_multiplier.find(it->second);
        if (mit != g_liq_multiplier.end()) return mit->second;
    }
    int sid = chimera::sym_id(sym);    // else: real per-symbol tier (not flat 0.5)
    if (sid >= 0 && sid < chimera::MAX_SYMBOLS) return SYMBOL_LIQ_DEFAULT[sid];
    return 0.3;                        // truly-unknown sym -> conservative THIN
}
static double tier_mult_for_tag(const std::string& tag) {
    auto it = g_engine_tier.find(tag);
    if (it == g_engine_tier.end()) return 1.0;
    auto mit = g_tier_multiplier.find(it->second);
    if (mit == g_tier_multiplier.end()) return 1.0;
    return mit->second;
}
// S47: tier-aware per-trade risk-budget scaler. The ELITE/STRONG engines are
// fine-fill + walk-forward + crash validated (several are crash-POSITIVE), so
// they earn a larger gap budget -> the $-clamp lets their bigger lots + deeper
// pyramids through instead of capping them at the STANDARD ceiling. Maximises
// profit on the trusted names while keeping the backstop proportional to proof.
static double tier_risk_mult(const std::string& tag) {
    auto it = g_engine_tier.find(tag);
    if (it == g_engine_tier.end()) return 1.0;
    if (it->second == "ELITE")  return 1.5;
    if (it->second == "STRONG") return 1.25;
    return 1.0;
}
// GUI: per-engine backtest ranking (tier/PF/Sharpe/rank) from the gated fine-fill
// validation. Loaded from data/engine_rankings.csv, served in /api/state2 so the
// dashboard can show every live engine ranked best->worst.
struct EngineRank { std::string tier; double pf = 0.0, sharpe = 0.0; int rank = 0; };
static std::map<std::string, EngineRank> g_engine_rank;
static void load_engine_rankings() {
    std::ifstream f("data/engine_rankings.csv");
    if (!f.is_open()) return;
    std::string line; std::getline(f, line);  // header
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string tag, tier, pf, sh, n, rk;
        std::getline(ss, tag, ','); std::getline(ss, tier, ','); std::getline(ss, pf, ',');
        std::getline(ss, sh, ',');  std::getline(ss, n, ',');    std::getline(ss, rk, ',');
        if (tag.empty()) continue;
        EngineRank er; er.tier = tier; er.pf = atof(pf.c_str());
        er.sharpe = atof(sh.c_str()); er.rank = atoi(rk.c_str());
        g_engine_rank[tag] = er;
    }
    std::printf("[RANK] loaded %zu engine rankings\n", g_engine_rank.size());
    std::fflush(stdout);
}
static int tier_pyramid_max_for_tag(const std::string& tag) {
    auto it = g_engine_tier.find(tag);
    if (it == g_engine_tier.end()) return 4;  // default
    auto mit = g_tier_pyramid_max.find(it->second);
    if (mit == g_tier_pyramid_max.end()) return 4;
    return mit->second;
}

// S44L C: per-symbol daily loss cap. Across all engines on same symbol, if
// cumulative net_bp in last 24h <= -300, block new entries for 24h.
static std::atomic<int64_t> g_sym_daily_blocked_until_ms[chimera::MAX_SYMBOLS]{};
static constexpr double SYM_DAILY_CAP_BP = -150.0;   // S54 derisk: -300->-150 (halt bleeder sooner)
// S44M #1: per-symbol SL-COUNT circuit breaker. 3+ SL hits on same symbol
// within 4h -> halt symbol entries for 4h. Triggers BEFORE bp threshold.
static std::atomic<int64_t> g_sym_sl_circuit_blocked_until_ms[chimera::MAX_SYMBOLS]{};
// S44O: per-symbol post-SL cooldown. After ANY engine SLs on a symbol,
// block ALL engines on that symbol for 30 min. Catches chop earlier
// than the 3-SL circuit. Engine cooldown is per-engine; this is sym-wide.
static std::atomic<int64_t> g_sym_post_sl_cooldown_until_ms[chimera::MAX_SYMBOLS]{};
static constexpr int64_t SYM_POST_SL_COOLDOWN_MS = 30LL * 60 * 1000;
static constexpr int     SYM_SL_COUNT_THRESHOLD = 3;
static constexpr int64_t SYM_SL_WINDOW_MS       = 4LL * 3600 * 1000;
static constexpr int64_t SYM_SL_BLOCK_MS        = 4LL * 3600 * 1000;

// ── S45: CORRELATION-CLUSTER EXPOSURE CAP ───────────────────────────────────
// Crypto is one beta block in a crash: when many correlated longs are open and
// BTC dumps, they all stop out together for an amplified loss. The reactive
// breakers (daily cap, SL circuit) only fire AFTER that loss. This is the
// proactive guard that was flagged-but-never-built after the 30-May drawdown
// (6 JTO engines fired the same direction, all stopped together).
//
// Two limits, enforced together every tick from live open-position counts:
//   1. per-SYMBOL: at most CLUSTER_MAX_PER_SYMBOL open positions on one symbol
//      (directly kills the "6 engines, one name" failure).
//   2. per-CLUSTER: at most CLUSTER_MAX_PER_CLUSTER open across a correlated
//      bucket (majors / L1 / defi / memes / other).
// Self-resetting: an engine's cluster_gate re-opens automatically once a
// correlated position exits and the count drops back under the cap.
enum CryptoCluster { CL_MAJORS = 0, CL_L1 = 1, CL_DEFI = 2, CL_MEME = 3, CL_OTHER = 4, CL_COUNT = 5 };
static constexpr int CLUSTER_MAX_PER_SYMBOL  = 1;   // max concurrent opens per symbol
                                                    // S54: 2->1. Long-only book ->
                                                    // 2 simultaneous longs on one coin =
                                                    // pure 2x leverage + double fees, ZERO
                                                    // diversification. The XLM double-fill
                                                    // (-196.5 x2 = -393bp single-name) was
                                                    // the only path past the -170 hard floor.
static constexpr int CLUSTER_MAX_PER_CLUSTER = 5;   // max concurrent opens per correlated bucket

// Symbol-id -> cluster. Indexed by chimera::SymbolId (0..61). Buckets group
// names that move together vs BTC beta / sector.
static constexpr int SYMBOL_CLUSTER[chimera::MAX_SYMBOLS] = {
    /*0 BTC */CL_MAJORS, /*1 ETH */CL_MAJORS, /*2 SOL */CL_L1,   /*3 BNB */CL_MAJORS,
    /*4 AVAX*/CL_L1,     /*5 LINK*/CL_DEFI,   /*6 XRP */CL_OTHER, /*7 DOGE*/CL_MEME,
    /*8 SUI */CL_L1,     /*9 APT */CL_L1,     /*10 NEAR*/CL_L1,   /*11 ARB */CL_OTHER,
    /*12 PEPE*/CL_MEME,  /*13 WIF */CL_MEME,  /*14 FET */CL_DEFI, /*15 ONDO*/CL_DEFI,
    /*16 TIA */CL_L1,    /*17 HBAR*/CL_L1,    /*18 INJ */CL_DEFI, /*19 ADA */CL_L1,
    /*20 TRX */CL_L1,    /*21 SEI */CL_L1,    /*22 OP  */CL_OTHER,/*23 MATIC*/CL_OTHER,
    /*24 ATOM*/CL_L1,    /*25 FIL */CL_OTHER, /*26 AAVE*/CL_DEFI, /*27 UNI */CL_DEFI,
    /*28 LDO */CL_DEFI,  /*29 ENA */CL_DEFI,  /*30 JUP */CL_DEFI, /*31 TON */CL_L1,
    /*32 DOT */CL_L1,    /*33 ICP */CL_L1,    /*34 RENDER*/CL_OTHER,/*35 PYTH*/CL_DEFI,
    /*36 GRT */CL_DEFI,  /*37 SAND*/CL_OTHER, /*38 MANA*/CL_OTHER,/*39 CRV */CL_DEFI,
    /*40 COMP*/CL_DEFI,  /*41 MKR */CL_DEFI,  /*42 IMX */CL_OTHER,/*43 STX */CL_L1,
    /*44 ARKM*/CL_OTHER, /*45 MASK*/CL_OTHER, /*46 RUNE*/CL_DEFI, /*47 JTO */CL_DEFI,
    /*48 W   */CL_OTHER, /*49 TURBO*/CL_MEME, /*50 BOME*/CL_MEME, /*51 FLOKI*/CL_MEME,
    /*52 ETHFI*/CL_DEFI, /*53 EIGEN*/CL_OTHER,/*54 ZRO */CL_OTHER,/*55 GMT */CL_MEME,
    /*56 SHIB*/CL_MEME,  /*57 BCH */CL_MAJORS,/*58 LTC */CL_MAJORS,/*59 ETC */CL_MAJORS,
    /*60 XLM */CL_L1,    /*61 VET */CL_OTHER,
};
static inline int symbol_cluster(int sid) {
    return (sid >= 0 && sid < chimera::MAX_SYMBOLS) ? SYMBOL_CLUSTER[sid] : CL_OTHER;
}
// Live exposure snapshot, recomputed each tick (single-threaded gate block).
static int g_cluster_open_sym[chimera::MAX_SYMBOLS] = {0};
static int g_cluster_open_bucket[CL_COUNT]          = {0};

// S45: per-CLUSTER 24h loss circuit breaker. The per-tick caps above stop
// SIMULTANEOUS correlated exposure; this stops a SEQUENTIAL correlated bleed
// (the 29-May overnight run: RENDER/FET/SEI/NEAR/JUP/PYTH/INJ stopping out one
// after another). When a bucket's rolling-24h net <= CLUSTER_DAILY_CAP_BP, ALL
// entries in that cluster are halted for 24h. Set in the monitor loop.
static std::atomic<int64_t> g_cluster_blocked_until_ms[CL_COUNT]{};
static constexpr double  CLUSTER_DAILY_CAP_BP = -150.0;  // S54 derisk: -250->-150  // halt whole cluster 24h below this

// P1/S46: per-trade $-risk budget. Cap each entry's notional so a worst-case
// overnight GAP (the one thing a stop cannot price-guarantee) cannot lose more
// than MAX_TRADE_RISK_USD. worst-case gap is cluster-tiered (thin/meme names gap
// hardest). This backstops the boost stack (tier x funding x crash up to ~2x)
// from inflating a thin alt to full size. majors get full size (250bp*10k=$250).
// The clamp is a BACKSTOP against the boost stack (tier x funding x crash)
// inflating a position, NOT the primary sizer — it must bind only on extremes,
// leaving the liquidity tier and engine tier free to differentiate size. The
// worst-gap figures below are PLAUSIBLE overnight gaps (not the extreme -1376bp
// P3 stress tail, which over-tightened the clamp and flattened all sizing). The
// rare extreme tail is bounded instead by liq down-sizing + the 5/cluster cap.
static constexpr double  MAX_TRADE_RISK_USD = 400.0;
static constexpr double  CLUSTER_WORST_GAP_BP[CL_COUNT] = {
    /*majors*/300.0, /*l1*/500.0, /*defi*/600.0, /*meme*/800.0, /*other*/500.0 };

// S45: BEAR-REGIME entry halt. The book is spot-LONG-only — it cannot profit in
// a falling market, so it must sit out entirely rather than feed longs into a
// downtrend. Block all new entries unless BTC regime is BULL_TREND(3).
// (Cutting EXISTING losers is handled by the real -170 hard floor + emergency
// flatten; this only gates new entries.) g_regime: 0=CRASH 1=BEAR 2=BULL_CHOP 3=BULL_TREND.
// S54: 2->3. CHOP TRADING DISABLED — backtest-verified unprofitable: every
// mean-rev kind (BOLLINGER/KELTNER/RSI/STOCH) x liquid syms x H2/H4, recent +
// held-out, best-cell-per-combo summed to -4173bp, 0/12 reached PF>=1.3. Long-
// only spot has no chop edge (dips keep dropping, cost eats the bounce; reversal
// edge needs shorts). So NOTHING trades in BULL_CHOP now — only BULL_TREND.
// (is_trend_kind() gate kept for structure; both branches now require 3. To
// re-enable a chop sleeve later, lower this to 2 AFTER a viable edge is proven.)
static constexpr int REGIME_MIN_FOR_ENTRY = 3;

// S44L D: regime-conditional pyramid. Disable pyramid adds when regime is
// BULL_CHOP (2), BEAR (1), or CRASH (0). Only allow in BULL_TREND (3).
static bool pyramid_allowed_in_regime(int regime) { return regime == 3; }

// S44h: per-engine SL cooldown. After SL hit, engine blocked from entry for
// 1 bar duration (tf_secs). Prevents immediate re-entry into chop.
static std::mutex g_cooldown_mtx;
static std::map<chimera::EdgeEngine*, int64_t> g_engine_cooldown_until;
static void engine_enter_cooldown(chimera::EdgeEngine* e, int64_t now_ms, int64_t hold_ms) {
    if (!e) return;
    std::lock_guard<std::mutex> lk(g_cooldown_mtx);
    g_engine_cooldown_until[e] = now_ms + hold_ms;
    e->set_portfolio_gate(false);
}
static void engine_check_cooldown_expiry(chimera::EdgeEngine* e, int64_t now_ms) {
    if (!e) return;
    std::lock_guard<std::mutex> lk(g_cooldown_mtx);
    auto it = g_engine_cooldown_until.find(e);
    if (it == g_engine_cooldown_until.end()) return;
    if (now_ms >= it->second) {
        e->set_portfolio_gate(true);
        g_engine_cooldown_until.erase(it);
    }
}

// S44h: EMERGENCY HALT — triggered when portfolio drawdown spikes fast or BTC
// crashes. Force-closes all open positions + halts entries for 4 hours.
static std::atomic<int64_t> g_emergency_halt_until_ms{0};
static std::atomic<int> g_recent_sl_count{0};  // SLs in last 30 min, updated by detector
static constexpr int64_t EMERGENCY_HALT_DURATION_MS = 4 * 3600 * 1000;
static constexpr int     EMERGENCY_SL_THRESHOLD     = 7;    // S54: 10->7 (catch chop churn faster)
static constexpr double  EMERGENCY_DD_BP_THRESHOLD  = 150.0; // S54: 300->150 (halt at -150 not -300; the
                                                             // book is ~1.8 effective bets so a chop bleed
                                                             // is correlated + fast — 31-May churned -645
                                                             // via 72 small-loss trades before the old -300
                                                             // gate caught it)
static constexpr double  EMERGENCY_BTC_PCT          = -3.0;  // -3% in 15 min

// S44f #3: confluence detector — when >=3 engines on same symbol fire same
// direction in a short window, boost first add. Tracks (symbol, is_buy)
// pairs with timestamps; intent callback queries this then increments.
static std::mutex g_confluence_mtx;
struct ConfluenceKey { std::string symbol; bool is_buy; };
struct ConfluenceTrack { std::vector<int64_t> recent_ts; };  // ms
static std::map<std::string, ConfluenceTrack> g_confluence;  // key = symbol + "L"/"S"
// S44h: confluence boost DISABLED — backfired during 30 May regime drawdown.
// 6 JTO engines fired same direction, all stopped out together for amplified
// loss. Reverting to 1.0x (no boost) until correlation-cluster guard added.
static constexpr int64_t CONFLUENCE_WINDOW_MS = 15 * 60 * 1000; // 15 min
static constexpr int     CONFLUENCE_THRESHOLD = 999;            // effectively off
static constexpr double  CONFLUENCE_BOOST     = 1.0;
// Returns boost multiplier (>= 1.0). Pushes current timestamp into tracker.
static double check_and_record_confluence(const std::string& symbol, bool is_buy, int64_t now_ms) {
    std::string key = symbol + (is_buy ? "L" : "S");
    std::lock_guard<std::mutex> lk(g_confluence_mtx);
    auto& t = g_confluence[key];
    // Prune stale
    int64_t cutoff = now_ms - CONFLUENCE_WINDOW_MS;
    t.recent_ts.erase(
        std::remove_if(t.recent_ts.begin(), t.recent_ts.end(),
                       [cutoff](int64_t ts){ return ts < cutoff; }),
        t.recent_ts.end());
    t.recent_ts.push_back(now_ms);
    return (t.recent_ts.size() >= (size_t)CONFLUENCE_THRESHOLD) ? CONFLUENCE_BOOST : 1.0;
}
// S44d: sticky alert log for ratchet/lock events. Persists across queries.
static std::mutex           g_alerts_mtx;
static std::vector<std::string> g_alerts;
static void push_alert(const std::string& msg) {
    int64_t ts = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::lock_guard<std::mutex> lk(g_alerts_mtx);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "{\"ts_ms\":%lld,\"msg\":\"", (long long)ts);
    g_alerts.push_back(std::string(buf) + msg + "\"}");
    // Keep last 100 alerts
    if (g_alerts.size() > 100) {
        g_alerts.erase(g_alerts.begin(), g_alerts.begin() + (g_alerts.size() - 100));
    }
}
static std::atomic<bool>    g_streak_halted{false};
static std::atomic<uint64_t> g_session_cum_bp_bits{0};
static std::atomic<uint64_t> g_session_peak_bp_bits{0};
static std::atomic<uint64_t> g_recent_pnl_bp_bits{0};       // rolling 4h DD
static std::atomic<uint64_t> g_unrealized_bp_bits{0};
static std::atomic<int>     g_open_positions_count{0};
static std::atomic<int64_t> g_last_trade_exit_ms{0};

// ── S34: Multi-tier protection state (PERSISTED across restarts) ─────────
// PER anchored to all-time trade_log cumulative, not process start.
// Daily loss kill, regime gate (TREND/CHOP/CRASH), DD-throttled size,
// per-strategy concurrent cap.
// ── S54 MACRO 200d-MA GATE ───────────────────────────────────────────────
// A long-only spot book has NO edge below BTC's 200d MA (validated on unbiased
// forward returns: alt fwd-24h = +17.6bp when BTC>200dMA vs +0.8bp when below,
// i.e. < round-trip cost). So HALT ALL long entries when BTC < 200d-MA — the
// bull/bear master switch. Seeded from REST daily closes at startup, one daily
// sample appended live. Default FLAT (false) until seeded -> fail-safe.
static constexpr int MACRO_MA_DAYS = 200;
static std::atomic<uint64_t> g_btc_200dma_bits{0};         // 200d SMA of BTC daily close
static std::atomic<bool>     g_macro_bull{false};          // btc_spot > 200d MA (default flat)
// S55: idle-capital yield. Operationally = park flat USDT in exchange Earn (~6%
// APY). Modeled here as a SEPARATE accrual (NOT mixed into trading PnL) so the
// dashboard shows the real opportunity cost of sitting flat (esp in a macro-bear).
static constexpr double      IDLE_YIELD_APY = 0.06;
static std::atomic<uint64_t> g_yield_cum_bp_bits{0};       // accrued idle yield (bp), persisted-worthy
static std::atomic<int64_t>  g_yield_last_ms{0};
static std::atomic<int64_t>  g_macro_last_day{0};          // UTC day index of last daily sample
static double g_btc_daily_ring[256] = {0};                 // daily closes (regime-loop thread + startup only)
static int    g_btc_daily_head = 0;                        // next write idx
static int    g_btc_daily_n    = 0;                        // count appended
static std::atomic<uint64_t> g_all_time_peak_bp_bits{0};    // persisted
static std::atomic<uint64_t> g_all_time_cum_bp_bits{0};     // recomputed each loop
static std::atomic<int64_t>  g_daily_kill_until_ms{0};      // persisted; halt entries until this ts
// S54: session-reset epoch. Risk-cap rolling windows (24h daily / per-symbol /
// per-cluster) only count trades with exit_ts >= this. POST /api/session_reset
// sets it to now, voiding pre-fix bug losses from the live risk accounting and
// clearing every halt. The trade ledger itself is untouched (history intact).
static std::atomic<int64_t>  g_pnl_epoch_ms{0};             // persisted; 0 = count all
static std::atomic<uint64_t> g_pnl_epoch_baseline_bp_bits{0}; // persisted; cum seed at last reset
static std::atomic<bool>     g_force_ratchet_unlock{false};  // set by session_reset -> ratchet loop clears its lock
static std::atomic<uint64_t> g_daily_pnl_bp_bits{0};        // rolling 24h pnl
static std::atomic<int>      g_regime{2};                    // 0=CRASH, 1=CHOP, 2=TREND
static std::atomic<uint64_t> g_size_throttle_bits{0};        // current sizing multiplier (0..1)
static std::atomic<int>      g_tsmom_open_count{0};          // concurrent open TSMOM positions
static std::atomic<uint64_t> g_btc_short_ret_bits{0};        // recent BTC % move (rally detector)
// S34: BTC spot history for live chart (last ~1h of ~5s ticks)
static constexpr int BTC_CHART_N = 200;
static std::atomic<uint64_t> g_btc_chart_px[BTC_CHART_N]{};
static std::atomic<int>      g_btc_chart_idx{0};
static std::atomic<int>      g_btc_chart_filled{0};

// S34: rally-detector buffer (lifted from function-static so REST seed can fill)
static constexpr int BTC_RALLY_BUF = 120;
static std::atomic<uint64_t> g_btc_rally_px[BTC_RALLY_BUF]{};
static std::atomic<int>      g_btc_rally_idx{0};
static std::atomic<int>      g_btc_rally_filled{0};
static std::atomic<int64_t>  g_btc_rally_last_ms{0};

// S34-r8: per-symbol regime — each tradable symbol gets its own rally
// detector + regime classification. Lets alts trade when they have edge
// even if BTC is flat. Indexed by chimera::SymbolId.
static constexpr int SYM_RALLY_BUF = 120;
static std::atomic<uint64_t> g_sym_rally_px[chimera::MAX_SYMBOLS][SYM_RALLY_BUF]{};
static std::atomic<int>      g_sym_rally_idx[chimera::MAX_SYMBOLS]{};
static std::atomic<int>      g_sym_rally_filled[chimera::MAX_SYMBOLS]{};
static std::atomic<int64_t>  g_sym_rally_last_ms[chimera::MAX_SYMBOLS]{};
static std::atomic<int>      g_sym_regime[chimera::MAX_SYMBOLS]{};   // 0..3 per symbol
static std::atomic<uint64_t> g_sym_short_ret[chimera::MAX_SYMBOLS]{}; // recent % move per symbol

// ── Session 30: Liquidation cascade detector ────────────────────────────────
static chimera::LiquidationCascadeDetector g_liq_detector;
static chimera::LiquidationWSFeed g_liq_feed;

// AUDIT-2026 portfolio overlay: cross-sectional 28d momentum + 20d vol scaling.
// Multiplier composed with size_throttle in per-engine apply (~L5867).
static chimera::PortfolioOverlay g_portfolio_overlay;

// ── AUDIT-2026 BLOWOFF GUARD: per-engine momentum cache (tag -> momentum_pct) ──
// Populated by on_bar_callback at every bar close. Consumed by tick handler
// to set EdgeEngine portfolio_gate(false) when extension > BLOWOFF_THRESHOLD.
// Suppresses chase-the-top entries (mom > 50% above lookback close).
//
// Calibration (counterfactual backtest, 27 active-roster trades May 2026):
//   thr=100% (no filter): +1090.2bp historical
//   thr=80%  (optimal):   +1100.8bp (+10.6bp) — blocks NEAR-H8 wipeout pair
//   thr=50%  (initial):   +1087.8bp (-2.4bp)  — also blocks NEAR-H12 winner
//   thr=30%  (tight):      +1005.9bp (-84bp)  — kills too many winners
// 80% catches the NEAR-TSMOM-H8 EARLY_KILL (-47.3bp at mom=86%) while
// preserving the NEAR-TSMOM-H12 SL winner (+12.9bp at mom=77%).
static std::map<std::string, double> g_last_momentum_pct;
static std::mutex g_momentum_mtx;
// AUDIT-2026-S35: per-engine tier sizing multiplier.
// Allocates more capital to high-conviction engines, less to marginal ones.
// Composed with size_throttle * overlay_mult in the main loop.
//   ELITE  (sharpe >= 3.5 AND pf >= 2.0) -> 1.3x
//   STRONG (sharpe >= 2.0)               -> 1.1x
//   WEAK   (sharpe <  1.0 OR pf <  1.5)  -> 0.7x
//   BASE   (default)                     -> 1.0x
// Range [0.7, 1.3] = modest by design; portfolio-level DD throttle and
// MAX_PER_SYMBOL cap remain the primary risk bounds.
static inline double tier_sizing_mult(double sharpe, double pf) {
    if (sharpe >= 3.5 && pf >= 2.0) return 1.3;
    if (sharpe >= 2.0)              return 1.1;
    if (sharpe <  1.0 || pf <  1.5) return 0.7;
    return 1.0;
}

// AUDIT-2026-S35-DYN: live PF + Sharpe rolling stats per engine.
// Populated by recompute_live_tiers() every 60s from g_trade_log.
struct LiveStats {
    double pf;
    double sharpe;
    int    n;
};
static std::map<std::string, LiveStats> g_live_stats;
static std::mutex g_live_stats_mtx;
static std::atomic<int64_t> g_last_live_recalc_ms{0};

// Bayesian-ish blend: backtest tier as prior, live stats as update.
// Live weight ramps with sample size (capped 0.5 at n>=50).
//   n<10  -> 100% backtest
//   n=10  -> 20% live
//   n=20  -> 30% live
//   n=30  -> 40% live
//   n>=50 -> 50% live
static inline double live_blend_weight(int n) {
    if (n < 10)  return 0.0;
    if (n >= 50) return 0.5;
    // linear 10 -> 0.2, 50 -> 0.5
    return 0.2 + (n - 10) * (0.30 / 40.0);
}

// Blended tier — falls back to backtest if no live data.
static inline double tier_sizing_mult_blended(const std::string& tag,
                                              double bt_sharpe, double bt_pf) {
    LiveStats ls{0,0,0};
    {
        std::lock_guard<std::mutex> lk(g_live_stats_mtx);
        auto it = g_live_stats.find(tag);
        if (it != g_live_stats.end()) ls = it->second;
    }
    if (ls.n < 10) return tier_sizing_mult(bt_sharpe, bt_pf);
    double w = live_blend_weight(ls.n);
    double sh = (1.0 - w) * bt_sharpe + w * ls.sharpe;
    double pf = (1.0 - w) * bt_pf     + w * ls.pf;
    return tier_sizing_mult(sh, pf);
}

// Definition of recompute_live_tiers is below g_trade_log decl at ~L440.

// AUDIT-2026-S35: per-TF blowoff threshold.
// Calibrated via backtest on 243 disabled engines (5yr Binance data, fresh
// through 2026-05-27). Uniform 80% miscalibrated for short TFs — hurts 97
// engines while helping 10. Per-TF tuning:
//   D2+    (>= 172800s) : 80%  (slow lookbacks — 100% mom = real blowoff top)
//   D1     (>= 86400s)  : 120% (daily rallies fine to 100%+, real top by 120%)
//   H4-H16 (>= 14400s)  : 150% (short-TF noise — 150% lookback still tradeable)
//   H1-H3  (< 14400s)   : 200% (high-freq tolerates more extension)
// Validated +491k cum_bp vs uniform 80% across surviving engines.
static inline double blowoff_threshold_for_tf(int64_t tf_secs) {
    if (tf_secs >= 172800) return 80.0;
    if (tf_secs >= 86400)  return 120.0;
    if (tf_secs >= 14400)  return 150.0;
    return 200.0;
}

// ── Trade journal — persist to JSON, serve via /api/trades ───────────────────
static constexpr const char* TRADES_FILE = "data/trades.json";
static std::vector<chimera::EdgeEngine::TradeRecord> g_trade_log;
static std::mutex g_trades_mtx;

// AUDIT-2026-S35-DYN: recompute live PF/Sharpe rolling stats from last 30
// trades per engine. Called from main loop with 60s cadence. See LiveStats
// + tier_sizing_mult_blended() above (~L340-380).
static void recompute_live_tiers() {
    std::map<std::string, std::vector<double>> nets_by_tag;
    {
        std::lock_guard<std::mutex> lk(g_trades_mtx);
        // Walk backward so most-recent trades come first; cap at 30 per tag.
        for (auto it = g_trade_log.rbegin(); it != g_trade_log.rend(); ++it) {
            auto& v = nets_by_tag[it->tag];
            if ((int)v.size() < 30) v.push_back(it->net_bp);
        }
    }
    std::map<std::string, LiveStats> next;
    for (auto& kv : nets_by_tag) {
        const auto& nets = kv.second;
        if (nets.size() < 5) continue;  // need minimum sample for any stat
        double wins = 0.0, losses = 0.0, sum = 0.0, sumsq = 0.0;
        for (double n : nets) {
            sum += n;
            sumsq += n * n;
            if (n > 0)      wins   += n;
            else if (n < 0) losses += -n;
        }
        double mean = sum / nets.size();
        double var  = sumsq / nets.size() - mean * mean;
        double sd   = (var > 0) ? std::sqrt(var) : 0.0;
        LiveStats ls;
        ls.pf = (losses > 0.0) ? (wins / losses) : ((wins > 0.0) ? 99.0 : 0.0);
        // Approx annualised Sharpe: per-trade SR * sqrt(trades_per_year_estimate).
        // 200 tpy mid-range across roster — rough proxy vs backtest oos_sharpe.
        ls.sharpe = (sd > 0.0) ? (mean / sd) * std::sqrt(200.0) : 0.0;
        ls.n      = (int)nets.size();
        next.emplace(kv.first, ls);
    }
    {
        std::lock_guard<std::mutex> lk(g_live_stats_mtx);
        g_live_stats.swap(next);
    }
}

// Format a TradeRecord as a JSON object string
static std::string trade_to_json(const chimera::EdgeEngine::TradeRecord& t) {
    std::ostringstream js;
    js << std::fixed;
    js << "{";
    js << "\"tag\":\"" << t.tag << "\",";
    js << "\"symbol\":\"" << t.symbol << "\",";
    js << "\"strategy\":\"" << t.strategy << "\",";
    js << "\"reason\":\"" << t.reason << "\",";
    js << "\"entry_ts\":" << t.entry_ts_ms << ",";
    js << "\"exit_ts\":" << t.exit_ts_ms << ",";
    js << std::setprecision(6);
    js << "\"entry_px\":" << t.entry_px << ",";
    js << "\"exit_px\":" << t.exit_px << ",";
    js << "\"sl_px\":" << t.sl_px << ",";
    js << std::setprecision(2);
    js << "\"gross_bp\":" << t.gross_bp << ",";
    js << "\"net_bp\":" << t.net_bp << ",";
    js << "\"mfe_bp\":" << t.mfe_bp << ",";
    js << "\"trade_num\":" << t.trade_num << ",";
    js << "\"shadow\":" << (t.shadow ? "true" : "false");
    js << "}";
    return js.str();
}

// Append one trade to the JSON file on disk (newline-delimited JSON for
// simplicity — each line is a valid JSON object, easy to parse and append).
static void persist_trade(const chimera::EdgeEngine::TradeRecord& t) {
    FILE* f = fopen(TRADES_FILE, "a");
    if (!f) {
        std::fprintf(stderr, "[JOURNAL] Failed to open %s for append\n", TRADES_FILE);
        return;
    }
    std::string line = trade_to_json(t) + "\n";
    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
    std::printf("[JOURNAL] Trade #%d persisted: %s %s %+.1fbp\n",
        t.trade_num, t.tag.c_str(), t.reason.c_str(), t.net_bp);
    std::fflush(stdout);
}

// ── S34: Multi-tier protection state persistence ───────────────────────────
// Persists g_all_time_peak_bp, g_all_time_cum_bp, g_daily_kill_until_ms so
// protections survive process restarts. Without this PER (Peak Equity Ratchet)
// resets every deploy and lets the bot rebuild and bleed a peak repeatedly.
static constexpr const char* PROTECTION_FILE = "data/protection_state.json";

static void save_protection_state() {
    FILE* f = fopen(PROTECTION_FILE, "w");
    if (!f) {
        std::fprintf(stderr, "[PROTECTION] Failed to open %s for write\n", PROTECTION_FILE);
        return;
    }
    std::fprintf(f, "{\"all_time_peak_bp\":%.4f,\"all_time_cum_bp\":%.4f,\"daily_kill_until_ms\":%lld,\"pnl_epoch_ms\":%lld,\"pnl_epoch_baseline_bp\":%.4f}\n",
        load_dbl_atomic(g_all_time_peak_bp_bits),
        load_dbl_atomic(g_all_time_cum_bp_bits),
        (long long)g_daily_kill_until_ms.load(std::memory_order_relaxed),
        (long long)g_pnl_epoch_ms.load(std::memory_order_relaxed),
        load_dbl_atomic(g_pnl_epoch_baseline_bp_bits));
    fclose(f);
}

static void load_protection_state() {
    // Default sizing throttle = 1.0x
    store_dbl_atomic(g_size_throttle_bits, 1.0);

    FILE* f = fopen(PROTECTION_FILE, "r");
    if (!f) {
        std::printf("[PROTECTION] No state file — fresh start\n");
        std::fflush(stdout);
        return;
    }
    char buf[1024] = {};
    size_t got = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    if (got == 0) return;

    std::string s(buf, got);
    auto extract_num = [&](const char* key) -> double {
        auto p = s.find(std::string("\"") + key + "\":");
        if (p == std::string::npos) return 0.0;
        p += strlen(key) + 3;
        try { return std::stod(s.substr(p)); } catch (...) { return 0.0; }
    };
    double peak = extract_num("all_time_peak_bp");
    double cum  = extract_num("all_time_cum_bp");
    int64_t kill_until = (int64_t)extract_num("daily_kill_until_ms");
    int64_t pnl_epoch  = (int64_t)extract_num("pnl_epoch_ms");
    double  epoch_base = extract_num("pnl_epoch_baseline_bp");

    store_dbl_atomic(g_all_time_peak_bp_bits, peak);
    store_dbl_atomic(g_all_time_cum_bp_bits,  cum);
    g_daily_kill_until_ms.store(kill_until, std::memory_order_relaxed);
    g_pnl_epoch_ms.store(pnl_epoch, std::memory_order_relaxed);
    store_dbl_atomic(g_pnl_epoch_baseline_bp_bits, epoch_base);

    std::printf("[PROTECTION] Loaded: all_time_peak=%+.1fbp  all_time_cum=%+.1fbp  daily_kill_until_ms=%lld  pnl_epoch_ms=%lld\n",
        peak, cum, (long long)kill_until, (long long)pnl_epoch);
    std::fflush(stdout);
}

// ── S34: PF filter — load batch validation CSV at startup ──────────────────
// Reads data/batch_validate_results.csv, populates slot.bt_pf and slot.bt_trades
// per tag, marks slot.pf_blocked = true when bt_pf < S34_MIN_PF.
// Sim shows simple PF >= 1.3 filter on existing trades would have flipped
// 153 trades from -40.8bp to +1864bp over the last 3 days.
static constexpr double S34_MIN_PF = 1.3;
static constexpr int    S34_MIN_PF_SAMPLE = 20;  // ignore PF if sample too small

static void load_pf_data_into_slots() {
    FILE* f = fopen("data/batch_validate_results.csv", "r");
    if (!f) {
        std::fprintf(stderr, "[PF_FILTER] No batch_validate_results.csv — PF filter disabled\n");
        return;
    }
    char line[2048];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return; }  // skip header
    // Build tag -> (pf, trades) map
    std::map<std::string, std::pair<double,int>> pf_map;
    while (fgets(line, sizeof(line), f)) {
        std::string s(line);
        // CSV: tag,symbol,kind,tf,trades,pf,...
        auto next_field = [&](size_t& pos) -> std::string {
            auto end = s.find(',', pos);
            std::string r = s.substr(pos, end - pos);
            pos = (end == std::string::npos) ? s.size() : end + 1;
            return r;
        };
        size_t pos = 0;
        std::string tag    = next_field(pos);
        next_field(pos);  // symbol
        next_field(pos);  // kind
        next_field(pos);  // tf
        std::string trades = next_field(pos);
        std::string pf     = next_field(pos);
        try {
            double p = std::stod(pf);
            int    n = std::stoi(trades);
            pf_map[tag] = {p, n};
        } catch (...) {}
    }
    fclose(f);

    int blocked = 0, kept = 0, no_data = 0;
    for (auto& s : g_slots) {
        auto it = pf_map.find(s.tag);
        if (it == pf_map.end()) { no_data++; continue; }
        s.bt_pf     = it->second.first;
        s.bt_trades = it->second.second;
        if (s.bt_pf < S34_MIN_PF && s.bt_trades >= S34_MIN_PF_SAMPLE) {
            s.pf_blocked = true;
            blocked++;
            std::printf("[PF_FILTER] BLOCK %-26s  PF=%.2f  n=%d\n", s.tag.c_str(), s.bt_pf, s.bt_trades);
        } else {
            kept++;
        }
    }
    std::printf("[PF_FILTER] loaded %d entries from CSV. Kept=%d  Blocked=%d  NoData=%d  (MIN_PF=%.2f, min_sample=%d)\n",
        (int)pf_map.size(), kept, blocked, no_data, S34_MIN_PF, S34_MIN_PF_SAMPLE);
    std::fflush(stdout);
}

// Load trade history from disk on startup (newline-delimited JSON)
static void load_trade_history() {
    FILE* f = fopen(TRADES_FILE, "r");
    if (!f) {
        std::printf("[JOURNAL] No trade history file — starting fresh\n");
        std::fflush(stdout);
        return;
    }
    char line[2048];
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        // Minimal parse — extract key fields from the JSON line
        chimera::EdgeEngine::TradeRecord rec;
        std::string s(line);

        auto extract_str = [&](const char* key) -> std::string {
            auto pos = s.find(std::string("\"") + key + "\":\"");
            if (pos == std::string::npos) return "";
            pos += strlen(key) + 4;
            auto end = s.find('"', pos);
            return (end != std::string::npos) ? s.substr(pos, end - pos) : "";
        };
        auto extract_num = [&](const char* key) -> double {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0.0;
            pos += strlen(key) + 3;
            return std::stod(s.substr(pos));
        };
        auto extract_int64 = [&](const char* key) -> int64_t {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0;
            pos += strlen(key) + 3;
            return std::stoll(s.substr(pos));
        };
        auto extract_bool = [&](const char* key) -> bool {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return false;
            pos += strlen(key) + 3;
            return s.substr(pos, 4) == "true";
        };

        rec.tag         = extract_str("tag");
        rec.symbol      = extract_str("symbol");
        rec.strategy    = extract_str("strategy");
        rec.reason      = extract_str("reason");
        rec.entry_ts_ms = extract_int64("entry_ts");
        rec.exit_ts_ms  = extract_int64("exit_ts");
        rec.entry_px    = extract_num("entry_px");
        rec.exit_px     = extract_num("exit_px");
        rec.sl_px       = extract_num("sl_px");
        rec.gross_bp    = extract_num("gross_bp");
        rec.net_bp      = extract_num("net_bp");
        rec.mfe_bp      = extract_num("mfe_bp");
        rec.trade_num   = (int)extract_num("trade_num");
        rec.shadow      = extract_bool("shadow");

        if (!rec.tag.empty()) {
            std::lock_guard<std::mutex> lk(g_trades_mtx);
            g_trade_log.push_back(rec);
            count++;
        }
    }
    fclose(f);
    std::printf("[JOURNAL] Loaded %d historical trades from %s\n", count, TRADES_FILE);
    std::fflush(stdout);
}

// Build JSON array of all trades for /api/trades
static std::string build_trades_json() {
    std::lock_guard<std::mutex> lk(g_trades_mtx);
    std::ostringstream js;
    js << "[";
    for (size_t i = 0; i < g_trade_log.size(); ++i) {
        if (i > 0) js << ",";
        js << trade_to_json(g_trade_log[i]);
    }
    js << "]";
    return js.str();
}

// Trade callback — called by EdgeEngine on every exit
static void on_trade_callback(const chimera::EdgeEngine::TradeRecord& rec) {
    {
        std::lock_guard<std::mutex> lk(g_trades_mtx);
        g_trade_log.push_back(rec);
    }
    persist_trade(rec);

    // S34: refresh persisted all-time cumulative + peak so PER survives
    // process restarts. Without this the peak resets to 0 on every deploy.
    if (rec.reason != "SHUTDOWN") {
        double cur_cum = load_dbl_atomic(g_all_time_cum_bp_bits) + rec.net_bp;
        double cur_peak = load_dbl_atomic(g_all_time_peak_bp_bits);
        if (cur_cum > cur_peak) cur_peak = cur_cum;
        store_dbl_atomic(g_all_time_cum_bp_bits, cur_cum);
        store_dbl_atomic(g_all_time_peak_bp_bits, cur_peak);
        save_protection_state();
    }
}

// ── Bar journal — persist completed bars for warm-start + audit trail ────────
// Each engine gets its own file: data/bars/{TAG}.ndjson
// This is the critical fix: bars are now saved to disk so H3 engines (which
// have no REST seed source) don't lose days of accumulated data on restart,
// and ALL engines have a full audit trail of every bar + signal evaluation.

static std::string bar_file_for_tag(const std::string& tag) {
    return "data/bars/" + tag + ".ndjson";
}

static std::string bar_to_json(const chimera::EdgeEngine::BarRecord& b) {
    std::ostringstream js;
    js << std::fixed;
    js << "{";
    js << "\"tag\":\"" << b.tag << "\",";
    js << "\"open_ts_ms\":" << b.open_ts_ms << ",";
    js << "\"tf_secs\":" << b.tf_secs << ",";
    js << std::setprecision(8);
    js << "\"o\":" << b.o << ",";
    js << "\"h\":" << b.h << ",";
    js << "\"l\":" << b.l << ",";
    js << "\"c\":" << b.c << ",";
    js << std::setprecision(6);
    js << "\"atr\":" << b.atr << ",";
    js << std::setprecision(4);
    js << "\"momentum_pct\":" << b.momentum_pct << ",";
    js << "\"signal_ready\":" << (b.signal_ready ? "true" : "false") << ",";
    js << "\"signal_fired\":" << (b.signal_fired ? "true" : "false") << ",";
    js << "\"in_position\":" << (b.in_position ? "true" : "false") << ",";
    js << "\"bars_in_buffer\":" << b.bars_in_buffer;
    js << "}";
    return js.str();
}

static void persist_bar(const chimera::EdgeEngine::BarRecord& b) {
    std::string path = bar_file_for_tag(b.tag);
    FILE* f = fopen(path.c_str(), "a");
    if (!f) {
        std::fprintf(stderr, "[BAR_LOG] Failed to open %s for append\n", path.c_str());
        return;
    }
    std::string line = bar_to_json(b) + "\n";
    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
}

// ── UPJUMP companion clip book (S-2026-07-03, Slice 4b) ─────────────────────
// STANDALONE ADDITIVE paper book: each UPJUMP leg gets an independent clip
// companion that OBSERVES (never touches / closes / moves) the parent. Registered
// by parent tag; driven from on_bar_callback on every completed bar. Judged
// STANDALONE, never vs-WIDE (feedback-companion-independent-engine). Emits its
// own ledger only — no order, no callback into the parent.
static constexpr const char* COMPANION_TRADES_FILE = "data/companion_trades.json";
static std::map<std::string, std::pair<chimera::EdgeEngine*, chimera::UpJumpLadderCompanion*>> g_companion_by_parent;
static std::mutex g_companion_mtx;

static void persist_companion_clip(const chimera::UpJumpLadderCompanion::ClipRecord& r) {
    FILE* f = fopen(COMPANION_TRADES_FILE, "a");
    if (!f) { std::fprintf(stderr, "[CLIP_LOG] failed to open %s for append\n", COMPANION_TRADES_FILE); return; }
    std::ostringstream js; js << std::fixed;
    js << "{\"tag\":\"" << r.tag << "\",\"symbol\":\"" << r.symbol << "\",\"reason\":\"" << r.reason << "\","
       << "\"entry_ts\":" << r.entry_ts_ms << ",\"exit_ts\":" << r.exit_ts_ms << std::setprecision(6)
       << ",\"entry_px\":" << r.entry_px << ",\"exit_px\":" << r.exit_px << std::setprecision(2)
       << ",\"gross_bp\":" << r.gross_bp << ",\"net_bp\":" << r.net_bp
       << ",\"gross_bp_real\":" << r.gross_bp_real << ",\"net_bp_real\":" << r.net_bp_real
       << ",\"mult\":" << r.size_mult << ",\"mfe_pct\":" << r.mfe_pct
       << ",\"bars_held\":" << r.bars_held << ",\"clip_num\":" << r.clip_num
       << ",\"shadow\":" << (r.shadow ? "true" : "false") << "}\n";
    std::string line = js.str();
    fwrite(line.c_str(), 1, line.size(), f);
    fclose(f);
}

// Rehydrate cumulative clip counters (count + summed net_bp) per companion tag from
// the append-only durable clip log, so the desk panel clips/bank_bp survive a restart.
// Crude line-scan parse (the file is our own one-object-per-line ndjson) -> no JSON dep.
struct ClipTotals { int n = 0; double net = 0.0; double net_real = 0.0; double net_real_w = 0.0; };
static std::map<std::string, ClipTotals> load_companion_clip_totals() {
    std::map<std::string, ClipTotals> totals;  // tag -> (clip_count, sum net_bp, sum net_bp_real)
    std::ifstream f(COMPANION_TRADES_FILE);
    if (!f) return totals;
    std::string line;
    while (std::getline(f, line)) {
        auto tp = line.find("\"tag\":\"");
        if (tp == std::string::npos) continue;
        tp += 7;
        auto te = line.find("\"", tp);
        if (te == std::string::npos) continue;
        std::string tag = line.substr(tp, te - tp);
        // Multi-leg ladder tags are "<COIN>-UPJUMP-CLIP-T1/-T2/-L1..". Collapse to the
        // book base tag ("<COIN>-UPJUMP-CLIP") so rehydrate (keyed on config().tag)
        // sums clips/bank across every sub-leg of the book.
        auto cp = tag.find("-CLIP");
        if (cp != std::string::npos) tag = tag.substr(0, cp + 5);
        double net = 0.0, net_real = 0.0, mult = 1.0;
        auto np = line.find("\"net_bp\":");
        if (np != std::string::npos) { try { net = std::stod(line.substr(np + 9)); } catch (...) {} }
        auto nrp = line.find("\"net_bp_real\":");
        if (nrp != std::string::npos) { try { net_real = std::stod(line.substr(nrp + 14)); } catch (...) {} }
        auto mp = line.find("\"mult\":");
        if (mp != std::string::npos) { try { mult = std::stod(line.substr(mp + 7)); } catch (...) {} }
        // pre-real-column history lines lack net_bp_real -> counted as 0 (unknown real value);
        // pre-weighting lines lack mult -> weighted at 1.0 (they were booked at x1).
        auto& agg = totals[tag];
        agg.n          += 1;
        agg.net        += net;
        agg.net_real   += net_real;
        agg.net_real_w += net_real * mult;
    }
    return totals;
}

// ── S-2026-07-08 companion detector-state persistence (restart-path fix) ────
// The det_w books self-detect their windows on the roster per-coin W/thr — a
// DIFFERENT window family than the live parents (uniform 4h/+2%). The old boot
// path seeded them from the parent's position: phantom windows in, genuine
// in-flight windows eaten. Now the detector state itself is persisted each H1
// close and restored verbatim at boot. Caller MUST hold g_companion_mtx.
static constexpr const char* COMPANION_DET_STATE_FILE = "data/companion_det_state.json";
static void save_companion_det_state() {
    std::ostringstream js;
    for (const auto& kv : g_companion_by_parent) {
        const auto& cfg = kv.second.second->config();
        if (cfg.det_w <= 0) continue;
        js << kv.second.second->det_state_json() << "\n";
    }
    const std::string tmp = std::string(COMPANION_DET_STATE_FILE) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) return;
    const std::string s = js.str();
    fwrite(s.c_str(), 1, s.size(), f);
    fclose(f);
    std::rename(tmp.c_str(), COMPANION_DET_STATE_FILE);
}
// crude ndjson line-scan restore (same style as load_companion_clip_totals).
static void restore_companion_det_state() {
    std::ifstream f(COMPANION_DET_STATE_FILE);
    if (!f) { std::printf("[CLIP-DETSEED] no %s — det_w books cold-start their windows honestly\n",
                          COMPANION_DET_STATE_FILE); return; }
    int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string line;
    while (std::getline(f, line)) {
        auto tp = line.find("\"tag\":\"");
        if (tp == std::string::npos) continue;
        tp += 7; auto te = line.find("\"", tp);
        if (te == std::string::npos) continue;
        std::string tag = line.substr(tp, te - tp);
        chimera::UpJumpLadderCompanion* comp = nullptr;
        for (auto& kv : g_companion_by_parent)
            if (kv.second.second->config().tag == tag) { comp = kv.second.second; break; }
        if (!comp) continue;
        auto num = [&](const char* key, double dflt) -> double {
            auto p = line.find(key); if (p == std::string::npos) return dflt;
            try { return std::stod(line.substr(p + std::strlen(key))); } catch (...) { return dflt; }
        };
        bool   det_in    = num("\"det_in\":", 0) > 0.5;
        double det_entry = num("\"det_entry\":", 0.0);
        int64_t det_bar  = (int64_t)num("\"det_bar\":", -1);
        double det_close = num("\"det_close\":", 0.0);
        std::vector<double> ring;
        auto hp = line.find("\"h1c\":[");
        if (hp != std::string::npos) {
            auto he = line.find("]", hp);
            std::string arr = line.substr(hp + 7, he - hp - 7);
            std::stringstream ss(arr); std::string tok;
            while (std::getline(ss, tok, ',')) { if (!tok.empty()) { try { ring.push_back(std::stod(tok)); } catch (...) {} } }
        }
        comp->restore_det_state(det_in, det_entry, det_bar, det_close, ring, now_ms);
    }
}

// Live per-leg companion snapshot for the Omega desk CRYPTO COMPANIONS panel.
// Schema: {"ts":<unix>,"legs":[{sym,armed,peak_mfe_pct,bars_since_high,clips,bank_bp}]}.
// Cross-box pushed to C:\Omega\crypto_companion_state.json (Mac launchd scp) where
// /api/crypto_companion serves it. Caller MUST hold g_companion_mtx. Atomic write
// (tmp+rename). Read-only view of settled companion state — never touches a parent.
static constexpr const char* COMPANION_STATE_FILE = "data/crypto_companion_state.json";
static void emit_companion_state() {
    std::ostringstream js; js << std::fixed;
    js << "{\"ts\":" << (long long)std::time(nullptr) << ",\"legs\":[";
    bool first = true;
    for (const auto& kv : g_companion_by_parent) {
        const auto snap = kv.second.second->snapshot();
        std::string sym = kv.first.substr(0, kv.first.find('-'));  // "BTC-UPJUMP-H1" -> "BTC"
        if (!first) js << ",";
        first = false;
        js << "{\"sym\":\"" << sym << "\",\"armed\":" << (snap.armed ? "true" : "false")
           << std::setprecision(4) << ",\"peak_mfe_pct\":" << snap.peak_mfe_pct
           << ",\"bars_since_high\":" << snap.bars_since_high
           << ",\"clips\":" << snap.clips
           << std::setprecision(2) << ",\"bank_bp\":" << snap.bank_bp
           << ",\"bank_bp_real\":" << snap.bank_bp_real
           << ",\"bank_bp_real_w\":" << snap.bank_bp_real_w
           << ",\"mult\":" << snap.size_mult
           << ",\"retired\":" << (snap.retired ? "true" : "false");
        // Per-leg breakdown (S-2026-07-05b tiered ladder): T1/T2 base + L1..Ln ladder
        // legs currently OPEN, each with its own armed/peak/stall for the Omega desk
        // CRYPTO COMPANIONS multi-leg render. sym-level fields above remain the book
        // aggregate (back-compat with the pre-ladder single-leg panel).
        js << ",\"sublegs\":[";
        bool sfirst = true;
        for (const auto& ls : kv.second.second->leg_snapshots()) {
            if (!sfirst) js << ",";
            sfirst = false;
            js << "{\"id\":\"" << ls.label << "\",\"armed\":" << (ls.armed ? "true" : "false")
               << std::setprecision(4) << ",\"peak_mfe_pct\":" << ls.peak_mfe_pct
               << ",\"bars_since_high\":" << ls.bars_since_high << "}";
        }
        js << "]}";
    }
    js << "]}";
    const std::string tmp = std::string(COMPANION_STATE_FILE) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) { std::fprintf(stderr, "[CC_STATE] failed to open %s\n", tmp.c_str()); return; }
    const std::string s = js.str();
    fwrite(s.c_str(), 1, s.size(), f);
    fclose(f);
    std::rename(tmp.c_str(), COMPANION_STATE_FILE);
}

// Bar callback — called by EdgeEngine on every bar close
static void on_bar_callback(const chimera::EdgeEngine::BarRecord& rec) {
    persist_bar(rec);
    // AUDIT-2026 BLOWOFF GUARD: cache momentum_pct so tick handler can gate.
    {
        std::lock_guard<std::mutex> lk(g_momentum_mtx);
        g_last_momentum_pct[rec.tag] = rec.momentum_pct;
    }
    // Drive the UPJUMP clip companion for this leg (if registered). Reads the
    // parent's settled position only — never modifies it. Additive standalone book.
    {
        std::lock_guard<std::mutex> lk(g_companion_mtx);
        auto it = g_companion_by_parent.find(rec.tag);
        if (it != g_companion_by_parent.end()) {
            chimera::EdgeEngine* par = it->second.first;
            it->second.second->observe(par->in_position(), par->entry_px(), rec.c, rec.open_ts_ms);
        }
        // Re-emit the full live roster snapshot for the Omega desk panel (every
        // bar close, any leg). Lock already held — iterates all registered legs.
        if (!g_companion_by_parent.empty()) {
            emit_companion_state();
            save_companion_det_state();   // S-2026-07-08: det window survives restarts
        }
    }
}

// Load saved bars for warm-start. Returns a vector of SeedBar sorted by open_ts_ms.
// Reads from data/bars/{tag}.ndjson, keeping only the most recent max_bars bars.
static std::vector<chimera::EdgeEngine::SeedBar> load_saved_bars(const std::string& tag, int max_bars = 64) {
    std::vector<chimera::EdgeEngine::SeedBar> result;
    std::string path = bar_file_for_tag(tag);
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return result;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        std::string s(line);
        // Minimal JSON parse — extract o, h, l, c, open_ts_ms
        auto extract_num = [&](const char* key) -> double {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0.0;
            pos += strlen(key) + 3;
            return std::stod(s.substr(pos));
        };
        auto extract_int64 = [&](const char* key) -> int64_t {
            auto pos = s.find(std::string("\"") + key + "\":");
            if (pos == std::string::npos) return 0;
            pos += strlen(key) + 3;
            return std::stoll(s.substr(pos));
        };

        chimera::EdgeEngine::SeedBar sb;
        sb.open_ts_ms = extract_int64("open_ts_ms");
        sb.o = extract_num("o");
        sb.h = extract_num("h");
        sb.l = extract_num("l");
        sb.c = extract_num("c");
        if (sb.open_ts_ms > 0 && sb.o > 0.0 && sb.c > 0.0) {
            result.push_back(sb);
        }
    }
    fclose(f);

    // Keep only the most recent max_bars (already in chronological order)
    if ((int)result.size() > max_bars) {
        result.erase(result.begin(), result.begin() + ((int)result.size() - max_bars));
    }

    std::printf("[WARM_START][%s] Loaded %d saved bars from %s\n",
        tag.c_str(), (int)result.size(), path.c_str());
    std::fflush(stdout);
    return result;
}

// Build the structured JSON for /api/state2.
// Extends engine state_json with backtest metadata from EngineSlot.
static std::string build_state_json() {
    std::ostringstream js;
    js << "{\"build\":\"" << BUILD_VERSION << "\",";
    js << "\"startup_ts\":" << g_startup_ts_ms << ",";
    js << "\"engine_count\":" << g_all_wired.size() << ",";
    js << "\"slot_count\":" << g_slots.size() << ",";
    js << "\"protection_disabled_for_testing\":"
       << (g_protection_disabled_for_testing.load() ? "true" : "false") << ",";
    // S44d alerts queue
    {
        std::lock_guard<std::mutex> lk(g_alerts_mtx);
        js << "\"alerts\":[";
        for (size_t i = 0; i < g_alerts.size(); ++i) {
            if (i > 0) js << ",";
            js << g_alerts[i];
        }
        js << "],";
    }

    // ── S33: portfolio gate state (drives GUI banner) ────────────────────
    js << "\"portfolio_gate\":{";
    js << "\"gate_open\":" << (g_gate_open.load() ? "true" : "false") << ",";
    js << "\"ratchet_locked\":" << (g_ratchet_locked.load() ? "true" : "false") << ",";
    js << "\"streak_halted\":" << (g_streak_halted.load() ? "true" : "false") << ",";
    js << std::fixed << std::setprecision(1);
    js << "\"session_cum_bp\":" << load_dbl_atomic(g_session_cum_bp_bits) << ",";
    js << "\"session_peak_bp\":" << load_dbl_atomic(g_session_peak_bp_bits) << ",";
    js << "\"recent_pnl_bp\":" << load_dbl_atomic(g_recent_pnl_bp_bits) << ",";
    js << "\"unrealized_bp\":" << load_dbl_atomic(g_unrealized_bp_bits) << ",";
    js << "\"open_positions\":" << g_open_positions_count.load() << ",";
    js << "\"last_trade_exit_ms\":" << g_last_trade_exit_ms.load() << ",";
    // S34: multi-tier protection state
    js << "\"all_time_peak_bp\":" << load_dbl_atomic(g_all_time_peak_bp_bits) << ",";
    js << "\"all_time_cum_bp\":" << load_dbl_atomic(g_all_time_cum_bp_bits) << ",";
    js << "\"daily_pnl_bp\":" << load_dbl_atomic(g_daily_pnl_bp_bits) << ",";
    js << "\"daily_kill_until_ms\":" << g_daily_kill_until_ms.load() << ",";
    js << std::setprecision(2);
    js << "\"size_throttle\":" << load_dbl_atomic(g_size_throttle_bits) << ",";
    js << std::setprecision(1);
    js << "\"tsmom_open\":" << g_tsmom_open_count.load() << ",";
    js << std::setprecision(2);
    js << "\"btc_short_ret_pct\":" << load_dbl_atomic(g_btc_short_ret_bits) << ",";
    js << std::setprecision(1);
    // S34: PF filter counts + tier breakdown
    {
        int blocked = 0, active = 0, elite = 0;
        for (auto& s : g_slots) {
            if (s.pf_blocked) blocked++;
            else if (s.bt_pf >= 2.0 && s.bt_trades >= 30) { active++; elite++; }
            else if (s.bt_pf >= S34_MIN_PF) active++;
        }
        js << "\"pf_min\":" << S34_MIN_PF << ",";
        js << "\"pf_blocked_count\":" << blocked << ",";
        js << "\"pf_active_count\":" << active << ",";
        js << "\"pf_elite_count\":" << elite << ",";
    }
    // S45: correlation-cluster exposure snapshot (read by GUI / monitor)
    {
        static constexpr const char* CLN[CL_COUNT] = {"majors","l1","defi","meme","other"};
        js << "\"cluster_max_per_symbol\":" << CLUSTER_MAX_PER_SYMBOL << ",";
        js << "\"cluster_max_per_cluster\":" << CLUSTER_MAX_PER_CLUSTER << ",";
        js << "\"cluster_open\":{";
        for (int c = 0; c < CL_COUNT; ++c) {
            js << "\"" << CLN[c] << "\":" << g_cluster_open_bucket[c];
            if (c + 1 < CL_COUNT) js << ",";
        }
        js << "},";
        // S45: which clusters are loss-halted (24h breaker) + bear-regime entry halt
        int64_t now_ck = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        js << "\"cluster_loss_halted\":[";
        { bool first = true;
          for (int c = 0; c < CL_COUNT; ++c)
            if (now_ck < g_cluster_blocked_until_ms[c].load()) {
                if (!first) js << ","; js << "\"" << CLN[c] << "\""; first = false;
            } }
        js << "],";
        js << "\"regime_entry_halt\":"
           << ((g_regime.load() < REGIME_MIN_FOR_ENTRY) ? "true" : "false") << ",";
    }
    {
        int r = g_regime.load();
        const char* rs = (r == 0) ? "CRASH"
                       : (r == 1) ? "BEAR"
                       : (r == 2) ? "BULL_CHOP"
                       : "BULL_TREND";
        js << "\"regime\":\"" << rs << "\"";
        js << ",\"macro_bull\":" << (g_macro_bull.load() ? "true" : "false");
        js << ",\"btc_200dma\":" << (int64_t)load_dbl_atomic(g_btc_200dma_bits);
    }
    js << "},";

    // S55: grid sleeve summary (shadow maker market-making)
    {
        double gsum = 0; int gfills = 0, glots = 0;
        js << "\"grid_sleeve\":{\"grids\":[";
        bool first = true;
        for (auto* gr : g_grids) {
            if (!first) js << ",";
            js << "{\"tag\":\"" << gr->cfg().tag << "\",\"realized_bp\":" << (int64_t)gr->total_bp()
               << ",\"fills\":" << gr->fills() << ",\"open_lots\":" << gr->open_lots() << "}";
            gsum += gr->total_bp(); gfills += gr->fills(); glots += gr->open_lots();
            first = false;
        }
        js << "],\"total_realized_bp\":" << (int64_t)gsum << ",\"total_fills\":" << gfills
           << ",\"total_open_lots\":" << glots << "},";
    }
    js << "\"idle_yield\":{\"apy\":" << IDLE_YIELD_APY
       << ",\"accrued_bp\":" << (int64_t)load_dbl_atomic(g_yield_cum_bp_bits) << "},";
    if (g_macro_base) {
        js << "\"macro_base\":{\"invested\":" << (g_macro_base->invested() ? "true" : "false")
           << ",\"dd_locked\":" << (g_macro_base->dd_locked() ? "true" : "false")
           << ",\"nav\":" << g_macro_base->nav()
           << ",\"peak\":" << g_macro_base->peak()
           << ",\"ret_bp\":" << (int64_t)g_macro_base->ret_bp()
           << ",\"flips\":" << g_macro_base->flips() << "},";
    }

    // ── AUDIT-2026 portfolio overlay (xsec mom + vol scaling) ───────────────
    g_portfolio_overlay.to_json(js);
    js << ",";

    // ── spot_prices ─────────────────────────────────────────────────────────
    js << "\"spot_prices\":{";
    js << std::fixed << std::setprecision(8);
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
        if (i > 0) js << ",";
        double px = load_dbl_atomic(g_last_spot_px_bits[i]);
        js << "\"" << chimera::sym_full(i) << "\":" << px;
    }
    js << "},";

    // ── engines — merge state_json() with slot metadata (g_slots first) ─────
    // S44b: also include S43/S43b engines that are wired via wire_engine
    // but not in g_slots, so the dashboard table shows every live engine.
    js << "\"engines\":[";
    bool first = true;
    // GUI FIX: emit ONLY the live wired engines (g_all_wired = active roster
    // after dedup + cull). Inert culled g_slots are NOT in g_all_wired, so they
    // no longer pollute the table or the count. Each engine carries its gated
    // fine-fill ranking (tier/PF/Sharpe/rank) so the dashboard sorts best->worst.
    for (auto* e : g_all_wired) {
        if (!e) continue;
        if (!first) js << ",";
        first = false;
        std::string ej = e->state_json();
        std::ostringstream m;
        m << std::fixed << std::setprecision(2);
        auto rit = g_engine_rank.find(e->cfg().tag);
        if (rit != g_engine_rank.end()) {
            m << ",\"tier\":\"" << rit->second.tier << "\"";
            m << ",\"bt_pf\":"     << rit->second.pf;
            m << ",\"bt_sharpe\":" << rit->second.sharpe;
            m << ",\"rank\":"      << rit->second.rank;
        } else {
            m << ",\"tier\":\"\",\"bt_pf\":0,\"bt_sharpe\":0,\"rank\":9999";
        }
        int sid = chimera::symbol_to_id(e->cfg().symbol);
        if (sid >= 0 && sid < chimera::MAX_SYMBOLS) {
            int sr = g_sym_regime[sid].load();
            const char* srs = (sr == 0) ? "CRASH" : (sr == 1) ? "BEAR" : (sr == 2) ? "BULL_CHOP" : "BULL_TREND";
            m << ",\"sym_regime\":\"" << srs << "\"";
        }
        std::string meta = m.str();
        if (!ej.empty() && ej.back() == '}') { ej.pop_back(); ej += meta + "}"; }
        js << ej;
    }
    js << "]}";
    return js.str();
}

static void http_server_thread(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
    listen(server_fd, 16);
    std::printf("[HTTP] GUI on port %d | root: %s\n", port, gui_root.c_str());
    std::fflush(stdout);

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) break;
        struct timeval tv{2, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char req[512] = {};
        if (read(client, req, sizeof(req)-1) < 0) { /* ignore */ }

        std::string body;
        const char* ct = "application/json";
        int status = 200;

        if (strstr(req, "GET /api/bars/")) {
            // /api/bars/{TAG} — return last 100 saved bars for an engine
            const char* tag_start = strstr(req, "/api/bars/") + 10;
            char tag_buf[128] = {};
            int ti = 0;
            while (tag_start[ti] && tag_start[ti] != ' ' && tag_start[ti] != '?' && ti < 127) {
                tag_buf[ti] = tag_start[ti]; ti++;
            }
            std::string tag_str(tag_buf, ti);
            std::string bar_path = bar_file_for_tag(tag_str);
            // Read last 100 lines from the file
            std::vector<std::string> lines;
            FILE* bf = fopen(bar_path.c_str(), "r");
            if (bf) {
                char bline[1024];
                while (fgets(bline, sizeof(bline), bf)) {
                    lines.push_back(std::string(bline));
                    if (lines.size() > 100) lines.erase(lines.begin());
                }
                fclose(bf);
            }
            std::ostringstream bjs;
            bjs << "[";
            for (size_t bi = 0; bi < lines.size(); ++bi) {
                if (bi > 0) bjs << ",";
                // Each line is already a JSON object, just strip newline
                std::string& l = lines[bi];
                while (!l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
                bjs << l;
            }
            bjs << "]";
            body = bjs.str();
        } else if (strstr(req, "GET /api/positions")) {
            // /api/positions — return only engines currently holding a position
            // with full details: tag, symbol, strategy, timeframe, entry price,
            // stop-loss, trailing stop, unrealised P&L, bars held, MFE
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            std::ostringstream pjs;
            pjs << std::fixed << std::setprecision(6);
            pjs << "{\"positions\":[";
            int count = 0;
            for (size_t i = 0; i < g_slots.size(); ++i) {
                if (!g_slots[i].engine || !g_slots[i].engine->in_position()) continue;
                if (count > 0) pjs << ",";
                // Get spot price for unrealised P&L
                double spot = load_dbl_atomic(g_last_spot_px_bits[g_slots[i].symbol_id]);
                double entry = 0.0;
                // Parse entry_px from state_json (engine has no public accessor)
                std::string ej = g_slots[i].engine->state_json();
                // Extract entry_px
                auto extract = [&](const char* key) -> double {
                    auto pos = ej.find(std::string("\"") + key + "\":");
                    if (pos == std::string::npos) return 0.0;
                    pos += strlen(key) + 3;
                    return std::stod(ej.substr(pos));
                };
                entry = extract("entry_px");
                double sl = extract("sl_px");
                double trail_stop = extract("trail_stop_px");
                double mfe_bp = extract("mfe_bp");
                int bars_held = (int)extract("bars_held");
                bool trail_armed = (ej.find("\"trail_armed\":true") != std::string::npos);
                double unreal_bp = (spot > 0.0 && entry > 0.0)
                    ? (spot / entry - 1.0) * 1e4 : 0.0;

                pjs << "{";
                pjs << "\"tag\":\"" << g_slots[i].tag << "\",";
                pjs << "\"symbol\":\"" << g_slots[i].symbol_str << "\",";
                pjs << "\"tf_secs\":" << g_slots[i].tf_secs << ",";
                pjs << "\"tf_human\":\"";
                if (g_slots[i].tf_secs >= 86400) pjs << (g_slots[i].tf_secs/86400) << "D";
                else pjs << (g_slots[i].tf_secs/3600) << "H";
                pjs << "\",";
                pjs << "\"entry_px\":" << entry << ",";
                pjs << "\"spot_px\":" << spot << ",";
                pjs << "\"sl_px\":" << sl << ",";
                pjs << "\"trail_stop_px\":" << trail_stop << ",";
                pjs << "\"trail_armed\":" << (trail_armed ? "true" : "false") << ",";
                pjs << std::setprecision(2);
                pjs << "\"unreal_bp\":" << unreal_bp << ",";
                pjs << "\"mfe_bp\":" << mfe_bp << ",";
                pjs << "\"bars_held\":" << bars_held << ",";
                pjs << std::setprecision(2);
                pjs << "\"oos_pf\":" << g_slots[i].oos_pf << ",";
                pjs << "\"oos_sharpe\":" << g_slots[i].oos_sharpe << ",";
                pjs << "\"session\":" << g_slots[i].session;
                pjs << std::setprecision(6);
                pjs << "}";
                count++;
            }
            pjs << "],\"count\":" << count << "}";
            body = pjs.str();
        } else if (strstr(req, "GET /api/trades")) {
            body = build_trades_json();
        } else if (strstr(req, "POST /api/kill")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            for (auto& s : g_slots) {
                if (!s.engine) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                s.engine->kill_all(px, now_ms);
            }
            body = "{\"ok\":true}";
        } else if (strstr(req, "POST /api/ratchet_reset")) {
            // S34: manual peak reset. Sets all_time_peak = all_time_cum,
            // unlocks ratchet, clears daily_kill. Use after absorbing a DD
            // to allow the bot to start fresh. PER will re-arm on new gains.
            double cum = load_dbl_atomic(g_all_time_cum_bp_bits);
            double old_peak = load_dbl_atomic(g_all_time_peak_bp_bits);
            store_dbl_atomic(g_all_time_peak_bp_bits, cum);
            g_daily_kill_until_ms.store(0, std::memory_order_relaxed);
            save_protection_state();
            std::printf("[PROTECTION] MANUAL RESET via /api/ratchet_reset: peak %.1fbp -> %.1fbp; daily_kill cleared\n",
                old_peak, cum);
            std::fflush(stdout);
            std::ostringstream rj;
            rj << "{\"ok\":true,\"old_peak_bp\":" << old_peak
               << ",\"new_peak_bp\":" << cum
               << ",\"all_time_cum_bp\":" << cum << "}";
            body = rj.str();
        } else if (strstr(req, "GET /api/btc_chart")) {
            // S34: BTC tick history for live dashboard chart
            std::ostringstream cj;
            cj << std::fixed << std::setprecision(2);
            cj << "{\"prices\":[";
            int filled = g_btc_chart_filled.load();
            int idx = g_btc_chart_idx.load();
            int start = (filled < BTC_CHART_N) ? 0 : idx;
            bool first = true;
            for (int k = 0; k < filled; k++) {
                int i = (start + k) % BTC_CHART_N;
                double px = load_dbl_atomic(g_btc_chart_px[i]);
                if (!first) cj << ",";
                cj << px;
                first = false;
            }
            cj << "]}";
            body = cj.str();
        } else if (strstr(req, "POST /api/daily_kill_clear")) {
            // S34: clear just the 24h daily-loss halt (keep PER active)
            int64_t old = g_daily_kill_until_ms.load(std::memory_order_relaxed);
            g_daily_kill_until_ms.store(0, std::memory_order_relaxed);
            save_protection_state();
            std::printf("[PROTECTION] daily_kill cleared via API (was until %lld)\n", (long long)old);
            std::fflush(stdout);
            body = "{\"ok\":true}";
        } else if (strstr(req, "POST /api/session_reset")) {
            // S54 WARM RESET: clear every HALT (daily-kill / cluster / symbol /
            // SL-circuit / emergency / ratchet-lock) so trading resumes, while
            // KEEPING the book warm — real lifetime equity + peak are preserved so
            // the drawdown-throttle and peak ratchet stay calibrated to reality,
            // never restarting from 0 (arms full size + re-opens bleed room) or a
            // phantom peak (which the ratchet then defends and locks the gate).
            // The epoch makes only the rolling 24h RISK windows forget pre-epoch
            // trades (so a fired daily/cluster cap doesn't instantly re-trigger);
            // lifetime cum is rebased onto its OWN current value -> continuous.
            // Trade ledger (history) never modified.
            int64_t now = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            double cur_cum  = load_dbl_atomic(g_all_time_cum_bp_bits);
            double cur_peak = load_dbl_atomic(g_all_time_peak_bp_bits);
            // Optional ?cum=<bp> sets a deliberate warm baseline (peak>=cum so no
            // phantom giveback). Default = keep current real equity (warm).
            if (const char* q = strstr(req, "cum=")) {
                cur_cum = std::atof(q + 4);
                if (cur_peak < cur_cum) cur_peak = cur_cum;
            }
            g_pnl_epoch_ms.store(now, std::memory_order_relaxed);
            store_dbl_atomic(g_pnl_epoch_baseline_bp_bits, cur_cum); // rebase -> recompute stays warm+continuous
            store_dbl_atomic(g_all_time_cum_bp_bits,  cur_cum);
            store_dbl_atomic(g_all_time_peak_bp_bits, cur_peak);     // KEEP real peak (warm)
            store_dbl_atomic(g_session_cum_bp_bits,   cur_cum);
            store_dbl_atomic(g_session_peak_bp_bits,  cur_peak);
            g_daily_kill_until_ms.store(0, std::memory_order_relaxed);
            g_emergency_halt_until_ms.store(0, std::memory_order_relaxed);
            for (int c = 0; c < CL_COUNT; ++c) g_cluster_blocked_until_ms[c].store(0, std::memory_order_relaxed);
            for (int s = 0; s < chimera::MAX_SYMBOLS; ++s) {
                g_sym_daily_blocked_until_ms[s].store(0, std::memory_order_relaxed);
                g_sym_sl_circuit_blocked_until_ms[s].store(0, std::memory_order_relaxed);
            }
            g_force_ratchet_unlock.store(true, std::memory_order_relaxed);
            save_protection_state();
            std::printf("[PROTECTION] WARM SESSION_RESET: epoch=%lld warm_cum=%.1fbp peak=%.1fbp — halts cleared, equity kept warm\n",
                (long long)now, cur_cum, cur_peak);
            std::fflush(stdout);
            body = "{\"ok\":true,\"epoch_ms\":" + std::to_string(now) +
                   ",\"warm_cum_bp\":" + std::to_string(cur_cum) + "}";
        } else if (strstr(req, "GET /api/state2") || strstr(req, "GET /api/state")) {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            body = build_state_json();
        } else if (strstr(req, "GET /app.js")) {
            ct = "application/javascript";
            body = read_file(gui_root + "/app.js");
            if (body.empty()) { status = 404; body = "not found"; }
        } else if (strstr(req, "GET /style.css")) {
            ct = "text/css";
            body = read_file(gui_root + "/style.css");
            if (body.empty()) { status = 404; body = "not found"; }
        } else if (strstr(req, "GET /favicon.svg")) {
            ct = "image/svg+xml";
            body = read_file(gui_root + "/favicon.svg");
            if (body.empty()) { status = 404; body = "not found"; }
        } else {
            ct = "text/html";
            body = read_file(gui_root + "/index.html");
            if (body.empty()) { status = 404; body = "index.html not found"; }
        }

        std::ostringstream resp;
        resp << "HTTP/1.1 " << status << " OK\r\n"
             << "Content-Type: " << ct << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Cache-Control: no-cache\r\n"
             << "Connection: close\r\n\r\n"
             << body;
        auto s = resp.str();
        // Write the whole response, retrying on partial writes.
        size_t sent = 0;
        const char* p = s.c_str();
        size_t remaining = s.size();
        while (remaining > 0) {
            ssize_t n = write(client, p + sent, remaining);
            if (n <= 0) break;
            sent     += (size_t)n;
            remaining -= (size_t)n;
        }
        // S39: graceful close — half-close write side, drain client bytes,
        // then close. Prevents RST-on-close (which nginx sees as 104
        // "Connection reset by peer while reading upstream").
        shutdown(client, SHUT_WR);
        char drain[1024];
        for (int i = 0; i < 8; ++i) {
            ssize_t n = read(client, drain, sizeof(drain));
            if (n <= 0) break;
        }
        close(client);
    }
    close(server_fd);
}

// ── Signal handler ────────────────────────────────────────────────────────────
static std::atomic<bool> g_running{true};
void signal_handler(int) { g_running = false; }

// ── LiveRuntimeConfig — reads config/live_config.json so the runtime is
// actually driven by that file instead of the hard-coded credentials path.
// ───────────────────────────────────────────────────────────────────────
struct LiveRuntimeConfig {
    std::string credentials_file = "config/binance_credentials.json";
    bool        shadow_mode      = true;
    double      max_position_usd = 10000.0;
    double      min_edge_bps     = 10.0;
    // S44d: testing-mode protection bypass. When TRUE + shadow_mode=true,
    // the portfolio ratchet does NOT halt entries (lets testing flow
    // through). Going live (shadow_mode=false) with this set = FATAL abort.
    bool        protection_disabled_for_testing = false;
};

static std::string lrc_extract_string(const std::string& s, const char* key) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return "";
    pos = s.find('"', pos + needle.size());
    if (pos == std::string::npos) return "";
    auto end = s.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return s.substr(pos + 1, end - pos - 1);
}

static double lrc_extract_double(const std::string& s, const char* key, double fallback) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return fallback;
    pos += needle.size();
    try { return std::stod(s.substr(pos)); } catch (...) { return fallback; }
}

static bool lrc_extract_bool(const std::string& s, const char* key, bool fallback) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = s.find(needle);
    if (pos == std::string::npos) return fallback;
    pos += needle.size();
    auto vstart = s.find_first_not_of(" \t\r\n", pos);
    if (vstart == std::string::npos) return fallback;
    if (s.compare(vstart, 4, "true")  == 0) return true;
    if (s.compare(vstart, 5, "false") == 0) return false;
    return fallback;
}

static LiveRuntimeConfig load_live_runtime_config(const std::string& path = "config/live_config.json") {
    LiveRuntimeConfig cfg;
    std::vector<std::string> candidates = {
        path,
        "../config/live_config.json",
        "../../config/live_config.json",
    };
    std::ifstream f;
    std::string opened;
    for (const auto& p : candidates) {
        f.open(p);
        if (f.is_open()) { opened = p; break; }
    }
    if (!f.is_open()) {
        std::fprintf(stderr, "[STARTUP] live_config.json not found — using defaults\n");
        return cfg;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    std::string creds = lrc_extract_string(content, "credentials_file");
    if (!creds.empty()) cfg.credentials_file = creds;
    cfg.shadow_mode      = lrc_extract_bool(content, "shadow_mode", true);
    cfg.max_position_usd = lrc_extract_double(content, "max_position_usd", 10000.0);
    cfg.min_edge_bps     = lrc_extract_double(content, "min_edge_bps", 10.0);
    cfg.protection_disabled_for_testing =
        lrc_extract_bool(content, "protection_disabled_for_testing", false);
    std::printf("[STARTUP] live_config loaded from %s: creds=%s shadow=%d max_pos=%.2f min_edge=%.2fbp protection_disabled=%d\n",
                opened.c_str(), cfg.credentials_file.c_str(),
                cfg.shadow_mode ? 1 : 0, cfg.max_position_usd, cfg.min_edge_bps,
                cfg.protection_disabled_for_testing ? 1 : 0);
    std::fflush(stdout);

    // S44d TRIP-SWITCH: going live with protection disabled = HARD ABORT.
    if (!cfg.shadow_mode && cfg.protection_disabled_for_testing) {
        std::fprintf(stderr,
            "\n\n"
            "==================================================================\n"
            " FATAL: shadow_mode=false (LIVE TRADING) but\n"
            "        protection_disabled_for_testing=true (PROTECTIONS BYPASSED).\n"
            "\n"
            " This combination would route real orders without ratchet/drawdown\n"
            " halts. Refusing to start.\n"
            "\n"
            " To go live: set protection_disabled_for_testing=false in\n"
            " config/live_config.json, OR remove the key entirely.\n"
            "==================================================================\n\n");
        std::exit(1);
    }
    return cfg;
}

// ── Map EdgeEngine tf_secs -> Binance kline interval string ────────────────
// Returns empty string if the timeframe doesn't map to a Binance interval.
static const char* tf_to_binance_interval(int64_t tf_secs) {
    switch (tf_secs) {
        case 60:     return "1m";
        case 180:    return "3m";
        case 300:    return "5m";
        case 900:    return "15m";
        case 1800:   return "30m";
        case 3600:   return "1h";
        case 7200:   return "2h";
        case 14400:  return "4h";
        case 21600:  return "6h";
        case 28800:  return "8h";
        case 43200:  return "12h";
        case 86400:  return "1d";
        case 259200: return "3d";
        case 604800: return "1w";
        default:     return "";
    }
}

// ── Seed one engine from REST klines (called once per engine at startup) ───
static void seed_engine_from_history(chimera::BinanceREST& rest,
                                     chimera::EdgeEngine& engine,
                                     const std::string& symbol,
                                     int64_t tf_secs,
                                     const std::string& tag,
                                     int limit = 64)
{
    const char* interval = tf_to_binance_interval(tf_secs);
    if (!interval || !*interval) {
        std::fprintf(stderr, "[SEED][%s] no Binance interval for tf_secs=%lld — skip\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    auto klines = rest.fetch_klines(symbol, interval, limit);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED][%s] fetch_klines returned 0 bars (symbol=%s interval=%s) — "
                              "engine will cold-start from live ticks\n",
                     tag.c_str(), symbol.c_str(), interval);
        return;
    }

    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(klines.size());
    for (const auto& k : klines) {
        chimera::EdgeEngine::SeedBar b{};
        b.open_ts_ms = k.open_ts_ms;
        b.o = k.o; b.h = k.h; b.l = k.l; b.c = k.c;
        seed.push_back(b);
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED][%s] symbol=%s interval=%s fetched=%d kept=%d\n",
                tag.c_str(), symbol.c_str(), interval, (int)klines.size(), kept);
    std::fflush(stdout);
}

// S54: seed the BTC 200d-MA macro gate from REST daily closes at startup.
static void init_macro_ma(chimera::BinanceREST& rest) {
    auto kl = rest.fetch_klines("btcusdt", "1d", MACRO_MA_DAYS + 10);
    if ((int)kl.size() < MACRO_MA_DAYS) {
        std::printf("[MACRO] only %d daily bars (<%d) — macro gate stays FLAT until warm\n",
                    (int)kl.size(), MACRO_MA_DAYS);
        std::fflush(stdout);
        return;
    }
    int start = (int)kl.size() - MACRO_MA_DAYS;
    g_btc_daily_head = 0; g_btc_daily_n = 0;
    double sum = 0.0;
    for (int i = start; i < (int)kl.size(); ++i) {
        g_btc_daily_ring[g_btc_daily_head] = kl[i].c;
        g_btc_daily_head = (g_btc_daily_head + 1) % 256;
        g_btc_daily_n++;
        sum += kl[i].c;
    }
    double ma = sum / (double)MACRO_MA_DAYS;
    store_dbl_atomic(g_btc_200dma_bits, ma);
    g_macro_last_day.store(kl.back().open_ts_ms / 86400000LL, std::memory_order_relaxed);
    double last_close = kl.back().c;
    g_macro_bull.store(last_close > ma, std::memory_order_relaxed);
    std::printf("[MACRO] init: BTC 200d-MA=%.0f last_close=%.0f -> %s\n",
                ma, last_close, last_close > ma ? "BULL (longs ON)" : "BEAR (longs HALTED)");
    std::fflush(stdout);
}

// ── Seed one engine from D1 klines aggregated to target timeframe ─────────
// For engines whose bar period is >= 1 day but has no native Binance interval
// (D2 = 172800s), we fetch D1 klines and aggregate them. This gives far more
// history than H1 aggregation: 1000 D1 bars / 2 = 500 D2 bars (vs 20 from H1).
// Shadow tuning 2026-05-17: fixes 11 under-seeded D2 engines.
// ───────────────────────────────────────────────────────────────────────────
static void seed_engine_from_d1_aggregation(chimera::BinanceREST& rest,
                                            chimera::EdgeEngine& engine,
                                            const std::string& symbol,
                                            int64_t tf_secs,
                                            const std::string& tag,
                                            int target_bars = 64)
{
    int d1_per_target = (int)(tf_secs / 86400);
    if (d1_per_target < 2) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] tf_secs=%lld too small for D1 aggregation\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    int d1_needed = d1_per_target * target_bars;
    if (d1_needed > 1000) d1_needed = 1000;  // Binance hard limit

    auto klines = rest.fetch_klines(symbol, "1d", d1_needed);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] fetch_klines(1d) returned 0 — falling through\n",
                     tag.c_str());
        return;
    }

    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(target_bars);

    chimera::EdgeEngine::SeedBar cur{};
    int64_t cur_bar_id = -1;

    for (const auto& k : klines) {
        int64_t bar_id = (k.open_ts_ms / 1000) / tf_secs;

        if (bar_id != cur_bar_id) {
            if (cur_bar_id >= 0 && cur.o > 0.0) {
                seed.push_back(cur);
            }
            cur_bar_id     = bar_id;
            cur.open_ts_ms = bar_id * tf_secs * 1000;
            cur.o = k.o;
            cur.h = k.h;
            cur.l = k.l;
            cur.c = k.c;
        } else {
            if (k.h > cur.h) cur.h = k.h;
            if (k.l < cur.l) cur.l = k.l;
            cur.c = k.c;
        }
    }

    if (cur_bar_id >= 0 && cur.o > 0.0) {
        seed.push_back(cur);
    }

    if (seed.empty()) {
        std::fprintf(stderr, "[SEED-D1AGG][%s] aggregation produced 0 bars\n",
                     tag.c_str());
        return;
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED-D1AGG][%s] symbol=%s d1_fetched=%d -> aggregated=%d bars (tf=%llds) kept=%d\n",
                tag.c_str(), symbol.c_str(), (int)klines.size(),
                (int)seed.size(), (long long)tf_secs, kept);
    std::fflush(stdout);
}

// ── Seed one engine from H1 klines aggregated to target timeframe ─────────
// For engines whose timeframe has no native Binance interval (H3, H16),
// we fetch H1 klines (which Binance DOES provide) and aggregate them into
// the target timeframe. This eliminates cold-start entirely.
//
// E.g. for H3 (10800s): fetch 192 H1 bars → aggregate into 64 H3 bars.
// For H16 (57600s): fetch 1000 H1 bars → aggregate into 62 H16 bars.
// For D2 (172800s): fetch 1000 H1 bars → aggregate into 20 D2 bars.
// ───────────────────────────────────────────────────────────────────────────
static void seed_engine_from_h1_aggregation(chimera::BinanceREST& rest,
                                            chimera::EdgeEngine& engine,
                                            const std::string& symbol,
                                            int64_t tf_secs,
                                            const std::string& tag,
                                            int target_bars = 64)
{
    // How many H1 bars do we need to produce target_bars of the target TF?
    int h1_per_target = (int)(tf_secs / 3600);
    if (h1_per_target < 2) {
        std::fprintf(stderr, "[SEED-AGG][%s] tf_secs=%lld too small for H1 aggregation\n",
                     tag.c_str(), (long long)tf_secs);
        return;
    }

    int h1_needed = h1_per_target * target_bars;
    if (h1_needed > 1000) h1_needed = 1000;  // Binance hard limit

    auto klines = rest.fetch_klines(symbol, "1h", h1_needed);
    if (klines.empty()) {
        std::fprintf(stderr, "[SEED-AGG][%s] fetch_klines(1h) returned 0 — cold-start from ticks\n",
                     tag.c_str());
        return;
    }

    // Aggregate H1 bars into target timeframe bars.
    // Bar boundaries are aligned to epoch: bar_id = (open_ts_ms / 1000) / tf_secs
    std::vector<chimera::EdgeEngine::SeedBar> seed;
    seed.reserve(target_bars);

    chimera::EdgeEngine::SeedBar cur{};
    int64_t cur_bar_id = -1;

    for (const auto& k : klines) {
        int64_t bar_id = (k.open_ts_ms / 1000) / tf_secs;

        if (bar_id != cur_bar_id) {
            // Save the previous completed bar (if any)
            if (cur_bar_id >= 0 && cur.o > 0.0) {
                seed.push_back(cur);
            }
            // Start a new bar
            cur_bar_id     = bar_id;
            cur.open_ts_ms = bar_id * tf_secs * 1000;
            cur.o = k.o;
            cur.h = k.h;
            cur.l = k.l;
            cur.c = k.c;
        } else {
            // Extend the current bar
            if (k.h > cur.h) cur.h = k.h;
            if (k.l < cur.l) cur.l = k.l;
            cur.c = k.c;  // last close wins
        }
    }

    // Push the final bar (may be partially complete — that's OK).
    // The engine's on_tick() will extend or roll it forward correctly.
    // Including it is critical for D2 engines where 1000 H1 bars only
    // produces ~20 complete bars and lookback=20 needs 21 in the buffer.
    if (cur_bar_id >= 0 && cur.o > 0.0) {
        seed.push_back(cur);
    }

    if (seed.empty()) {
        std::fprintf(stderr, "[SEED-AGG][%s] aggregation produced 0 bars\n",
                     tag.c_str());
        return;
    }

    int kept = engine.seed_bars(seed);
    std::printf("[SEED-AGG][%s] symbol=%s h1_fetched=%d -> aggregated=%d bars (tf=%llds) kept=%d\n",
                tag.c_str(), symbol.c_str(), (int)klines.size(),
                (int)seed.size(), (long long)tf_secs, kept);
    std::fflush(stdout);
}

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    g_startup_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::printf("[STARTUP] Chimera — Tier-2 Edge Engines (285 active) | build=%s\n", BUILD_VERSION);
    std::fflush(stdout);

    acquire_instance_lock();
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ── AUDIT-2026: warm-start portfolio overlay BEFORE engine seeding (cheap,
    // CSV-only, no network). Runs first so its log isn't buried behind 30-60s
    // of seeding output and so multipliers are ready before first tick fires.
    g_portfolio_overlay.warm_start_from_csv("data/klines_spot");

    // Runtime config — drives credentials path + shadow_mode + position sizing.
    LiveRuntimeConfig runtime_cfg = load_live_runtime_config();

    // Executor — engines mirror entry/exit intents into this via on_order_intent.
    chimera::SpotExecutor executor;
    // GUI: load per-engine gated-backtest rankings for the dashboard table
    load_engine_rankings();
    // S44e: load tier map for per-engine lot sizing
    {
        std::ifstream tf("data/engine_tiers.json");
        if (tf.is_open()) {
            std::string content((std::istreambuf_iterator<char>(tf)),
                                 std::istreambuf_iterator<char>());
            tf.close();
            // Parse tiers (tag -> tier string)
            auto tpos = content.find("\"tiers\":");
            auto mpos = content.find("\"multipliers\":");
            if (tpos != std::string::npos && mpos != std::string::npos) {
                std::string tiers_blob = content.substr(tpos, mpos - tpos);
                // Match "TAG":"TIER"
                size_t p = 0;
                while ((p = tiers_blob.find('"', p)) != std::string::npos) {
                    size_t key_end = tiers_blob.find('"', p + 1);
                    if (key_end == std::string::npos) break;
                    std::string key = tiers_blob.substr(p + 1, key_end - p - 1);
                    if (key == "tiers") { p = key_end + 1; continue; }
                    size_t v_start = tiers_blob.find('"', key_end + 1);
                    size_t v_end   = (v_start != std::string::npos) ? tiers_blob.find('"', v_start + 1) : std::string::npos;
                    if (v_start == std::string::npos || v_end == std::string::npos) break;
                    std::string val = tiers_blob.substr(v_start + 1, v_end - v_start - 1);
                    g_engine_tier[key] = val;
                    p = v_end + 1;
                }
                // Parse multipliers + pyramid_max sections
                auto parse_section = [&](const std::string& key, std::map<std::string, double>* out_d,
                                         std::map<std::string, int>* out_i) {
                    auto sp = content.find("\"" + key + "\":");
                    if (sp == std::string::npos) return;
                    // Find end of section (next "...":{...} OR end of object)
                    auto end_brace = content.find('}', sp);
                    if (end_brace == std::string::npos) return;
                    std::string blob = content.substr(sp, end_brace - sp);
                    size_t q = 0;
                    while ((q = blob.find('"', q)) != std::string::npos) {
                        size_t k_end = blob.find('"', q + 1);
                        if (k_end == std::string::npos) break;
                        std::string mk = blob.substr(q + 1, k_end - q - 1);
                        if (mk == key) { q = k_end + 1; continue; }
                        size_t colon = blob.find(':', k_end);
                        if (colon == std::string::npos) break;
                        size_t num_start = blob.find_first_of("0123456789.-", colon);
                        if (num_start == std::string::npos) break;
                        try {
                            if (out_d) (*out_d)[mk] = std::stod(blob.substr(num_start));
                            if (out_i) (*out_i)[mk] = std::stoi(blob.substr(num_start));
                        } catch (...) {}
                        q = num_start + 1;
                    }
                };
                parse_section("multipliers", &g_tier_multiplier, nullptr);
                parse_section("pyramid_max", nullptr, &g_tier_pyramid_max);
            }
            std::printf("[STARTUP] tier map loaded: %d engines, %d tier multipliers\n",
                        (int)g_engine_tier.size(), (int)g_tier_multiplier.size());
            for (auto& [t,m] : g_tier_multiplier) {
                std::printf("  %-10s -> %.2fx\n", t.c_str(), m);
            }
            std::fflush(stdout);
        } else {
            std::fprintf(stderr, "[STARTUP] data/engine_tiers.json not found — all engines = 1.0x\n");
        }
    }

    // S44g: load per-symbol liquidity tier
    {
        std::ifstream lf("data/symbol_liquidity.json");
        if (lf.is_open()) {
            std::string content((std::istreambuf_iterator<char>(lf)),
                                 std::istreambuf_iterator<char>());
            lf.close();
            auto parse_section = [&](const std::string& key, std::map<std::string, std::string>* out_s,
                                     std::map<std::string, double>* out_d) {
                auto sp = content.find("\"" + key + "\":");
                if (sp == std::string::npos) return;
                auto end_brace = content.find('}', sp);
                if (end_brace == std::string::npos) return;
                std::string blob = content.substr(sp, end_brace - sp);
                size_t q = 0;
                while ((q = blob.find('"', q)) != std::string::npos) {
                    size_t k_end = blob.find('"', q + 1);
                    if (k_end == std::string::npos) break;
                    std::string mk = blob.substr(q + 1, k_end - q - 1);
                    if (mk == key) { q = k_end + 1; continue; }
                    size_t colon = blob.find(':', k_end);
                    if (colon == std::string::npos) break;
                    if (out_s) {
                        size_t v_start = blob.find('"', colon);
                        size_t v_end   = (v_start != std::string::npos) ? blob.find('"', v_start + 1) : std::string::npos;
                        if (v_start == std::string::npos || v_end == std::string::npos) break;
                        (*out_s)[mk] = blob.substr(v_start + 1, v_end - v_start - 1);
                        q = v_end + 1;
                    } else if (out_d) {
                        size_t num_start = blob.find_first_of("0123456789.-", colon);
                        if (num_start == std::string::npos) break;
                        try { (*out_d)[mk] = std::stod(blob.substr(num_start)); } catch (...) {}
                        q = num_start + 1;
                    }
                }
            };
            parse_section("liquidity",   &g_symbol_liq, nullptr);
            parse_section("multipliers", nullptr, &g_liq_multiplier);
            std::printf("[STARTUP] liquidity map loaded: %d symbols, %d tier multipliers\n",
                        (int)g_symbol_liq.size(), (int)g_liq_multiplier.size());
            for (auto& [t,m] : g_liq_multiplier) {
                int n = 0;
                for (auto& [k,v] : g_symbol_liq) if (v == t) n++;
                std::printf("  %-6s -> %.2fx  base=%.0f USD  syms=%d\n",
                            t.c_str(), m, m * 10000.0, n);
            }
            std::fflush(stdout);
        } else {
            std::fprintf(stderr, "[STARTUP] data/symbol_liquidity.json not found — all syms = 0.5x conservative MID\n");
        }
    }

    // S44d: publish testing-bypass flag to global so the gate logic sees it
    g_protection_disabled_for_testing.store(
        runtime_cfg.protection_disabled_for_testing, std::memory_order_relaxed);
    if (runtime_cfg.protection_disabled_for_testing) {
        std::printf("\n"
            "==================================================================\n"
            " WARNING: protection_disabled_for_testing=TRUE in live_config.json\n"
            " Ratchet halts are BYPASSED. Shadow mode trades flow freely.\n"
            " REMEMBER: set this back to false (or remove) before going live.\n"
            "==================================================================\n\n");
        std::fflush(stdout);
        push_alert("PROTECTION DISABLED for testing — ratchet halts bypassed");
    }
    bool exec_ok = executor.init(runtime_cfg.credentials_file);
    if (!exec_ok) {
        std::fprintf(stderr, "[STARTUP] WARNING: executor init failed — order intents will drop\n");
    }

    // ════════════════════════════════════════════════════════════════════
    // S-2026-06-18: CrossSectionalMomentumEngine — FIRST OOS-validated Chimera
    // edge, installed as a STANDALONE allocator (not a sizing tilt). Curated
    // QUALITY universe (ex-meme), lb30/top3/rebal14/inverse-vol, BTC>200d gate
    // -> cash. Faithful BT reproduces backtest/cross_sectional.py EXACTLY
    // (2021 +1596% / 2023 +324% / 2024 +159% / 2025-holdout +39%). SHADOW.
    // ════════════════════════════════════════════════════════════════════
    // DUAL-SLEEVE: BTC-gated (BTC-led regimes) + breadth-gated (alt-decoupled
    // bulls). Validated cross-regime (C++ faithful BT == Python). Broad 32-quality
    // universe (no overfit cull — culling drags BROKE the 2025 holdout). 60/40 split.
    chimera::XSecConfig xsec_btc_cfg; xsec_btc_cfg.gate_mode = 0;                                  // BTC>200d
    chimera::XSecConfig xsec_br_cfg;  xsec_br_cfg.gate_mode = 1; xsec_br_cfg.breadth_thresh = 0.65; // alts' own health
    chimera::CrossSectionalMomentumEngine xsec_btc(xsec_btc_cfg);
    chimera::CrossSectionalMomentumEngine xsec_br(xsec_br_cfg);
    {
        std::vector<std::string> xu = {
            "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
            "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
            "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"};
        xsec_btc.set_universe(xu); xsec_br.set_universe(xu);
        int xseeded = 0;
        for (const auto& s : xu) {
            std::ifstream xf("data/xsec_seed/" + s + "USDT_1d.csv");
            if (!xf) continue;
            std::string xl; std::getline(xf, xl);  // header
            int xr = 0;
            while (std::getline(xf, xl)) {
                const char* p = xl.c_str(); char* e;
                long long ts = strtoll(p, &e, 10); if (*e != ',') continue;
                const char* q = e + 1;
                for (int k = 0; k < 3 && q; ++k) { q = std::strchr(q, ','); if (q) ++q; }
                if (!q) continue;
                double c = strtod(q, nullptr);
                if (c > 0) { long long d = ts / 86400000LL;
                    xsec_btc.seed_daily_close(s, d, c); xsec_br.seed_daily_close(s, d, c); ++xr; }
            }
            if (xr > 0) ++xseeded;
        }
        double nav = runtime_cfg.max_position_usd > 0.0 ? runtime_cfg.max_position_usd : 10000.0;
        double nav_btc = nav * 0.6, nav_br = nav * 0.4;
        std::printf("[XSEC] dual-sleeve installed: warm-seeded %d/%zu symbols, %zu days; lb30/top3/rebal14d/inv-vol | "
                    "BTC-sleeve nav=$%.0f + BREADTH(>=0.65)-sleeve nav=$%.0f SHADOW\n",
                    xseeded, xu.size(), xsec_btc.num_days(), nav_btc, nav_br);
        auto fmt = [](const std::map<std::string,double>& w){ std::string p;
            for (const auto& kv : w) if (kv.second > 0.0) { char z[48];
                std::snprintf(z, sizeof z, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); p += z; }
            return p.empty() ? std::string("(none)") : p; };
        { size_t i_btc = xsec_btc.num_days()?xsec_btc.num_days()-1:0, i_br = xsec_br.num_days()?xsec_br.num_days()-1:0;
          bool bb, br; auto wb = xsec_btc.compute_target_weights(i_btc, bb); auto wr = xsec_br.compute_target_weights(i_br, br);
          std::printf("[XSEC] startup: BTC-sleeve bull=%d %s | BREADTH-sleeve bull=%d %s\n",
                      bb?1:0, bb?fmt(wb).c_str():"CASH", br?1:0, br?fmt(wr).c_str():"CASH"); }
        std::fflush(stdout);
        auto fire = [&executor, &exec_ok](const char* tag, double sleeve_nav,
                    std::map<std::string,double>& hold, int64_t day,
                    const std::map<std::string,double>& tw, bool bull) {
            std::string picks; for (const auto& kv : tw) if (kv.second > 0.0) { char b[48];
                std::snprintf(b, sizeof b, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); picks += b; }
            std::printf("[%s] REBALANCE day=%lld bull=%d -> %s\n", tag, (long long)day, bull?1:0,
                        bull ? (picks.empty()?"(none)":picks.c_str()) : "CASH (gate)"); std::fflush(stdout);
            if (!exec_ok || !executor.is_ready()) return;
            for (const auto& kv : tw) {
                std::string lc = kv.first; for (auto& ch : lc) if (ch >= 'A' && ch <= 'Z') ch += 32;
                int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                double tgt = kv.second * sleeve_nav, dusd = tgt - hold[kv.first];
                if (std::fabs(dusd) < 25.0) continue;
                executor.execute(kv.first + "USDT", dusd > 0.0, std::fabs(dusd)/px, px); hold[kv.first] = tgt;
            }
        };
        xsec_btc.set_rebalance_callback([fire, nav_btc](int64_t d, const std::map<std::string,double>& tw, bool b){
            static std::map<std::string,double> hold; fire("XSEC-BTC", nav_btc, hold, d, tw, b); });
        xsec_br.set_rebalance_callback([fire, nav_br](int64_t d, const std::map<std::string,double>& tw, bool b){
            static std::map<std::string,double> hold; fire("XSEC-BR", nav_br, hold, d, tw, b); });
    }

    // ── Sleeve 3: RipRiderEngine — per-symbol regime-gated rip-rider ──────────
    // Catches individual coin RIPS, rides them (no tight trail — that amputates the
    // tail), exits on regime-flip (BTC<200d). Validated FAITHFUL (C++ BT == python
    // crypto_momo_rider.py): 2021 +18701 / 2023 +2038 / 2024 +214 / 2025 +1747 /
    // 2022+2026 flat. Per-symbol momentum-continuation, complements the XSec
    // portfolio sleeves. SHADOW. This is "trade the rips" done so it keeps the tail.
    chimera::RipRiderEngine riprider;  // defaults = validated (ig20%/5d/gate+regime-exit/maxhold60/pure-ride)
    {
        std::vector<std::string> ru = {
            "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
            "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
            "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"};
        riprider.set_universe(ru);
        int rseeded = 0;
        for (const auto& s : ru) {
            std::ifstream rf("data/xsec_seed/" + s + "USDT_1d.csv");
            if (!rf) continue;
            std::string rl; std::getline(rf, rl);  // header
            int rr = 0;
            while (std::getline(rf, rl)) {
                char* p = rl.data(); char* e; long long ts = strtoll(p, &e, 10); if (*e != ',') continue;
                double op = strtod(e+1, nullptr);                                  // col1 = open
                const char* q = e + 1; for (int k = 0; k < 3 && q; ++k) { q = std::strchr(q, ','); if (q) ++q; }
                if (!q) continue; double c = strtod(q, nullptr);                    // col4 = close
                if (c > 0 && op > 0) { riprider.seed_daily_bar(s, ts / 86400000LL, op, c); ++rr; }
            }
            if (rr > 0) ++rseeded;
        }
        double rip_nav = (runtime_cfg.max_position_usd > 0.0 ? runtime_cfg.max_position_usd : 10000.0);
        std::printf("[RIP] rip-rider installed: warm-seeded %d/%zu symbols, %zu days; "
                    "ig20%%/5d BTC>200d-gate + regime-exit, pure-ride maxhold60 SHADOW\n",
                    rseeded, ru.size(), riprider.num_days());
        std::fflush(stdout);
        riprider.set_entry_callback([&executor, &exec_ok, rip_nav](const std::string& sym, double px, int64_t ts){
            (void)ts;
            std::printf("[RIP] ENTRY %s @ %.6f\n", sym.c_str(), px); std::fflush(stdout);
            if (exec_ok && executor.is_ready() && px > 0) executor.execute(sym + "USDT", true, (rip_nav/8.0)/px, px);
        });
        riprider.set_close_callback([&executor, &exec_ok, rip_nav](const chimera::RipClose& t){
            double ret = t.entryPrice>0 ? (t.exitPrice/t.entryPrice-1)*100.0 : 0.0;
            std::printf("[RIP] EXIT %s entry=%.6f exit=%.6f ret=%+.1f%% %s\n",
                        t.symbol.c_str(), t.entryPrice, t.exitPrice, ret, t.exitReason.c_str()); std::fflush(stdout);
            if (exec_ok && executor.is_ready() && t.exitPrice > 0) executor.execute(t.symbol + "USDT", false, (rip_nav/8.0)/t.exitPrice, t.exitPrice);
        });
    }

    // wire_engine — single helper applied to every engine. Sets shadow_mode
    // from runtime config, on_trade for dashboard history, on_bar for
    // warm-start persistence, and on_order_intent to mirror entries/exits
    // into SpotExecutor (shadow mode -> signed-but-not-posted log).
    auto wire_engine = [&](chimera::EdgeEngine& engine) {
        // S-2026-06-18: the 285 per-symbol EdgeEngines have NO validated cross-cycle
        // edge (live = 0 positions). CULLED — replaced by the validated cross-sectional
        // momentum allocator (g_xsec, installed below). Restore via CHIMERA_WIRE_LEGACY=1.
        if (!std::getenv("CHIMERA_WIRE_LEGACY")) return;
        // S46 dedup: the same tag appears in BOTH engines_s43_repromote.cpp and
        // engines_s43b_holdout.cpp (repromoted + re-discovered), so 20 tags were
        // wired twice = redundant concurrent exposure on one signal, wasting the
        // per-symbol cap. Keep the first-wired (repromote is #included first =
        // stricter WF validation) and skip the later duplicate.
        static std::set<std::string> wired_tags;
        if (!wired_tags.insert(engine.cfg().tag).second) {
            std::printf("[DEDUP] skipped duplicate engine tag=%s\n", engine.cfg().tag.c_str());
            return;
        }
        // S47 cull: skip engines below profit on the fine-fill (real H1 intrabar)
        // backtest. Loaded once from config/culled_engines.txt. Reversible.
        static std::set<std::string> culled_tags = []{
            std::set<std::string> s; std::ifstream cf("config/culled_engines.txt");
            std::string ln;
            while (std::getline(cf, ln)) {
                if (!ln.empty() && ln[0] != '#') {
                    ln.erase(0, ln.find_first_not_of(" \t"));
                    ln.erase(ln.find_last_not_of(" \t\r\n") + 1);
                    if (!ln.empty()) s.insert(ln);
                }
            }
            std::printf("[CULL] loaded %zu culled engine tags\n", s.size());
            return s;
        }();
        if (culled_tags.count(engine.cfg().tag)) {
            std::printf("[CULL] skipped below-profit engine tag=%s\n", engine.cfg().tag.c_str());
            return;
        }
        engine.shadow_mode = runtime_cfg.shadow_mode;
        // S44: pyramid_elite for ALL wired engines (incl S43/S43b includes
        // which aren't in g_slots). Validated +2.8% portfolio bp.
        engine.enable_pyramid_elite();
        // S44f: per-tier pyramid_max override
        int pmax = tier_pyramid_max_for_tag(engine.cfg().tag);
        engine.set_pyramid_max_adds(pmax);
        // S44h: wrap on_trade with per-engine SL cooldown. After SL, block
        // entries for 1 bar duration (tf_secs). Prevents immediate re-entry
        // into chop.
        chimera::EdgeEngine* engine_ptr = &engine;
        int64_t tf_ms = (int64_t)engine.cfg().tf_secs * 1000;
        engine.set_on_trade([engine_ptr, tf_ms](const chimera::EdgeEngine::TradeRecord& rec) {
            on_trade_callback(rec);
            if (rec.reason == "SL" || rec.reason == "EARLY_KILL") {
                int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                engine_enter_cooldown(engine_ptr, now_ms, tf_ms);
                // S44O: per-symbol post-SL cooldown — block all engines on
                // this symbol for 30 min. Catches chop fast.
                int sid = chimera::symbol_to_id(rec.symbol);
                if (sid >= 0 && sid < chimera::MAX_SYMBOLS) {
                    g_sym_post_sl_cooldown_until_ms[sid].store(
                        now_ms + SYM_POST_SL_COOLDOWN_MS);
                }
            }
        });
        engine.set_on_bar(on_bar_callback);
        // S44b: apply safety preset (staged BE-lock ratchet, destructive
        // layers disabled) + filters by strategy type. Same overlay set
        // that g_slots engines get — now applied uniformly.
        engine.apply_safety_preset();
        // S44M #5: signal_confirm=2 — require 2 consecutive bar closes in
        // trend direction. Sweep validated: +0.4% bp, -21% DD, bp/dd 5.50->6.96.
        // Safety preset sets this to 1; override to 2 here.
        engine.set_signal_confirm_bars(2);
        // S44k (A): tighter PPB — be_arm=25 (was 32), lock_pct=0.85 (was 0.75).
        // Sweep validated bp/dd 5.50 -> 5.60.
        engine.set_be_arm_bp(25.0);
        engine.set_ratchet_lock_pct(0.85);
        // S44j: per-trade HARD FLOOR at -170 bp.
        // Sweep -80 to -200 step 10 on 405-engine 180d showed -170 = bp/DD
        // peak (5.50). 83% of no-floor profit retained, 19% DD cut.
        // Caps the 30 May catastrophes (SEI -503 -> -170, etc) without
        // killing normal winners that dip 30-80bp before going green.
        engine.set_hard_floor_bp(-170.0);
        engine.enable_volume_gate(true);
        if (engine.is_trend_following()) {
            // S51: ADX chop filter DROPPED. Sweep (fine-fill + regime-gate, 365d)
            // showed ADX@25 cut ~40% of profit to save only ~15% DD -> ret/DD
            // fell 9.75 -> 7.66. Chop losses are already capped by the -170 floor
            // + ratchet; the edge is volume. Filtering chop discards marginal
            // winners too. ADX off = best risk-adjusted. (was: enable_adx_filter)
            engine.enable_adx_filter(false);
        } else {
            engine.enable_vol_filter(true);
            if (engine.cfg().tf_secs < 86400) {
                engine.enable_mtf_gate(true);
            }
        }
        if (engine.cfg().symbol != "btcusdt") {
            engine.enable_corr_filter(true);
        }
        // Dedup: some engines call wire_engine twice — don't double-count.
        auto it = std::find(g_all_wired.begin(), g_all_wired.end(), &engine);
        if (it == g_all_wired.end()) g_all_wired.push_back(&engine);
        engine.set_on_order_intent(
            [&](const chimera::EdgeEngine::OrderIntentRecord& intent) {
                if (!exec_ok || !executor.is_ready()) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] executor not ready tag=%s symbol=%s side=%s px=%.8f\n",
                        intent.tag.c_str(), intent.symbol.c_str(),
                        intent.is_buy ? "BUY" : "SELL", intent.ref_px);
                    return;
                }
                if (intent.ref_px <= 0.0 || runtime_cfg.max_position_usd <= 0.0) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] invalid sizing tag=%s px=%.8f max_pos=%.2f\n",
                        intent.tag.c_str(), intent.ref_px, runtime_cfg.max_position_usd);
                    return;
                }
                // S44e/f: composite sizing — tier × funding × confluence ×
                // counter-trend-crash carve-out. Each is multiplicative.
                double tier_mult = tier_mult_for_tag(intent.tag);

                // S44f #1: funding-rate bias. Negative funding = shorts pay
                // longs = squeeze tailwind for TSMOM longs. +20%/-20% modifier.
                double funding_mult = 1.0;
                int sym_id = chimera::symbol_to_id(intent.symbol);
                if (sym_id >= 0 && g_funding_filter.is_ready() && intent.is_buy) {
                    if (g_funding_filter.has_tailwind(sym_id))      funding_mult = 1.2;
                    else if (g_funding_filter.has_headwind(sym_id)) funding_mult = 0.8;
                }

                // S44f #3: confluence — if >=3 engines on same symbol same
                // direction fire within 15min window, boost first add 1.3x.
                int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                double confluence_mult = check_and_record_confluence(
                    intent.symbol, intent.is_buy, now_ms);

                // S44f #4: counter-trend crash carve-out. Global size_throttle
                // = 0 in CRASH/BEAR kills all entries. Counter-trend strats
                // (mean-reversion) actually benefit in those regimes — bypass
                // throttle and boost 1.3x.
                double crash_mult = 1.0;
                if (sym_id >= 0) {
                    int sym_reg = g_sym_regime[sym_id].load();
                    if ((sym_reg == 0 || sym_reg == 1) && !engine.is_trend_following()) {
                        crash_mult = 1.3;
                    }
                }

                // S44g: liquidity floor — caps THIN alts to 2k base (× edges).
                // Edge boosts still apply but on a smaller base, so even
                // TOP_ELITE × full stack on JTO stays under ~6.5k notional.
                double liq_mult = liq_mult_for_symbol(intent.symbol);
                double sym_base_usd = runtime_cfg.max_position_usd * liq_mult;
                double total_mult = tier_mult * funding_mult * confluence_mult * crash_mult;
                double qty = (sym_base_usd * total_mult) / intent.ref_px;
                // P1/S46: ENTRIES ONLY — apply DD-throttle x vol-overlay (was dead)
                // and the per-trade $-risk-budget clamp. Exits (sells) flatten the
                // held position and must NOT be shrunk, so leave them untouched.
                if (intent.is_buy) {
                    qty *= intent.risk_mult;
                    int _cl = symbol_cluster(chimera::symbol_to_id(intent.symbol));
                    // S47: tier-aware budget — trusted engines get a bigger ceiling
                    double max_notional = MAX_TRADE_RISK_USD * tier_risk_mult(intent.tag)
                                          / (CLUSTER_WORST_GAP_BP[_cl] / 1e4);
                    if (qty * intent.ref_px > max_notional) qty = max_notional / intent.ref_px;
                }
                auto _ti = g_engine_tier.find(intent.tag);
                const char* tier_str = (_ti != g_engine_tier.end()) ? _ti->second.c_str() : "UNKNOWN";
                auto _li = g_symbol_liq.find(intent.symbol);
                const char* liq_str = (_li != g_symbol_liq.end()) ? _li->second.c_str() : "UNKNOWN";
                std::printf("[ORDER-INTENT] tag=%s symbol=%s side=%s qty=%.8f px=%.4f tier=%s liq=%s "
                            "tier_mult=%.2fx liq_mult=%.2fx funding=%.2fx conflu=%.2fx crash=%.2fx total=%.2fx notional=%.0f\n",
                    intent.tag.c_str(), intent.symbol.c_str(),
                    intent.is_buy ? "BUY" : "SELL", qty, intent.ref_px,
                    tier_str, liq_str,
                    tier_mult, liq_mult, funding_mult, confluence_mult, crash_mult, total_mult,
                    qty * intent.ref_px);
                std::fflush(stdout);
                auto result = executor.execute(intent.symbol, intent.is_buy, qty, intent.ref_px);
                if (!result.ok) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] execute failed tag=%s symbol=%s err=%s\n",
                        intent.tag.c_str(), intent.symbol.c_str(), result.error.c_str());
                }
            });
        // ── Smart Pyramid callback: only fires after BE-lock armed + profit >= pyramid_arm_atr ─
        // EdgeEngine guarantees base trail_stop floors at entry+round_trip_bp before any add.
        // Pyramid add size = base_size * size_mult (default 50%). Shared trail stop covers all legs.
        engine.set_on_pyramid(
            [&](const std::string& tag, double price, double size_mult, int add_num) {
                if (!exec_ok || !executor.is_ready()) {
                    std::fprintf(stderr,
                        "[PYRAMID] executor not ready tag=%s add=%d px=%.8f\n",
                        tag.c_str(), add_num, price);
                    return;
                }
                if (price <= 0.0 || runtime_cfg.max_position_usd <= 0.0 || size_mult <= 0.0) {
                    std::fprintf(stderr,
                        "[PYRAMID] invalid sizing tag=%s px=%.8f size_mult=%.2f\n",
                        tag.c_str(), price, size_mult);
                    return;
                }
                // S44L D: regime-conditional pyramid. Only add in BULL_TREND.
                // CHOP/BEAR/CRASH regime -> skip pyramid add (whipsaw kills it).
                int sym_id_d = chimera::symbol_to_id(engine.cfg().symbol);
                if (sym_id_d >= 0) {
                    int sym_reg = g_sym_regime[sym_id_d].load();
                    if (!pyramid_allowed_in_regime(sym_reg)) {
                        std::printf("[PYRAMID-SKIP] tag=%s regime=%d (not BULL_TREND)\n",
                            tag.c_str(), sym_reg);
                        std::fflush(stdout);
                        return;
                    }
                }
                // S44e/f: tier × funding × confluence (no crash carve on
                // pyramid — adds happen on existing position so regime check
                // is already past).
                double tier_mult = tier_mult_for_tag(tag);
                double funding_mult = 1.0;
                int sym_id = chimera::symbol_to_id(engine.cfg().symbol);
                if (sym_id >= 0 && g_funding_filter.is_ready()) {
                    if (g_funding_filter.has_tailwind(sym_id))      funding_mult = 1.2;
                    else if (g_funding_filter.has_headwind(sym_id)) funding_mult = 0.8;
                }
                // S44g: liquidity floor applies to pyramid adds too.
                double liq_mult = liq_mult_for_symbol(engine.cfg().symbol);
                double sym_base_usd = runtime_cfg.max_position_usd * liq_mult;
                double total_mult = tier_mult * funding_mult;
                double add_usd = sym_base_usd * size_mult * total_mult;
                // P1/S46: per-trade $-risk-budget clamp on pyramid adds too.
                {
                    int _cl = symbol_cluster(chimera::symbol_to_id(engine.cfg().symbol));
                    double max_add = MAX_TRADE_RISK_USD * tier_risk_mult(engine.cfg().tag)
                                     / (CLUSTER_WORST_GAP_BP[_cl] / 1e4);
                    if (add_usd > max_add) add_usd = max_add;
                }
                double qty = add_usd / price;
                std::printf("[PYRAMID-INTENT] tag=%s add=%d size_mult=%.0f%% tier=%.2fx liq=%.2fx funding=%.2fx total=%.2fx add_usd=%.2f qty=%.8f\n",
                    tag.c_str(), add_num, size_mult * 100.0, tier_mult, liq_mult, funding_mult, total_mult, add_usd, qty);
                std::fflush(stdout);
                // Pyramid uses engine's symbol context; resolve via tag prefix is non-trivial,
                // so use engine.symbol field captured at config time.
                auto result = executor.execute(engine.cfg().symbol, true, qty, price);
                if (!result.ok) {
                    std::fprintf(stderr,
                        "[PYRAMID] execute failed tag=%s err=%s\n",
                        tag.c_str(), result.error.c_str());
                }
            });
    };

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION A: D1 ENGINES (Sessions 13-14) ──────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE A1: BTC-TSMOM-D1 — PF=1.92, Sharpe=1.67, Nbr=85%
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
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d1(btc_d1_cfg);
    wire_engine(btc_tsmom_d1);

    // ── UPJUMP-H1 (S-2026-07-05b): INTRADAY per-coin window + tiered ladder ──
    // PER-COIN INTRADAY WINDOW W (4-8h), symmetric up-jump, long-only, ride-to-flip:
    // NO trade-level price stops (vault UpMoveTrailLossMitigation — stops destroy the
    // up-move edge; protection = the separate companion ladder book only). Exit only
    // on symmetric down-jump flip. The 24h wide window was too slow to catch the
    // intraday movers; per-coin W/thr from crypto_upjump_tiered_ladder_sweep.py (best
    // net W<=8 passing standalone all-6). RT cost 20bp. make_upjump now takes W.
    auto make_upjump = [](const char* sym, const char* tag, int w, double thr) {
        return chimera::EdgeEngine::Config{
            .symbol         = sym,
            .tag            = tag,
            .kind           = chimera::StrategyKind::UPJUMP,
            .tf_secs        = 3600,
            .atr_period     = 14,
            .upjump_w       = w,
            .upjump_thr     = thr,
            .ride_to_flip   = true,
            .round_trip_bp  = 20.0,
            .max_history    = 720,   // ~30d of 1h klines so the up-jump event is visible on restart
        };
    };
    // per-coin intraday W(h)/thr — crypto_upjump_tiered_ladder_sweep.py roster (05-07b)
    chimera::EdgeEngine::Config btc_upjump_cfg  = make_upjump("btcusdt",  "BTC-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config eth_upjump_cfg  = make_upjump("ethusdt",  "ETH-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config sol_upjump_cfg  = make_upjump("solusdt",  "SOL-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config doge_upjump_cfg = make_upjump("dogeusdt", "DOGE-UPJUMP-H1", 4, 0.02);
    chimera::EdgeEngine::Config bnb_upjump_cfg  = make_upjump("bnbusdt",  "BNB-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine btc_upjump_h1(btc_upjump_cfg);
    chimera::EdgeEngine eth_upjump_h1(eth_upjump_cfg);
    chimera::EdgeEngine sol_upjump_h1(sol_upjump_cfg);
    chimera::EdgeEngine doge_upjump_h1(doge_upjump_cfg);
    chimera::EdgeEngine bnb_upjump_h1(bnb_upjump_cfg);
    wire_engine(btc_upjump_h1);
    wire_engine(eth_upjump_h1);
    wire_engine(sol_upjump_h1);
    wire_engine(doge_upjump_h1);
    wire_engine(bnb_upjump_h1);
    // UPJUMP-H1 remaining 5 legs — S-2026-07-03b: feeds already subscribed (all 62
    // SYM_FULL). Per-coin thr from Crypto 437337c. OP = parent-only (no companion).
    chimera::EdgeEngine::Config ada_upjump_cfg  = make_upjump("adausdt",  "ADA-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config trx_upjump_cfg  = make_upjump("trxusdt",  "TRX-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config aave_upjump_cfg = make_upjump("aaveusdt", "AAVE-UPJUMP-H1", 4, 0.02);  // parent-only (companion DROPPED task1: PF1.04/H1~0)
    chimera::EdgeEngine::Config near_upjump_cfg = make_upjump("nearusdt", "NEAR-UPJUMP-H1", 4, 0.02);
    chimera::EdgeEngine::Config op_upjump_cfg   = make_upjump("opusdt",   "OP-UPJUMP-H1",   4, 0.02); // parent-only (fails all-6 any W)
    chimera::EdgeEngine ada_upjump_h1(ada_upjump_cfg);
    chimera::EdgeEngine trx_upjump_h1(trx_upjump_cfg);
    chimera::EdgeEngine aave_upjump_h1(aave_upjump_cfg);
    chimera::EdgeEngine near_upjump_h1(near_upjump_cfg);
    chimera::EdgeEngine op_upjump_h1(op_upjump_cfg);
    wire_engine(ada_upjump_h1);
    wire_engine(trx_upjump_h1);
    wire_engine(aave_upjump_h1);
    wire_engine(near_upjump_h1);
    wire_engine(op_upjump_h1);

    // ── UPJUMP clip companions — NO-FLOOR TIERED LADDER + STACKED ARMS + cap8 ──
    // (S-2026-07-07w, operator item 5 — REVERT from BE-floor mode.) The BE-floor book
    // is real-fill DEAD both ways: close-eval = slip bleed (-1.13Mbp real, 07-07f audit),
    // per-tick stops = churn bleed (-2.57M, PF 0.04) — Crypto/backtest/latearm/. The only
    // trail family surviving real fills is this NO-FLOOR giveback ladder. WINNER
    // (backtest/upjump_concurrent_arms_2026-07-07.txt): roster_cfg.csv per-coin tiers
    // + STACKED BASE ARMS +2/+4/+6% (g50 rev-only) + self-funding ladder cap 8 =
    // +18,360% vs +10,283% roster cap5, 8/8 coins all-6, 2x-cost robust (BTC bear -12
    // marginal). Pure cap raises on 2 tiers BREAK ADA H1 — stacked arms scale better
    // than deeper ladders. NO confirm gate (winner swept with cg=0/confirm=0).
    // Trigger = INTERNAL per-coin roster W/thr detector (the windows the winner was
    // swept on) — NOT the live parents (uniform 4h/+2% since 52c0d31: different window
    // family; feedback-test-operator-spec: never conflate). parent_tag = price feed +
    // panel key only. Dual-column stays (ladder books model==real, cost debited).
    // Shadow: own ledger, observe-only, never touches the parent
    // (feedback-companion-independent-engine). Cost 20bp RT (0.20% Binance spot taker).
    // ── S-2026-07-08 WEIGHTING + AUTO-RETIREMENT (Crypto backtest/upjump_weighting_bt.cpp,
    // outputs/CRYPTO_WEIGHTING_RETIREMENT_2026-07-08.md) ─────────────────────────
    // size mult: x2 = robust top performer (honest all-6 PASS + >=2σ over the 20-seed
    // random-entry control). Only SOL clears both bars (net +5980% PF 5.78, z=2.3).
    // DOGE passes all-6 but is only 1.1σ over random (bull-beta risk) -> x1.
    // rank-out: reserved for BT-net-negative books — NONE (ADA's forward -244bp was
    // the seed_open rehydrate artifact, already fixed; ADA BT = +1594% PF 1.31).
    // retire_bp: -2x the worst per-book drawdown episode in the validated 2021-2026
    // BT of the exact live config (cumulative real-net curve, raw per-leg bp) —
    // a level beyond anything the validated backtest ever produced.
    // Un-retire = operator act: list the tag in data/companion_unretire.flags, restart.
    auto unretired = [](const char* ctag) {
        std::ifstream f("data/companion_unretire.flags"); std::string ln;
        while (f && std::getline(f, ln)) {
            while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
            if (ln == ctag) return true;
        }
        return false;
    };
    auto make_lad_companion = [&unretired](const char* ptag, const char* ctag, const char* sym,
                                 int det_w, double det_thr,
                                 chimera::UpJumpLadderCompanion::Tier tight,
                                 chimera::UpJumpLadderCompanion::Tier wide,
                                 double size_mult, double retire_bp) {
        chimera::UpJumpLadderCompanion::Config c;
        c.parent_tag = ptag;  // price feed + desk panel key only; parent position never read
        c.tag = ctag; c.symbol = sym;
        c.tight = tight; c.wide = wide;   // Tier{arm%, stall_bars, gb_frac, trail_bp(unused)}
        c.extra_base = { {2.0, 0, 0.50, 0}, {4.0, 0, 0.50, 0}, {6.0, 0, 0.50, 0} };  // stacked arms S1/S2/S3
        c.reclip_pct = 0.05; c.cap = 8;                    // 5 base + up to 3 self-funded ladder legs
        c.cost_gate_bp = 0.0; c.confirm_bp = 0.0;          // faithful to the winner sweep
        c.be_floor = false;                                // NO FLOOR anywhere
        c.det_w = det_w; c.det_thr = det_thr;              // roster per-coin W(h1 bars)/thr window
        c.tf_secs = 3600; c.round_trip_bp = 20.0;
        c.size_mult = size_mult; c.rank_out = false;       // no BT-net-negative book 2026-07-08
        c.retire_bp = retire_bp;
        c.retire_override = unretired(ctag);
        return c;
    };
    using LTier = chimera::UpJumpLadderCompanion::Tier;
    // roster_cfg.csv rows (S-2026-07-05 all-6 roster; W/thr = detector window):
    //                                                                                        W  thr    TIGHT{arm,stall,gb}   WIDE{arm,stall,gb}   mult  retire_bp(-2xBTmaxDD)
    chimera::UpJumpLadderCompanion btc_clip (make_lad_companion("BTC-UPJUMP-H1",  "BTC-UPJUMP-CLIP",  "btcusdt",  4, 0.08, LTier{3,0,0.5,0},  LTier{5,0,0.5,0},  1.0,  -32000.0));
    chimera::UpJumpLadderCompanion eth_clip (make_lad_companion("ETH-UPJUMP-H1",  "ETH-UPJUMP-CLIP",  "ethusdt",  6, 0.05, LTier{3,0,0.5,0},  LTier{8,0,0.5,0},  1.0, -101000.0));
    chimera::UpJumpLadderCompanion sol_clip (make_lad_companion("SOL-UPJUMP-H1",  "SOL-UPJUMP-CLIP",  "solusdt",  4, 0.12, LTier{2,0,0.5,0},  LTier{8,0,0.5,0},  2.0,  -78500.0));
    chimera::UpJumpLadderCompanion doge_clip(make_lad_companion("DOGE-UPJUMP-H1", "DOGE-UPJUMP-CLIP", "dogeusdt", 8, 0.05, LTier{3,3,0,0},    LTier{8,8,0.4,0},  1.0, -141000.0));
    chimera::UpJumpLadderCompanion bnb_clip (make_lad_companion("BNB-UPJUMP-H1",  "BNB-UPJUMP-CLIP",  "bnbusdt",  4, 0.05, LTier{3,3,0.3,0},  LTier{8,0,0.5,0},  1.0,  -52000.0));
    chimera::UpJumpLadderCompanion ada_clip (make_lad_companion("ADA-UPJUMP-H1",  "ADA-UPJUMP-CLIP",  "adausdt",  6, 0.05, LTier{3,4,0.5,0},  LTier{5,6,0,0},    1.0, -102500.0));
    chimera::UpJumpLadderCompanion trx_clip (make_lad_companion("TRX-UPJUMP-H1",  "TRX-UPJUMP-CLIP",  "trxusdt",  8, 0.08, LTier{3,0,0.3,0},  LTier{8,6,0,0},    1.0,  -85000.0));
    chimera::UpJumpLadderCompanion near_clip(make_lad_companion("NEAR-UPJUMP-H1", "NEAR-UPJUMP-CLIP", "nearusdt", 6, 0.05, LTier{3,0,0.5,0},  LTier{8,0,0.5,0},  1.0, -136000.0));
    // AAVE/OP: parent-only (not in the winner roster — AAVE PF1.04 noise, OP fails all-6).
    // DOGE/NEAR re-added: the 05-07d drop was the confirm-25 tax; winner runs confirm=0.
    chimera::UpJumpLadderCompanion* _all_clips[] = {
        &btc_clip,&eth_clip,&sol_clip,&doge_clip,&bnb_clip,&ada_clip,&trx_clip,&near_clip };  // the 8/8 winner roster
    chimera::EdgeEngine* _all_clip_parents[] = {
        &btc_upjump_h1,&eth_upjump_h1,&sol_upjump_h1,&doge_upjump_h1,&bnb_upjump_h1,
        &ada_upjump_h1,&trx_upjump_h1,&near_upjump_h1 };  // 1:1 with _all_clips (price feed only)
    {
        std::lock_guard<std::mutex> lk(g_companion_mtx);
        auto _clip_totals = load_companion_clip_totals();
        const int _NCLIP = (int)(sizeof(_all_clips)/sizeof(_all_clips[0]));  // was hard-coded 9; now size-derived (DOGE dropped)
        for (int i = 0; i < _NCLIP; ++i) {
            _all_clips[i]->shadow_mode = true;
            {   // durable-counter rehydrate: panel clips/bank_bp survive restarts.
                // 4th arg = weighted real bank; rehydrate() also runs the auto-retire
                // check against the restored RAW real bank (one-shot [CLIP-RETIRE]).
                auto _ct = _clip_totals.find(_all_clips[i]->config().tag);
                if (_ct != _clip_totals.end())
                    _all_clips[i]->rehydrate(_ct->second.n, _ct->second.net, _ct->second.net_real, _ct->second.net_real_w);
            }
            _all_clips[i]->set_on_clip(persist_companion_clip);
            g_companion_by_parent[_all_clips[i]->config().parent_tag] =
                std::make_pair(_all_clip_parents[i], _all_clips[i]);
            {
                const auto& cc = _all_clips[i]->config();
                if (cc.be_floor)
                    std::printf("[CLIP-INIT] %s -> price %s  BE-FLOOR be=%.0fbp trail(T%.0f/W%.0f)bp det=%dh/%+.0f%% cap=%d shadow=1\n",
                        cc.tag.c_str(), cc.parent_tag.c_str(), cc.be_bp,
                        cc.tight.trail_bp, cc.wide.trail_bp, cc.det_w, cc.det_thr * 100, cc.cap);
                else
                    std::printf("[CLIP-INIT] %s -> det=%dh/%+.0f%% (self)  TIGHT(a%.0f/s%d/g%.2f) WIDE(a%.0f/s%d/g%.2f) +%d stacked-arm(s) reclip=%.2f cap=%d cg=%.0f confirm=%.0fbp mult=x%.1f retire@%.0fbp%s NO-FLOOR shadow=1\n",
                        cc.tag.c_str(), cc.det_w, cc.det_thr * 100,
                        cc.tight.arm, cc.tight.stall, cc.tight.gb,
                        cc.wide.arm, cc.wide.stall, cc.wide.gb,
                        (int)cc.extra_base.size(),
                        cc.reclip_pct, cc.cap, cc.cost_gate_bp, cc.confirm_bp,
                        cc.size_mult, cc.retire_bp,
                        _all_clips[i]->is_retired() ? " [RETIRED]" : (cc.rank_out ? " [RANK-OUT]" : ""));
            }
        }
        // S-2026-07-08 weighting split (one loud boot line, operator-auditable):
        {
            std::string x2, x1, ro, rt;
            for (int i = 0; i < _NCLIP; ++i) {
                const auto& cc = _all_clips[i]->config();
                std::string sym = cc.tag.substr(0, cc.tag.find('-'));
                if      (_all_clips[i]->is_retired()) rt += (rt.empty() ? "" : ",") + sym;
                else if (cc.rank_out)                 ro += (ro.empty() ? "" : ",") + sym;
                else if (cc.size_mult >= 2.0)         x2 += (x2.empty() ? "" : ",") + sym;
                else                                  x1 += (x1.empty() ? "" : ",") + sym;
            }
            std::printf("[CLIP-WEIGHTS] x2={%s} x1={%s} rank-out={%s} retired={%s} "
                        "(basis: BT 2021-2026 honest all-6 + random-entry control, "
                        "outputs/CRYPTO_WEIGHTING_RETIREMENT_2026-07-08.md)\n",
                        x2.c_str(), x1.c_str(), ro.c_str(), rt.c_str());
        }
        emit_companion_state();   // one-shot startup emit so the Omega desk panel lights up immediately (not after 1st H1 close)
    }
    std::fflush(stdout);

    // ENGINE A2: ETH-TSMOM-D1 — PF=3.15, Sharpe=3.17, Nbr=91%
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
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d1(eth_d1_cfg);
// S44-CULL:     wire_engine(eth_tsmom_d1);

    // ENGINE A3: SOL-TSMOM-D1 — PF=2.25, Sharpe=2.41, Nbr=89%
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
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sol_tsmom_d1(sol_d1_cfg);
// S44-CULL:     wire_engine(sol_tsmom_d1);

    // ENGINE A4: LINK-TSMOM-D1 — PF=2.18, Sharpe=1.92, Nbr=100%
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
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d1(link_d1_cfg);
// S44-CULL:     wire_engine(link_tsmom_d1);

    // ENGINE A5: BNB-TSMOM-D1 — PF=3.16, Sharpe=2.91, Nbr=90%
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
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d1(bnb_d1_cfg);
    wire_engine(bnb_tsmom_d1);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION B: H12 ENGINES (Session 14) ─────────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE B1: BTC-TSMOM-H12 — PF=3.63, Sharpe=3.40, 31 trades, Nbr=96%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h12(btc_h12_cfg);
    wire_engine(btc_tsmom_h12);

    // ENGINE B2: DOGE-TSMOM-H12 — PF=2.78, Sharpe=3.66, 82 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h12(doge_h12_cfg);
    wire_engine(doge_tsmom_h12);


    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION C: H6 ENGINES (Session 15 — strongest TF discovered) ────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE C1: XRP-TSMOM-H6 — PF=2.68, Sharpe=4.41, 120 trades, Nbr=100%
    // 98.9% of ALL parameter combos profitable — most robust edge in the system
    chimera::EdgeEngine::Config xrp_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h6(xrp_h6_cfg);
// S44-CULL:     wire_engine(xrp_tsmom_h6);

    // ENGINE C2: BTC-TSMOM-H6 — PF=2.59, Sharpe=5.16, 169 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h6(btc_h6_cfg);
    wire_engine(btc_tsmom_h6);

    // ENGINE C3: ETH-TSMOM-H6 — PF=2.07, Sharpe=3.70, 151 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_h6_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h6(eth_h6_cfg);
    wire_engine(eth_tsmom_h6);

    // ENGINE C4: SOL-TSMOM-H6 — PF=2.07, Sharpe=3.25, 127 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h6(sol_h6_cfg);
// S44-CULL:     wire_engine(sol_tsmom_h6);

    // ENGINE C5: BNB-TSMOM-H6 — PF=2.07, Sharpe=2.76, 95 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_h6_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h6(bnb_h6_cfg);
    wire_engine(bnb_tsmom_h6);

    // ENGINE C6: LINK-TSMOM-H6 — PF=2.07, Sharpe=3.13, 81 trades, Nbr=100%
    chimera::EdgeEngine::Config link_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h6(link_h6_cfg);
    wire_engine(link_tsmom_h6);

    // ENGINE C7: DOGE-TSMOM-H6 — PF=1.72, Sharpe=2.24, 91 trades, Nbr=77%
    chimera::EdgeEngine::Config doge_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h6(doge_h6_cfg);
    wire_engine(doge_tsmom_h6);

    // ENGINE C8: AVAX-TSMOM-H6 — PF=1.37, Sharpe=1.82, 207 trades, Nbr=67%
    chimera::EdgeEngine::Config avax_h6_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h6(avax_h6_cfg);
// S44-CULL:     wire_engine(avax_tsmom_h6);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION D: H4 ENGINES (Session 14) ──────────────────────────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE D1: XRP-TSMOM-H4 — PF=2.43, Sharpe=5.80, 267 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h4(xrp_h4_cfg);
    wire_engine(xrp_tsmom_h4);

    // ENGINE D2: BNB-TSMOM-H4 — PF=1.91, Sharpe=3.79, 291 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h4(bnb_h4_cfg);
// S44-CULL:     wire_engine(bnb_tsmom_h4);

    // ENGINE D3: LINK-TSMOM-H4 — PF=1.91, Sharpe=4.07, 205 trades, Nbr=95%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h4(link_h4_cfg);
    wire_engine(link_tsmom_h4);

    // ENGINE D4: SOL-TSMOM-H4 — PF=1.89, Sharpe=3.82, 208 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h4(sol_h4_cfg);
    wire_engine(sol_tsmom_h4);

    // ENGINE D5: BTC-TSMOM-H4 — PF=1.82, Sharpe=3.54, 167 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h4(btc_h4_cfg);
// S44-CULL:     wire_engine(btc_tsmom_h4);

    // ENGINE D6: ETH-TSMOM-H4 — PF=1.76, Sharpe=3.26, 196 trades, Nbr=100%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h4(eth_h4_cfg);
    wire_engine(eth_tsmom_h4);

    // ENGINE D7: AVAX-TSMOM-H4 — PF=1.47, Sharpe=2.17, 231 trades, Nbr=83%
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h4(avax_h4_cfg);
    wire_engine(avax_tsmom_h4);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION E: H1 ENGINES (Session 15 — XRP/SOL/LINK only) ──────────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE E1: XRP-TSMOM-H1 — PF=1.66, Sharpe=3.73, 327 trades, Nbr=100%
    // 69.3% of combos profitable — genuine edge even at H1 cost drag



    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION F: H2 ENGINES (Session 17 — new timeframe discovery) ────
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE F1: BTC-TSMOM-H2 — PF=1.99, Sharpe=4.98, 281 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h2(btc_h2_cfg);
// S44-CULL:     wire_engine(btc_tsmom_h2);

    // ENGINE F2: ETH-TSMOM-H2 — PF=1.50, Sharpe=3.02, 359 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_h2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h2(eth_h2_cfg);
// S44-CULL:     wire_engine(eth_tsmom_h2);

    // ENGINE F3: SOL-TSMOM-H2 — PF=1.78, Sharpe=4.17, 340 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_h2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h2(sol_h2_cfg);
    wire_engine(sol_tsmom_h2);

    // ENGINE F4: XRP-TSMOM-H2 — PF=2.00, Sharpe=4.70, 320 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_h2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 7200,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h2(xrp_h2_cfg);
// S44-CULL:     wire_engine(xrp_tsmom_h2);




    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION G: H3 ENGINES (Session 17 — no native Binance candles) ──
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE G1: BTC-TSMOM-H3 — PF=1.96, Sharpe=3.52, 156 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_h3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h3(btc_h3_cfg);
// S44-CULL:     wire_engine(btc_tsmom_h3);

    // ENGINE G2: ETH-TSMOM-H3 — PF=1.74, Sharpe=3.65, 278 trades, Nbr=98%
    chimera::EdgeEngine::Config eth_h3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h3(eth_h3_cfg);
// S44-CULL:     wire_engine(eth_tsmom_h3);

    // ENGINE G3: SOL-TSMOM-H3 — PF=1.92, Sharpe=4.15, 259 trades, Nbr=93%
    chimera::EdgeEngine::Config sol_h3_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h3(sol_h3_cfg);
// S44-CULL:     wire_engine(sol_tsmom_h3);

    // ENGINE G4: XRP-TSMOM-H3 — PF=2.19, Sharpe=4.70, 243 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_h3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h3(xrp_h3_cfg);
    wire_engine(xrp_tsmom_h3);

    // ENGINE G5: LINK-TSMOM-H3 — PF=1.94, Sharpe=4.19, 254 trades, Nbr=100%
    chimera::EdgeEngine::Config link_h3_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
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
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h3(link_h3_cfg);
// S44-CULL:     wire_engine(link_tsmom_h3);

    // ENGINE G6: BNB-TSMOM-H3 — PF=1.55, Sharpe=2.74, 349 trades, Nbr=97%
    chimera::EdgeEngine::Config bnb_h3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h3(bnb_h3_cfg);
// S44-CULL:     wire_engine(bnb_tsmom_h3);

    // ENGINE G7: DOGE-TSMOM-H3 — PF=1.25, Sharpe=1.48, 309 trades, Nbr=87%
    chimera::EdgeEngine::Config doge_h3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h3(doge_h3_cfg);
// S44-CULL:     wire_engine(doge_tsmom_h3);

    // ══════════════════════════════════════════════════════════════════════
    // BEAR-MARKET COUNTER-TREND ENGINES — Session 19 (2026-05-16)
    //
    // These fire when TSMOM sits flat (bearish/ranging markets):
    //   - RSI_REVERT: buy oversold bounce (long-only spot dip-buy)
    //   - BOLLINGER K=2.5/3.0: buy extreme lower-band pierce (long-only spot)
    //
    // All spot-LONG-only. Shadow mode. Validated via optimizer_v2 with
    // neighbourhood stability scoring across 446K backtests.
    // ══════════════════════════════════════════════════════════════════════

    // ── TIER 1 — STRONG edges (Nbr >= 60%, PF > 1.3, >= 10 trades) ─────


    // ENGINE R2: ETH-RSI30-H4 — PF=10.12, Sharpe=4.00, 12 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_rsi30_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI30-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi30_h4(eth_rsi30_h4_cfg);
// S44-CULL:     wire_engine(eth_rsi30_h4);





    // ENGINE B1: BNB-BOLL25-H3 — PF=7.46, Sharpe=2.81, 10 trades, Nbr=66%
    chimera::EdgeEngine::Config bnb_boll25_h3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL25-H3",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 10800,
        .lookback       = 15,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.5,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_boll25_h3(bnb_boll25_h3_cfg);
// S44-CULL:     wire_engine(bnb_boll25_h3);

    // ── TIER 2 — GOOD edges (Nbr >= 40%, PF > 1.15) ────────────────────






    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION H: NEW SYMBOL ENGINES — Session 20 (SUI/APT/NEAR/ARB) ───
    // ══════════════════════════════════════════════════════════════════════
    // Discovered by running optimizer_general on freshly downloaded H1 klines.
    // NEAR is the strongest single symbol ever tested — passes D1 through H3.
    // All use cost=22bp (higher spread on newer/smaller-cap alts).

    // ENGINE H1: NEAR-TSMOM-D1 — PF=2.79, Sharpe=2.61, 46 trades, Nbr=100%
    chimera::EdgeEngine::Config near_d1_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-D1",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 86400,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine near_tsmom_d1(near_d1_cfg);
// S44-CULL:     wire_engine(near_tsmom_d1);

    // ENGINE H2: NEAR-TSMOM-H12 — PF=1.92, Sharpe=3.03, 126 trades, Nbr=95%
    chimera::EdgeEngine::Config near_h12_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h12(near_h12_cfg);
    wire_engine(near_tsmom_h12);

    // ENGINE H3: NEAR-TSMOM-H6 — PF=1.85, Sharpe=3.62, 257 trades, Nbr=100%
    chimera::EdgeEngine::Config near_h6_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h6(near_h6_cfg);
    wire_engine(near_tsmom_h6);

    // ENGINE H4: NEAR-TSMOM-H4 — PF=2.17, Sharpe=3.59, 209 trades, Nbr=100%
    chimera::EdgeEngine::Config near_h4_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h4(near_h4_cfg);
    wire_engine(near_tsmom_h4);

    // ENGINE H5: NEAR-TSMOM-H3 — PF=1.75, Sharpe=3.65, 351 trades, Nbr=87%
    chimera::EdgeEngine::Config near_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h3(near_h3_cfg);
    wire_engine(near_tsmom_h3);

    // ENGINE H6: SUI-TSMOM-H6 — PF=1.80, Sharpe=3.22, 129 trades, Nbr=100%
    chimera::EdgeEngine::Config sui_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h6(sui_h6_cfg);
    wire_engine(sui_tsmom_h6);

    // ENGINE H7: SUI-TSMOM-H4 — PF=1.44, Sharpe=2.11, 169 trades, Nbr=88%
    chimera::EdgeEngine::Config sui_h4_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H4",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 14400,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h4(sui_h4_cfg);
    wire_engine(sui_tsmom_h4);

    // ENGINE H8: APT-TSMOM-H6 — PF=1.82, Sharpe=3.32, 149 trades, Nbr=92%
    chimera::EdgeEngine::Config apt_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_tsmom_h6(apt_h6_cfg);
    wire_engine(apt_tsmom_h6);

    // ENGINE H9: ARB-TSMOM-H6 — PF=1.48, Sharpe=2.31, 131 trades, Nbr=80%
    chimera::EdgeEngine::Config arb_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H6",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h6(arb_h6_cfg);
    wire_engine(arb_tsmom_h6);

    // ── SECTION I: COUNTER-TREND ON NEW SYMBOLS — Session 21 (RSI_REVERT + BOLLINGER) ───
//
// 31 new engines: 17 RSI_REVERT + 14 BOLLINGER on NEAR/SUI/APT/ARB
// These symbols show exceptional mean-reversion properties.
//



    // ENGINE S21-3: NEAR-RSI-H3 — PF=2.39, Sharpe=1.78, 26 trades, Nbr=47%
    chimera::EdgeEngine::Config near_rsi_h3_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_rsi_h3(near_rsi_h3_cfg);
    // wire_engine(near_rsi_h3);  // S37-KILL: prod_tiered PF=0.559 bp=-594


    // ENGINE S21-5: SUI-RSI-H3 — PF=5.87, Sharpe=1.89, 11 trades, Nbr=49%
    chimera::EdgeEngine::Config sui_rsi_h3_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h3(sui_rsi_h3_cfg);
// S44-CULL:     wire_engine(sui_rsi_h3);

    // ENGINE S21-6: SUI-RSI-H2 — PF=2.05, Sharpe=1.16, 13 trades, Nbr=49%
    chimera::EdgeEngine::Config sui_rsi_h2_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h2(sui_rsi_h2_cfg);
    // wire_engine(sui_rsi_h2);  // S37-KILL: prod_tiered PF=0.361 bp=-1621

    // ENGINE S21-7: SUI-RSI-H4 — PF=1.62, Sharpe=0.86, 17 trades, Nbr=42%
    chimera::EdgeEngine::Config sui_rsi_h4_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_rsi_h4(sui_rsi_h4_cfg);
    // wire_engine(sui_rsi_h4);  // S37-KILL: prod_tiered PF=0.685 bp=-186


    // ENGINE S21-9: APT-RSI-H1 — PF=1.81, Sharpe=0.73, 39 trades, Nbr=83%
    chimera::EdgeEngine::Config apt_rsi_h1_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H1",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 3600,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h1(apt_rsi_h1_cfg);
// S44-CULL:     wire_engine(apt_rsi_h1);



    // ENGINE S21-12: APT-RSI-H3 — PF=2.90, Sharpe=1.61, 12 trades, Nbr=46%
    chimera::EdgeEngine::Config apt_rsi_h3_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_rsi_h3(apt_rsi_h3_cfg);
// S44-CULL:     wire_engine(apt_rsi_h3);

    // ENGINE S21-13: ARB-RSI-H2 — PF=4.80, Sharpe=2.66, 13 trades, Nbr=100%
    chimera::EdgeEngine::Config arb_rsi_h2_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H2",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 7200,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h2(arb_rsi_h2_cfg);
// S44-CULL:     wire_engine(arb_rsi_h2);

    // ENGINE S21-14: ARB-RSI-H4 — PF=2.46, Sharpe=1.54, 14 trades, Nbr=84%
    chimera::EdgeEngine::Config arb_rsi_h4_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h4(arb_rsi_h4_cfg);
// S44-CULL:     wire_engine(arb_rsi_h4);



    // ENGINE S21-17: ARB-RSI-H3 — PF=3.71, Sharpe=2.22, 11 trades, Nbr=66%
    chimera::EdgeEngine::Config arb_rsi_h3_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-RSI-H3",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 10800,
        .lookback       = 35,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_rsi_h3(arb_rsi_h3_cfg);
// S44-CULL:     wire_engine(arb_rsi_h3);





    // ENGINE S21-22: APT-BOLL-H2 — PF=4.55, Sharpe=3.05, 29 trades, Nbr=100%
    chimera::EdgeEngine::Config apt_boll_h2_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_boll_h2(apt_boll_h2_cfg);
// S44-CULL:     wire_engine(apt_boll_h2);






    // ENGINE S21-28: SUI-BOLL-H6 — PF=6.72, Sharpe=2.96, 14 trades, Nbr=51%
    chimera::EdgeEngine::Config sui_boll_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_boll_h6(sui_boll_h6_cfg);
// S44-CULL:     wire_engine(sui_boll_h6);




    // ── SECTION J: EXOTIC TFs + EXTENDED COUNTER-TREND — Session 21 ───────
    //
    // 100 new engines from full sweep: H8/H16/D2/D3 TSMOM + H6/H8/H12 counter-trend + DONCHIAN
    //

    // ENGINE S21X-1: BTC-TSMOM-H8 — PF=1.99, Sharpe=2.55, 77 trades, Nbr=82%
    chimera::EdgeEngine::Config btc_tsmom_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h8(btc_tsmom_h8_cfg);
// S44-CULL:     wire_engine(btc_tsmom_h8);
    // ENGINE S21X-2: ETH-TSMOM-H8 — PF=2.90, Sharpe=5.10, 121 trades, Nbr=100%
    chimera::EdgeEngine::Config eth_tsmom_h8_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_tsmom_h8(eth_tsmom_h8_cfg);
    wire_engine(eth_tsmom_h8);
    // ENGINE S21X-5: LINK-TSMOM-H8 — PF=2.95, Sharpe=4.78, 119 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h8(link_tsmom_h8_cfg);
// S44-CULL:     wire_engine(link_tsmom_h8);
    // ENGINE S21X-6: NEAR-TSMOM-H8 — PF=2.10, Sharpe=3.79, 171 trades, Nbr=97%
    chimera::EdgeEngine::Config near_tsmom_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_tsmom_h8(near_tsmom_h8_cfg);
    wire_engine(near_tsmom_h8);
    // ENGINE S21X-7: BNB-TSMOM-H8 — PF=2.86, Sharpe=3.67, 138 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h8(bnb_tsmom_h8_cfg);
    wire_engine(bnb_tsmom_h8);
    // ENGINE S21X-8: DOGE-TSMOM-H8 — PF=2.02, Sharpe=2.54, 107 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_tsmom_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h8(doge_tsmom_h8_cfg);
    wire_engine(doge_tsmom_h8);
    // ENGINE S21X-9: AVAX-TSMOM-H8 — PF=1.90, Sharpe=2.33, 101 trades, Nbr=77%
    chimera::EdgeEngine::Config avax_tsmom_h8_cfg{
        .symbol         = "avaxusdt",
        .tag            = "AVAX-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine avax_tsmom_h8(avax_tsmom_h8_cfg);
    wire_engine(avax_tsmom_h8);
    // ENGINE S21X-10: SUI-TSMOM-H8 — PF=2.27, Sharpe=2.50, 62 trades, Nbr=81%
    chimera::EdgeEngine::Config sui_tsmom_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 35,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h8(sui_tsmom_h8_cfg);
    wire_engine(sui_tsmom_h8);
    // ENGINE S21X-12: ARB-TSMOM-H8 — PF=2.01, Sharpe=2.84, 86 trades, Nbr=50%
    chimera::EdgeEngine::Config arb_tsmom_h8_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h8(arb_tsmom_h8_cfg);
// S44-CULL:     wire_engine(arb_tsmom_h8);
    // ENGINE S21X-13: BTC-TSMOM-H16 — PF=5.16, Sharpe=4.01, 22 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_tsmom_h16(btc_tsmom_h16_cfg);
// S44-CULL:     wire_engine(btc_tsmom_h16);
    // ENGINE S21X-15: SOL-TSMOM-H16 — PF=3.47, Sharpe=3.77, 54 trades, Nbr=100%
    chimera::EdgeEngine::Config sol_tsmom_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h16(sol_tsmom_h16_cfg);
// S44-CULL:     wire_engine(sol_tsmom_h16);
    // ENGINE S21X-16: XRP-TSMOM-H16 — PF=4.72, Sharpe=4.14, 55 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h16(xrp_tsmom_h16_cfg);
    wire_engine(xrp_tsmom_h16);
    // ENGINE S21X-17: LINK-TSMOM-H16 — PF=3.15, Sharpe=3.17, 39 trades, Nbr=97%
    chimera::EdgeEngine::Config link_tsmom_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h16(link_tsmom_h16_cfg);
    wire_engine(link_tsmom_h16);
    // ENGINE S21X-19: BNB-TSMOM-H16 — PF=2.76, Sharpe=2.70, 61 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h16(bnb_tsmom_h16_cfg);
// S44-CULL:     wire_engine(bnb_tsmom_h16);
    // ENGINE S21X-20: DOGE-TSMOM-H16 — PF=2.16, Sharpe=2.33, 54 trades, Nbr=92%
    chimera::EdgeEngine::Config doge_tsmom_h16_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_tsmom_h16(doge_tsmom_h16_cfg);
    wire_engine(doge_tsmom_h16);
    // ENGINE S21X-22: SUI-TSMOM-H16 — PF=2.13, Sharpe=2.16, 40 trades, Nbr=85%
    chimera::EdgeEngine::Config sui_tsmom_h16_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_tsmom_h16(sui_tsmom_h16_cfg);
// S44-CULL:     wire_engine(sui_tsmom_h16);
    // ENGINE S21X-24: ARB-TSMOM-H16 — PF=2.33, Sharpe=2.84, 43 trades, Nbr=40%
    chimera::EdgeEngine::Config arb_tsmom_h16_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-H16",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 57600,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_tsmom_h16(arb_tsmom_h16_cfg);
    wire_engine(arb_tsmom_h16);
    // ENGINE S21X-25: BTC-TSMOM-D2 — PF=38.30, Sharpe=7.17, 10 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_d2_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 10,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d2(btc_tsmom_d2_cfg);
// S44-CULL:     wire_engine(btc_tsmom_d2);
    // ENGINE S21X-26: ETH-TSMOM-D2 — PF=5.99, Sharpe=2.48, 10 trades, Nbr=88%
    chimera::EdgeEngine::Config eth_tsmom_d2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d2(eth_tsmom_d2_cfg);
// S44-CULL:     wire_engine(eth_tsmom_d2);
    // ENGINE S21X-27: SOL-TSMOM-D2 — PF=5.97, Sharpe=3.30, 14 trades, Nbr=82%
    chimera::EdgeEngine::Config sol_tsmom_d2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sol_tsmom_d2(sol_tsmom_d2_cfg);
// S44-CULL:     wire_engine(sol_tsmom_d2);
    // ENGINE S21X-28: XRP-TSMOM-D2 — PF=26.86, Sharpe=3.35, 22 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_d2_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 10,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_tsmom_d2(xrp_tsmom_d2_cfg);
// S44-CULL:     wire_engine(xrp_tsmom_d2);
    // ENGINE S21X-29: LINK-TSMOM-D2 — PF=28.76, Sharpe=3.56, 14 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_d2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d2(link_tsmom_d2_cfg);
// S44-CULL:     wire_engine(link_tsmom_d2);
    // ENGINE S21X-30: NEAR-TSMOM-D2 — PF=4.09, Sharpe=2.62, 13 trades, Nbr=53%
    chimera::EdgeEngine::Config near_tsmom_d2_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine near_tsmom_d2(near_tsmom_d2_cfg);
// S44-CULL:     wire_engine(near_tsmom_d2);
    // ENGINE S21X-31: BNB-TSMOM-D2 — PF=13.09, Sharpe=2.78, 31 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_d2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 40,
        .hold_bars      = 8,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d2(bnb_tsmom_d2_cfg);
// S44-CULL:     wire_engine(bnb_tsmom_d2);
    // ENGINE S21X-32: DOGE-TSMOM-D2 — PF=4.99, Sharpe=3.51, 20 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_tsmom_d2_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 40,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_tsmom_d2(doge_tsmom_d2_cfg);
// S44-CULL:     wire_engine(doge_tsmom_d2);
    // ENGINE S21X-34: SUI-TSMOM-D2 — PF=3.84, Sharpe=2.10, 13 trades, Nbr=91%
    chimera::EdgeEngine::Config sui_tsmom_d2_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 25,
        .hold_bars      = 24,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sui_tsmom_d2(sui_tsmom_d2_cfg);
// S44-CULL:     wire_engine(sui_tsmom_d2);
    // ENGINE S21X-36: BTC-TSMOM-D3 — PF=242.75, Sharpe=6.40, 15 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_tsmom_d3_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine btc_tsmom_d3(btc_tsmom_d3_cfg);
// S44-CULL:     wire_engine(btc_tsmom_d3);
    // ENGINE S21X-37: ETH-TSMOM-D3 — PF=7.40, Sharpe=2.70, 11 trades, Nbr=89%
    chimera::EdgeEngine::Config eth_tsmom_d3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 25,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine eth_tsmom_d3(eth_tsmom_d3_cfg);
// S44-CULL:     wire_engine(eth_tsmom_d3);
    // ENGINE S21X-39: XRP-TSMOM-D3 — PF=45.74, Sharpe=3.90, 11 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_tsmom_d3_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 15,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine xrp_tsmom_d3(xrp_tsmom_d3_cfg);
    // wire_engine(xrp_tsmom_d3);  // S37-KILL: prod_tiered PF=0.923 bp=-1481
    // ENGINE S21X-40: LINK-TSMOM-D3 — PF=10.45, Sharpe=3.74, 10 trades, Nbr=100%
    chimera::EdgeEngine::Config link_tsmom_d3_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine link_tsmom_d3(link_tsmom_d3_cfg);
// S44-CULL:     wire_engine(link_tsmom_d3);
    // ENGINE S21X-41: NEAR-TSMOM-D3 — DISABLED (FAILED re-validation: OOS PF=0.86, edge not confirmed)
    // chimera::EdgeEngine::Config near_tsmom_d3_cfg{...};
    // chimera::EdgeEngine near_tsmom_d3(near_tsmom_d3_cfg);
    // ENGINE S21X-42: BNB-TSMOM-D3 — PF=34.14, Sharpe=2.67, 13 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_d3_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 30,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.8,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_tsmom_d3(bnb_tsmom_d3_cfg);
// S44-CULL:     wire_engine(bnb_tsmom_d3);
    // ENGINE S21X-43: DOGE-TSMOM-D3 — S36-RETUNE 2026-05-28
    // Original (lb=25, hold=4, sl=3.0) under prod_tiered: 5yr OOS -3629bp PF=0.20
    // Original under staged_only:                           5yr OOS  -673bp PF=0.93
    // S36-RETUNE (lb=50, hold=3, sl=1.5) under staged_only: 5yr OOS +11188bp PF=4.87
    // Verified robust: 730d PF=2.57, 365d PF=4.88, 5yr PF=4.87; param neighbourhood
    // lookback=50, hold ∈ {3,6,8,12,20} all PF>3.0. Other strategies on this TF
    // (DONCHIAN/SUPERTREND/ICHIMOKU) generate 0 signals; TSMOM only viable kind.
    chimera::EdgeEngine::Config doge_tsmom_d3_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 50,   // S36: was 25
        .hold_bars      = 3,    // S36: was 4
        .sl_atr_mult    = 1.5,  // S36: was 3.0 (tight SL viable because BE-ratchet rescues)
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,  // S36
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine doge_tsmom_d3(doge_tsmom_d3_cfg);
// S44-CULL:     wire_engine(doge_tsmom_d3);
    // ENGINE S21X-45: SUI-TSMOM-D3 — PF=2.38, Sharpe=1.43, 10 trades, Nbr=56%
    chimera::EdgeEngine::Config sui_tsmom_d3_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-TSMOM-D3",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 259200,
        .lookback       = 8,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine sui_tsmom_d3(sui_tsmom_d3_cfg);
// S44-CULL:     wire_engine(sui_tsmom_d3);
    // ENGINE S21X-49: SOL-RSI-H6 — PF=3.64, Sharpe=1.83, 10 trades, Nbr=80%
    // ENGINE S21X-51: BNB-RSI-H6 — DISABLED (FAILED re-validation: OOS PF=1.08, original 373.9 was mirage)
    // chimera::EdgeEngine::Config bnb_rsi_h6_cfg{...};
    // chimera::EdgeEngine bnb_rsi_h6(bnb_rsi_h6_cfg);
    // ENGINE S21X-52: DOGE-RSI-H6 — PF=3.72, Sharpe=1.96, 16 trades, Nbr=67%
    chimera::EdgeEngine::Config doge_rsi_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-RSI-H6",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 21600,
        .lookback       = 25,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_rsi_h6(doge_rsi_h6_cfg);
// S44-CULL:     wire_engine(doge_rsi_h6);
    // ENGINE S21X-54: BTC-BOLL-H6 — PF=8.04, Sharpe=3.24, 18 trades, Nbr=100%
    chimera::EdgeEngine::Config btc_boll_h6_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h6(btc_boll_h6_cfg);
// S44-CULL:     wire_engine(btc_boll_h6);
    // ENGINE S21X-56: SOL-BOLL-H6 — PF=5.77, Sharpe=3.23, 14 trades, Nbr=84%
    chimera::EdgeEngine::Config sol_boll_h6_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 1.0,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h6(sol_boll_h6_cfg);
// S44-CULL:     wire_engine(sol_boll_h6);
    // ENGINE S21X-58: LINK-BOLL-H6 — PF=99.90, Sharpe=6.18, 14 trades, Nbr=100%
    chimera::EdgeEngine::Config link_boll_h6_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h6(link_boll_h6_cfg);
// S44-CULL:     wire_engine(link_boll_h6);
    // ENGINE S21X-60: DOGE-BOLL-H6 — PF=99.90, Sharpe=4.78, 15 trades, Nbr=100%
    chimera::EdgeEngine::Config doge_boll_h6_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H6",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 21600,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_boll_h6(doge_boll_h6_cfg);
// S44-CULL:     wire_engine(doge_boll_h6);
    // ENGINE S21X-65: XRP-RSI-H12 — DISABLED (FAILED re-validation: OOS PF=1.09, recent PF=0.89, no edge)
    // chimera::EdgeEngine::Config xrp_rsi_h12_cfg{...};
    // chimera::EdgeEngine xrp_rsi_h12(xrp_rsi_h12_cfg);
    // ENGINE S21X-69: BTC-BOLL-H12 — PF=6.45, Sharpe=2.64, 10 trades, Nbr=97%
    chimera::EdgeEngine::Config btc_boll_h12_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h12(btc_boll_h12_cfg);
// S44-CULL:     wire_engine(btc_boll_h12);
    // ENGINE S21X-71: XRP-BOLL-H12 — PF=54.35, Sharpe=2.52, 10 trades, Nbr=96%
    chimera::EdgeEngine::Config xrp_boll_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h12(xrp_boll_h12_cfg);
// S44-CULL:     wire_engine(xrp_boll_h12);
    // ENGINE S21X-75: ETH-RSI-H8 — PF=99.90, Sharpe=3.99, 13 trades, Nbr=66%
    // ENGINE S21X-76: XRP-RSI-H8 — PF=10.46, Sharpe=2.69, 12 trades, Nbr=100%
    // ENGINE S21X-77: LINK-RSI-H8 — PF=4.82, Sharpe=2.15, 14 trades, Nbr=67%
    // ENGINE S21X-78: BNB-RSI-H8 — PF=1.95, Sharpe=1.78, 31 trades, Nbr=96%
    // ENGINE S21X-79: NEAR-RSI-H8 — PF=2.09, Sharpe=1.35, 27 trades, Nbr=81%
    // ENGINE S21X-80: APT-RSI-H8 — PF=3.03, Sharpe=1.55, 11 trades, Nbr=68%
    // ENGINE S21X-81: ARB-RSI-H8 — PF=1.56, Sharpe=0.70, 14 trades, Nbr=53%
    // ENGINE S21X-82: BTC-BOLL-H8 — PF=7.73, Sharpe=2.59, 10 trades, Nbr=70%
    // ENGINE S21X-83: ETH-BOLL-H8 — PF=2.91, Sharpe=1.93, 11 trades, Nbr=95%
    // ENGINE S21X-84: XRP-BOLL-H8 — PF=14.69, Sharpe=3.04, 16 trades, Nbr=91%
    // ENGINE S21X-85: LINK-BOLL-H8 — PF=16.22, Sharpe=3.65, 13 trades, Nbr=100%
    // ENGINE S21X-86: BNB-BOLL-H8 — PF=5.10, Sharpe=2.25, 12 trades, Nbr=45%
    // ENGINE S21X-87: NEAR-BOLL-H8 — PF=2.57, Sharpe=1.56, 14 trades, Nbr=69%
    // ENGINE S21X-88: APT-BOLL-H8 — PF=1.96, Sharpe=0.92, 11 trades, Nbr=61%
    // ENGINE S21X-89: ARB-BOLL-H8 — PF=2.53, Sharpe=1.56, 12 trades, Nbr=64%
    // ENGINE S21X-91: NEAR-DONCH-H8 — PF=2.99, Sharpe=2.46, 40 trades, Nbr=76%
    // ENGINE S21X-92: SUI-DONCH-H6 — PF=1.83, Sharpe=1.23, 15 trades, Nbr=46%
    chimera::EdgeEngine::Config sui_donch_h6_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest showed only +1-2% CAGR lift (avg 0.04 adds/trade) — trail
        // tightens before profit reaches arm threshold. SUI exits too fast to extend.
    };
    chimera::EdgeEngine sui_donch_h6(sui_donch_h6_cfg);
    // wire_engine(sui_donch_h6);  // S37-KILL: prod_tiered PF=0.576 bp=-2314
    // ENGINE S21X-93: SUI-DONCH-H8 — PF=5.02, Sharpe=2.18, 11 trades, Nbr=100%
    // ENGINE S21X-97: ARB-DONCH-H8 — PF=2.00, Sharpe=1.22, 10 trades, Nbr=51%

    // ══════════════════════════════════════════════════════════════════════
    // SESSION 22 — Extended-data validated engines (57 new)
    // RSI_REVERT H8, BOLLINGER H8, RSI_REVERT H16, BOLLINGER H16,
    // DONCHIAN H8/H16/D2/D3
    // ══════════════════════════════════════════════════════════════════════










    chimera::EdgeEngine::Config btc_boll_h8_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine btc_boll_h8(btc_boll_h8_cfg);
// S44-CULL:     wire_engine(btc_boll_h8);


    chimera::EdgeEngine::Config sol_boll_h8_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 8,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h8(sol_boll_h8_cfg);
// S44-CULL:     wire_engine(sol_boll_h8);

    chimera::EdgeEngine::Config bnb_boll_h8_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 10,
        .hold_bars      = 20,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_boll_h8(bnb_boll_h8_cfg);
// S44-CULL:     wire_engine(bnb_boll_h8);


    chimera::EdgeEngine::Config link_boll_h8_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_boll_h8(link_boll_h8_cfg);
// S44-CULL:     wire_engine(link_boll_h8);

    chimera::EdgeEngine::Config xrp_boll_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h8(xrp_boll_h8_cfg);
// S44-CULL:     wire_engine(xrp_boll_h8);

    chimera::EdgeEngine::Config doge_boll_h8_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine doge_boll_h8(doge_boll_h8_cfg);
// S44-CULL:     wire_engine(doge_boll_h8);

    chimera::EdgeEngine::Config sui_boll_h8_cfg{
        .symbol         = "suiusdt",
        .tag            = "SUI-BOLL-H8",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 28800,
        .lookback       = 25,
        .hold_bars      = 10,
        .sl_atr_mult    = 1.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sui_boll_h8(sui_boll_h8_cfg);
// S44-CULL:     wire_engine(sui_boll_h8);




    // ETH-RSI-H16 — DISABLED (FAILED re-validation: OOS PF=0.49, recent PF=0.24, actively losing)
    // chimera::EdgeEngine::Config eth_rsi_h16_cfg{...};
    // chimera::EdgeEngine eth_rsi_h16(eth_rsi_h16_cfg);











    chimera::EdgeEngine::Config near_boll_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-BOLL-H16",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_boll_h16(near_boll_h16_cfg);
    // wire_engine(near_boll_h16);  // S37-KILL: prod_tiered PF=0.963 bp=-62


    chimera::EdgeEngine::Config xrp_donch_h8_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 20,
        .hold_bars      = 10,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h8(xrp_donch_h8_cfg);
// S44-CULL:     wire_engine(xrp_donch_h8);

    chimera::EdgeEngine::Config near_donch_h8_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H8",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 28800,
        .lookback       = 15,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h8(near_donch_h8_cfg);
// S44-CULL:     wire_engine(near_donch_h8);




    chimera::EdgeEngine::Config xrp_donch_h16_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 8,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h16(xrp_donch_h16_cfg);
// S44-CULL:     wire_engine(xrp_donch_h16);

    chimera::EdgeEngine::Config bnb_donch_h16_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest 2021-2026 showed +3% CAGR lift (avg 0.27 adds/trade),
        // not worth the operational complexity. Trail tightens at 1.5 ATR before adds trigger.
    };
    chimera::EdgeEngine bnb_donch_h16(bnb_donch_h16_cfg);
// S44-CULL:     wire_engine(bnb_donch_h16);

    chimera::EdgeEngine::Config btc_donch_h16_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
        // Pyramid disabled: backtest showed +7% CAGR (avg 0.25 adds/trade) — below 10% lift threshold.
    };
    chimera::EdgeEngine btc_donch_h16(btc_donch_h16_cfg);
    // wire_engine(btc_donch_h16);  // S37-KILL: prod_tiered PF=0.703 bp=-1554



    chimera::EdgeEngine::Config near_donch_h16_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 8,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h16(near_donch_h16_cfg);
// S44-CULL:     wire_engine(near_donch_h16);

    chimera::EdgeEngine::Config sol_donch_h16_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_donch_h16(sol_donch_h16_cfg);
    // wire_engine(sol_donch_h16);  // S37-KILL: prod_tiered PF=0.731 bp=-1198


    chimera::EdgeEngine::Config bnb_donch_d2_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 5,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine bnb_donch_d2(bnb_donch_d2_cfg);
// S44-CULL:     wire_engine(bnb_donch_d2);










    // ══════════════════════════════════════════════════════════════════════
    // ── SESSION 24: DONCHIAN gap-fill (H2/H3/H4/H6/H12) + TSMOM H12 ───
    // ══════════════════════════════════════════════════════════════════════






    // ENGINE S24-6: XRP-DONCH-H6 — PF=2.63, Sharpe=2.25, 45 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_donch_h6_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 2.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h6(xrp_donch_h6_cfg);
// S44-CULL:     wire_engine(xrp_donch_h6);

    // ENGINE S24-7: BNB-DONCH-H6 — PF=2.08, Sharpe=1.84, 31 trades, Nbr=64%
    chimera::EdgeEngine::Config bnb_donch_h6_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 24,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.6,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_donch_h6(bnb_donch_h6_cfg);
// S44-CULL:     wire_engine(bnb_donch_h6);

    // ENGINE S24-8: XRP-DONCH-H12 — PF=3.40, Sharpe=2.09, 26 trades, Nbr=100%
    chimera::EdgeEngine::Config xrp_donch_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-DONCH-H12",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 12,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_donch_h12(xrp_donch_h12_cfg);
// S44-CULL:     wire_engine(xrp_donch_h12);

    // ENGINE S24-9: NEAR-DONCH-H12 — PF=2.25, Sharpe=1.43, 21 trades, Nbr=65%
    chimera::EdgeEngine::Config near_donch_h12_cfg{
        .symbol         = "nearusdt",
        .tag            = "NEAR-DONCH-H12",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine near_donch_h12(near_donch_h12_cfg);
// S44-CULL:     wire_engine(near_donch_h12);


    // ENGINE S24-11: SOL-TSMOM-H12 — PF=1.91, Sharpe=2.30, 120 trades, Nbr=86%
    chimera::EdgeEngine::Config sol_tsmom_h12_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_tsmom_h12(sol_tsmom_h12_cfg);
// S44-CULL:     wire_engine(sol_tsmom_h12);

    // ENGINE S24-12: BNB-TSMOM-H12 — PF=2.45, Sharpe=3.08, 96 trades, Nbr=100%
    chimera::EdgeEngine::Config bnb_tsmom_h12_cfg{
        .symbol         = "bnbusdt",
        .tag            = "BNB-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 40,
        .hold_bars      = 20,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine bnb_tsmom_h12(bnb_tsmom_h12_cfg);
    wire_engine(bnb_tsmom_h12);

    // ENGINE S24-13: LINK-TSMOM-H12 — PF=1.62, Sharpe=2.00, 187 trades, Nbr=90%
    chimera::EdgeEngine::Config link_tsmom_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 35,
        .hold_bars      = 4,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine link_tsmom_h12(link_tsmom_h12_cfg);
    wire_engine(link_tsmom_h12);

    // ENGINE S24-14: XRP-TSMOM-H12 — PF=1.54, Sharpe=1.52, 153 trades, Nbr=73%
    chimera::EdgeEngine::Config xrp_tsmom_h12_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-TSMOM-H12",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 10,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.4,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_tsmom_h12(xrp_tsmom_h12_cfg);
    wire_engine(xrp_tsmom_h12);


    // ── Session 26 — RSI_REVERT H4 + BOLLINGER H4/H2 new edges ──────────

    // ENGINE S26-1: ETH-RSI-H4 — PF=1.82, Sharpe=1.45, 38 trades, Nbr=72%
    chimera::EdgeEngine::Config eth_rsi_h4_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 12,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h4(eth_rsi_h4_cfg);
// S44-CULL:     wire_engine(eth_rsi_h4);


    // ENGINE S26-3: XRP-RSI-H4 — PF=1.74, Sharpe=1.38, 45 trades, Nbr=70%
    chimera::EdgeEngine::Config xrp_rsi_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-RSI-H4",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 14400,
        .lookback       = 30,
        .hold_bars      = 16,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_rsi_h4(xrp_rsi_h4_cfg);
// S44-CULL:     wire_engine(xrp_rsi_h4);





    // ENGINE S26-8: SOL-BOLL-H2 — PF=1.53, Sharpe=1.19, 52 trades, Nbr=61%
    chimera::EdgeEngine::Config sol_boll_h2_cfg{
        .symbol         = "solusdt",
        .tag            = "SOL-BOLL-H2",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 7200,
        .lookback       = 8,
        .hold_bars      = 16,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine sol_boll_h2(sol_boll_h2_cfg);
// S44-CULL:     wire_engine(sol_boll_h2);


    // ENGINE S26-10: XRP-BOLL-H4 — PF=1.56, Sharpe=1.21, 29 trades, Nbr=62%
    chimera::EdgeEngine::Config xrp_boll_h4_cfg{
        .symbol         = "xrpusdt",
        .tag            = "XRP-BOLL-H4",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 14400,
        .lookback       = 25,
        .hold_bars      = 16,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine xrp_boll_h4(xrp_boll_h4_cfg);
// S44-CULL:     wire_engine(xrp_boll_h4);

    // ENGINE S27-1: SUI-RSI-H12 — PF=1.61, Sharpe=1.12, 21 trades, Nbr=96%
    // Walk-forward: IS PF=0.94, OOS PF=1.66, Stability=100%

    // ENGINE S27-2: ETH-BOLL-H12 — PF=45.90, Sharpe=3.14, 12 trades, Nbr=100%
    // Walk-forward: IS PF=0.56, OOS PF=48.01, Stability=100%
    chimera::EdgeEngine::Config eth_boll_h12_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-BOLL-H12",
        .kind           = chimera::StrategyKind::BOLLINGER,
        .tf_secs        = 43200,
        .lookback       = 5,
        .hold_bars      = 6,
        .sl_atr_mult    = 2.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr  = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_boll_h12(eth_boll_h12_cfg);
// S44-CULL:     wire_engine(eth_boll_h12);


    // ══════════════════════════════════════════════════════════════════════
    // SESSION 28 — KELTNER_REVERT + DUAL_THRUST engines (8 engines)
    // ══════════════════════════════════════════════════════════════════════

    // ENGINE S28-1: DOGE-KELTNER-H6 — PF=5.24, Sharpe=2.43, 15 trades, Nbr=95%
    // Walk-forward: IS PF=0.87, OOS PF=5.24, Stability=95%

    // ENGINE S28-2: LINK-KELTNER-H12 — PF=6.85, Sharpe=2.21, 11 trades, Nbr=66%
    // Walk-forward: IS PF=1.07, OOS PF=6.85, Stability=66%
    chimera::EdgeEngine::Config link_keltner_h12_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-KELTNER-H12",
        .kind           = chimera::StrategyKind::KELTNER_REVERT,
        .tf_secs        = 43200,
        .lookback       = 30,
        .hold_bars      = 24,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.8,
        .keltner_ema_len  = 30,
        .keltner_atr_mult = 2.5,
    };
    chimera::EdgeEngine link_keltner_h12(link_keltner_h12_cfg);
// S44-CULL:     wire_engine(link_keltner_h12);

    // ENGINE S28-3: DOGE-KELTNER-H8 — PF=4.38, Sharpe=2.45, 18 trades, Nbr=64%
    // Walk-forward: IS PF=0.97, OOS PF=4.38, Stability=64%

    // ENGINE S28-4: BTC-KELTNER-H12 — PF=3.03, Sharpe=1.69, 30 trades, Nbr=58%
    // Walk-forward: IS PF=1.03, OOS PF=3.03, Stability=58%

    // ENGINE S28-5: SUI-KELTNER-H12 — PF=4.82, Sharpe=1.99, 16 trades, Nbr=40%
    // Walk-forward: IS PF=0.96, OOS PF=4.82, Stability=40%

    // ENGINE S28-6: APT-KELTNER-H8 — PF=3.21, Sharpe=1.40, 11 trades, Nbr=46%
    // Walk-forward: IS PF=3.60, OOS PF=3.21, Stability=46%

    // ENGINE S28-7: SOL-DT-H12 — PF=3.08, Sharpe=2.42, 29 trades, Nbr=56%
    // Walk-forward: IS PF=1.72, OOS PF=3.01, Stability=56%

    // ENGINE S28-8: XRP-DT-H8 — PF=1.71, Sharpe=1.38, 101 trades, Nbr=64%
    // Walk-forward: IS PF=0.90, OOS PF=1.70, Stability=64%


    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION K: ICHIMOKU ENGINES (Session 29) ────────────────────────
    // Cloud breakout + Tenkan/Kijun cross. Trend-following, complementary
    // to TSMOM (different signal timing — waits for cloud confirmation).
    // Params tuned for crypto: Tenkan=20, Kijun=60, SenkouB=120
    // (standard 9/26/52 is too fast for crypto's noise).
    // ══════════════════════════════════════════════════════════════════════






    // ENGINE S29-6: DOGE-ICHI-H12
    chimera::EdgeEngine::Config doge_ichi_h12_cfg{
        .symbol         = "dogeusdt",
        .tag            = "DOGE-ICHI-H12",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 43200,
        .lookback       = 20,
        .hold_bars      = 12,
        .sl_atr_mult    = 2.5,
        .atr_period     = 14,
        .round_trip_bp  = 10.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine doge_ichi_h12(doge_ichi_h12_cfg);
// S44-CULL:     wire_engine(doge_ichi_h12);

    // S37-NEW: BTC-ICHI-D1 — sweep found 5yr PF=3.77 Sh=2.86 n=99 +10060bp
    // Walk-forward: 365d PF=3.23, 730d PF=4.43, 5yr PF=3.77 — robust.
    chimera::EdgeEngine::Config btc_ichi_d1_cfg{
        .symbol         = "btcusdt",
        .tag            = "BTC-ICHI-D1",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 86400,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .round_trip_bp  = 17.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine btc_ichi_d1(btc_ichi_d1_cfg);
// S44-CULL:     wire_engine(btc_ichi_d1);

    // S37-NEW: ETH-ICHI-D1 — sweep found 5yr PF=8.61 Sh=4.38 n=79 +15046bp
    // Walk-forward: 365d PF=5.83, 730d PF=7.02, 5yr PF=8.61 — top edge found this session.
    chimera::EdgeEngine::Config eth_ichi_d1_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-ICHI-D1",
        .kind           = chimera::StrategyKind::ICHIMOKU,
        .tf_secs        = 86400,
        .lookback       = 8,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .round_trip_bp  = 17.0,
        .max_history    = 130,
        .trail_arm_atr  = 1.5,
        .trail_dist_atr = 0.5,
        .ichi_tenkan_period  = 20,
        .ichi_kijun_period   = 60,
        .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine eth_ichi_d1(eth_ichi_d1_cfg);
// S44-CULL:     wire_engine(eth_ichi_d1);

    // S37-EDGE-PASS: 9 more ICHIMOKU + KELTNER engines passed broad sweep
    // (PF>=2.5 5yr AND no walk-forward window PF<1.5 with n>=10).

    // SOL-ICHI-D1: 5yr PF=4.34 Sh=3.45 n=53, 365d PF=4.47 730d PF=2.41
    chimera::EdgeEngine::Config sol_ichi_d1_cfg{
        .symbol = "solusdt", .tag = "SOL-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 6, .sl_atr_mult = 1.5, .atr_period = 14,
        .round_trip_bp = 20.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine sol_ichi_d1(sol_ichi_d1_cfg); wire_engine(sol_ichi_d1);

    // BNB-ICHI-D1: 5yr PF=13.34 Sh=6.71 n=61, 365d PF=6.37 730d PF=4.41
    chimera::EdgeEngine::Config bnb_ichi_d1_cfg{
        .symbol = "bnbusdt", .tag = "BNB-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 4, .sl_atr_mult = 2.0, .atr_period = 14,
        .round_trip_bp = 20.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine bnb_ichi_d1(bnb_ichi_d1_cfg); wire_engine(bnb_ichi_d1);

    // LINK-ICHI-D1: 5yr PF=9.95 Sh=3.50 n=38, 365d PF=4.58 730d PF=3.88
    chimera::EdgeEngine::Config link_ichi_d1_cfg{
        .symbol = "linkusdt", .tag = "LINK-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 20, .sl_atr_mult = 4.0, .atr_period = 14,
        .round_trip_bp = 22.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine link_ichi_d1(link_ichi_d1_cfg); wire_engine(link_ichi_d1);

    // AVAX-ICHI-D1: 5yr PF=4.02 Sh=2.49 n=34, 365d PF=4.02 730d PF=5.07
    chimera::EdgeEngine::Config avax_ichi_d1_cfg{
        .symbol = "avaxusdt", .tag = "AVAX-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 8, .sl_atr_mult = 3.0, .atr_period = 14,
        .round_trip_bp = 22.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine avax_ichi_d1(avax_ichi_d1_cfg); wire_engine(avax_ichi_d1);

    // DOGE-ICHI-D1: 5yr PF=3.38 Sh=2.45 n=25, 365d PF=3.38 730d PF=2.00
    chimera::EdgeEngine::Config doge_ichi_d1_cfg{
        .symbol = "dogeusdt", .tag = "DOGE-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 3, .sl_atr_mult = 2.0, .atr_period = 14,
        .round_trip_bp = 20.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine doge_ichi_d1(doge_ichi_d1_cfg); wire_engine(doge_ichi_d1);

    // XRP-ICHI-D1: 5yr PF=4.98 Sh=2.86 n=51, 365d PF=2.37 730d PF=3.34
    chimera::EdgeEngine::Config xrp_ichi_d1_cfg{
        .symbol = "xrpusdt", .tag = "XRP-ICHI-D1",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 86400,
        .lookback = 8, .hold_bars = 20, .sl_atr_mult = 4.0, .atr_period = 14,
        .round_trip_bp = 20.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine xrp_ichi_d1(xrp_ichi_d1_cfg); wire_engine(xrp_ichi_d1);

    // ETH-ICHI-H12: 5yr PF=3.87 Sh=3.86 n=169, 365d PF=5.35 730d PF=2.85 180d PF=3.44
    chimera::EdgeEngine::Config eth_ichi_h12_cfg{
        .symbol = "ethusdt", .tag = "ETH-ICHI-H12",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 43200,
        .lookback = 8, .hold_bars = 20, .sl_atr_mult = 4.0, .atr_period = 14,
        .round_trip_bp = 17.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
// S44-CULL:     chimera::EdgeEngine eth_ichi_h12(eth_ichi_h12_cfg); wire_engine(eth_ichi_h12);

    // SOL-ICHI-H8: 5yr PF=5.92 Sh=6.20 n=188, 365d PF=3.68 730d PF=2.45 180d PF=41.6
    chimera::EdgeEngine::Config sol_ichi_h8_cfg{
        .symbol = "solusdt", .tag = "SOL-ICHI-H8",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 28800,
        .lookback = 8, .hold_bars = 20, .sl_atr_mult = 3.0, .atr_period = 14,
        .round_trip_bp = 20.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine sol_ichi_h8(sol_ichi_h8_cfg); wire_engine(sol_ichi_h8);

    // LINK-KELTNER-H6: 5yr PF=6.12 Sh=2.69 n=35, 365d PF=3.99 180d PF=3.46
    chimera::EdgeEngine::Config link_keltner_h6_cfg{
        .symbol = "linkusdt", .tag = "LINK-KELTNER-H6",
        .kind = chimera::StrategyKind::KELTNER_REVERT, .tf_secs = 21600,
        .lookback = 8, .hold_bars = 8, .sl_atr_mult = 2.5, .atr_period = 14,
        .round_trip_bp = 22.0, .max_history = 64,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .keltner_ema_len = 30, .keltner_atr_mult = 2.5,
    };
// S44-CULL:     chimera::EdgeEngine link_keltner_h6(link_keltner_h6_cfg); wire_engine(link_keltner_h6);

    // NEAR-ICHI-H8: 5yr PF=2.92 Sh=4.67 n=192, 730d PF=2.74 365d PF=2.35
    // 180d PF=2.45 134d PF=2.48 — VERY robust across all windows incl bear.
    chimera::EdgeEngine::Config near_ichi_h8_cfg{
        .symbol = "nearusdt", .tag = "NEAR-ICHI-H8",
        .kind = chimera::StrategyKind::ICHIMOKU, .tf_secs = 28800,
        .lookback = 8, .hold_bars = 12, .sl_atr_mult = 1.5, .atr_period = 14,
        .round_trip_bp = 22.0, .max_history = 130,
        .trail_arm_atr = 0.5, .trail_dist_atr = 0.5,
        .ichi_tenkan_period = 20, .ichi_kijun_period = 60, .ichi_senkou_b_period = 120,
    };
    chimera::EdgeEngine near_ichi_h8(near_ichi_h8_cfg); wire_engine(near_ichi_h8);

    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION L: SUPERTREND ENGINES (Session 29) ──────────────────────
    // ATR-based trailing trend indicator. Enters on flip from bearish to
    // bullish. Very popular in crypto — different signal timing than TSMOM
    // (requires actual price/ATR flip vs simple lookback momentum).
    // Params: multiplier=3.0, ATR period=10 (standard crypto config).
    // ══════════════════════════════════════════════════════════════════════








    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION M: WILLIAMS %R ENGINES (Session 29b) ────────────────────
    // Mean-reversion using Williams %R oscillator. Different normalization
    // than RSI — uses (HH-Close)/(HH-LL) which is more responsive to
    // recent price extremes. Fires at different times than RSI_REVERT.
    // ══════════════════════════════════════════════════════════════════════





    // ══════════════════════════════════════════════════════════════════════
    // ── SECTION N: STOCHASTIC RSI ENGINES (Session 29b) ─────────────────
    // Stochastic RSI = RSI normalized within its own range. Faster than
    // raw RSI — catches reversals sooner. Ideal for timing mean-reversion
    // entries when RSI is oscillating in a narrow band (StochRSI breaks out
    // of 0/100 more readily than RSI does from 30/70).
    // ══════════════════════════════════════════════════════════════════════






    // ══════════════════════════════════════════════════════════════════════
    // SESSION 30 — W1 Mean-Reversion Engines (Edge 6)
    // Weekly timeframe RSI/BOLL on BTC + ETH. Few trades but huge PF on
    // big drawdown reversals. tf_secs=604800 (7 days).
    // Parameters: conservative oversold levels for weekly — fewer but higher quality
    // ══════════════════════════════════════════════════════════════════════

    // BTC W1 RSI_REVERT — weekly oversold reversals
    chimera::EdgeEngine::Config btc_rsi_w1_cfg;
    btc_rsi_w1_cfg.symbol       = "btcusdt";
    btc_rsi_w1_cfg.tag          = "BTC-RSI-W1";
    btc_rsi_w1_cfg.kind         = chimera::StrategyKind::RSI_REVERT;
    btc_rsi_w1_cfg.tf_secs      = 604800;  // W1
    btc_rsi_w1_cfg.lookback     = 14;
    btc_rsi_w1_cfg.hold_bars    = 4;       // hold 4 weeks
    btc_rsi_w1_cfg.sl_atr_mult  = 3.0;    // wider stop for weekly
    btc_rsi_w1_cfg.atr_period   = 14;
    btc_rsi_w1_cfg.rsi_threshold = 25.0;   // very oversold on weekly = strong reversal
    btc_rsi_w1_cfg.trail_arm_atr = 1.5;
    btc_rsi_w1_cfg.trail_dist_atr = 0.8;
    btc_rsi_w1_cfg.max_history  = 128;     // need more bars for weekly ATR

    // ETH W1 RSI_REVERT
    chimera::EdgeEngine::Config eth_rsi_w1_cfg;
    eth_rsi_w1_cfg.symbol       = "ethusdt";
    eth_rsi_w1_cfg.tag          = "ETH-RSI-W1";
    eth_rsi_w1_cfg.kind         = chimera::StrategyKind::RSI_REVERT;
    eth_rsi_w1_cfg.tf_secs      = 604800;
    eth_rsi_w1_cfg.lookback     = 14;
    eth_rsi_w1_cfg.hold_bars    = 4;
    eth_rsi_w1_cfg.sl_atr_mult  = 3.0;
    eth_rsi_w1_cfg.atr_period   = 14;
    eth_rsi_w1_cfg.rsi_threshold = 25.0;
    eth_rsi_w1_cfg.trail_arm_atr = 1.5;
    eth_rsi_w1_cfg.trail_dist_atr = 0.8;
    eth_rsi_w1_cfg.max_history  = 128;

    // BTC W1 BOLLINGER — weekly lower-band touch reversals
    chimera::EdgeEngine::Config btc_boll_w1_cfg;
    btc_boll_w1_cfg.symbol       = "btcusdt";
    btc_boll_w1_cfg.tag          = "BTC-BOLL-W1";
    btc_boll_w1_cfg.kind         = chimera::StrategyKind::BOLLINGER;
    btc_boll_w1_cfg.tf_secs      = 604800;
    btc_boll_w1_cfg.lookback     = 20;     // 20-week Bollinger
    btc_boll_w1_cfg.hold_bars    = 3;      // hold 3 weeks
    btc_boll_w1_cfg.sl_atr_mult  = 3.5;
    btc_boll_w1_cfg.atr_period   = 14;
    btc_boll_w1_cfg.bb_k         = 2.5;    // wider bands for weekly (more conservative)
    btc_boll_w1_cfg.trail_arm_atr = 2.0;
    btc_boll_w1_cfg.trail_dist_atr = 1.0;
    btc_boll_w1_cfg.max_history  = 128;

    // ETH W1 BOLLINGER
    chimera::EdgeEngine::Config eth_boll_w1_cfg;
    eth_boll_w1_cfg.symbol       = "ethusdt";
    eth_boll_w1_cfg.tag          = "ETH-BOLL-W1";
    eth_boll_w1_cfg.kind         = chimera::StrategyKind::BOLLINGER;
    eth_boll_w1_cfg.tf_secs      = 604800;
    eth_boll_w1_cfg.lookback     = 20;
    eth_boll_w1_cfg.hold_bars    = 3;
    eth_boll_w1_cfg.sl_atr_mult  = 3.5;
    eth_boll_w1_cfg.atr_period   = 14;
    eth_boll_w1_cfg.bb_k         = 2.5;
    eth_boll_w1_cfg.trail_arm_atr = 2.0;
    eth_boll_w1_cfg.trail_dist_atr = 1.0;
    eth_boll_w1_cfg.max_history  = 128;

    // ══════════════════════════════════════════════════════════════════════
    // AUDIT-2026 DARK-SYMBOL REVIVAL — top ETH/LINK DONCHIAN candidates from
    // batch_validate_results.csv. Configs ported from backtest/new_engines_configs.cpp.
    //   ETH-DONCH-D3   PF=9.77  n=30   Sh=1.82   (top dark engine overall)
    //   ETH-DONCH-D2   PF=2.05  n=39   Sh=0.71
    //   LINK-DONCH-D2  PF=1.89  n=37   Sh=0.66
    //   LINK-DONCH-H16 PF=1.75  n=72   Sh=0.67
    // ══════════════════════════════════════════════════════════════════════
    chimera::EdgeEngine::Config eth_donch_d3_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D3",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 259200,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine eth_donch_d3(eth_donch_d3_cfg);
// S44-CULL:     wire_engine(eth_donch_d3);

    chimera::EdgeEngine::Config eth_donch_d2_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 25,
        .hold_bars      = 20,
        .sl_atr_mult    = 4.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine eth_donch_d2(eth_donch_d2_cfg);
// S44-CULL:     wire_engine(eth_donch_d2);

    chimera::EdgeEngine::Config link_donch_d2_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-D2",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_donch_d2(link_donch_d2_cfg);
// S44-CULL:     wire_engine(link_donch_d2);

    chimera::EdgeEngine::Config link_donch_h16_cfg{
        .symbol         = "linkusdt",
        .tag            = "LINK-DONCH-H16",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 57600,
        .lookback       = 40,
        .hold_bars      = 10,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
    };
    chimera::EdgeEngine link_donch_h16(link_donch_h16_cfg);
// S44-CULL:     wire_engine(link_donch_h16);

    // ── AUDIT-2026 ROUND 2: ETH 3rd engine + APT/ARB revival ───────────────
    //   ETH-RSI-H12   PF=2.37  n=39   Sh=0.92  (mean-revert overlay)
    //   APT-TSMOM-H8  PF=1.49  n=404  Sh=1.42  (huge sample)
    //   APT-DONCH-H6  PF=1.67  n=97   Sh=1.03
    //   ARB-TSMOM-D2  PF=1.98  n=81   Sh=1.38
    //   ARB-DONCH-H6  PF=1.70  n=83   Sh=1.13
    // Params from backtest/engines.json (live-validated).
    chimera::EdgeEngine::Config eth_rsi_h12_cfg{
        .symbol         = "ethusdt",
        .tag            = "ETH-RSI-H12",
        .kind           = chimera::StrategyKind::RSI_REVERT,
        .tf_secs        = 43200,
        .lookback       = 8,
        .hold_bars      = 8,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 20.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.0,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr      = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine eth_rsi_h12(eth_rsi_h12_cfg);
    // wire_engine(eth_rsi_h12);  // S37-KILL: prod_tiered PF=0.966 bp=-59

    // APT-TSMOM-H8 — S36-RETUNE 2026-05-28
    // Original (lb=25, hold=20, sl=4.0) under S36 presets: 5yr OOS -831bp PF=0.95
    // RETUNE (lb=40, hold=3, sl=1.5): 5yr OOS +14495bp PF=2.13 Sh=4.35 N=201
    // Robust: 90d PF=2.51, 365d PF=2.02, 730d PF=2.28 — all PF>2, 82-84% WR
    chimera::EdgeEngine::Config apt_tsmom_h8_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-TSMOM-H8",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 28800,
        .lookback       = 40,   // S36: was 25
        .hold_bars      = 3,    // S36: was 20
        .sl_atr_mult    = 1.5,  // S36: was 4.0
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
    chimera::EdgeEngine apt_tsmom_h8(apt_tsmom_h8_cfg);
    wire_engine(apt_tsmom_h8);

    chimera::EdgeEngine::Config apt_donch_h6_cfg{
        .symbol         = "aptusdt",
        .tag            = "APT-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 35,
        .hold_bars      = 6,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.8,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr      = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine apt_donch_h6(apt_donch_h6_cfg);
// S44-CULL:     wire_engine(apt_donch_h6);

    chimera::EdgeEngine::Config arb_tsmom_d2_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-TSMOM-D2",
        .kind           = chimera::StrategyKind::TSMOM,
        .tf_secs        = 172800,
        .lookback       = 20,
        .hold_bars      = 4,
        .sl_atr_mult    = 3.0,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 17.0,
        .max_history    = 64,
        .trail_arm_atr  = 0.5,
        .trail_dist_atr = 0.3,
        .trail_tighten_atr      = 3.0,
        .trail_tighten_dist_atr = 0.25,
    };
    chimera::EdgeEngine arb_tsmom_d2(arb_tsmom_d2_cfg);
// S44-CULL:     wire_engine(arb_tsmom_d2);

    chimera::EdgeEngine::Config arb_donch_h6_cfg{
        .symbol         = "arbusdt",
        .tag            = "ARB-DONCH-H6",
        .kind           = chimera::StrategyKind::DONCHIAN,
        .tf_secs        = 21600,
        .lookback       = 40,
        .hold_bars      = 24,
        .sl_atr_mult    = 3.5,
        .atr_period     = 14,
        .bb_k           = 2.0,
        .rsi_threshold  = 30.0,
        .round_trip_bp  = 22.0,
        .max_history    = 64,
        .trail_arm_atr  = 1.2,
        .trail_dist_atr = 0.5,
        .trail_tighten_atr      = 1.5,
        .trail_tighten_dist_atr = 0.15,
    };
    chimera::EdgeEngine arb_donch_h6(arb_donch_h6_cfg);
// S44-CULL:     wire_engine(arb_donch_h6);

    // ══════════════════════════════════════════════════════════════════════
    // AUDIT-2026-S35: NEW SYMBOL engines (FET / TIA / ONDO).
    // Validated 5yr H1 Binance data through 2026-05-27 with full protection
    // stack + per-TF blowoff. PEPE/WIF excluded — memecoin tail risk too high
    // for first promotion wave; revisit after live shadow data.
    // ══════════════════════════════════════════════════════════════════════
    chimera::EdgeEngine::Config fet_tsmom_h8_cfg{
        .symbol="fetusdt", .tag="FET-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine fet_tsmom_h8(fet_tsmom_h8_cfg);
    wire_engine(fet_tsmom_h8);

    chimera::EdgeEngine::Config fet_tsmom_h12_cfg{
        .symbol="fetusdt", .tag="FET-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine fet_tsmom_h12(fet_tsmom_h12_cfg);
    wire_engine(fet_tsmom_h12);

    chimera::EdgeEngine::Config fet_tsmom_d1_cfg{
        .symbol="fetusdt", .tag="FET-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine fet_tsmom_d1(fet_tsmom_d1_cfg);
// S44-CULL:     wire_engine(fet_tsmom_d1);

    chimera::EdgeEngine::Config tia_tsmom_h8_cfg{
        .symbol="tiausdt", .tag="TIA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine tia_tsmom_h8(tia_tsmom_h8_cfg);
    wire_engine(tia_tsmom_h8);

    chimera::EdgeEngine::Config tia_tsmom_h12_cfg{
        .symbol="tiausdt", .tag="TIA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine tia_tsmom_h12(tia_tsmom_h12_cfg);
    wire_engine(tia_tsmom_h12);

    chimera::EdgeEngine::Config ondo_tsmom_h12_cfg{
        .symbol="ondousdt", .tag="ONDO-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ondo_tsmom_h12(ondo_tsmom_h12_cfg);
// S44-CULL:     wire_engine(ondo_tsmom_h12);

    // ══════════════════════════════════════════════════════════════════════
    // AUDIT-2026-S35 WAVE 2: HBAR / INJ / ADA / TRX / SEI.
    // Validated 5yr H1 Binance (SEI 1.5yr) with full protection stack + per-TF
    // blowoff. All TSMOM (DONCH samples too small). TRX limited to D1/2D —
    // shorter TFs showed high MDD (-3000bp+) due to stable-carry mean-reversion.
    // ══════════════════════════════════════════════════════════════════════
    chimera::EdgeEngine::Config hbar_tsmom_h8_cfg{
        .symbol="hbarusdt", .tag="HBAR-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine hbar_tsmom_h8(hbar_tsmom_h8_cfg);
// S44-CULL:     wire_engine(hbar_tsmom_h8);

    chimera::EdgeEngine::Config hbar_tsmom_h12_cfg{
        .symbol="hbarusdt", .tag="HBAR-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine hbar_tsmom_h12(hbar_tsmom_h12_cfg);
// S44-CULL:     wire_engine(hbar_tsmom_h12);

    chimera::EdgeEngine::Config hbar_tsmom_d1_cfg{
        .symbol="hbarusdt", .tag="HBAR-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine hbar_tsmom_d1(hbar_tsmom_d1_cfg);
// S44-CULL:     wire_engine(hbar_tsmom_d1);

    chimera::EdgeEngine::Config inj_tsmom_h8_cfg{
        .symbol="injusdt", .tag="INJ-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine inj_tsmom_h8(inj_tsmom_h8_cfg);
    wire_engine(inj_tsmom_h8);

    chimera::EdgeEngine::Config inj_tsmom_h12_cfg{
        .symbol="injusdt", .tag="INJ-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine inj_tsmom_h12(inj_tsmom_h12_cfg);
    wire_engine(inj_tsmom_h12);

    chimera::EdgeEngine::Config inj_tsmom_d1_cfg{
        .symbol="injusdt", .tag="INJ-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine inj_tsmom_d1(inj_tsmom_d1_cfg);
// S44-CULL:     wire_engine(inj_tsmom_d1);

    chimera::EdgeEngine::Config ada_tsmom_h8_cfg{
        .symbol="adausdt", .tag="ADA-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ada_tsmom_h8(ada_tsmom_h8_cfg);
// S44-CULL:     wire_engine(ada_tsmom_h8);

    chimera::EdgeEngine::Config ada_tsmom_h12_cfg{
        .symbol="adausdt", .tag="ADA-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ada_tsmom_h12(ada_tsmom_h12_cfg);
// S44-CULL:     wire_engine(ada_tsmom_h12);

    chimera::EdgeEngine::Config ada_tsmom_d1_cfg{
        .symbol="adausdt", .tag="ADA-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ada_tsmom_d1(ada_tsmom_d1_cfg);
// S44-CULL:     wire_engine(ada_tsmom_d1);

    chimera::EdgeEngine::Config trx_tsmom_d1_cfg{
        .symbol="trxusdt", .tag="TRX-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine trx_tsmom_d1(trx_tsmom_d1_cfg);
// S44-CULL:     wire_engine(trx_tsmom_d1);

    chimera::EdgeEngine::Config trx_tsmom_d2_cfg{
        .symbol="trxusdt", .tag="TRX-TSMOM-D2", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=172800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine trx_tsmom_d2(trx_tsmom_d2_cfg);
// S44-CULL:     wire_engine(trx_tsmom_d2);

    chimera::EdgeEngine::Config sei_tsmom_h8_cfg{
        .symbol="seiusdt", .tag="SEI-TSMOM-H8", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=28800, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine sei_tsmom_h8(sei_tsmom_h8_cfg);
    wire_engine(sei_tsmom_h8);

    chimera::EdgeEngine::Config sei_tsmom_h12_cfg{
        .symbol="seiusdt", .tag="SEI-TSMOM-H12", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=43200, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine sei_tsmom_h12(sei_tsmom_h12_cfg);
// S44-CULL:     wire_engine(sei_tsmom_h12);

    chimera::EdgeEngine::Config sei_tsmom_d1_cfg{
        .symbol="seiusdt", .tag="SEI-TSMOM-D1", .kind=chimera::StrategyKind::TSMOM,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine sei_tsmom_d1(sei_tsmom_d1_cfg);
// S44-CULL:     wire_engine(sei_tsmom_d1);

    // ══════════════════════════════════════════════════════════════════════
    // DISABLED ENGINES — No OOS edge after costs (Sessions 13-15)
    // ══════════════════════════════════════════════════════════════════════
    // ETH-BB-H6 (PF=0.72), SOL-DONCH-H6 (PF=0.83), XRP-DONCH-H1 (PF=0.82),
    // LINK-RSI-H6 (PF=1.17/4trades), BTC-OVERNIGHT-H1 (PF=0.31),
    // BTC-WEEKDAY-D1 (PF=0.44), DOGE-TSMOM-H4 (Nbr=49%),
    // BTC-TSMOM-H1 (PF=1.17/Nbr=76%), ETH-TSMOM-H1 (PF=1.13/Nbr=59%),
    // BNB-TSMOM-H1 (PF=1.11/Nbr=16%), DOGE-TSMOM-H1 (PF=1.11/Nbr=48%),
    // AVAX-TSMOM-H1 (PF=1.03/Nbr=8%)
    //
    // DISABLED ENGINES — Failed re-validation (Session 29c, 2026-05-17)
    // ──────────────────────────────────────────────────────────────────────
    // BNB-RSI-H6   (Orig PF=373.9 → OOS PF=1.08, 6 trades, no edge — inflated by 1 lucky window)
    // ETH-RSI-H16  (Orig PF=158.2 → OOS PF=0.49, 6 trades, recent PF=0.24 — actively losing)
    // NEAR-TSMOM-D3 (Orig PF=99.9 → OOS PF=0.86, 12 trades — edge not confirmed in extended OOS)
    // XRP-RSI-H12  (Orig PF=63.2 → OOS PF=1.09, 7 trades, recent PF=0.89 — breakeven, fading)

    // ══════════════════════════════════════════════════════════════════════
    // Register all active engines with backtest metadata
    // ══════════════════════════════════════════════════════════════════════

    // D1 engines (5)
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_d1,   "btcusdt",  86400, "BTC-TSMOM-D1",   1.92, 1.67,  85,  24, 13});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_d1,   "ethusdt",  86400, "ETH-TSMOM-D1",   3.15, 3.17,  91,  26, 13});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_d1,   "solusdt",  86400, "SOL-TSMOM-D1",   2.25, 2.41,  89,  15, 13});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d1,  "linkusdt", 86400, "LINK-TSMOM-D1",  2.18, 1.92, 100,  23, 13});
    g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_d1,   "bnbusdt",  86400, "BNB-TSMOM-D1",   3.16, 2.91,  90,  32, 14});

    // UPJUMP-H1 (5) — S-2026-07-03 faithful CryptoUpJump port, ride-to-flip, no stops
    g_slots.push_back({chimera::SYM_BTC,  &btc_upjump_h1,  "btcusdt",  3600, "BTC-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_ETH,  &eth_upjump_h1,  "ethusdt",  3600, "ETH-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_SOL,  &sol_upjump_h1,  "solusdt",  3600, "SOL-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_DOGE, &doge_upjump_h1, "dogeusdt", 3600, "DOGE-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_BNB,  &bnb_upjump_h1,  "bnbusdt",  3600, "BNB-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // UPJUMP-H1 remaining 5 (S-2026-07-03b) — ADA/TRX/AAVE/NEAR/OP (OP parent-only)
    g_slots.push_back({chimera::SYM_ADA,  &ada_upjump_h1,  "adausdt",  3600, "ADA-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_TRX,  &trx_upjump_h1,  "trxusdt",  3600, "TRX-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_AAVE, &aave_upjump_h1, "aaveusdt", 3600, "AAVE-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_NEAR, &near_upjump_h1, "nearusdt", 3600, "NEAR-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_OP,   &op_upjump_h1,   "opusdt",   3600, "OP-UPJUMP-H1",   0.0, 0.0, 0, 0, 57});

    // H12 engines (3)
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h12,  "btcusdt",  43200, "BTC-TSMOM-H12",  3.63, 3.40,  96,  31, 14});
    g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h12, "dogeusdt", 43200, "DOGE-TSMOM-H12", 2.78, 3.66, 100,  82, 14});

    // H6 engines (8) — NEW Session 15
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h6,   "xrpusdt",  21600, "XRP-TSMOM-H6",   2.68, 4.41, 100, 120, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h6,    "btcusdt",  21600, "BTC-TSMOM-H6",   2.59, 5.16, 100, 169, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h6,    "ethusdt",  21600, "ETH-TSMOM-H6",   2.07, 3.70, 100, 151, 15});
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h6,    "solusdt",  21600, "SOL-TSMOM-H6",   2.07, 3.25, 100, 127, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h6,    "bnbusdt",  21600, "BNB-TSMOM-H6",   2.07, 2.76, 100,  95, 15});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h6,   "linkusdt", 21600, "LINK-TSMOM-H6",  1.33, 1.12, 100, 549, 15});  // AUDIT-2026 revived: bvr PF=1.33 n=549 Sh=1.12
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h6,   "dogeusdt", 21600, "DOGE-TSMOM-H6",  1.72, 2.24,  77,  91, 15});
// S44-CULL:     g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h6,   "avaxusdt", 21600, "AVAX-TSMOM-H6",  1.48, 2.03, 100, 1157, 15});  // AUDIT-2026 revived: bvr PF=1.48 n=1157 Sh=2.03 (top dark engine)

    // H4 engines (7)
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h4,   "xrpusdt",  14400, "XRP-TSMOM-H4",   2.43, 5.80, 100, 267, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h4,    "bnbusdt",  14400, "BNB-TSMOM-H4",   1.91, 3.79, 100, 291, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h4,   "linkusdt", 14400, "LINK-TSMOM-H4",  1.91, 4.07,  95, 205, 14});
    g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h4,    "solusdt",  14400, "SOL-TSMOM-H4",   1.89, 3.82, 100, 208, 14});  // AUDIT-2026-S35 revived: protected-bvr PF=19.26 Sharpe=15.6 worst=-70bp MDD/cum=0.1%
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h4,    "btcusdt",  14400, "BTC-TSMOM-H4",   1.82, 3.54, 100, 167, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h4,    "ethusdt",  14400, "ETH-TSMOM-H4",   1.76, 3.26, 100, 196, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h4,   "avaxusdt", 14400, "AVAX-TSMOM-H4",  1.47, 2.17,  83, 231, 14});

    // H1 engines (3) — Session 15

    // H2 engines (5) — NEW Session 17
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h2,   "btcusdt",   7200, "BTC-TSMOM-H2",   1.99, 4.98, 100, 281, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h2,   "ethusdt",   7200, "ETH-TSMOM-H2",   1.50, 3.02, 100, 359, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h2,   "solusdt",   7200, "SOL-TSMOM-H2",   1.78, 4.17, 100, 340, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h2,   "xrpusdt",   7200, "XRP-TSMOM-H2",   2.00, 4.70, 100, 320, 17});

    // H3 engines (6) — NEW Session 17 (no native Binance candles — cold-start from ticks)
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h3,   "btcusdt",  10800, "BTC-TSMOM-H3",   1.96, 3.52, 100, 156, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h3,   "ethusdt",  10800, "ETH-TSMOM-H3",   1.74, 3.65,  98, 278, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h3,   "solusdt",  10800, "SOL-TSMOM-H3",   1.92, 4.15,  93, 259, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h3,   "xrpusdt",  10800, "XRP-TSMOM-H3",   2.19, 4.70, 100, 243, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h3,  "linkusdt", 10800, "LINK-TSMOM-H3",  1.94, 4.19, 100, 254, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h3,   "bnbusdt",  10800, "BNB-TSMOM-H3",   1.55, 2.74,  97, 349, 17});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h3,  "dogeusdt", 10800, "DOGE-TSMOM-H3",  1.25, 1.48,  87, 309, 20});

    // ── COUNTER-TREND engines (RSI_REVERT + BOLLINGER dip-buy) ─── Session 19 ──
    // TIER 1 — strong OOS edge + high neighbourhood stability
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi30_h4,   "ethusdt",  14400, "ETH-RSI30-H4",   2.13, 1.95,  88,  62, 19});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_BNB,  &bnb_boll25_h3,  "bnbusdt",  10800, "BNB-BOLL25-H3",  2.08, 1.93,  86,  68, 19});
    // TIER 2 — moderate OOS edge, still deploying for shadow observation

    // ── NEW SYMBOL engines (Session 20) — NEAR/SUI/APT/ARB ─────────────────
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d1,  "nearusdt", 86400, "NEAR-TSMOM-D1",  2.79, 2.61, 100,  46, 20});
    g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h12, "nearusdt", 43200, "NEAR-TSMOM-H12", 1.92, 3.03,  95, 126, 20});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h6,  "nearusdt", 21600, "NEAR-TSMOM-H6",  1.85, 3.62, 100, 257, 20});
    g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h4,  "nearusdt", 14400, "NEAR-TSMOM-H4",  2.17, 3.59, 100, 209, 20});  // AUDIT-2026-S35 revived: protected-bvr PF=22.66 Sharpe=13.6 worst=-72bp MDD/cum=0.1%
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h3,  "nearusdt", 10800, "NEAR-TSMOM-H3",  1.75, 3.65,  87, 351, 20});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h6,   "suiusdt",  21600, "SUI-TSMOM-H6",   1.80, 3.22, 100, 129, 20});
    g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h4,   "suiusdt",  14400, "SUI-TSMOM-H4",   1.44, 2.11,  88, 169, 20});  // AUDIT-2026-S35 revived: protected-bvr PF=16.46 Sharpe=16.6 worst=-72bp MDD/cum=0.2%
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_APT,  &apt_tsmom_h6,   "aptusdt",  21600, "APT-TSMOM-H6",   1.82, 3.32,  92, 149, 20});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ARB,  &arb_tsmom_h6,   "arbusdt",  21600, "ARB-TSMOM-H6",   1.48, 2.31,  80, 131, 20});

// ── Counter-trend on new symbols (Session 21) — NEAR/SUI/APT/ARB ────
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_NEAR, &near_rsi_h3, "nearusdt", 10800, "NEAR-RSI-H3", 2.39, 1.78, 47, 26, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h3, "suiusdt", 10800, "SUI-RSI-H3", 5.87, 1.89, 49, 11, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h2, "suiusdt", 7200, "SUI-RSI-H2", 2.05, 1.16, 49, 13, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SUI, &sui_rsi_h4, "suiusdt", 14400, "SUI-RSI-H4", 1.62, 0.86, 42, 17, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h1, "aptusdt", 3600, "APT-RSI-H1", 1.81, 0.73, 83, 39, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_APT, &apt_rsi_h3, "aptusdt", 10800, "APT-RSI-H3", 2.90, 1.61, 46, 12, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h2, "arbusdt", 7200, "ARB-RSI-H2", 4.80, 2.66, 100, 13, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h4, "arbusdt", 14400, "ARB-RSI-H4", 2.46, 1.54, 84, 14, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ARB, &arb_rsi_h3, "arbusdt", 10800, "ARB-RSI-H3", 3.71, 2.22, 66, 11, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_APT, &apt_boll_h2, "aptusdt", 7200, "APT-BOLL-H2", 4.55, 3.05, 100, 29, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_SUI, &sui_boll_h6, "suiusdt", 21600, "SUI-BOLL-H6", 1.54, 1.05, 86, 146, 31});  // tier-A re-validated 5yr Binance

    // ── Exotic TFs + extended counter-trend (Session 21) — 100 engines ────
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_h8, "btcusdt", 28800, "BTC-TSMOM-H8", 1.99, 2.55, 82, 77, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_h8, "ethusdt", 28800, "ETH-TSMOM-H8", 2.90, 5.10, 100, 121, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h8, "linkusdt", 28800, "LINK-TSMOM-H8", 2.95, 4.78, 100, 119, 21});  // AUDIT-2026-S35 revived: protected-bvr PF=31.02 Sharpe=13.7 worst=-72bp MDD/cum=0.1%
    g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h8, "nearusdt", 28800, "NEAR-TSMOM-H8", 2.10, 3.79, 97, 171, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h8, "bnbusdt", 28800, "BNB-TSMOM-H8", 2.86, 3.67, 100, 138, 21});
    g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h8, "dogeusdt", 28800, "DOGE-TSMOM-H8", 2.02, 2.54, 100, 107, 21});
    g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h8, "avaxusdt", 28800, "AVAX-TSMOM-H8", 1.38, 1.05, 100, 609, 21});  // AUDIT-2026 revived: bvr PF=1.38 n=609 Sh=1.05
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_h8, "suiusdt", 28800, "SUI-TSMOM-H8", 2.27, 2.50, 81, 62, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_h8, "arbusdt", 28800, "ARB-TSMOM-H8", 2.01, 2.84, 50, 86, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_h16, "btcusdt", 57600, "BTC-TSMOM-H16", 5.16, 4.01, 100, 22, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_h16, "solusdt", 57600, "SOL-TSMOM-H16", 3.47, 3.77, 100, 54, 21});
    g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_h16, "xrpusdt", 57600, "XRP-TSMOM-H16", 4.72, 4.14, 100, 55, 21});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h16, "linkusdt", 57600, "LINK-TSMOM-H16", 1.37, 0.85, 100, 272, 21});  // AUDIT-2026 revived: bvr PF=1.37 n=272 Sh=0.85
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h16, "bnbusdt", 57600, "BNB-TSMOM-H16", 2.76, 2.70, 100, 61, 21});
    g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h16, "dogeusdt", 57600, "DOGE-TSMOM-H16", 2.16, 2.33, 92, 54, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_h16, "suiusdt", 57600, "SUI-TSMOM-H16", 2.13, 2.16, 85, 40, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_h16, "arbusdt", 57600, "ARB-TSMOM-H16", 2.33, 2.84, 40, 43, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_d2, "btcusdt", 172800, "BTC-TSMOM-D2", 38.30, 7.17, 100, 10, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_d2, "ethusdt", 172800, "ETH-TSMOM-D2", 5.99, 2.48, 88, 10, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_d2, "solusdt", 172800, "SOL-TSMOM-D2", 5.97, 3.30, 82, 14, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_d2, "xrpusdt", 172800, "XRP-TSMOM-D2", 26.86, 3.35, 100, 22, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d2, "linkusdt", 172800, "LINK-TSMOM-D2", 28.76, 3.56, 100, 14, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d2, "nearusdt", 172800, "NEAR-TSMOM-D2", 4.09, 2.62, 53, 13, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_d2, "bnbusdt", 172800, "BNB-TSMOM-D2", 13.09, 2.78, 100, 31, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_d2, "dogeusdt", 172800, "DOGE-TSMOM-D2", 4.99, 3.51, 100, 20, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_d2, "suiusdt", 172800, "SUI-TSMOM-D2", 3.84, 2.10, 91, 13, 21});

    // AUDIT-2026-S35: NEW SYMBOL engines — FET/TIA/ONDO TSMOM at slower TFs.
    // Validated protected-bvr stats noted below (sym, tf, PF, Sharpe, MDD bp).
    g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_h8,   "fetusdt",  28800, "FET-TSMOM-H8",   68.23, 13.71, 100, 1479, 35});  // S35 new: PF=68 Sh=13.7 worst=-72bp MDD=-144bp
    g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_h12,  "fetusdt",  43200, "FET-TSMOM-H12", 120.29, 11.57, 100,  980, 35});  // S35 new: PF=120 Sh=11.6
// S44-CULL:     g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_d1,   "fetusdt",  86400, "FET-TSMOM-D1",  331.33,  8.66, 100,  451, 35});  // S35 new: PF=331 Sh=8.7
    g_slots.push_back({chimera::SYM_TIA,  &tia_tsmom_h8,   "tiausdt",  28800, "TIA-TSMOM-H8",   56.38, 12.73, 100,  631, 35});  // S35 new: PF=56 Sh=12.7
    g_slots.push_back({chimera::SYM_TIA,  &tia_tsmom_h12,  "tiausdt",  43200, "TIA-TSMOM-H12",  61.89, 10.53, 100,  425, 35});  // S35 new: PF=62 Sh=10.5
// S44-CULL:     g_slots.push_back({chimera::SYM_ONDO, &ondo_tsmom_h12, "ondousdt", 43200, "ONDO-TSMOM-H12", 33.54, 11.08, 100,  180, 35});  // S35 new: PF=33 Sh=11.1 (shortest history, ~1yr)

    // AUDIT-2026-S35 WAVE 2: HBAR / INJ / ADA / TRX / SEI TSMOM engines.
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_h8,  "hbarusdt", 28800, "HBAR-TSMOM-H8",  34.94, 10.00, 100, 1458, 35});  // S35 w2: PF=35 Sh=10.0
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_h12, "hbarusdt", 43200, "HBAR-TSMOM-H12", 47.04,  8.36, 100,  934, 35});  // S35 w2: PF=47 Sh=8.4
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_d1,  "hbarusdt", 86400, "HBAR-TSMOM-D1",  72.48,  7.40, 100,  430, 35});  // S35 w2: PF=72 Sh=7.4
    g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_h8,   "injusdt",  28800, "INJ-TSMOM-H8",   42.00, 13.00, 100, 1546, 35});  // S35 w2: PF=42 Sh=13.0
    g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_h12,  "injusdt",  43200, "INJ-TSMOM-H12",  80.88, 11.17, 100, 1043, 35});  // S35 w2: PF=81 Sh=11.2
// S44-CULL:     g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_d1,   "injusdt",  86400, "INJ-TSMOM-D1",  146.79,  8.81, 100,  479, 35});  // S35 w2: PF=147 Sh=8.8
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_h8,   "adausdt",  28800, "ADA-TSMOM-H8",   27.17, 11.48, 100, 1438, 35});  // S35 w2: PF=27 Sh=11.5
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_h12,  "adausdt",  43200, "ADA-TSMOM-H12",  41.00, 10.27, 100,  954, 35});  // S35 w2: PF=41 Sh=10.3
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_d1,   "adausdt",  86400, "ADA-TSMOM-D1",   72.66,  7.20, 100,  423, 35});  // S35 w2: PF=73 Sh=7.2
// S44-CULL:     g_slots.push_back({chimera::SYM_TRX,  &trx_tsmom_d1,   "trxusdt",  86400, "TRX-TSMOM-D1",   23.00,  6.61, 100,  612, 35});  // S35 w2: PF=23 Sh=6.6 (stable carry — slow TF only)
// S44-CULL:     g_slots.push_back({chimera::SYM_TRX,  &trx_tsmom_d2,   "trxusdt", 172800, "TRX-TSMOM-D2",   42.34,  5.62, 100,  305, 35});  // S35 w2: PF=42 Sh=5.6
    g_slots.push_back({chimera::SYM_SEI,  &sei_tsmom_h8,   "seiusdt",  28800, "SEI-TSMOM-H8",   49.69, 13.39, 100,  722, 35});  // S35 w2: PF=50 Sh=13.4
// S44-CULL:     g_slots.push_back({chimera::SYM_SEI,  &sei_tsmom_h12,  "seiusdt",  43200, "SEI-TSMOM-H12",  66.76, 10.13, 100,  461, 35});  // S35 w2: PF=67 Sh=10.1
// S44-CULL:     g_slots.push_back({chimera::SYM_SEI,  &sei_tsmom_d1,   "seiusdt",  86400, "SEI-TSMOM-D1",  124.40,  7.42, 100,  208, 35});  // S35 w2: PF=124 Sh=7.4
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_d3, "btcusdt", 259200, "BTC-TSMOM-D3", 242.75, 6.40, 100, 15, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH, &eth_tsmom_d3, "ethusdt", 259200, "ETH-TSMOM-D3", 7.40, 2.70, 89, 11, 21});
    // S37-KILL: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_d3, "xrpusdt", 259200, "XRP-TSMOM-D3", 45.74, 3.90, 100, 11, 21});  // prod_tiered PF=0.923 bp=-1481 n=37
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_d3, "linkusdt", 259200, "LINK-TSMOM-D3", 10.45, 3.74, 100, 10, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_d3, "nearusdt", 259200, "NEAR-TSMOM-D3", 99.90, 6.08, 44, 10, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_d3, "bnbusdt", 259200, "BNB-TSMOM-D3", 34.14, 2.67, 100, 13, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_d3, "dogeusdt", 259200, "DOGE-TSMOM-D3", 3.72, 2.05, 57, 13, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_d3, "suiusdt", 259200, "SUI-TSMOM-D3", 2.38, 1.43, 56, 10, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_BNB, &bnb_rsi_h6, "bnbusdt", 21600, "BNB-RSI-H6", 373.91, 2.49, 100, 14, 21});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_DOGE, &doge_rsi_h6, "dogeusdt", 21600, "DOGE-RSI-H6", 2.25, 1.10, 65, 108, 31});  // tier-A re-validated 5yr Binance
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h6, "btcusdt", 21600, "BTC-BOLL-H6", 8.04, 3.24, 100, 18, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SOL, &sol_boll_h6, "solusdt", 21600, "SOL-BOLL-H6", 5.77, 3.23, 84, 14, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_boll_h6, "linkusdt", 21600, "LINK-BOLL-H6", 99.90, 6.18, 100, 14, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_DOGE, &doge_boll_h6, "dogeusdt", 21600, "DOGE-BOLL-H6", 99.90, 4.78, 100, 15, 21});
    // DISABLED: g_slots.push_back({chimera::SYM_XRP, &xrp_rsi_h12, "xrpusdt", 43200, "XRP-RSI-H12", 63.22, 2.33, 80, 11, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h12, "btcusdt", 43200, "BTC-BOLL-H12", 6.45, 2.64, 97, 10, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h12, "xrpusdt", 43200, "XRP-BOLL-H12", 54.35, 2.52, 96, 10, 21});
    // S37-KILL: g_slots.push_back({chimera::SYM_SUI, &sui_donch_h6, "suiusdt", 21600, "SUI-DONCH-H6", 3.34, 2.49, 86, 80, 31});  // prod_tiered PF=0.576 bp=-2314 n=21 (tier-A claim stale — fresh OOS fails)

    // Session 22 engines
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_boll_h8, "btcusdt", 28800, "BTC-BOLL-H8", 2.02, 0.93, 44, 20, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SOL, &sol_boll_h8, "solusdt", 28800, "SOL-BOLL-H8", 4.44, 1.90, 81, 11, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_boll_h8, "bnbusdt", 28800, "BNB-BOLL-H8", 5.10, 2.64, 45, 12, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_boll_h8, "linkusdt", 28800, "LINK-BOLL-H8", 6.83, 2.70, 86, 24, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP, &xrp_boll_h8, "xrpusdt", 28800, "XRP-BOLL-H8", 2.56, 1.45, 62, 34, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_DOGE, &doge_boll_h8, "dogeusdt", 28800, "DOGE-BOLL-H8", 4.24, 2.11, 50, 14, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI, &sui_boll_h8, "suiusdt", 28800, "SUI-BOLL-H8", 2.49, 1.55, 66, 13, 22});
    // DISABLED: g_slots.push_back({chimera::SYM_ETH, &eth_rsi_h16, "ethusdt", 57600, "ETH-RSI-H16", 158.17, 9.07, 100, 24, 22});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_NEAR, &near_boll_h16, "nearusdt", 57600, "NEAR-BOLL-H16", 2.94, 1.15, 55, 10, 22});
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP, &xrp_donch_h8, "xrpusdt", 28800, "XRP-DONCH-H8", 3.05, 2.21, 100, 45, 22});
// S44-CULL:     g_slots.push_back({chimera::SYM_NEAR, &near_donch_h8, "nearusdt", 28800, "NEAR-DONCH-H8", 2.43, 2.13, 73, 55, 22});
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP, &xrp_donch_h16, "xrpusdt", 57600, "XRP-DONCH-H16", 4.88, 1.88, 100, 19, 22});
// S44-CULL:     g_slots.push_back({chimera::SYM_BNB, &bnb_donch_h16, "bnbusdt", 57600, "BNB-DONCH-H16", 5.33, 1.75, 87, 79, 31});  // tier-A re-validated 5yr Binance
    // S37-KILL: g_slots.push_back({chimera::SYM_BTC, &btc_donch_h16, "btcusdt", 57600, "BTC-DONCH-H16", 2.31, 1.11, 88, 76, 31});  // prod_tiered PF=0.703 bp=-1554 n=38
// S44-CULL:     g_slots.push_back({chimera::SYM_NEAR, &near_donch_h16, "nearusdt", 57600, "NEAR-DONCH-H16", 1.87, 1.36, 58, 29, 22});
    // S37-KILL: g_slots.push_back({chimera::SYM_SOL, &sol_donch_h16, "solusdt", 57600, "SOL-DONCH-H16", 2.08, 0.95, 49, 14, 22});  // prod_tiered PF=0.731 bp=-1198 n=26
// S44-CULL:     g_slots.push_back({chimera::SYM_BNB, &bnb_donch_d2, "bnbusdt", 172800, "BNB-DONCH-D2", 99.90, 5.55, 93, 14, 22});

    // Session 24 engines — DONCHIAN gap-fill + TSMOM H12 fill (15 engines)
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h6,   "xrpusdt",  21600, "XRP-DONCH-H6",   2.63, 2.25, 100,  45, 24});
    // S37-WF-KILL: g_slots.push_back({chimera::SYM_BNB,  &bnb_donch_h6,   "bnbusdt",  21600, "BNB-DONCH-H6",   2.08, 1.84,  64,  31, 24});  // walk-forward: PF 90d=0.45 365d=0.95 730d=0.77 5yr=1.02 — losing 3 of 4 windows
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP,  &xrp_donch_h12,  "xrpusdt",  43200, "XRP-DONCH-H12",  3.40, 2.09, 100,  26, 24});
    // S37-WF-KILL: g_slots.push_back({chimera::SYM_NEAR, &near_donch_h12, "nearusdt", 43200, "NEAR-DONCH-H12", 2.25, 1.43,  65,  21, 24});  // walk-forward: PF 90d=0.39 365d=0.82 730d=1.06 5yr=1.28 — losing 4 of 4 windows
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h12,  "solusdt",  43200, "SOL-TSMOM-H12",  1.91, 2.30,  86, 120, 24});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h12,  "bnbusdt",  43200, "BNB-TSMOM-H12",  2.45, 3.08, 100,  96, 24});
    g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h12, "linkusdt", 43200, "LINK-TSMOM-H12", 1.34, 1.24, 100, 643, 24});  // AUDIT-2026 revived: bvr PF=1.34 n=643 Sh=1.24
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h12,  "xrpusdt",  43200, "XRP-TSMOM-H12",  1.54, 1.52,  73, 153, 24});

    // ── Session 26 — RSI_REVERT H4 + BOLLINGER H4/H2 (10 engines) ────────
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi_h4,     "ethusdt",  14400, "ETH-RSI-H4",     1.82, 1.45,  72,  38, 26});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_XRP,  &xrp_rsi_h4,     "xrpusdt",  14400, "XRP-RSI-H4",     1.74, 1.38,  70,  45, 26});
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_SOL,  &sol_boll_h2,    "solusdt",   7200, "SOL-BOLL-H2",    1.53, 1.19,  61,  52, 26});
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP,  &xrp_boll_h4,    "xrpusdt",  14400, "XRP-BOLL-H4",    1.56, 1.21,  62,  29, 26});

    // ── Session 27 — H12 gap-fill (2 engines) ──────────────────────────────
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH,  &eth_boll_h12,   "ethusdt",  43200, "ETH-BOLL-H12",  48.01, 3.02, 100,  12, 27});

    // ── Session 28 — KELTNER_REVERT + DUAL_THRUST (8 engines) ───────────────
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_keltner_h12, "linkusdt", 43200, "LINK-KELTNER-H12", 6.85, 2.21,  66,  11, 28});

    // Ichimoku engines (6) — Session 29
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_DOGE, &doge_ichi_h12,    "dogeusdt", 43200, "DOGE-ICHI-H12",    0.00, 0.00,   0,   0, 29});

    // S37-NEW: ICHIMOKU D1 sweep found edges
// S44-CULL:     g_slots.push_back({chimera::SYM_BTC, &btc_ichi_d1, "btcusdt", 86400, "BTC-ICHI-D1", 3.77, 2.86, 100, 99, 37});  // S37: sweep PF=3.77 Sh=2.86 5yr OOS, walk-forward 365d=3.23 730d=4.43
// S44-CULL:     g_slots.push_back({chimera::SYM_ETH, &eth_ichi_d1, "ethusdt", 86400, "ETH-ICHI-D1", 8.61, 4.38, 100, 79, 37});  // S37: sweep PF=8.61 Sh=4.38 5yr OOS, walk-forward 365d=5.83 730d=7.02

    // S37-EDGE-PASS: 9 more engines passed broad sweep + walk-forward
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL,  &sol_ichi_d1,  "solusdt",  86400, "SOL-ICHI-D1",  4.34, 3.45,  91,  53, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_BNB,  &bnb_ichi_d1,  "bnbusdt",  86400, "BNB-ICHI-D1", 13.34, 6.71,  95,  61, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_ichi_d1, "linkusdt", 86400, "LINK-ICHI-D1", 9.95, 3.50,  95,  38, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_AVAX, &avax_ichi_d1, "avaxusdt", 86400, "AVAX-ICHI-D1", 4.02, 2.49,  88,  34, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_DOGE, &doge_ichi_d1, "dogeusdt", 86400, "DOGE-ICHI-D1", 3.38, 2.45,  84,  25, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_XRP,  &xrp_ichi_d1,  "xrpusdt",  86400, "XRP-ICHI-D1",  4.98, 2.86,  94,  51, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_ETH,  &eth_ichi_h12, "ethusdt",  43200, "ETH-ICHI-H12", 3.87, 3.86,  94, 169, 37});
    g_slots.push_back({chimera::SYM_SOL,  &sol_ichi_h8,  "solusdt",  28800, "SOL-ICHI-H8",  5.92, 6.20,  94, 188, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_keltner_h6, "linkusdt", 21600, "LINK-KELTNER-H6", 6.12, 2.69, 91, 35, 37});
    g_slots.push_back({chimera::SYM_NEAR, &near_ichi_h8, "nearusdt", 28800, "NEAR-ICHI-H8", 2.92, 4.67, 89, 192, 37});  // S37: bear-stress passed all 4 windows PF>=2.35

    // SuperTrend engines (6) — Session 29

    // Williams %R engines (4) — Session 29b

    // Stochastic RSI engines (4) — Session 29b

    // W1 mean-reversion engines (Session 30, Edge 6)

    // ── AUDIT-2026 DARK-SYMBOL REVIVAL (ETH/LINK DONCHIAN) ─────────────────
// S44-CULL:     g_slots.push_back({chimera::SYM_ETH,  &eth_donch_d3,   "ethusdt",  259200, "ETH-DONCH-D3",  9.77, 1.82,  90, 30, 31});
// S44-CULL:     g_slots.push_back({chimera::SYM_ETH,  &eth_donch_d2,   "ethusdt",  172800, "ETH-DONCH-D2",  2.05, 0.71,  90, 39, 31});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_donch_d2,  "linkusdt", 172800, "LINK-DONCH-D2", 1.89, 0.66,  76, 37, 31});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_donch_h16, "linkusdt",  57600, "LINK-DONCH-H16",1.75, 0.67,  85, 72, 31});
    // ── AUDIT-2026 ROUND 2: ETH-RSI + APT/ARB revival ──────────────────────
    // S37-KILL: g_slots.push_back({chimera::SYM_ETH,  &eth_rsi_h12,    "ethusdt",   43200, "ETH-RSI-H12",   2.37, 0.92,  69, 39, 31});  // prod_tiered PF=0.966 bp=-59 n=7
    g_slots.push_back({chimera::SYM_APT,  &apt_tsmom_h8,   "aptusdt",   28800, "APT-TSMOM-H8",  1.49, 1.42,  85, 404, 31});
    // S37-WF-KILL: g_slots.push_back({chimera::SYM_APT,  &apt_donch_h6,   "aptusdt",   21600, "APT-DONCH-H6",  1.67, 1.03,  69, 97, 31});  // walk-forward: PF 90d=0.90 365d=0.81 730d=0.77 5yr=1.32 — losing 3 of 4 windows
// S44-CULL:     g_slots.push_back({chimera::SYM_ARB,  &arb_tsmom_d2,   "arbusdt",  172800, "ARB-TSMOM-D2",  1.98, 1.38,  74, 81, 31});
// S44-CULL:     g_slots.push_back({chimera::SYM_ARB,  &arb_donch_h6,   "arbusdt",   21600, "ARB-DONCH-H6",  1.70, 1.13,  80, 83, 31});

// S43-CULL-2026-05-29: S41 + S42 tombstoned. Their "4 WF windows" gate was 4 IS
// lookback slices (134/180/365/730d), NOT held-out OOS. 278 engines re-tested
// via prod backtest_harness.cpp with TRUE held-out windows ([-1460,-1095] IS and
// [-1095,-730] OOS, both pre-S42-discover cutoff at -730d).
// 137 PASSED   (PF>=1.3 both windows, n>=20, bp>0) -> repromoted as S43.
//  18 FAIL/COND (PF<1.3 in OOS or negative bp) -> stay culled.
// 124 SKIPPED  (symbol launched after holdout window — cannot WF-validate).
// Backup: /Users/jo/Chimera_Baselines/pre_cull_20260529_162344
// Verdict CSV: /tmp/cull_holdout_verdict.csv
// #include "engines_s41_consolidated.cpp"  // CULLED
// #include "engines_s42_consolidated.cpp"  // CULLED
#include "engines_s43_repromote.cpp"        // 137 strict-WF-validated engines
#include "engines_s43b_holdout.cpp"         // 142 fresh-discover holdout-validated
#include "engines_gems.cpp"                  // S52: 13 strict-validated salvaged gems (s41/s42)
#include "engines_mr.cpp"                    // S53: 34 mean-reversion dip-buyers (spot-long, profit in chop)
#include "engines_lowturn.cpp"               // S54m: 8 low-turnover trend engines (wide stop/no ratchet, macro-gated, shadow)

    // ── S42b: SYMBOL WHITELIST FILTER (Binance 50-sym cap) ────────────────
    // Reads config/symbol_whitelist.json, drops g_slots entries for non-
    // whitelisted symbols. Prevents firing orders for syms not in user's
    // Binance API whitelist. Update file + restart to change.
    {
        std::ifstream wf("config/symbol_whitelist.json");
        std::set<std::string> whitelist;
        if (wf.is_open()) {
            std::string txt((std::istreambuf_iterator<char>(wf)),
                            std::istreambuf_iterator<char>());
            wf.close();
            // Crude JSON array extract: pull every "xxxusdt" quoted token in the array.
            size_t pos = 0;
            while ((pos = txt.find("\"", pos)) != std::string::npos) {
                size_t end = txt.find("\"", pos + 1);
                if (end == std::string::npos) break;
                std::string tok = txt.substr(pos + 1, end - pos - 1);
                if (tok.size() > 4 && tok.substr(tok.size() - 4) == "usdt") {
                    whitelist.insert(tok);
                }
                pos = end + 1;
            }
        }
        if (whitelist.empty()) {
            std::printf("[WHITELIST] config/symbol_whitelist.json missing/empty — no filter applied\n");
        } else {
            int before = (int)g_slots.size();
            std::map<std::string,int> dropped_by_sym;
            std::vector<EngineSlot> kept;
            kept.reserve(g_slots.size());
            for (const auto& s : g_slots) {
                if (whitelist.count(s.symbol_str)) {
                    kept.push_back(s);
                } else {
                    dropped_by_sym[s.symbol_str]++;
                }
            }
            g_slots = std::move(kept);
            int after = (int)g_slots.size();
            std::printf("[WHITELIST] %zu syms allowed, dropped %d slots (was %d -> now %d)\n",
                whitelist.size(), before - after, before, after);
            for (const auto& [sym, cnt] : dropped_by_sym) {
                std::printf("[WHITELIST]   dropped %s: %d engine(s)\n", sym.c_str(), cnt);
            }
            std::fflush(stdout);
        }
    }

    // ── S34 PF FILTER: load batch-validation PFs and disable bleed engines ─
    load_pf_data_into_slots();

    // ── S34: sort g_slots so highest-PF engines evaluated first per symbol ─
    // Combined with MAX_PER_SYMBOL=1, this ensures the BEST engine for a
    // symbol wins the singleton slot when multiple signal same bar.
    std::sort(g_slots.begin(), g_slots.end(), [](const EngineSlot& a, const EngineSlot& b){
        return a.bt_pf > b.bt_pf;
    });
    std::printf("[STARTUP] g_slots sorted by bt_pf desc — top-PF engines win singleton slots\n");
    std::fflush(stdout);

    // ── S34 SAFETY PRESET — TIERED ────────────────────────────────────────
    // Three tiers:
    //   ELITE (bt_pf >= 2.0 AND bt_trades >= 30):
    //     apply_protection_only_preset — keep bespoke trail config, override
    //     only loss caps + BE lock + giveback. Run "as specified" with
    //     profit-protection overlay.
    //   ACCEPTABLE (1.3 <= bt_pf < 2.0):
    //     apply_safety_preset — full tight preset incl. trail overrides.
    //   BLOCKED (bt_pf < 1.3 OR no data):
    //     pf_blocked already set by load_pf_data_into_slots(); still apply
    //     full safety preset as belt-and-suspenders (in case unblocked later).
    {
        int elite = 0, tight = 0, blocked_tight = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (slot.pf_blocked) {
                slot.engine->apply_safety_preset();
                blocked_tight++;
            } else if (slot.bt_pf >= 2.0 && slot.bt_trades >= 30) {
                slot.engine->apply_protection_only_preset();
                elite++;
            } else {
                slot.engine->apply_safety_preset();
                tight++;
            }
        }
        std::printf("[SAFETY] tiered preset applied: elite=%d (PF>=2.0, bespoke trail kept), tight=%d (PF>=1.3, tight trail), blocked_tight=%d\n",
            elite, tight, blocked_tight);
        std::printf("[SAFETY] S36-rewrite: staged-ratchet ONLY (BE-lock@RT+10mfe, progressive lock 75/85/90/95%%). DISABLED: hard_floor / early_kill / giveback / signal_confirm. Verified +1.07M bp recovery vs old presets across 17-engine 5yr OOS.\n");
        std::fflush(stdout);

        // S44N: re-apply S44k/M/i overrides AFTER slot-loop's preset call
        // (which resets be_arm to rt+10, signal_confirm to 1, hard_floor to 0).
        // wire_engine already set these but slot loop just overrode them.
        // Idempotent for non-slot engines (no-op since already set).
        int n_reapplied = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            slot.engine->set_be_arm_bp(25.0);
            slot.engine->set_ratchet_lock_pct(0.85);
            slot.engine->set_hard_floor_bp(-170.0);
            slot.engine->set_signal_confirm_bars(2);
            n_reapplied++;
        }
        std::printf("[S44N] Re-applied S44k/M/i overrides on %d slot engines (be_arm=25 lock=0.85 hard_floor=-170 signal_confirm=2)\n", n_reapplied);
        std::fflush(stdout);

        // ── S45: FLOOR FIX — enforce the REAL hard floor on ALL wired engines ──
        // ROOT CAUSE of the 29-May overnight blow-up: the slot-loop preset above
        // resets hard_floor to 0, and S44N only restored it on the 24 g_slots
        // engines. The 308 holdout (S43/S43b) engines were left at hard_floor=0,
        // so their stops sat at full ATR width — SEI ran to -422bp, FET to -2159bp
        // before exit, while the journal cosmetically showed -170. This loop makes
        // the -170 floor a real entry-time stop on EVERY engine. A loser can now
        // never exceed -170bp. Idempotent for slots (already set).
        int n_floor = 0;
        for (auto* e : g_all_wired) {
            if (!e) continue;
            // S54m: LOW-TURNOVER sleeve (tag "LT-") is EXEMPT from the standard
            // tight-stop/ratchet override. It deliberately uses wide stops, NO
            // ratchet, long hold -> ~71% fewer trades -> less cost drag. Validated
            // 2.25x better than the standard config in bull slices (the only regime
            // it trades, via the macro-gate). Its params come from its Config; here
            // we only widen the floor so the wide stop is the real exit.
            if (e->cfg().tag.rfind("LT-", 0) == 0) {
                e->set_hard_floor_bp(-800.0);     // wide; the 8-ATR stop is the exit
                e->set_signal_confirm_bars(2);
                e->set_ratchet_start_bp(0.0);     // ratchet OFF (no churn)
                e->set_be_arm_bp(1e9);            // BE-lock OFF (hold the trend)
                n_floor++;
                continue;
            }
            e->set_hard_floor_bp(-170.0);     // real SL tightened to -170bp at entry
            e->set_signal_confirm_bars(2);    // 2-bar confirm — fewer chop entries
            // S54: close the "no protection below cost-MFE" gap. The ratchet only
            // engaged at mfe >= ratchet_start (=rt~22), so trades that popped +8..
            // +22bp then reversed (XLM +19.8, ENA +22.5, DOT +8.3 on 31-May) had
            // ZERO stop-tightening and rode to the -170 floor. Lower the gate to 8
            // (Stage-2 ramp rescues "almost made it" trades) and the profit-lock to
            // 15. Validated 3 windows (full / held-out-120d / recent-90d): total
            // +111k->+250k, PF 7.5->11.1, maxdd 60k->42k, PASS 31->53/65 — drawdown
            // DOWN, so edge-preserving (ratchet trails up; only pop-faders caught).
            e->set_ratchet_start_bp(8.0);
            e->set_be_arm_bp(15.0);
            n_floor++;
        }
        std::printf("[S45-FLOOR] Enforced hard_floor=-170 + signal_confirm=2 + S54 ratchet_start=8/be_arm=15 on ALL %d wired engines (protection-gap closed)\n", n_floor);
        std::fflush(stdout);
    }

    // ── S38b: PYRAMID-XLOW — enable aggressive pyramid on all slots ───────
    // Backtest result (26k configs × 4 WF windows): 99.2% of high-PF
    // engines gain +500-12000 bp from pyramid_arm_atr=0.5; zero engines
    // hurt. Pyramid only arms after BE-locked trail. Worst case bounded.
    // S44: PYRAMID-ELITE now applied via wire_engine() lambda for ALL engines
    // (incl S43/S43b includes not in g_slots). See line ~1471.
    std::printf("[S44] PYRAMID-ELITE armed via wire_engine on all wired engines\n");
    std::fflush(stdout);

    // ── Wire up bar callbacks for persistence + audit trail ────────────────
    for (auto& slot : g_slots) {
        if (slot.engine) slot.engine->set_on_bar(on_bar_callback);
    }

    // ── Activate vol_filter + mtf_gate on all counter-trend engines ──────
    // Counter-trend = RSI_REVERT, BOLLINGER, KELTNER_REVERT
    // These benefit from suppression during chaos and bearish D1 trends.
    {
        int vol_count = 0, mtf_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (!slot.engine->is_trend_following()) {
                // Counter-trend engine — enable both filters
                slot.engine->enable_vol_filter(true);
                vol_count++;
                // Only apply MTF gate to sub-D1 timeframes (D1 engines ARE the reference)
                if (slot.tf_secs < 86400) {
                    slot.engine->enable_mtf_gate(true);
                    mtf_count++;
                }
            }
        }
        std::printf("[STARTUP] Activated vol_filter on %d counter-trend engines\n", vol_count);
        std::printf("[STARTUP] Activated mtf_gate on %d sub-D1 counter-trend engines\n", mtf_count);
        std::fflush(stdout);
    }

    // ── Activate ADX filter on all trend-following engines (Session 29) ──────
    // ADX(14) < 25 = ranging market = suppress TSMOM/DONCHIAN/DUAL_THRUST/
    // ICHIMOKU/SUPERTREND entries (they need directional movement to profit).
    {
        int adx_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (slot.engine->is_trend_following()) {
                slot.engine->enable_adx_filter(true);
                slot.engine->set_adx_threshold(25.0);  // S32d: raised from 12 (was too lax). Real Wilder ADX min for trend = 25.
                adx_count++;
            }
        }
        std::printf("[STARTUP] Activated adx_filter (threshold=25) on %d trend-following engines\n", adx_count);
        std::fflush(stdout);
    }

    // ── Activate volume gate on ALL engines (Session 29) ────────────────────
    // Tick-count volume proxy: suppress entries when bar activity < 30% of
    // rolling average (detects weekend dead zones and exchange outages).
    {
        int vol_gate_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            slot.engine->enable_volume_gate(true);
            vol_gate_count++;
        }
        std::printf("[STARTUP] Activated volume_gate on %d engines\n", vol_gate_count);
        std::fflush(stdout);
    }

    // ── Activate correlation filter on non-BTC engines (Session 29b) ────────
    // When BTC correlation is extreme, altcoin entries get suppressed.
    {
        int corr_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            if (slot.symbol_id != chimera::SYM_BTC) {
                slot.engine->enable_corr_filter(true);
                corr_count++;
            }
        }
        std::printf("[STARTUP] Activated corr_filter on %d non-BTC engines\n", corr_count);
        std::fflush(stdout);
    }

    // ── Session filter DISABLED (shadow tuning 2026-05-17) ─────────────────
    // Was suppressing sub-H4 entries during 00-08 UTC (= 8am-4pm SGT).
    // On a Singapore VPS this kills prime trading hours. Disabled so all
    // engines can generate shadow trades 24/7 for proper PnL validation.
    // Re-evaluate once we have 4+ weeks of shadow data.
    {
        std::printf("[STARTUP] session_filter DISABLED — shadow mode needs 24/7 signal flow\n");
        std::fflush(stdout);
    }

    // ── Session 30: Initial funding rate fetch + position sizing setup ────
    {
        std::printf("[STARTUP] Fetching multi-symbol funding rates...\n");
        std::fflush(stdout);
        g_funding_filter.fetch_all();
        g_last_funding_fetch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Propagate initial funding state to all engines
        std::lock_guard<std::mutex> lk(g_engine_mtx);
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            slot.engine->set_funding_tailwind(g_funding_filter.has_tailwind(slot.symbol_id));
            slot.engine->set_funding_headwind(g_funding_filter.has_headwind(slot.symbol_id));
        }

        // Session 30, Edge 4: Set position sizing multipliers for top engines
        // Based on walk-forward validated OOS performance (PF * sqrt(trades) ranking)
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            // Top-tier engines get 1.5x sizing, default = 1.0
            // Identified from revalidation: LINK-BOLL-H6, BNB-DONCH-D2, XRP-BOLL-H12
            // Also top TSMOM: BTC-H12 PF=3.63, ETH-D1 PF=3.15, BNB-D1 PF=3.16
            if (slot.tag.find("LINK-BOLL-H6") != std::string::npos ||
                slot.tag.find("BNB-DONCH-D2") != std::string::npos ||
                slot.tag.find("XRP-BOLL-H12") != std::string::npos ||
                slot.tag.find("BTC-TSMOM-H12") != std::string::npos ||
                slot.tag.find("ETH-TSMOM-D1") != std::string::npos ||
                slot.tag.find("BNB-TSMOM-D1") != std::string::npos) {
                slot.engine->set_sizing_mult(1.5);
            }
        }
        std::printf("[STARTUP] Funding filter + position sizing initialized\n");
        std::fflush(stdout);
    }

    // ── Ensure data directories exist ────────────────────────────────────
    ::mkdir("data", 0755);        // no-op if exists
    ::mkdir("data/bars", 0755);   // bar persistence directory

    // ── Seed engine bar buffers ──────────────────────────────────────────
    // Strategy:
    //   1. If the engine has a NATIVE Binance interval (H1,H2,H4,H6,H8,H12,D1,D3):
    //      → Always fetch from REST (guaranteed 64 bars, signal-ready immediately)
    //      → Saved bars are ignored (REST is always fresher and more complete)
    //   2. If the engine has NO native interval (H3, H16, D2):
    //      → Try H1 aggregation first (fetch H1 klines, aggregate to target TF)
    //      → Fall back to saved bars if aggregation fails
    //      → Cold-start from ticks as last resort
    //
    // This fixes the bug where partial saved bars (e.g. 11 bars from short
    // runs) would block the REST fetch that provides 64 bars, leaving engines
    // stuck below their lookback threshold and unable to evaluate signals.
    {
        chimera::BinanceREST seed_rest;
        int total = (int)g_slots.size();
        std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
        std::printf("[STARTUP] SEEDING %d engines from Binance REST API...\n", total);
        std::printf("[STARTUP] (This takes 30-60s on first boot — REST + H1 aggregation)\n");
        std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
        std::fflush(stdout);

        int seeded_rest = 0, seeded_agg = 0, seeded_saved = 0, cold = 0;
        int progress = 0;

        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            progress++;

            // Progress update every 25 engines
            if (progress % 25 == 0 || progress == total) {
                std::printf("[SEED] Progress: %d/%d engines seeded...\n", progress, total);
                std::fflush(stdout);
            }

            const char* interval = tf_to_binance_interval(slot.tf_secs);
            bool has_native_interval = (interval && *interval);

            // ── Determine how many bars this engine actually needs ────────
            // Use the engine's computed max_history (accounts for lookback,
            // indicator periods like ichi_senkou_b=120, ADX, Keltner, etc.)
            int seed_bars_needed = slot.engine->max_history_needed();
            if (seed_bars_needed < 64) seed_bars_needed = 64;  // minimum floor

            std::printf("[SEED][%s] needs %d bars (max_history)\n",
                slot.tag.c_str(), seed_bars_needed);

            if (has_native_interval) {
                // ── Native interval: always use REST ──
                seed_engine_from_history(seed_rest, *slot.engine,
                                         slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                seeded_rest++;
            } else {
                // ── No native interval: choose best aggregation source ──────────
                // D2+ engines (tf >= 2 days): aggregate from D1 bars (gives 500+ bars)
                // Sub-day engines (H3, H16): aggregate from H1 bars (gives 62-333 bars)
                bool seeded_ok = false;

                if (slot.tf_secs >= 172800) {
                    // D2 or larger — use D1 aggregation for much deeper history
                    seed_engine_from_d1_aggregation(seed_rest, *slot.engine,
                                                    slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                    if (slot.engine->bars_in_buffer() > 0) {
                        seeded_ok = true;
                        seeded_agg++;
                    }
                }

                if (!seeded_ok) {
                    // H3/H16 or D1-agg fallback — use H1 aggregation
                    seed_engine_from_h1_aggregation(seed_rest, *slot.engine,
                                                    slot.symbol_str, slot.tf_secs, slot.tag, seed_bars_needed);
                    if (slot.engine->bars_in_buffer() > 0) {
                        seeded_ok = true;
                        seeded_agg++;
                    }
                }

                if (!seeded_ok) {
                    // Both aggregation paths failed — try saved bars as fallback
                    auto saved = load_saved_bars(slot.tag, seed_bars_needed);
                    if (!saved.empty()) {
                        int kept = slot.engine->seed_bars(saved);
                        std::printf("[WARM_START][%s] Seeded from saved bars: loaded=%d kept=%d\n",
                            slot.tag.c_str(), (int)saved.size(), kept);
                        std::fflush(stdout);
                        seeded_saved++;
                    } else {
                        std::fprintf(stderr, "[SEED][%s] No data source — cold-start from live ticks\n",
                            slot.tag.c_str());
                        cold++;
                    }
                }
            }
        }

        std::printf("[STARTUP] Seeding complete (g_slots): REST=%d  H1-agg=%d  saved=%d  cold=%d\n",
            seeded_rest, seeded_agg, seeded_saved, cold);
        std::fflush(stdout);

        // S44c: seed the non-slot wired engines (S43 + S43b cohorts).
        // Same logic as slot loop above but iterates g_all_wired instead.
        {
            int extra_rest = 0, extra_agg = 0, extra_cold = 0, extra_skipped = 0;
            int total_extra = 0;
            for (auto* e : g_all_wired) {
                if (!e) continue;
                bool in_slots = false;
                for (auto& s : g_slots) if (s.engine == e) { in_slots = true; break; }
                if (in_slots) { extra_skipped++; continue; }
                total_extra++;
            }
            std::printf("[STARTUP] Seeding %d non-slot wired engines (S43/S43b)\n", total_extra);
            std::fflush(stdout);
            int progress2 = 0;
            for (auto* e : g_all_wired) {
                if (!e) continue;
                bool in_slots = false;
                for (auto& s : g_slots) if (s.engine == e) { in_slots = true; break; }
                if (in_slots) continue;
                progress2++;
                if (progress2 % 25 == 0 || progress2 == total_extra) {
                    std::printf("[SEED-EXTRA] Progress: %d/%d\n", progress2, total_extra);
                    std::fflush(stdout);
                }
                const std::string& symstr = e->cfg().symbol;
                int64_t tf_secs = e->cfg().tf_secs;
                const std::string& tag = e->cfg().tag;
                int need = e->max_history_needed();
                if (need < 64) need = 64;
                const char* iv = tf_to_binance_interval(tf_secs);
                if (iv && *iv) {
                    seed_engine_from_history(seed_rest, *e, symstr, tf_secs, tag, need);
                    extra_rest++;
                } else {
                    // try H1 aggregation for sub-day, D1 agg for D2+
                    if (tf_secs >= 172800) {
                        auto kl = seed_rest.fetch_klines(symstr, "1d", need * (int)(tf_secs/86400) + 5);
                        if (!kl.empty()) {
                            // aggregate
                            std::vector<chimera::EdgeEngine::SeedBar> seeds;
                            seeds.reserve(kl.size());
                            for (auto& k : kl) {
                                chimera::EdgeEngine::SeedBar sb;
                                sb.open_ts_ms = k.open_ts_ms; sb.o=k.o; sb.h=k.h; sb.l=k.l; sb.c=k.c;
                                seeds.push_back(sb);
                            }
                            e->seed_bars(seeds);
                            extra_agg++;
                        } else extra_cold++;
                    } else {
                        auto kl = seed_rest.fetch_klines(symstr, "1h", need * (int)(tf_secs/3600) + 5);
                        if (!kl.empty()) {
                            std::vector<chimera::EdgeEngine::SeedBar> seeds;
                            seeds.reserve(kl.size());
                            for (auto& k : kl) {
                                chimera::EdgeEngine::SeedBar sb;
                                sb.open_ts_ms = k.open_ts_ms; sb.o=k.o; sb.h=k.h; sb.l=k.l; sb.c=k.c;
                                seeds.push_back(sb);
                            }
                            e->seed_bars(seeds);
                            extra_agg++;
                        } else extra_cold++;
                    }
                }
            }
            std::printf("[STARTUP] Seeding extra complete: REST=%d  agg=%d  cold=%d  skipped(in_slots)=%d\n",
                extra_rest, extra_agg, extra_cold, extra_skipped);
            std::fflush(stdout);
        }

        // ── S34-r8: seed per-symbol rally buffers from 1m klines ─────────
        // So per-symbol regime detector works from first second after restart.
        {
            const char* sym_binance_names[] = {
                "btcusdt","ethusdt","solusdt","bnbusdt","dogeusdt","xrpusdt",
                "linkusdt","avaxusdt","nearusdt","suiusdt","aptusdt","arbusdt"
            };
            const int sym_ids[] = {
                chimera::SYM_BTC, chimera::SYM_ETH, chimera::SYM_SOL, chimera::SYM_BNB,
                chimera::SYM_DOGE, chimera::SYM_XRP, chimera::SYM_LINK, chimera::SYM_AVAX,
                chimera::SYM_NEAR, chimera::SYM_SUI, chimera::SYM_APT, chimera::SYM_ARB
            };
            int sym_seeded = 0;
            for (size_t si = 0; si < sizeof(sym_ids)/sizeof(sym_ids[0]); si++) {
                int sid = sym_ids[si];
                if (sid < 0 || sid >= chimera::MAX_SYMBOLS) continue;
                auto kl = seed_rest.fetch_klines(sym_binance_names[si], "1m", 60);
                if (kl.empty()) continue;
                int n = (int)kl.size();
                int rally_n = std::min(n, SYM_RALLY_BUF);
                for (int i = n - rally_n; i < n; i++) {
                    int idx = g_sym_rally_idx[sid].load();
                    store_dbl_atomic(g_sym_rally_px[sid][idx], kl[i].c);
                    g_sym_rally_idx[sid].store((idx + 1) % SYM_RALLY_BUF);
                    int filled = g_sym_rally_filled[sid].load();
                    if (filled < SYM_RALLY_BUF) g_sym_rally_filled[sid].store(filled + 1);
                }
                if (n >= 2) {
                    double ret = (kl.back().c / kl.front().c - 1.0) * 100.0;
                    store_dbl_atomic(g_sym_short_ret[sid], ret);
                }
                sym_seeded++;
            }
            std::printf("[STARTUP] Per-symbol rally buffers: seeded %d symbols (60min each)\n", sym_seeded);
            std::fflush(stdout);
        }

        // ── S34: seed BTC rally + chart buffers from 1m klines ───────────
        // Without seed, buffers are empty after restart -> no rally signal
        // for ~10min. Seed with last 1m closes so detection works instantly.
        {
            auto kl = seed_rest.fetch_klines("btcusdt", "1m", 60);
            if (!kl.empty()) {
                int n = (int)kl.size();
                std::printf("[STARTUP] BTC buffers: seeded %d 1m closes\n", n);
                std::fflush(stdout);
                // Fill rally detector buffer (file-scope global)
                int rally_n = std::min(n, BTC_RALLY_BUF);
                for (int i = n - rally_n; i < n; i++) {
                    int idx = g_btc_rally_idx.load(std::memory_order_relaxed);
                    store_dbl_atomic(g_btc_rally_px[idx], kl[i].c);
                    g_btc_rally_idx.store((idx + 1) % BTC_RALLY_BUF, std::memory_order_relaxed);
                    int filled = g_btc_rally_filled.load(std::memory_order_relaxed);
                    if (filled < BTC_RALLY_BUF) g_btc_rally_filled.store(filled + 1, std::memory_order_relaxed);
                }
                // Fill chart buffer
                int chart_n = std::min(n, BTC_CHART_N);
                for (int i = n - chart_n; i < n; i++) {
                    int idx = g_btc_chart_idx.load(std::memory_order_relaxed);
                    store_dbl_atomic(g_btc_chart_px[idx], kl[i].c);
                    g_btc_chart_idx.store((idx + 1) % BTC_CHART_N, std::memory_order_relaxed);
                    int filled = g_btc_chart_filled.load(std::memory_order_relaxed);
                    if (filled < BTC_CHART_N) g_btc_chart_filled.store(filled + 1, std::memory_order_relaxed);
                }
                if (n >= 2) {
                    double ret_pct = (kl.back().c / kl.front().c - 1.0) * 100.0;
                    store_dbl_atomic(g_btc_short_ret_bits, ret_pct);
                    std::printf("[STARTUP] BTC seeded short-ret over %dmin: %+.2f%%\n", n, ret_pct);
                    std::fflush(stdout);
                }
            }
        }

        // ── Post-seed audit: warn about under-seeded engines ──────────
        int under_seeded = 0;
        for (const auto& slot : g_slots) {
            if (!slot.engine) continue;
            int have = slot.engine->bars_in_buffer();
            int need = slot.engine->max_history_needed();
            if (have < need) {
                std::fprintf(stderr, "[SEED-WARN][%s] UNDER-SEEDED: have=%d need=%d — "
                             "signal won't fire until %d more bars accumulate from live data\n",
                             slot.tag.c_str(), have, need, need - have);
                under_seeded++;
            }
        }
        if (under_seeded > 0) {
            std::fprintf(stderr, "[STARTUP] WARNING: %d engines under-seeded — "
                         "check REST fetch limits or aggregation source\n", under_seeded);
        } else {
            std::printf("[STARTUP] All %d engines fully seeded — ready to trade immediately\n",
                        (int)g_slots.size());
        }
        std::fflush(stdout);

        // S54: seed the BTC 200d-MA macro gate (bull/bear master switch).
        init_macro_ma(seed_rest);
        init_grids();                         // S55: maker grid sleeve (shadow)
        init_macro_base();                    // S55: macro-bull base / bull-beta core (shadow)
    }

    // ── Position resume: restore open positions after restart ────────────
    // Reads data/open_positions.json (written every 60s by the snapshot loop)
    // and injects saved positions back into their matching engines.
    // This allows the bot to survive restarts/deploys without losing trades.
    {
        std::ifstream pf("data/open_positions.json");
        if (pf.is_open()) {
            std::string content((std::istreambuf_iterator<char>(pf)),
                                 std::istreambuf_iterator<char>());
            pf.close();

            int resumed = 0;
            // Simple JSON field extractor (no lib dependency)
            auto extract_str = [&](const std::string& blob, const char* key) -> std::string {
                std::string k = std::string("\"") + key + "\":\"";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return "";
                pos += k.size();
                auto end = blob.find('"', pos);
                if (end == std::string::npos) return "";
                return blob.substr(pos, end - pos);
            };
            auto extract_dbl = [&](const std::string& blob, const char* key) -> double {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0.0;
                pos += k.size();
                try { return std::stod(blob.substr(pos, 30)); }
                catch (...) { return 0.0; }
            };
            auto extract_int64 = [&](const std::string& blob, const char* key) -> int64_t {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0;
                pos += k.size();
                try { return std::stoll(blob.substr(pos, 20)); }
                catch (...) { return 0; }
            };
            auto extract_int = [&](const std::string& blob, const char* key) -> int {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return 0;
                pos += k.size();
                try { return std::stoi(blob.substr(pos, 10)); }
                catch (...) { return 0; }
            };
            auto extract_bool = [&](const std::string& blob, const char* key) -> bool {
                std::string k = std::string("\"") + key + "\":";
                auto pos = blob.find(k);
                if (pos == std::string::npos) return false;
                pos += k.size();
                return (blob.substr(pos, 4) == "true");
            };

            // Find each position object in the "positions" array
            std::string positions_key = "\"positions\":[";
            auto arr_pos = content.find(positions_key);
            if (arr_pos != std::string::npos) {
                arr_pos += positions_key.size();
                // Split on },{
                size_t search_from = arr_pos;
                while (search_from < content.size()) {
                    auto obj_start = content.find('{', search_from);
                    if (obj_start == std::string::npos) break;
                    auto obj_end = content.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    std::string obj = content.substr(obj_start, obj_end - obj_start + 1);
                    search_from = obj_end + 1;

                    std::string tag = extract_str(obj, "tag");
                    if (tag.empty()) continue;

                    double entry_px = extract_dbl(obj, "entry_px");
                    if (entry_px <= 0.0) continue;

                    // Build resume state
                    chimera::EdgeEngine::ResumeState rs;
                    rs.entry_px        = entry_px;
                    rs.sl_px           = extract_dbl(obj, "sl_px");
                    rs.atr_at_entry    = extract_dbl(obj, "atr_at_entry");
                    rs.entry_ts_ms     = extract_int64(obj, "entry_ts");
                    rs.time_exit_ts_ms = extract_int64(obj, "time_exit_ts");
                    rs.bars_held       = extract_int(obj, "bars_held");
                    rs.trail_armed     = extract_bool(obj, "trail_armed");
                    rs.trail_stop_px   = extract_dbl(obj, "trail_stop_px");
                    rs.trail_arm_px    = extract_dbl(obj, "trail_arm_px");
                    rs.mfe_px          = extract_dbl(obj, "mfe_px");
                    rs.mfe_bp          = extract_dbl(obj, "mfe_bp");

                    // Find matching engine by tag
                    for (auto& s : g_slots) {
                        if (s.engine && s.tag == tag) {
                            if (s.engine->resume_position(rs)) {
                                resumed++;
                            }
                            break;
                        }
                    }
                }
            }

            if (resumed > 0) {
                std::printf("[STARTUP] ✓ Resumed %d open position(s) from snapshot\n", resumed);
            } else {
                std::printf("[STARTUP] No positions to resume (all flat)\n");
            }
            std::fflush(stdout);
        } else {
            std::printf("[STARTUP] No position snapshot found — starting fresh\n");
            std::fflush(stdout);
        }
    }

    // ── Companion peak rehydrate (S-2026-07-05) ──────────────────────────
    // Seed each UpJump companion OPEN + armed from its parent's just-restored live
    // position, so it reflects the parent's true peak-to-date immediately instead of
    // sitting dark until the next H1 close (companion peak/mfe is per-session ephemeral
    // and otherwise re-anchors to the current bar's fav). Runs AFTER resume_position()
    // so parents carry entry_px/mfe_px/entry_ts. Observe-only, shadow — never touches
    // the parent. entry_ref_ aligns with the parent entry so the next observe() no-ops.
    {
        std::lock_guard<std::mutex> lk(g_companion_mtx);
        // S-2026-07-08 RESTART-PATH FIX: det_w books restore their OWN detector window
        // (data/companion_det_state.json) — never the parent's (uniform 4h/+2% = a
        // different window family; the old parent-seed injected phantom windows on
        // every restart and ate genuine in-flight detector windows). Parent-seed is
        // kept ONLY for det_w==0 books (none live) and seed_open() itself refuses
        // det_w books as defense-in-depth.
        restore_companion_det_state();
        int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int seeded = 0;
        for (auto& kv : g_companion_by_parent) {
            chimera::EdgeEngine*             par  = kv.second.first;
            chimera::UpJumpLadderCompanion*  comp = kv.second.second;
            if (!par || !comp || comp->config().det_w > 0 || !par->in_position()) continue;
            comp->seed_open(par->entry_px(), par->entry_ts_ms(), par->mfe_px(), now_ms);
            auto s = comp->snapshot();
            std::printf("[CLIP-SEED] %s open from live parent: entry=%.6f peak_mfe=%.2f%% armed=%d\n",
                comp->config().tag.c_str(), par->entry_px(), s.peak_mfe_pct, s.armed ? 1 : 0);
            seeded++;
        }
        emit_companion_state();   // refresh the desk panel immediately with the restored state
        if (seeded > 0)
            std::printf("[CLIP-SEED] %d companion(s) rehydrated from live parents\n", seeded);
        std::fflush(stdout);
    }

    // ── Trade journal: load history ──────────────────────────────────────
    load_trade_history();

    // ── S34: Multi-tier protection state — load persisted PER + daily kill ─
    load_protection_state();

    // S34: If protection_state.json is missing or older than trade_log,
    // reconstruct all-time cum + peak by replaying trade_log so PER anchors
    // to the true all-time high even on first deploy.
    {
        std::lock_guard<std::mutex> tlk(g_trades_mtx);
        double cum = 0.0;
        double peak = load_dbl_atomic(g_all_time_peak_bp_bits);
        for (auto& tr : g_trade_log) {
            if (tr.reason == "SHUTDOWN") continue;
            cum += tr.net_bp;
            if (cum > peak) peak = cum;
        }
        // Use replayed values if they are larger than what was persisted
        // (catches the case where protection_state.json was deleted/wiped).
        double persisted_cum = load_dbl_atomic(g_all_time_cum_bp_bits);
        if (std::fabs(cum - persisted_cum) > 1.0) {
            std::printf("[PROTECTION] Rebuilt from trade_log: cum=%+.1fbp peak=%+.1fbp (persisted cum=%+.1fbp)\n",
                cum, peak, persisted_cum);
            std::fflush(stdout);
            store_dbl_atomic(g_all_time_cum_bp_bits, cum);
            store_dbl_atomic(g_all_time_peak_bp_bits, peak);
            save_protection_state();
        }
    }

    // GUI root
    {
        char exe[4096] = {};
        ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe)-1);
        if (len > 0) {
            std::string dir(exe, len);
            auto sl = dir.rfind('/');
            if (sl != std::string::npos) dir = dir.substr(0, sl);
            gui_root = dir + "/../gui";
        } else { gui_root = "../gui"; }
    }

    std::printf("[STARTUP] HTTP server starting on port 8080...\n");
    std::fflush(stdout);
    std::thread http_thread(http_server_thread, 8080);
    http_thread.detach();

    // ── Spot WebSocket feed (subscribes to all 12 symbols) ───────────────
    chimera::BinanceWSFeed feed;
    std::printf("[STARTUP] Subscribing to %d symbol streams on Binance...\n", chimera::MAX_SYMBOLS);
    std::fflush(stdout);
    for (int i = 0; i < chimera::MAX_SYMBOLS; ++i)
        feed.add_symbol(chimera::sym_full(i));

    feed.set_callback([&](const chimera::MarketTick& tick) {
        int id = chimera::sym_id(tick.symbol);
        if (id < 0) return;

        double mid = tick.mid_price > 0.0 ? tick.mid_price : tick.last_price;
        if (mid <= 0.0) return;

        store_dbl_atomic(g_last_spot_px_bits[id], mid);

        // S55: tick the maker-grid sleeve (shadow). Buys gated by macro 200d-MA.
        if (!g_grids.empty()) {
            int64_t gnow = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            bool mb = g_macro_bull.load(std::memory_order_relaxed);
            for (auto* gr : g_grids)
                if (chimera::sym_id(gr->cfg().symbol) == id) gr->on_tick(mid, gnow, mb);
            // S55: update the macro-base NAV once per BTC tick (drives the macro signal)
            if (g_macro_base && id == chimera::sym_id("btcusdt")) {
                double bma = load_dbl_atomic(g_btc_200dma_bits);
                std::vector<double> bspot;
                for (const auto& s : g_macro_base->cfg().symbols) {
                    int sid2 = chimera::sym_id(s);
                    bspot.push_back(sid2 >= 0 ? load_dbl_atomic(g_last_spot_px_bits[sid2]) : 0.0);
                }
                g_macro_base->update(mid, bma, bspot, gnow);
            }
        }

        // AUDIT-2026: feed overlay so it can roll daily-close deque per symbol.
        g_portfolio_overlay.on_tick(id, mid,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        // S-2026-06-18: feed BOTH validated XSec sleeves (roll daily close,
        // rebalance on the 14d clock -> [XSEC-BTC]/[XSEC-BR] log + shadow mirror).
        { auto xnow = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
          const char* xss = chimera::sym_short(id);
          xsec_btc.on_tick(xss, mid, xnow);
          xsec_br.on_tick(xss, mid, xnow);
          riprider.on_tick(xss, mid, xnow); }

        // S34: log BTC ticks into rolling chart buffer (~last 1000 ticks)
        if (id == chimera::SYM_BTC) {
            int idx = g_btc_chart_idx.load(std::memory_order_relaxed);
            store_dbl_atomic(g_btc_chart_px[idx], mid);
            int next_idx = (idx + 1) % BTC_CHART_N;
            g_btc_chart_idx.store(next_idx, std::memory_order_relaxed);
            int filled = g_btc_chart_filled.load(std::memory_order_relaxed);
            if (filled < BTC_CHART_N) g_btc_chart_filled.store(filled + 1, std::memory_order_relaxed);
        }

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Stale gate (5s — relaxed vs HFT)
        double tick_age_ms = 0.0;
        if (tick.trade_time > 0) {
            tick_age_ms = static_cast<double>(now_ms - tick.trade_time);
            if (tick_age_ms < 0.0) tick_age_ms = 0.0;
        }
        if (tick_age_ms > 5000.0) return;

        // Route to whichever engine slot matches this symbol id.
        // S44c: also feed S43/S43b engines from g_all_wired (they aren't
        // in g_slots so the slot loop misses them).
        // S44h: check cooldown expiry + emergency halt state.
        {
            int64_t now_ms_ck = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int64_t halt_until = g_emergency_halt_until_ms.load(std::memory_order_relaxed);
            bool in_emergency = (now_ms_ck < halt_until);
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            // S45: snapshot live open-position exposure per symbol + cluster
            // BEFORE feeding this tick. Recomputed every tick so it tracks
            // cross-symbol cluster state; incrementally updated below as
            // engines enter/exit during this tick (exact within-tick caps).
            std::memset(g_cluster_open_sym, 0, sizeof(g_cluster_open_sym));
            std::memset(g_cluster_open_bucket, 0, sizeof(g_cluster_open_bucket));
            for (auto* e : g_all_wired) {
                if (!e || !e->in_position()) continue;
                int sid = chimera::symbol_to_id(e->cfg().symbol);
                if (sid < 0 || sid >= chimera::MAX_SYMBOLS) continue;
                g_cluster_open_sym[sid]++;
                g_cluster_open_bucket[symbol_cluster(sid)]++;
            }
            // S44h: re-enable gates that have completed cooldown
            // S44L C: also apply per-symbol daily lock
            for (auto* e : g_all_wired) {
                if (!in_emergency) engine_check_cooldown_expiry(e, now_ms_ck);
                if (!e) continue;
                int sid = chimera::symbol_to_id(e->cfg().symbol);
                if (sid >= 0 && sid < chimera::MAX_SYMBOLS) {
                    int64_t blocked_until = g_sym_daily_blocked_until_ms[sid].load();
                    // S44M #1: also check SL-count circuit
                    int64_t sl_blocked = g_sym_sl_circuit_blocked_until_ms[sid].load();
                    // S44O: per-symbol post-SL cooldown (30 min after any SL)
                    int64_t post_sl_blocked = g_sym_post_sl_cooldown_until_ms[sid].load();
                    if (now_ms_ck < blocked_until || now_ms_ck < sl_blocked ||
                        now_ms_ck < post_sl_blocked) {
                        e->set_portfolio_gate(false);
                    }
                }
            }
            // S45: set the cluster gate for one engine from live counts, feed
            // the tick, then reconcile counts if it just entered/exited. A flat
            // engine is allowed only if BOTH its symbol and its cluster are
            // under cap; an in-position engine is never cluster-blocked (it is
            // not entering). Keeps the cap exact even when several engines on
            // the same symbol close a bar on the same tick.
            auto tick_with_cluster_gate = [&](chimera::EdgeEngine* e) {
                int sid = chimera::symbol_to_id(e->cfg().symbol);
                int cl  = symbol_cluster(sid);
                bool was_in = e->in_position();
                if (!was_in) {
                    // Unified hard guard (not overridden by blowoff/portfolio gate):
                    //   1. per-symbol concurrency cap     (simultaneous exposure)
                    //   2. per-cluster concurrency cap     (simultaneous correlated)
                    //   3. per-cluster 24h loss halt       (sequential correlated bleed)
                    //   4. bear-regime halt                (spot-long-only: no longs in a downtrend)
                    bool concurrency_ok = (g_cluster_open_sym[sid]  < CLUSTER_MAX_PER_SYMBOL) &&
                                          (g_cluster_open_bucket[cl] < CLUSTER_MAX_PER_CLUSTER);
                    bool cluster_loss_ok = (now_ms_ck >= g_cluster_blocked_until_ms[cl].load());
                    // P2/S46: require BOTH global BTC regime AND this symbol's own
                    // regime to be bullish. Closes the "BTC coattails" leak where a
                    // personally-collapsing alt was waved through on BTC's regime
                    // alone. Long-only: don't long a falling name.
                    // S54 CHOP-HALT: trend/breakout kinds require BULL_TREND(3) —
                    // they churn losses in chop (31-May -645bp from 72 micro-pop
                    // entries that reversed). Mean-revert/session kinds keep the
                    // BULL_CHOP(2) gate — they EARN in chop (this is also what
                    // turns the trend-only book into a regime-diversified one).
                    int min_reg = chimera::is_trend_kind(e->cfg().kind)
                                      ? 3                       // BULL_TREND only
                                      : REGIME_MIN_FOR_ENTRY;   // BULL_CHOP ok
                    bool btc_regime_ok = (g_regime.load(std::memory_order_relaxed) >= min_reg);
                    bool sym_regime_ok = (sid < 0) ||
                                         (g_sym_regime[sid].load(std::memory_order_relaxed) >= min_reg);
                    // S54 MACRO GATE: no long entries while BTC < 200d-MA (no
                    // long edge below it — validated). Applies to ALL kinds.
                    bool macro_ok        = g_macro_bull.load(std::memory_order_relaxed);
                    // S-2026-07-05 OPERATOR RULE: UPJUMP is a spike/uptrend catch —
                    // the W-bar up-jump trigger IS the signal (the coin's OWN uptrend).
                    // Spot-long-only book: when a coin runs up we ride it as hard as
                    // we can. The broad market-direction vetoes (200d-MA macro +
                    // BULL_TREND regime) are IRRELEVANT to a per-coin up-jump and were
                    // silently suppressing EVERY up-jump in a BTC-below-200DMA bear
                    // (only ETH — opened before the halt — rode, so only 1 companion
                    // ever armed). Operator directive, repeated + explicit: cut the
                    // 200DMA/regime direction gate for these trades. Genuine RISK caps
                    // stay in force: per-symbol + per-cluster concurrency + cluster
                    // 24h loss circuit-breaker (see set above).
                    bool direction_ok    = (e->cfg().kind == chimera::StrategyKind::UPJUMP)
                                               ? true
                                               : (btc_regime_ok && sym_regime_ok && macro_ok);
                    e->set_cluster_gate(concurrency_ok && cluster_loss_ok && direction_ok);
                } else {
                    e->set_cluster_gate(true);
                }
                e->on_tick(mid, now_ms);
                bool now_in = e->in_position();
                if (!was_in && now_in)      { g_cluster_open_sym[sid]++; g_cluster_open_bucket[cl]++; }
                else if (was_in && !now_in) { if (g_cluster_open_sym[sid] > 0) g_cluster_open_sym[sid]--;
                                              if (g_cluster_open_bucket[cl] > 0) g_cluster_open_bucket[cl]--; }
            };
            // First — non-slot wired engines (no blowoff guard, no slot
            // metadata, just raw on_tick).
            for (auto* e : g_all_wired) {
                if (!e) continue;
                if (chimera::symbol_to_id(e->cfg().symbol) != id) continue;
                // Skip if already in g_slots (g_slots loop handles below)
                bool in_slots = false;
                for (auto& s : g_slots) if (s.engine == e) { in_slots = true; break; }
                if (in_slots) continue;
                tick_with_cluster_gate(e);
            }
            for (auto& s : g_slots) {
                if (s.symbol_id == id && s.engine) {
                    // ── AUDIT-2026 BLOWOFF GUARD ──────────────────────────
                    // Live-read momentum_pct from state_json each tick.
                    // Cache was unreliable (stale values from mid-warmup
                    // got pinned). state_json is engine-local + cheap;
                    // ~333 calls/sec across all ticks → negligible CPU.
                    // EdgeEngine portfolio_gate(false) short-circuits
                    // entry in close_bar_ flow before enter_position_.
                    std::string state = s.engine->state_json();
                    // Key "momentum_pct": is 15 chars including the colon.
                    // pos+15 is the first digit of the value.
                    static constexpr size_t MOM_KEY_LEN = 15;  // strlen("\"momentum_pct\":")
                    auto pos = state.find("\"momentum_pct\":");
                    double mom = 0.0;
                    bool have_mom = false;
                    if (pos != std::string::npos) {
                        try {
                            mom = std::stod(state.substr(pos + MOM_KEY_LEN, 14));
                            have_mom = true;
                        } catch (...) {}
                    }
                    if (have_mom) {
                        double thr = blowoff_threshold_for_tf((int64_t)s.tf_secs);
                        bool allow = (mom <= thr);
                        bool was_allowed = s.engine->portfolio_entry_allowed();
                        if (!allow && was_allowed) {
                            std::printf("[BLOWOFF_GUARD] %s gate=closed momentum=%.1f%% > %.1f%%\n",
                                s.tag.c_str(), mom, thr);
                            std::fflush(stdout);
                        } else if (allow && !was_allowed) {
                            std::printf("[BLOWOFF_GUARD] %s gate=open momentum=%.1f%% <= %.1f%%\n",
                                s.tag.c_str(), mom, thr);
                            std::fflush(stdout);
                        }
                        s.engine->set_portfolio_gate(allow);
                    }
                    // S45: cluster-cap gate (independent of the blowoff /
                    // portfolio gate set just above) + tick.
                    tick_with_cluster_gate(s.engine);
                }
            }
        }

        // ── UPJUMP companion: drive on EVERY tick (S-2026-07-05) ─────────────
        // Was H1-bar-close ONLY (on_bar_callback) -> blind to any mover that
        // spiked+reversed WITHIN the hour: peak-MFE was sampled at the close and
        // the reversal-giveback clip could only fire on the hour boundary. Now
        // observe() runs per tick for this symbol's companions, so peak-MFE
        // tracks the true intra-hour high and the REVERSAL/RECLIP price gates
        // fire the instant they trip. The stall counter is unaffected: bar index
        // = ts/H1 still only advances on the hour, so STALL stays in H1 units
        // (backtested roster semantics preserved). This RESTORES the intra-bar
        // cadence the validated stall_accountant.py had — the native H1-only path
        // was a fidelity regression, not the design. on_bar_callback still drives
        // the canonical bar-close observe + emit; double-drive is safe (post-clip
        // re-arm gate blocks a double clip, peak only ratchets up).
        {
            std::lock_guard<std::mutex> lk(g_companion_mtx);
            if (!g_companion_by_parent.empty()) {
                int clips_before = 0, clips_after = 0;
                for (auto& kv : g_companion_by_parent) {
                    chimera::EdgeEngine* par            = kv.second.first;
                    chimera::UpJumpLadderCompanion* comp = kv.second.second;
                    if (!par || !comp) continue;
                    if (chimera::symbol_to_id(par->cfg().symbol) != id) continue;
                    clips_before += comp->clips();
                    comp->observe(par->in_position(), par->entry_px(), mid, now_ms);
                    clips_after  += comp->clips();
                }
                // Refresh the desk snapshot when a clip fired this tick, or on a
                // light 5s time-throttle so armed/peak/bank track intra-bar (the
                // desk panel polls ~15s). Avoids per-tick file I/O.
                static int64_t last_cc_emit_ms = 0;
                if (clips_after != clips_before || now_ms - last_cc_emit_ms >= 5000) {
                    last_cc_emit_ms = now_ms;
                    emit_companion_state();
                }
            }
        }

        static std::atomic<int> tc{0};
        int n = tc.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n % 10000 == 0) {
            std::printf("[TICK] n=%d | %s px=%.4f | age=%.1fms\n",
                n, tick.symbol.c_str(), mid, tick_age_ms);
            std::fflush(stdout);
        }
    });

    feed.start();

    // ── AUDIT-2026 BLOWOFF GUARD prefill ──────────────────────────────────
    // Seed-loaded bars do NOT fire on_bar callback, so g_last_momentum_pct
    // is empty at startup. Parse momentum_pct from each engine's state_json
    // once here so the guard is active from the first tick. Subsequent live
    // bar closes refresh the map via on_bar_callback.
    {
        std::lock_guard<std::mutex> lk(g_engine_mtx);
        int seeded = 0, blocked = 0;
        for (auto& s : g_slots) {
            if (!s.engine) continue;
            std::string state = s.engine->state_json();
            auto pos = state.find("\"momentum_pct\":");
            if (pos == std::string::npos) continue;
            double mom = 0.0;
            try { mom = std::stod(state.substr(pos + 15, 14)); } catch (...) { continue; }
            {
                std::lock_guard<std::mutex> lk2(g_momentum_mtx);
                g_last_momentum_pct[s.tag] = mom;
            }
            ++seeded;
            double thr = blowoff_threshold_for_tf((int64_t)s.tf_secs);
            if (mom > thr) {
                s.engine->set_portfolio_gate(false);
                ++blocked;
                std::printf("[BLOWOFF_GUARD] startup-prefill %s gate=closed momentum=%.1f%% > %.1f%%\n",
                    s.tag.c_str(), mom, thr);
            }
        }
        std::printf("[BLOWOFF_GUARD] prefilled %d engines, %d blocked at startup (per-TF thresholds 80-200%%)\n",
            seeded, blocked);
        std::fflush(stdout);
    }

    // ── Session 30, Edge 7: Start liquidation cascade feed ───────────────
    // AUDIT-2026: DISABLED — Binance Futures (fstream.binance.com) blocks
    // Singapore retail IPs (this VPS is in Singapore, AS14061 DigitalOcean
    // SGP). Binance.com restricts SG residents from futures since 2021.
    // All endpoints (!forceOrder@arr, combined-stream, per-symbol) accept
    // WS handshake then immediate close from server. To reanimate: route
    // via proxy in non-blocked region, swap to CoinGlass/Bybit liq feed,
    // or migrate VPS region. Callback still wired so re-enable is one-line.
    g_liq_feed.set_callback([](const chimera::LiquidationWSFeed::LiqEvent& ev) {
        g_liq_detector.on_liquidation(ev.symbol_id, ev.price, ev.qty, ev.is_long, ev.ts_ms);
    });
    std::printf("[LIQ-FEED] DISABLED (Binance Futures blocks Singapore retail). LiquidationCascadeDetector inactive.\n");
    std::fflush(stdout);
    // g_liq_feed.start();  // disabled — see comment above

    std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
    std::printf("[STARTUP] ✓ CHIMERA READY — %d engines running (shadow_mode=true)\n", (int)g_slots.size());
    std::printf("[STARTUP]   TSMOM=94+4W1 | RSI_REVERT=49+2W1 | BOLLINGER=44+2W1 | DONCHIAN=28 | ICHIMOKU=6 | SUPERTREND=6\n");
    std::printf("[STARTUP]   KELTNER=6 | DUAL_THRUST=2 | WILLIAMS_R=4 | STOCH_RSI=4\n");
    std::printf("[STARTUP]   Filters: vol+mtf+adx+vol_gate+corr+session+portfolio+funding+vol_regime+cross_tf+liq_cascade\n");
    std::printf("[STARTUP]   4 engines DISABLED (failed re-validation S29c)\n");
    std::printf("[STARTUP]   17 symbols | spot-long-only | all edges validated\n");
    std::printf("[STARTUP]   NEW S30: funding filter, vol regime, position sizing, cross-TF, W1 MR, liq cascade\n");
    std::printf("[STARTUP]   GUI: http://localhost:8080\n");
    std::printf("[STARTUP]   API: /api/state2  /api/positions  /api/trades\n");
    std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
    std::fflush(stdout);

    // ── Periodic open-position snapshot (every 60s) ────────────────────────
    // Writes data/open_positions.json so that if the process is SIGKILL'd or
    // OOM-killed, we know exactly what was open. On graceful shutdown this file
    // becomes stale (positions are closed), so the shutdown path clears it.
    constexpr int SNAPSHOT_INTERVAL_MS = 60000;
    int64_t last_snapshot_ms = 0;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now_ms - last_snapshot_ms >= SNAPSHOT_INTERVAL_MS) {
            last_snapshot_ms = now_ms;
            // Build snapshot of all open positions
            std::ostringstream snap;
            snap << "{\"snapshot_ts\":" << now_ms << ",\"positions\":[";
            int count = 0;
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine || !s.engine->in_position()) continue;
                    double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                    std::string pj = s.engine->position_snapshot_json(spot);
                    if (!pj.empty()) {
                        if (count > 0) snap << ",";
                        snap << pj;
                        count++;
                    }
                }
            }
            snap << "],\"open_count\":" << count << "}\n";
            FILE* f = fopen("data/open_positions.json", "w");
            if (f) {
                std::string out = snap.str();
                fwrite(out.c_str(), 1, out.size(), f);
                fclose(f);
            }

            // ── MTF gate: propagate D1 TSMOM trend to lower-TF engines ─────
            // For each symbol, find the D1 TSMOM engine and read its trend.
            // Then set d1_bullish on all non-D1 engines for that symbol.
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                // First pass: extract D1 trend per symbol
                bool d1_trend[chimera::MAX_SYMBOLS];
                for (int i = 0; i < chimera::MAX_SYMBOLS; ++i) {
                    d1_trend[i] = true;   // default bullish if no D1 engine
                }
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    // D1 TSMOM engines are our trend reference
                    if (s.tf_secs == 86400 && s.engine->is_trend_following()) {
                        d1_trend[s.symbol_id] = s.engine->trend_bullish();
                    }
                }
                // Second pass: propagate to all engines with mtf_gate enabled
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_d1_bullish(d1_trend[s.symbol_id]);
                }
            }

            // ── Session 30, Edge 1: Periodic funding rate refresh ────────────
            // Fetch every 8h (matches Binance funding interval).
            // Propagate tailwind/headwind flags to all engines.
            if (now_ms - g_last_funding_fetch_ms >= FUNDING_FETCH_INTERVAL_MS) {
                std::thread([](){
                    g_funding_filter.fetch_all();
                }).detach();
                g_last_funding_fetch_ms = now_ms;
            }
            // Always propagate current state (fetch may complete between loops)
            if (g_funding_filter.is_ready()) {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_funding_tailwind(g_funding_filter.has_tailwind(s.symbol_id));
                    s.engine->set_funding_headwind(g_funding_filter.has_headwind(s.symbol_id));
                }
            }

            // ── Session 30, Edge 3: Volatility regime classification ─────────
            // Use BTC's ATR(14)/ATR(50) from a D1 engine as the global vol regime.
            // LOW: ratio < 0.5 (deeply compressed — only extreme suppression)
            // HIGH: ratio > 1.8 (truly chaotic — only extreme suppression)
            // MEDIUM: 0.5-1.8 (both strategies active — widened for shadow mode)
            // Shadow tuning 2026-05-17: was 0.8/1.4 which muted half the fleet
            // in normal market conditions. Widened so engines generate trades.
            {
                chimera::EdgeEngine::VolRegime regime = chimera::EdgeEngine::VolRegime::MEDIUM;
                // Find BTC D1 TSMOM engine to read its vol_ratio
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC && s.tf_secs == 86400 &&
                        s.engine->is_trend_following()) {
                        double vr = s.engine->vol_ratio_public();
                        if (vr > 0.0) {  // 0 = not enough data yet
                            if (vr < 0.5) regime = chimera::EdgeEngine::VolRegime::LOW;
                            else if (vr > 1.8) regime = chimera::EdgeEngine::VolRegime::HIGH;
                        }
                        break;
                    }
                }
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_vol_regime(regime);
                }
            }

            // ── Session 30, Edge 5: Cross-TF momentum score ──────────────────
            // For each symbol, check if D1 + H6 + H4 are all bullish.
            // Score: 0.0 (no agreement), 0.33 (one TF), 0.67 (two TFs), 1.0 (all three)
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                double cross_tf_scores[chimera::MAX_SYMBOLS]{};
                for (int sym = 0; sym < chimera::MAX_SYMBOLS; ++sym) {
                    int bullish_count = 0;
                    int tf_count = 0;
                    for (auto& s : g_slots) {
                        if (!s.engine) continue;
                        if (s.symbol_id != sym) continue;
                        if (!s.engine->is_trend_following()) continue;
                        // Only count D1, H6, H4 as cross-TF references
                        if (s.tf_secs == 86400 || s.tf_secs == 21600 || s.tf_secs == 14400) {
                            tf_count++;
                            if (s.engine->trend_bullish()) bullish_count++;
                        }
                    }
                    cross_tf_scores[sym] = (tf_count > 0) ?
                        (double)bullish_count / (double)tf_count : 0.0;
                }
                // Propagate scores to all engines
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    s.engine->set_cross_tf_score(cross_tf_scores[s.symbol_id]);
                }

                // Edge 4: Dynamic sizing adjustment based on regime + conviction
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    double base_mult = s.engine->sizing_mult();
                    // Cap base at the statically-set value (don't compound)
                    if (base_mult > 2.0) base_mult = 2.0;
                    double dynamic_mult = base_mult;
                    // High conviction (cross-TF + funding) → boost 25%
                    if (s.engine->is_high_conviction()) {
                        dynamic_mult = base_mult * 1.25;
                    }
                    // High vol regime → reduce 25% (protect capital)
                    if (s.engine->vol_regime() == chimera::EdgeEngine::VolRegime::HIGH) {
                        dynamic_mult *= 0.75;
                    }
                    s.engine->set_sizing_mult(dynamic_mult);
                }
            }

            // ── Session 30, Edge 7: Liquidation cascade entry signals ───────���
            // When a cascade exhausts, boost the sizing mult for affected symbol
            // for the next 5 minutes (the dip-buy window). Also log prominently.
            {
                g_liq_detector.cleanup(now_ms);
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (int sym = 0; sym < chimera::MAX_SYMBOLS; ++sym) {
                    if (g_liq_detector.has_entry_signal(sym)) {
                        // Boost ALL engines for this symbol — cascade dip = high conviction
                        for (auto& s : g_slots) {
                            if (!s.engine || s.symbol_id != sym) continue;
                            double cur = s.engine->sizing_mult();
                            s.engine->set_sizing_mult(cur * 1.5);  // 50% boost for cascade dip
                        }
                        std::printf("[LIQ-ENTRY] %s: cascade entry boost active (sizing +50%%)\n",
                            chimera::SYM_SHORT[sym]);
                        std::fflush(stdout);
                        g_liq_detector.clear_entry_signal(sym);
                    }
                }
            }

            // ── S34: MULTI-TIER PORTFOLIO PROTECTION ───────────────────────
            // Tier 0: persistent PER (all-time anchored) + daily loss kill
            // Tier 1: BTC regime gate (TREND/CHOP/CRASH)
            // Tier 2: progressive size throttle (DD-based)
            // Tier 3: conviction filter (deep DD = only high-conviction entries)
            // Tier 4: per-strategy concurrent cap (TSMOM mono-culture defence)
            // PLUS preserved: agg_kill, streak halt, per-symbol cap, 4h DD
            {
                // S54 derisk: 25->14. Book is ~1.8 effective bets (avg symbol
                // corr 0.54, BTC-beta 0.73) -> 25 concurrent longs = one oversized
                // correlated position. 14 bounds the synchronized-bleed tail; the
                // missed positions are correlated with the open ones so upside loss
                // is marginal (asymmetric-favorable).
                constexpr int MAX_CONCURRENT_POSITIONS = 14;
                constexpr int64_t DRAWDOWN_LOOKBACK_MS = 4LL * 3600LL * 1000LL;
                constexpr double DRAWDOWN_HALT_BP = -1500.0;
                constexpr double AGG_KILL_BP = -2000.0;
                constexpr int    STREAK_LOOKBACK = 20;
                constexpr double STREAK_HALT_BP  = -5000.0;
                constexpr int64_t STREAK_HALT_MS = 1LL * 3600LL * 1000LL;
                static int64_t streak_halt_until_ms = 0;

                // ── S34 TIER 0: tightened PER + daily kill thresholds ────
                constexpr double RATCHET_ARM_BP       = 150.0;   // was 500
                constexpr double RATCHET_GIVEBACK_BP  = 100.0;   // was 300
                constexpr double RATCHET_GIVEBACK_PCT = 0.15;    // was 0.25
                constexpr double RATCHET_REARM_BP     = 50.0;    // was 200
                constexpr double DAILY_LOSS_KILL_BP   = -400.0;
                // Halt = 2h cooldown after flush (was 24h). 24h locked user out
                // even after conditions recovered. Edge-trigger ensures we
                // don't re-fire on lingering window losses; 2h gives enough
                // breathing room then resumes.
                constexpr int64_t DAILY_HALT_MS       = 2LL * 3600LL * 1000LL;
                constexpr int64_t DAILY_WINDOW_MS     = 24LL * 3600LL * 1000LL;

                std::lock_guard<std::mutex> lk(g_engine_mtx);
                int open_positions = 0;
                int tsmom_open = 0;
                int per_sym_open[chimera::MAX_SYMBOLS] = {0};
                // S35-cluster: ALT-L1 basket (high BTC correlation, 0.85+).
                // One TSMOM signal on the basket = signal on all 9 names.
                // Track concurrent open count and cap to prevent cluster wipeout.
                int alt_l1_open = 0;
                auto is_alt_l1 = [](int sid){
                    return sid == chimera::SYM_SUI  || sid == chimera::SYM_APT  ||
                           sid == chimera::SYM_NEAR || sid == chimera::SYM_ARB  ||
                           sid == chimera::SYM_FET  || sid == chimera::SYM_ONDO ||
                           sid == chimera::SYM_TIA  || sid == chimera::SYM_INJ  ||
                           sid == chimera::SYM_SEI;
                };
                // S35-cluster: per (symbol, family) open count. Blocks TF
                // stacking (NEAR-TSMOM-H4 + H8 + H12 = 3x correlated bet).
                auto family_of = [](const std::string& tag) -> std::string {
                    auto p1 = tag.find('-');
                    if (p1 == std::string::npos) return "";
                    auto p2 = tag.find('-', p1 + 1);
                    if (p2 == std::string::npos) return tag.substr(p1 + 1);
                    return tag.substr(p1 + 1, p2 - p1 - 1);
                };
                // Map key = (symbol_id * 256) + family_hash_lo. Small, fast.
                std::unordered_map<std::string, int> per_sym_fam_open;
                for (auto& s : g_slots) {
                    if (!s.engine || !s.engine->in_position()) continue;
                    open_positions++;
                    if (s.symbol_id >= 0 && s.symbol_id < chimera::MAX_SYMBOLS) {
                        per_sym_open[s.symbol_id]++;
                        if (is_alt_l1(s.symbol_id)) alt_l1_open++;
                    }
                    // Strategy family from tag like "BTC-TSMOM-D1"
                    if (s.tag.find("-TSMOM-") != std::string::npos) tsmom_open++;
                    std::string fam = family_of(s.tag);
                    if (!fam.empty() && s.symbol_id >= 0) {
                        per_sym_fam_open[std::to_string(s.symbol_id) + "|" + fam]++;
                    }
                }

                // S55: accrue idle-capital yield on the FLAT fraction (1 - open/MAX).
                // Models parking unused USDT in exchange Earn. Separate from trading PnL.
                {
                    int64_t ynow = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    int64_t ylast = g_yield_last_ms.load(std::memory_order_relaxed);
                    if (ylast > 0 && ynow > ylast) {
                        double dt_yr = (ynow - ylast) / 1000.0 / 31557600.0;
                        double flat_frac = 1.0 - (double)open_positions / (double)MAX_CONCURRENT_POSITIONS;
                        if (flat_frac < 0) flat_frac = 0;
                        double add_bp = IDLE_YIELD_APY * dt_yr * flat_frac * 1e4;
                        store_dbl_atomic(g_yield_cum_bp_bits, load_dbl_atomic(g_yield_cum_bp_bits) + add_bp);
                    }
                    g_yield_last_ms.store(ynow, std::memory_order_relaxed);
                }

                // 4h rolling DD + 24h daily P&L
                // S44h: also track last-30min SL count + 30min portfolio DD
                // for emergency-halt detector.
                double recent_pnl = 0.0;
                double daily_pnl  = 0.0;
                double pnl_30min  = 0.0;
                int    sl_30min   = 0;
                // S44L C: per-symbol 24h sum
                double sym_24h_bp[chimera::MAX_SYMBOLS] = {0};
                // S45: per-cluster 24h sum (sequential correlated-bleed breaker)
                double clu_24h_bp[CL_COUNT] = {0};
                // S44M #1: per-symbol SL count in last 4h
                int sym_sl_4h[chimera::MAX_SYMBOLS] = {0};
                {
                    std::lock_guard<std::mutex> tlk(g_trades_mtx);
                    int64_t cutoff4h  = now_ms - DRAWDOWN_LOOKBACK_MS;
                    // S54: never count trades before the session-reset epoch.
                    int64_t cutoff24h = std::max(now_ms - DAILY_WINDOW_MS,
                                                 g_pnl_epoch_ms.load(std::memory_order_relaxed));
                    int64_t cutoff30m = now_ms - 30LL * 60 * 1000;
                    for (int i = (int)g_trade_log.size() - 1; i >= 0; --i) {
                        const auto& tr = g_trade_log[i];
                        if (tr.exit_ts_ms < cutoff24h) break;
                        if (tr.reason == "SHUTDOWN") continue;
                        daily_pnl += tr.net_bp;
                        if (tr.exit_ts_ms >= cutoff4h) recent_pnl += tr.net_bp;
                        if (tr.exit_ts_ms >= cutoff30m) {
                            pnl_30min += tr.net_bp;
                            if (tr.reason == "SL" || tr.reason == "EARLY_KILL" ||
                                tr.reason == "KILL") ++sl_30min;
                        }
                        int sid = chimera::symbol_to_id(tr.symbol);
                        if (sid >= 0 && sid < chimera::MAX_SYMBOLS) {
                            sym_24h_bp[sid] += tr.net_bp;
                            clu_24h_bp[symbol_cluster(sid)] += tr.net_bp;  // S45
                            // S44M #1: count SLs in last 4h per symbol
                            if (tr.exit_ts_ms >= (now_ms - SYM_SL_WINDOW_MS) &&
                                (tr.reason == "SL" || tr.reason == "EARLY_KILL")) {
                                sym_sl_4h[sid]++;
                            }
                        }
                    }
                }
                // S44M #1: lock symbols hitting 3+ SLs in 4h
                for (int sid = 0; sid < chimera::MAX_SYMBOLS; ++sid) {
                    if (sym_sl_4h[sid] >= SYM_SL_COUNT_THRESHOLD) {
                        int64_t prev = g_sym_sl_circuit_blocked_until_ms[sid].load();
                        if (prev < now_ms) {
                            char buf[128];
                            std::snprintf(buf, sizeof(buf),
                                "SYM_SL_CIRCUIT sid=%d sl_4h=%d -> 4h block",
                                sid, sym_sl_4h[sid]);
                            std::printf("[%s]\n", buf);
                            std::fflush(stdout);
                            push_alert(buf);
                        }
                        g_sym_sl_circuit_blocked_until_ms[sid].store(now_ms + SYM_SL_BLOCK_MS);
                    }
                }
                // S44L C: lock symbols whose 24h cumulative <= -300bp
                for (int sid = 0; sid < chimera::MAX_SYMBOLS; ++sid) {
                    if (sym_24h_bp[sid] <= SYM_DAILY_CAP_BP) {
                        int64_t prev = g_sym_daily_blocked_until_ms[sid].load();
                        if (prev < now_ms) {
                            char buf[128];
                            std::snprintf(buf, sizeof(buf),
                                "SYM_DAILY_CAP sid=%d 24h=%.1fbp -> 24h block",
                                sid, sym_24h_bp[sid]);
                            std::printf("[%s]\n", buf);
                            std::fflush(stdout);
                            push_alert(buf);
                        }
                        g_sym_daily_blocked_until_ms[sid].store(now_ms + DAILY_WINDOW_MS);
                    }
                }
                // S45: halt a whole CLUSTER for 24h when its rolling-24h net
                // bleeds past the cap. Stops a sequential correlated dump dead
                // (the 29-May run: DEFI+L1 buckets bled together). With spot-
                // long-only we cannot recoup in a falling cluster — sit it out.
                {
                    static constexpr const char* CLN[CL_COUNT] = {"majors","l1","defi","meme","other"};
                    for (int c = 0; c < CL_COUNT; ++c) {
                        if (clu_24h_bp[c] <= CLUSTER_DAILY_CAP_BP) {
                            int64_t prev = g_cluster_blocked_until_ms[c].load();
                            if (prev < now_ms) {
                                char buf[128];
                                std::snprintf(buf, sizeof(buf),
                                    "CLUSTER_DAILY_CAP cluster=%s 24h=%.1fbp -> 24h halt",
                                    CLN[c], clu_24h_bp[c]);
                                std::printf("[%s]\n", buf);
                                std::fflush(stdout);
                                push_alert(buf);
                            }
                            g_cluster_blocked_until_ms[c].store(now_ms + DAILY_WINDOW_MS);
                        }
                    }
                }
                g_recent_sl_count.store(sl_30min, std::memory_order_relaxed);

                // ── S44h EMERGENCY HALT DETECTOR ─────────────────────────
                // Force-close all open positions + halt entries for 4h when:
                //   - SL count last 30min >= 10  OR
                //   - portfolio DD last 30min <= -300bp  OR
                //   - BTC dropped >= -3% in last 15min
                {
                    int64_t halt_until = g_emergency_halt_until_ms.load(std::memory_order_relaxed);
                    bool in_emergency = (now_ms < halt_until);
                    bool trigger_sl   = (sl_30min >= EMERGENCY_SL_THRESHOLD);
                    bool trigger_dd   = (pnl_30min <= -EMERGENCY_DD_BP_THRESHOLD);
                    // BTC drop check via per-symbol rally buffer
                    double btc_short = load_dbl_atomic(g_sym_short_ret[chimera::SYM_BTC]);
                    bool trigger_btc = (btc_short <= EMERGENCY_BTC_PCT);
                    if (!in_emergency && (trigger_sl || trigger_dd || trigger_btc)) {
                        const char* reason = trigger_sl ? "SL_CASCADE" :
                                             trigger_dd ? "PORTFOLIO_DD" : "BTC_CRASH";
                        char buf[256];
                        std::snprintf(buf, sizeof(buf),
                            "EMERGENCY HALT triggered: %s  sl_30min=%d  pnl_30min=%+.1fbp  btc_15min=%+.2f%%",
                            reason, sl_30min, pnl_30min, btc_short);
                        std::fprintf(stderr, "\n[EMERGENCY] %s\n\n", buf);
                        std::printf("[EMERGENCY] %s\n", buf);
                        std::fflush(stdout);
                        push_alert(buf);
                        g_emergency_halt_until_ms.store(now_ms + EMERGENCY_HALT_DURATION_MS,
                                                       std::memory_order_relaxed);
                        // Force-close every open position
                        int closed = 0;
                        for (auto* e : g_all_wired) {
                            if (!e || !e->in_position()) continue;
                            int sym_id = chimera::symbol_to_id(e->cfg().symbol);
                            if (sym_id < 0) continue;
                            double spot = load_dbl_atomic(g_last_spot_px_bits[sym_id]);
                            if (spot > 0.0) {
                                e->kill_all(spot, now_ms);
                                ++closed;
                            }
                        }
                        std::snprintf(buf, sizeof(buf), "Force-closed %d open positions", closed);
                        push_alert(buf);
                        std::printf("[EMERGENCY] %s\n", buf);
                        std::fflush(stdout);
                    } else if (in_emergency) {
                        // While halted, ensure entries blocked
                        for (auto* e : g_all_wired) {
                            if (e) e->set_portfolio_gate(false);
                        }
                    }
                }

                constexpr double UNREALIZED_HALT_BP = -500.0;
                double total_unrealized_bp = 0.0;
                for (auto& s : g_slots) {
                    if (!s.engine || !s.engine->in_position()) continue;
                    double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                    if (spot > 0.0) {
                        total_unrealized_bp += s.engine->unrealised_bp(spot);
                    }
                }

                // AGG_KILL — catastrophe brake on open unrealised loss
                if (total_unrealized_bp < AGG_KILL_BP) {
                    std::printf("[PORTFOLIO] AGG_KILL TRIPPED: total_unrealized=%+.1fbp < %.0fbp -> kill_all\n",
                        total_unrealized_bp, AGG_KILL_BP);
                    std::fflush(stdout);
                    int killed = 0;
                    for (auto& s : g_slots) {
                        if (!s.engine || !s.engine->in_position()) continue;
                        double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                        if (spot > 0.0) { s.engine->kill_all(spot, now_ms); killed++; }
                    }
                    std::printf("[PORTFOLIO] AGG_KILL flattened %d positions\n", killed);
                    std::fflush(stdout);
                }

                // ── S34 TIER 0: DAILY LOSS KILL ──────────────────────────
                // If trailing-24h realised P&L drops below DAILY_LOSS_KILL_BP,
                // flatten ALL open positions and halt entries for 24h.
                // Edge-triggered: only fires when daily_pnl FIRST drops below
                // threshold. Once daily_pnl recovers above threshold, re-arms.
                // This prevents re-firing while old losses linger in 24h window.
                int64_t daily_kill_until = g_daily_kill_until_ms.load(std::memory_order_relaxed);
                static bool daily_below_threshold = false;
                // Seed from persisted state on first iteration: if a kill is
                // still active OR was recent (within 48h), assume we're still
                // in the same trigger window — don't re-fire.
                static bool daily_seed_done = false;
                if (!daily_seed_done) {
                    daily_below_threshold = (daily_kill_until > now_ms) ||
                                            (daily_pnl < DAILY_LOSS_KILL_BP);
                    daily_seed_done = true;
                }
                bool now_below = (daily_pnl < DAILY_LOSS_KILL_BP);
                bool fresh_trigger = (!daily_below_threshold && now_below);
                daily_below_threshold = now_below;
                if (fresh_trigger && now_ms >= daily_kill_until) {
                    daily_kill_until = now_ms + DAILY_HALT_MS;
                    g_daily_kill_until_ms.store(daily_kill_until, std::memory_order_relaxed);
                    save_protection_state();
                    std::printf("[PORTFOLIO] DAILY_KILL TRIPPED: 24h_pnl=%+.1fbp < %.0fbp -> flatten + halt 24h\n",
                        daily_pnl, DAILY_LOSS_KILL_BP);
                    std::fflush(stdout);
                    int killed = 0;
                    for (auto& s : g_slots) {
                        if (!s.engine || !s.engine->in_position()) continue;
                        double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                        if (spot > 0.0) { s.engine->kill_all(spot, now_ms); killed++; }
                    }
                    std::printf("[PORTFOLIO] DAILY_KILL flattened %d positions\n", killed);
                    std::fflush(stdout);
                }
                bool daily_halted = (now_ms < daily_kill_until);

                // Streak halt (kept, neutralised by default thresholds)
                bool streak_halted = (now_ms < streak_halt_until_ms);
                if (!streak_halted) {
                    double streak_sum = 0.0;
                    int streak_n = 0;
                    {
                        std::lock_guard<std::mutex> tlk(g_trades_mtx);
                        for (int i = (int)g_trade_log.size() - 1; i >= 0 && streak_n < STREAK_LOOKBACK; --i) {
                            if (g_trade_log[i].reason == "SHUTDOWN") continue;
                            streak_sum += g_trade_log[i].net_bp;
                            streak_n++;
                        }
                    }
                    if (streak_n >= STREAK_LOOKBACK && streak_sum < STREAK_HALT_BP) {
                        streak_halt_until_ms = now_ms + STREAK_HALT_MS;
                        streak_halted = true;
                        std::printf("[PORTFOLIO] STREAK_HALT TRIPPED: last %d trades sum=%+.1fbp\n",
                            streak_n, streak_sum);
                        std::fflush(stdout);
                    }
                }

                // ── S34 TIER 0: PERSISTENT PER (anchored to all-time cum) ─
                // all_time_cum is refreshed in on_trade_callback. Recompute
                // here from trade_log as a safety net (handles bookkeeping
                // updates outside the callback). all_time_peak ratchets up.
                // S54: start from the session-reset baseline + count only trades
                // at/after the reset epoch, so a reset (which voids pre-fix bug
                // losses) actually sticks instead of being re-summed from zero.
                double all_time_cum = load_dbl_atomic(g_pnl_epoch_baseline_bp_bits);
                int64_t pnl_epoch = g_pnl_epoch_ms.load(std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> tlk(g_trades_mtx);
                    for (auto& tr : g_trade_log) {
                        if (tr.reason == "SHUTDOWN") continue;
                        if (pnl_epoch > 0 && tr.exit_ts_ms < pnl_epoch) continue;
                        all_time_cum += tr.net_bp;
                    }
                }
                double all_time_peak = load_dbl_atomic(g_all_time_peak_bp_bits);
                if (all_time_cum > all_time_peak) all_time_peak = all_time_cum;

                // S34 anti-deadlock: if DD from peak exceeds CLAMP, the peak
                // is no longer meaningful — loss already absorbed, peak is
                // historical noise. Reset peak to current cum so ratchet
                // rearms only on NEW gains from here. Prevents permanent
                // lockdown after a catastrophic past bleed.
                constexpr double MAX_TOLERATED_DD_BP = 800.0;
                if ((all_time_peak - all_time_cum) > MAX_TOLERATED_DD_BP) {
                    std::printf("[PROTECTION] PEAK_CLAMP: peak %.1fbp - cum %.1fbp = %.1fbp DD > %.0fbp -> reset peak to cum\n",
                        all_time_peak, all_time_cum,
                        all_time_peak - all_time_cum, MAX_TOLERATED_DD_BP);
                    std::fflush(stdout);
                    all_time_peak = all_time_cum;
                    save_protection_state();
                }

                store_dbl_atomic(g_all_time_cum_bp_bits,  all_time_cum);
                store_dbl_atomic(g_all_time_peak_bp_bits, all_time_peak);

                static bool ratchet_locked = false;
                static int64_t lock_start_ms = 0;
                // S54: a warm session_reset requests an explicit unlock (peak stays
                // warm so the peak<arm auto-unlock below won't fire).
                if (g_force_ratchet_unlock.exchange(false, std::memory_order_relaxed)) {
                    ratchet_locked = false; lock_start_ms = 0;
                }

                // ── S34 Variant C: PEAK DECAY WHILE LOCKED ───────────────
                // Sim showed this captures +283bp more than fixed-rearm
                // while keeping giveback to 40bp. Peak slowly decays toward
                // cum at PEAK_DECAY_PER_DAY. After ~1 day locked, peak has
                // dropped 10% so giveback shrinks below REARM naturally.
                // Self-healing — no manual reset needed for routine chop.
                constexpr double PEAK_DECAY_PER_DAY = 0.10;
                if (ratchet_locked && lock_start_ms > 0 && all_time_peak > all_time_cum) {
                    double days_locked = (double)(now_ms - lock_start_ms) / (24.0 * 3600.0 * 1000.0);
                    if (days_locked > 0.0) {
                        double decay_bp = all_time_peak * PEAK_DECAY_PER_DAY * days_locked;
                        double new_peak = std::max(all_time_cum, all_time_peak - decay_bp);
                        if (new_peak < all_time_peak) {
                            all_time_peak = new_peak;
                            store_dbl_atomic(g_all_time_peak_bp_bits, all_time_peak);
                            save_protection_state();
                        }
                        lock_start_ms = now_ms;  // anchor for next iter incremental decay
                    }
                }

                // S54 fix: if peak fell below the arm threshold (e.g. after a
                // session_reset or peak-decay), there is nothing left to defend —
                // force-unlock. Without this the unlock branch below is unreachable
                // (it lives inside the peak>=ARM guard) and the lock latches forever.
                if (ratchet_locked && all_time_peak < RATCHET_ARM_BP) {
                    std::printf("[RATCHET] UNLOCKED: peak=%+.1fbp < arm=%.0fbp -> nothing to defend, resume\n",
                        all_time_peak, RATCHET_ARM_BP);
                    std::fflush(stdout);
                    ratchet_locked = false;
                    lock_start_ms = 0;
                    push_alert("RATCHET UNLOCKED — peak below arm, entries resume");
                }
                if (all_time_peak >= RATCHET_ARM_BP) {
                    double giveback = all_time_peak - all_time_cum;
                    bool abs_trip = giveback > RATCHET_GIVEBACK_BP;
                    bool pct_trip = (all_time_peak > 0.0) &&
                                    ((giveback / all_time_peak) > RATCHET_GIVEBACK_PCT);
                    if (abs_trip || pct_trip) {
                        if (!ratchet_locked) {
                            std::fprintf(stderr,
                                "\n[RATCHET] **LOCKED** peak=%+.1fbp cum=%+.1fbp giveback=%.1fbp (%.1f%%) -> halt entries\n\n",
                                all_time_peak, all_time_cum, giveback,
                                100.0 * giveback / std::max(all_time_peak, 1.0));
                            std::printf("[RATCHET] LOCKED: peak=%+.1fbp cum=%+.1fbp giveback=%.1fbp (%.1f%%) -> halt entries\n",
                                all_time_peak, all_time_cum, giveback,
                                100.0 * giveback / std::max(all_time_peak, 1.0));
                            std::fflush(stdout);
                            ratchet_locked = true;
                            lock_start_ms = now_ms;
                            char buf[256];
                            std::snprintf(buf, sizeof(buf),
                                "RATCHET LOCKED peak=%+.1fbp cum=%+.1fbp giveback=%.1fbp (%.1f%%)",
                                all_time_peak, all_time_cum, giveback,
                                100.0 * giveback / std::max(all_time_peak, 1.0));
                            push_alert(buf);
                        }
                    } else if (ratchet_locked && giveback < RATCHET_REARM_BP) {
                        std::printf("[RATCHET] UNLOCKED: peak=%+.1fbp cum=%+.1fbp giveback=%.1fbp -> resume\n",
                            all_time_peak, all_time_cum, giveback);
                        std::fflush(stdout);
                        ratchet_locked = false;
                        lock_start_ms = 0;
                        push_alert("RATCHET UNLOCKED — entries resume");
                    }
                }

                // ── S34 TIER 1: SPOT-AWARE BTC regime classifier ─────────
                // Spot-only constraint: we can ONLY make money in bullish or
                // bullish-sideways conditions. In bear, mean-revert = knife
                // catching. So bear-trend = halt ALL entries (not just TSMOM).
                //
                // Reads BTC D1 TSMOM engine state: vol_ratio + trend_bullish.
                //   0 CRASH      vol spike OR confirmed bear -> halt all
                //   1 BEAR       BTC bearish trend -> halt all entries
                //   2 BULL_CHOP  BTC bullish + low vol -> mean-revert ok, TSMOM off
                //   3 BULL_TREND BTC bullish + normal vol -> all engines on
                int regime = 3;  // default BULL_TREND
                double btc_vol_ratio = 1.0;
                bool   btc_d1_bullish = true;
                bool   btc_h4_bullish = true;
                // S34: short-term rally detector — current spot vs spot N
                // samples ago. Uses file-scope buffer so REST seed at startup
                // can pre-fill it (otherwise no rally signal until ~10min after
                // every restart). Sample rate capped at 5s/sample.
                double btc_spot_now = load_dbl_atomic(g_last_spot_px_bits[chimera::SYM_BTC]);
                int64_t btc_last_sample_ms = g_btc_rally_last_ms.load(std::memory_order_relaxed);
                if (btc_spot_now > 0.0 && (now_ms - btc_last_sample_ms) >= 5000) {
                    int idx = g_btc_rally_idx.load(std::memory_order_relaxed);
                    store_dbl_atomic(g_btc_rally_px[idx], btc_spot_now);
                    g_btc_rally_idx.store((idx + 1) % BTC_RALLY_BUF, std::memory_order_relaxed);
                    int filled = g_btc_rally_filled.load(std::memory_order_relaxed);
                    if (filled < BTC_RALLY_BUF) g_btc_rally_filled.store(filled + 1, std::memory_order_relaxed);
                    g_btc_rally_last_ms.store(now_ms, std::memory_order_relaxed);
                }
                double btc_short_ret_pct = 0.0;
                int filled_now = g_btc_rally_filled.load(std::memory_order_relaxed);
                int idx_now = g_btc_rally_idx.load(std::memory_order_relaxed);
                if (filled_now >= 4 && btc_spot_now > 0.0) {
                    int back_idx = (idx_now - filled_now + BTC_RALLY_BUF) % BTC_RALLY_BUF;
                    double oldest = load_dbl_atomic(g_btc_rally_px[back_idx]);
                    if (oldest > 0.0) {
                        btc_short_ret_pct = (btc_spot_now / oldest - 1.0) * 100.0;
                    }
                }
                // +0.2% over recent samples = rally override
                bool short_rally = (btc_short_ret_pct > 0.2);

                // S54 MACRO 200d-MA: at each UTC day rollover append today's BTC
                // close to the ring + recompute the 200d MA. macro_bull is updated
                // EVERY tick (spot vs MA) so it flips intraday on a cross.
                if (btc_spot_now > 0.0) {
                    int64_t day = now_ms / 86400000LL;
                    if (day != g_macro_last_day.load(std::memory_order_relaxed) &&
                        g_btc_daily_n >= MACRO_MA_DAYS) {
                        g_btc_daily_ring[g_btc_daily_head] = btc_spot_now;  // today's close ~ current spot
                        g_btc_daily_head = (g_btc_daily_head + 1) % 256;
                        double s = 0.0;
                        for (int j = 0; j < MACRO_MA_DAYS; ++j) {
                            int idx = (g_btc_daily_head - 1 - j + 512) % 256;
                            s += g_btc_daily_ring[idx];
                        }
                        store_dbl_atomic(g_btc_200dma_bits, s / (double)MACRO_MA_DAYS);
                        g_macro_last_day.store(day, std::memory_order_relaxed);
                    }
                    double ma = load_dbl_atomic(g_btc_200dma_bits);
                    g_macro_bull.store(ma > 0.0 && btc_spot_now > ma, std::memory_order_relaxed);
                }

                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC && s.tag.find("-TSMOM-") != std::string::npos) {
                        if (s.tf_secs == 86400) {
                            btc_vol_ratio  = s.engine->vol_ratio_public();
                            btc_d1_bullish = s.engine->trend_bullish();
                        } else if (s.tf_secs == 14400) {  // H4
                            btc_h4_bullish = s.engine->trend_bullish();
                        }
                    }
                }
                // S34-r7: BEAR only triggers on confirmed crash (recent < -0.5%).
                // Otherwise mean-revert engines may earn in chop. Spot-only
                // needs SOME tradeable regime when not bullish.
                if (btc_vol_ratio > 2.0) {
                    regime = 0;  // CRASH — halt all
                } else if (!btc_d1_bullish && !btc_h4_bullish) {
                    // P2/S46: BEAR is now reachable on a PERSISTENT downtrend (both
                    // D1 and H4 bearish), independent of micro-bounces. Old code
                    // required !short_rally && short_ret<-0.5 so any 0.2% blip
                    // demoted a confirmed downtrend back to CHOP (which PASSES the
                    // entry gate) — that leak fed longs into grinding bears. For a
                    // spot-LONG-only book a both-TF downtrend means: sit out.
                    regime = 1;  // BEAR — halt all
                } else if (!btc_d1_bullish || btc_vol_ratio < 0.7 || btc_vol_ratio > 1.3 || short_rally) {
                    regime = 2;  // BULL_CHOP — only symbols personally bullish trade (P2 #2)
                } else {
                    regime = 3;  // BULL_TREND — all engines
                }

                // ── S34-r8: PER-SYMBOL regime classification ─────────────
                // Each symbol gets own short-rally + D1/H4 bullish read.
                // Lets alts trade when they have edge even if BTC flat.
                // Engine entry gate uses the SYMBOL's regime, not global.
                for (int sym = 0; sym < chimera::MAX_SYMBOLS; sym++) {
                    double sym_spot = load_dbl_atomic(g_last_spot_px_bits[sym]);
                    if (sym_spot <= 0.0) {
                        g_sym_regime[sym].store(1, std::memory_order_relaxed);  // unknown = BEAR-safe
                        continue;
                    }
                    int64_t sym_last_ms = g_sym_rally_last_ms[sym].load(std::memory_order_relaxed);
                    if ((now_ms - sym_last_ms) >= 5000) {
                        int sidx = g_sym_rally_idx[sym].load(std::memory_order_relaxed);
                        store_dbl_atomic(g_sym_rally_px[sym][sidx], sym_spot);
                        g_sym_rally_idx[sym].store((sidx + 1) % SYM_RALLY_BUF, std::memory_order_relaxed);
                        int sfilled = g_sym_rally_filled[sym].load(std::memory_order_relaxed);
                        if (sfilled < SYM_RALLY_BUF) g_sym_rally_filled[sym].store(sfilled + 1, std::memory_order_relaxed);
                        g_sym_rally_last_ms[sym].store(now_ms, std::memory_order_relaxed);
                    }
                    double sym_ret_pct = 0.0;
                    int sf = g_sym_rally_filled[sym].load(std::memory_order_relaxed);
                    int si = g_sym_rally_idx[sym].load(std::memory_order_relaxed);
                    if (sf >= 4) {
                        int back = (si - sf + SYM_RALLY_BUF) % SYM_RALLY_BUF;
                        double oldest = load_dbl_atomic(g_sym_rally_px[sym][back]);
                        if (oldest > 0.0) sym_ret_pct = (sym_spot / oldest - 1.0) * 100.0;
                    }
                    store_dbl_atomic(g_sym_short_ret[sym], sym_ret_pct);

                    // Read symbol's D1 + H4 trend
                    bool sym_d1_bull = true, sym_h4_bull = true;
                    double sym_vol_ratio = 1.0;
                    for (auto& s : g_slots) {
                        if (!s.engine || s.symbol_id != sym) continue;
                        if (s.tag.find("-TSMOM-") == std::string::npos) continue;
                        if (s.tf_secs == 86400) {
                            sym_d1_bull = s.engine->trend_bullish();
                            sym_vol_ratio = s.engine->vol_ratio_public();
                        } else if (s.tf_secs == 14400) {
                            sym_h4_bull = s.engine->trend_bullish();
                        }
                    }
                    bool sym_short_rally = (sym_ret_pct > 0.2);
                    int sym_reg = 3;
                    if (sym_vol_ratio > 2.0) sym_reg = 0;
                    else if (!sym_d1_bull && !sym_h4_bull && !sym_short_rally && sym_ret_pct < -0.5) sym_reg = 1;
                    // S35-cluster: tighter BULL_TREND gate. Require BOTH D1
                    // AND H4 bullish AND vol in trending band AND positive
                    // short-term return. Anything weaker -> BULL_CHOP (TSMOM
                    // disabled). Tape showed TSMOM firing in chop on 27-28
                    // May because old single-leg D1 bullish was enough.
                    else if (!sym_d1_bull || !sym_h4_bull ||
                             sym_vol_ratio < 0.7 || sym_vol_ratio > 1.3 ||
                             sym_short_rally || sym_ret_pct <= 0.0) sym_reg = 2;
                    else sym_reg = 3;
                    g_sym_regime[sym].store(sym_reg, std::memory_order_relaxed);
                }
                g_regime.store(regime, std::memory_order_relaxed);
                store_dbl_atomic(g_btc_short_ret_bits, btc_short_ret_pct);

                // ── S34 TIER 2: progressive size throttle ────────────────
                // Size scaled by all-time DD from peak.
                //   DD<=100bp  -> 1.0x
                //   100-300bp  -> 0.5x
                //   300-500bp  -> 0.25x
                //   >500bp     -> 0.0x (halt; ratchet should already be locked)
                double dd_from_peak = all_time_peak - all_time_cum;
                double size_throttle = 1.0;
                if      (dd_from_peak >= 500.0) size_throttle = 0.0;
                else if (dd_from_peak >= 300.0) size_throttle = 0.25;
                else if (dd_from_peak >= 100.0) size_throttle = 0.5;
                else                            size_throttle = 1.0;
                // S54 derisk: PROACTIVE concentration throttle. The book is ~1.8
                // effective bets, so many concurrent longs = one oversized bet a
                // regime flip hits all at once. Scale new-entry size down past a
                // soft cap so total gross is bounded BEFORE any drawdown shows.
                constexpr int CONC_SOFT_CAP = 10;
                if (open_positions > CONC_SOFT_CAP) {
                    double conc = (double)CONC_SOFT_CAP / (double)open_positions; // 10/14=0.71
                    if (conc < size_throttle) size_throttle = conc;              // tighter binds
                }
                // CRASH regime overrides to 0 (no entries)
                // CRASH or BEAR -> no entries
                if (regime == 0 || regime == 1) size_throttle = 0.0;
                store_dbl_atomic(g_size_throttle_bits, size_throttle);

                // AUDIT-2026-S35-DYN: refresh live tier stats every 60s.
                {
                    int64_t now_ms_l = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    int64_t last = g_last_live_recalc_ms.load(std::memory_order_relaxed);
                    if (now_ms_l - last >= 60000) {
                        recompute_live_tiers();
                        g_last_live_recalc_ms.store(now_ms_l, std::memory_order_relaxed);
                    }
                }

                // ── S34 TIER 4: per-strategy concurrent cap ──────────────
                // Mono-culture defence. TSMOM = 62% of engine roster.
                // Cap concurrent open TSMOMs to prevent N engines firing
                // same direction same bar then all stopping out same reversal.
                // S35-cluster: 6 -> 2. Live tape 28 May had 7-engine
                // simultaneous H8 TSMOM cluster wipe.
                constexpr int MAX_TSMOM_CONCURRENT = 2;
                // S35-cluster: ALT-L1 basket cap (NEAR/SEI/TIA/FET/INJ/APT/
                // ONDO/SUI/ARB — all ~0.85+ corr with BTC). Cap = 2.
                constexpr int MAX_ALT_L1_CONCURRENT = 2;
                // S35-cluster: per (symbol, family) cap = 1. Blocks
                // NEAR-TSMOM-H4 + H8 + H12 triple-stack.
                constexpr int MAX_PER_SYM_FAMILY = 1;
                g_tsmom_open_count.store(tsmom_open, std::memory_order_relaxed);

                // S44d: testing-mode bypass — ignore protective halts so
                // shadow trades can flow during research. Trip-switch:
                // shadow_mode=false + this flag = FATAL at startup.
                bool testing_bypass = g_protection_disabled_for_testing.load();
                bool gate_open = (open_positions < MAX_CONCURRENT_POSITIONS) &&
                                 (testing_bypass || recent_pnl > DRAWDOWN_HALT_BP) &&
                                 (testing_bypass || total_unrealized_bp > UNREALIZED_HALT_BP) &&
                                 (testing_bypass || !streak_halted) &&
                                 (testing_bypass || !ratchet_locked) &&
                                 (testing_bypass || !daily_halted) &&
                                 (testing_bypass || (regime != 0 && regime != 1)) &&
                                 (testing_bypass || size_throttle > 0.0);

                // Publish gate state to globals for /api/state2 + GUI banner
                g_gate_open.store(gate_open, std::memory_order_relaxed);
                g_ratchet_locked.store(ratchet_locked, std::memory_order_relaxed);
                g_streak_halted.store(streak_halted, std::memory_order_relaxed);
                store_dbl_atomic(g_session_cum_bp_bits, all_time_cum);
                store_dbl_atomic(g_session_peak_bp_bits, all_time_peak);
                store_dbl_atomic(g_recent_pnl_bp_bits, recent_pnl);
                store_dbl_atomic(g_unrealized_bp_bits, total_unrealized_bp);
                store_dbl_atomic(g_daily_pnl_bp_bits, daily_pnl);
                g_open_positions_count.store(open_positions, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> tlk(g_trades_mtx);
                    for (int i = (int)g_trade_log.size() - 1; i >= 0; --i) {
                        if (g_trade_log[i].reason == "SHUTDOWN") continue;
                        g_last_trade_exit_ms.store(g_trade_log[i].exit_ts_ms, std::memory_order_relaxed);
                        break;
                    }
                }

                // ── Per-engine apply: gate + sizing + strategy/symbol cap + conviction ─
                // S34 TIER 3 conviction filter: when DD from peak > 200bp,
                // only allow entries flagged is_high_conviction(). Existing
                // accessor checks funding tailwind + cross-TF score.
                bool conviction_only = (dd_from_peak >= 200.0);
                // S34: per-symbol concurrency cap.
                // Original cap=1 prevented correlated cluster losses pre-protections.
                // AUDIT-2026-S35 bumped to 3: with hard_floor_bp=-50 + early_kill_bp=-25
                // each trade is bounded, so worst-case 3-engine cluster loss = -210bp,
                // recoverable. Cap=3 sim showed +130% net_bp vs cap=1 over 7d window
                // (NEAR rally) — captures most of edge with bounded tail risk.
                constexpr int MAX_PER_SYMBOL = 3;
                for (auto& s : g_slots) {
                    if (!s.engine) continue;

                    bool sym_ok = true;
                    if (s.symbol_id >= 0 && s.symbol_id < chimera::MAX_SYMBOLS &&
                        per_sym_open[s.symbol_id] >= MAX_PER_SYMBOL) {
                        sym_ok = false;
                    }

                    bool is_tsmom = (s.tag.find("-TSMOM-") != std::string::npos);
                    bool strat_ok = true;
                    if (is_tsmom && tsmom_open >= MAX_TSMOM_CONCURRENT) strat_ok = false;
                    // S35-cluster: alt-L1 basket cap
                    if (s.symbol_id >= 0 && s.symbol_id < chimera::MAX_SYMBOLS &&
                        is_alt_l1(s.symbol_id) && alt_l1_open >= MAX_ALT_L1_CONCURRENT) {
                        strat_ok = false;
                    }
                    // S35-cluster: per (symbol, family) dedup. Already-open
                    // engine on this (sym, family) blocks the rest. Engine
                    // with its own slot still allowed (refresh on same engine
                    // not gated here — gate only blocks new entries when
                    // another slot already holds the (sym, family) pair).
                    {
                        std::string fam = family_of(s.tag);
                        if (!fam.empty() && s.symbol_id >= 0 &&
                            !s.engine->in_position()) {
                            auto it = per_sym_fam_open.find(
                                std::to_string(s.symbol_id) + "|" + fam);
                            if (it != per_sym_fam_open.end() &&
                                it->second >= MAX_PER_SYM_FAMILY) {
                                strat_ok = false;
                            }
                        }
                    }
                    // ── S34-r8: per-symbol regime overrides global ──────
                    // Use the SYMBOL's own regime, not BTC's. Lets alts trade
                    // when they have edge even if BTC is flat/bear.
                    int sym_reg = (s.symbol_id >= 0 && s.symbol_id < chimera::MAX_SYMBOLS)
                                ? g_sym_regime[s.symbol_id].load(std::memory_order_relaxed)
                                : regime;
                    if (sym_reg == 0 || sym_reg == 1) strat_ok = false;
                    else if (sym_reg == 2 && s.engine->is_trend_following()) strat_ok = false;

                    bool conv_ok = true;
                    if (conviction_only && !s.engine->is_high_conviction()) conv_ok = false;

                    // S34 PF filter: bleed engines stay permanently off
                    bool pf_ok = !s.pf_blocked;
                    s.engine->set_portfolio_gate(gate_open && sym_ok && strat_ok && conv_ok && pf_ok);
                    // Apply DD-based size throttle composed with AUDIT-2026 overlay
                    // (xsec 28d momentum rank × 20d vol scaling, clamped [0.25, 2.0])
                    // and S35-DYN per-engine tier — Bayesian blend of backtest + live
                    // (live weight ramps with sample size, capped 50% at n>=50).
                    double overlay_mult = g_portfolio_overlay.multiplier_for(s.symbol_id);
                    double tier_mult = tier_sizing_mult_blended(s.tag, s.oos_sharpe, s.bt_pf);
                    s.engine->set_sizing_mult(size_throttle * overlay_mult * tier_mult);
                }
                // P1/S46: set the REAL risk multiplier (DD-throttle x vol-overlay)
                // on EVERY wired engine — this is what the live qty calc now reads
                // (intent.risk_mult). Excludes tier (tier is applied separately in
                // the callback). This is what makes the drawdown throttle and
                // vol-scaling finally shrink position size; previously discarded.
                for (auto* e : g_all_wired) {
                    if (!e) continue;
                    int sid = chimera::symbol_to_id(e->cfg().symbol);
                    double ov = (sid >= 0) ? g_portfolio_overlay.multiplier_for(sid) : 1.0;
                    e->set_risk_mult(size_throttle * ov);
                }

                static bool prev_gate = true;
                if (!gate_open && prev_gate) {
                    const char* rs = (regime == 0) ? "CRASH"
                                   : (regime == 1) ? "BEAR"
                                   : (regime == 2) ? "BULL_CHOP"
                                   : "BULL_TREND";
                    std::printf("[PORTFOLIO] GATE CLOSED: pos=%d/%d | 4h=%+.1f | 24h=%+.1f | unreal=%+.1f | cum=%+.1f peak=%+.1f dd=%.1f | ratchet=%s daily=%s regime=%s throttle=%.2fx tsmom_open=%d\n",
                        open_positions, MAX_CONCURRENT_POSITIONS, recent_pnl, daily_pnl,
                        total_unrealized_bp, all_time_cum, all_time_peak, dd_from_peak,
                        ratchet_locked ? "1" : "0",
                        daily_halted ? "1" : "0",
                        rs, size_throttle, tsmom_open);
                    std::fflush(stdout);
                } else if (gate_open && !prev_gate) {
                    std::printf("[PORTFOLIO] GATE OPENED: cum=%+.1f peak=%+.1f throttle=%.2fx\n",
                        all_time_cum, all_time_peak, size_throttle);
                    std::fflush(stdout);
                }
                prev_gate = gate_open;
            }

            // ── Correlation regime: compute rolling corr(alt, BTC) ───────────
            // Uses last-seen spot prices to compute a simple 1-tick return
            // correlation proxy. For full rolling correlation, we'd need bar-level
            // returns — but a simpler heuristic works: if all alts moved in the
            // same direction as BTC by a similar magnitude in recent ticks,
            // correlation is high. We use the D1 engine returns as proxy.
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                // Get BTC return (from D1 engine momentum if available)
                double btc_momentum = 0.0;
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC && s.tf_secs == 86400 &&
                        s.engine->is_trend_following()) {
                        // Use trend_bullish as a simplification — if BTC and alt
                        // are both in same trend direction, correlation is high.
                        // More sophisticated: track rolling returns. For now, use
                        // a simple heuristic based on D1 trend agreement.
                        std::string state = s.engine->state_json();
                        auto pos = state.find("\"momentum_pct\":");
                        if (pos != std::string::npos) {
                            try { btc_momentum = std::stod(state.substr(pos + 16, 10)); }
                            catch (...) {}
                        }
                        break;
                    }
                }

                // For each non-BTC symbol, check if their D1 momentum is highly
                // aligned with BTC (same sign, similar magnitude = high correlation)
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    if (s.symbol_id == chimera::SYM_BTC) {
                        s.engine->set_corr_high(false);  // BTC never self-suppresses
                        continue;
                    }
                    // Simple correlation proxy: if BTC is moving strongly and
                    // this symbol has same-direction movement > 50% of BTC's move,
                    // flag as high correlation. This is a conservative heuristic.
                    // TODO: upgrade to true rolling Pearson correlation on bar returns.
                    bool corr_flag = false;
                    if (std::fabs(btc_momentum) > 5.0) {  // BTC moving > 5% (raised from 3% — shadow tuning: 3% moves are routine, suppressed too many alts)
                        // Check this symbol's D1 momentum
                        if (s.tf_secs == 86400 && s.engine->is_trend_following()) {
                            std::string state = s.engine->state_json();
                            auto pos = state.find("\"momentum_pct\":");
                            if (pos != std::string::npos) {
                                double sym_mom = 0.0;
                                try { sym_mom = std::stod(state.substr(pos + 16, 10)); }
                                catch (...) {}
                                // Same sign and magnitude > 50% of BTC
                                if ((btc_momentum > 0 && sym_mom > btc_momentum * 0.5) ||
                                    (btc_momentum < 0 && sym_mom < btc_momentum * 0.5)) {
                                    corr_flag = true;
                                }
                            }
                        }
                    }
                    s.engine->set_corr_high(corr_flag);
                }
            }
        }
    }

    std::printf("\n[SHUTDOWN] Stopping...\n");
    std::fflush(stdout);

    // ── Graceful shutdown: write final snapshot for resume on next start ──
    // Instead of force-closing positions (which realises them at shutdown price),
    // we persist the full position state. On next start, resume_position() picks
    // them back up seamlessly. This means deploys/restarts don't interrupt trades.
    {
        std::lock_guard<std::mutex> lk(g_engine_mtx);
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int open_count = 0;
        std::ostringstream snap;
        snap << "{\"snapshot_ts\":" << now_ms << ",\"positions\":[";
        for (auto& s : g_slots) {
            if (!s.engine || !s.engine->in_position()) continue;
            double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
            std::string pj = s.engine->position_snapshot_json(spot);
            if (!pj.empty()) {
                if (open_count > 0) snap << ",";
                snap << pj;
                open_count++;
            }
        }
        snap << "],\"open_count\":" << open_count << "}\n";
        FILE* f = fopen("data/open_positions.json", "w");
        if (f) {
            std::string out = snap.str();
            fwrite(out.c_str(), 1, out.size(), f);
            fclose(f);
        }
        std::printf("[SHUTDOWN] Snapshot saved: %d open position(s) will resume on next start\n", open_count);
        std::fflush(stdout);
    }

    std::atomic<bool> shutdown_done{false};
    std::thread watchdog([&](){
        for (int i = 0; i < 30; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (shutdown_done) return;
        }
        release_instance_lock(); std::_Exit(0);
    });

    feed.stop();
    shutdown_done = true;
    if (watchdog.joinable()) watchdog.join();

    int total_trades = 0;
    double total_bp = 0.0;
    for (auto& s : g_slots) {
        if (!s.engine) continue;
        total_trades += s.engine->trades();
        total_bp     += s.engine->total_bp();
    }
    std::printf("[SHUTDOWN] aggregate trades=%d  total=%+.1fbp\n", total_trades, total_bp);
    std::fflush(stdout);

    release_instance_lock();
    return 0;
}
