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
#include <cmath>
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
#include <random>                      // Phase-1: control-token generation
#include "live/SpotExecutor.hpp"
#include "live/RuntimeMode.hpp"        // Phase-1: ONE immutable process mode
#include "live/ExecutionGateway.hpp"   // Phase-1: single order chokepoint
#include "live/StartupReconciler.hpp"  // Phase-2: boot reconcile gate (ledger/filters/clock via gateway)
#include "live/UserStreamHaltGuard.hpp"// Phase-8G: user-stream heartbeat-lapse AUTO-HALT (go-live blocker)
#include "live/SpotPortfolioAllocator.hpp" // Phase-3: portfolio unification (merge/cap/net + regime/DD/factor overlays)
#include "live/EngineRegistry.hpp"     // Phase-4 item 20: honest lifecycle registry (reconcile declared vs actual)
#include "live/GateAttribution.hpp"    // Phase-4 item 21: gate-attribution + counterfactual + correlation-ID
#include "live/RealisticFill.hpp"      // Phase-4 item 22: additive realistic-fill shadow metric (parallel book)
#include "live/DataQuality.hpp"        // Phase-4 item 23: seed/feed schema+checksum+gap+stale validation
#include "live/HttpControlAuth.hpp"    // Phase-1: control-API auth/method helpers
#include "core/EdgeEngine.hpp"
#include "core/UpJumpLadderCompanion.hpp" // S-2026-07-05b: tiered-2 + self-funding ladder clip book for UPJUMP legs (shadow)
#include "core/CryptoCostLedger.hpp"       // S-2026-07-13 campaign architecture (13j §2.11): measured per-symbol effective cost
#include "core/CryptoOpportunityGate.hpp"  // S-2026-07-13 campaign architecture: cost-viability entry gate
#include "core/CryptoCampaignManager.hpp"  // S-2026-07-13 campaign architecture: virtual-lot parent campaigns (SHADOW, mimic OFF)
#include "core/GridEngine.hpp"            // S55: maker-native grid sleeve (shadow)
#include "core/MacroBaseEngine.hpp"       // S55: macro-bull base (bull-beta core, shadow)
#include "core/SymbolIndex.hpp"
#include "core/PortfolioOverlay.hpp"  // AUDIT-2026: cross-sec mom + vol-scale overlay
#include "core/CrossSectionalMomentumEngine.hpp"  // S-2026-06-18: validated standalone XSec allocator
#include "core/CrossSectionalMomentum2Engine.hpp" // S-2026-07-11: Phase-5 XSec 2.0 SHADOW comparison book
#include "core/RipRiderEngine.hpp"                 // S-2026-06-18: sleeve 3 — per-symbol regime-gated rip-rider
#include "core/TrendPullbackReclaimEngine.hpp"     // S-2026-07-11: Phase-6 family 1 — SHADOW OBSERVATION-ONLY
#include "core/CompressionBreakoutDailyEngine.hpp" // S-2026-07-11: Phase-6 family 2 — SHADOW OBSERVATION-ONLY
#include "core/BullRegimeMeanReversionEngine.hpp"  // S-2026-07-11: Phase-6 family 3 — SHADOW OBSERVATION-ONLY
#include "core/DerivativesSignals.hpp"             // S-2026-07-11: Phase-7 derivatives-data-as-signal — OBSERVATION-ONLY recorder
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

// ── Phase-4 (2026-07-11) observability globals ──────────────────────────────
// item 20: HONEST engine registry — reconciles each declared engine lifecycle
//   (DISABLED/SHADOW/PAPER/LIVE/HALTED/STALE) against the ACTUAL wired+connected
//   graph; startup aborts on a mismatch and the status count is generated from
//   connected_count() (not a hardcoded banner). Populated just before the READY
//   banner; /api/state2 reads its state_json.
static chimera::EngineRegistry   g_registry;
static int                       g_grid_clip_count = 0;   // real UpJump grid-cell count (set at grid init)
// item 21: gate-attribution sink (per-gate suppression reason + counterfactual,
//   correlation-ID threaded). Observational only — never alters signal/exit logic.
static chimera::GateAttribution  g_gate_attr;
// item 22: ADDITIVE realistic-fill shadow metric — a PARALLEL book (spread+
//   slippage+fee+queue) beside the signal-price book. Never touches the running
//   shadow ledger or the 32-cell grid; the operator compares the two books.
static chimera::ShadowFillComparator g_fill_realism;
// item 23: data-quality gate for seed/warm-start files. Structural checks
//   (schema/checksum/dup/out-of-order/invalid-price) REFUSE a malformed seed;
//   for the STATIC committed warm-seed CSVs staleness + gap rejection are OFF
//   (those files are inherently historical and may span exchange downtime), so
//   a good-but-old seed still loads while a CORRUPTED one is refused.
static chimera::DataQualityGate g_dq_gate;

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

// Phase-1 review (2026-07-11): control-API hardening. The HTTP server previously
// bound to INADDR_ANY (public IP) with no auth on kill/reset. Bind address now
// defaults to localhost; mutating endpoints require this token. Both set in
// main() from live_config.json / env before the server thread launches.
static std::string g_http_bind_addr = "127.0.0.1";
static std::string g_ctrl_token;

// ── Session 30: Multi-symbol funding filter ─────────────────────────────────
static chimera::MultiSymbolFundingFilter g_funding_filter;
static int64_t g_last_funding_fetch_ms = 0;
static std::thread g_funding_thread;   // CH-06 (audit 2026-07-13): owned funding worker
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
    /*60 XLM*/0.50, /*61 VET*/0.35, /*62 THETA*/0.35, /*63 SUSHI*/0.30,
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
    /*60 XLM */CL_L1,    /*61 VET */CL_OTHER, /*62 THETA*/CL_L1,   /*63 SUSHI*/CL_DEFI,
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

// ── CHIMERA→OMEGA DESK trade export (S-2026-07-12) ──────────────────────────
// Appends every CLOSED shadow trade (slot engines TSMOM/ICHI/BOLL + UPJUMP
// parents via set_on_trade in the g_slots wiring loop, UpJump companion clips
// via persist_companion_clip, XSec/XSec2 rebalance legs) to
// data/chimera_inbound.csv in the Omega desk's crypto_inbound schema plus a
// trailing reason column:
//   id,entry_ts,exit_ts,sym,strat,side,entry,exit,net_usd,reason
// (entry_ts/exit_ts in SECONDS — matches crypto_inbound.csv; the desk parser
//  gmtime()s field 2.)
// TRANSPORT: the Mac relay (Omega repo tools/gui/refresh_crypto_companion.sh,
// launchd com.omega.crypto-companion-push every 120s) pulls this file and
// pushes it to omega-new:C:/Omega/logs/trades/chimera_inbound.csv, where the
// :7779 /api/crypto_trades endpoint merges it into the LAST-15-TRADES panel
// tagged book="chimera".
// DISPLAY-ONLY on the desk: the ALL-TIME PnL fold reads crypto_inbound.csv
// only (Omega CryptoLedgerInbound) — chimera shadow PnL is NOT folded.
// net_usd uses the desk pool convention (operator 2026-07-04): $ = bp ×
// POOL/10000 with POOL=$10,000 → 1 bp = $1. These are REAL FORWARD shadow
// trades, never backtest/replay rows (feedback-no-backtest-in-live-gui).
// reason=="SHUTDOWN" closes are NOT exported — they are bookkeeping snapshots
// (the position resumes from open_positions.json on restart), not real exits.
static constexpr const char* DESK_INBOUND_FILE = "data/chimera_inbound.csv";
static std::mutex g_desk_export_mtx;
static void export_desk_trade(int64_t entry_ts_ms, int64_t exit_ts_ms,
                              std::string sym, const std::string& strat,
                              const char* side, double entry_px, double exit_px,
                              double net_bp, const std::string& reason) {
    if (reason == "SHUTDOWN") return;   // bookkeeping close, position resumes
    // "suiusdt" -> "SUI"
    for (auto& c : sym) if (c >= 'a' && c <= 'z') c -= 32;
    if (sym.size() > 4 && sym.compare(sym.size() - 4, 4, "USDT") == 0)
        sym.resize(sym.size() - 4);
    const double POOL_USD = 10000.0;                 // desk pool convention
    double net_usd = net_bp * POOL_USD / 10000.0;    // 1 bp = $1 at $10k pool
    std::lock_guard<std::mutex> lk(g_desk_export_mtx);
    bool fresh = false;
    { FILE* t = fopen(DESK_INBOUND_FILE, "r"); if (!t) fresh = true; else fclose(t); }
    FILE* f = fopen(DESK_INBOUND_FILE, "a");
    if (!f) { std::fprintf(stderr, "[DESK_EXPORT] failed to open %s\n", DESK_INBOUND_FILE); return; }
    if (fresh) std::fprintf(f, "id,entry_ts,exit_ts,sym,strat,side,entry,exit,net_usd,reason\n");
    std::fprintf(f, "%lld,%lld,%lld,%s,%s,%s,%.6f,%.6f,%.2f,%s\n",
        (long long)exit_ts_ms, (long long)(entry_ts_ms / 1000), (long long)(exit_ts_ms / 1000),
        sym.c_str(), strat.c_str(), side, entry_px, exit_px, net_usd, reason.c_str());
    fclose(f);
    std::printf("[DESK_EXPORT] %s %s %s net=$%.2f (%s)\n",
        strat.c_str(), sym.c_str(), side, net_usd, reason.c_str());
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
    // CHIMERA→OMEGA DESK export (S-2026-07-12): companion clips ride to the
    // desk trades list too. net = REAL column × per-coin weight (the honest
    // worse-of fill minus RT cost, S-2026-07-07f) — same figure the desk
    // companion panel folds. STANDALONE additive book, never vs-WIDE.
    export_desk_trade(r.entry_ts_ms, r.exit_ts_ms, r.symbol, r.tag, "BUY",
                      r.entry_px, r.exit_px, r.net_bp_real * r.size_mult,
                      r.reason);
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

// ── S-2026-07-13 CAMPAIGN ARCHITECTURE (13j §2.11 task 3) ───────────────────
// CryptoCostLedger + CryptoOpportunityGate + CryptoCampaignManager: SHADOW
// virtual-lot parent campaigns on the 4 PASS cells from
// Crypto/backtest/CAMPAIGN_LEVERS_2026-07-13.md (UNI-W1/W2 fused one-campaign-
// per-symbol, TRX-W8 small tier, LDO-W8 smallest tier). Mimic lots OFF (no
// robust standalone H1 edge — revisit at tick granularity). STANDALONE
// ADDITIVE books; the SWEET/REGIME companions above are untouched instruments.
// Clips ride the same companion ledger (persist_companion_clip) + desk export.
static chimera::CryptoCostLedger      g_camp_cost_ledger;
static chimera::CryptoOpportunityGate g_camp_gate;
static std::vector<chimera::CryptoCampaignManager*> g_campaigns;  // guarded by g_companion_mtx
static int g_campaign_cell_count = 0;
static constexpr const char* CAMPAIGN_STATE_FILE = "data/campaign_state.json";
// Caller MUST hold g_companion_mtx. Atomic write (tmp+rename), one line per manager.
static void save_campaign_state() {
    if (g_campaigns.empty()) return;
    std::ostringstream js;
    for (const auto* m : g_campaigns) js << m->state_json() << "\n";
    const std::string tmp = std::string(CAMPAIGN_STATE_FILE) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) return;
    const std::string s = js.str();
    fwrite(s.c_str(), 1, s.size(), f);
    fclose(f);
    std::rename(tmp.c_str(), CAMPAIGN_STATE_FILE);
}
static void restore_campaign_state() {
    std::ifstream f(CAMPAIGN_STATE_FILE);
    if (!f) { std::printf("[CAMP-SEED] no %s — campaign books cold-start their windows honestly\n",
                          CAMPAIGN_STATE_FILE); return; }
    int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string line;
    while (std::getline(f, line)) {
        auto sp = line.find("\"sym\":\"");
        if (sp == std::string::npos) continue;
        sp += 7; auto se = line.find("\"", sp);
        std::string sym = line.substr(sp, se - sp);
        for (auto* m : g_campaigns)
            if (m->config().symbol == sym) { m->restore_state(line, now_ms); break; }
    }
}

// Live per-leg companion snapshot for the Omega desk CRYPTO COMPANIONS panel.
// Schema (S-2026-07-12 grid identity fix): {"ts":<unix>,"legs":[{sym,tag,cell,det_w,
// det_thr_pct,parent_tag,parent_w,parent_thr_pct,canonical,armed,peak_mfe_pct,
// bars_since_high,clips,bank_bp,bank_bp_real,bank_bp_real_w,mult,retired,sublegs}]}.
// Pre-fix the legs carried ONLY the truncated coin sym ("ADA-UJ2" -> "ADA"), so the 4
// grid cells per coin were indistinguishable downstream and the desk panel fell back to
// a hardcoded pre-finalize roster (stale windows, no XRP). Now each leg carries its full
// cell identity + detector window/threshold + the tuned parent's window/threshold, and
// `canonical` marks the ONE promoted cell per coin (CRYPTO_SPOTLONG_PLAN.md): the cell
// whose det_thr equals its tuned parent's upjump_thr (ETH-UJ2/BTC-UJ4/BNB-UJ3/SOL-UJ5/
// DOGE-UJ4/ADA-UJ5/XRP-UJ4/TRX-UJ5) — data-driven, no second hand-kept list to rot.
// bank_bp_real stays per-leg (the desk _cctot fold sums it over ALL legs — shape kept).
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
        const auto& ccfg = kv.second.second->config();
        const auto& pcfg = kv.second.first->cfg();
        std::string sym  = kv.first.substr(0, kv.first.find('-'));  // "BTC-UJ4" -> "BTC"
        std::string cell = kv.first.find('-') == std::string::npos
                             ? "" : kv.first.substr(kv.first.find('-') + 1);  // "BTC-UJ4" -> "UJ4"
        // canonical = this cell's detector threshold IS the coin's tuned parent threshold
        const bool canonical = std::fabs(ccfg.det_thr - pcfg.upjump_thr) < 1e-9;
        if (!first) js << ",";
        first = false;
        js << "{\"sym\":\"" << sym << "\",\"tag\":\"" << ccfg.tag << "\",\"cell\":\"" << cell << "\""
           << ",\"det_w\":" << ccfg.det_w
           << std::setprecision(1) << ",\"det_thr_pct\":" << ccfg.det_thr * 100.0
           << ",\"parent_tag\":\"" << pcfg.tag << "\",\"parent_w\":" << pcfg.upjump_w
           << ",\"parent_thr_pct\":" << pcfg.upjump_thr * 100.0
           << ",\"canonical\":" << (canonical ? "true" : "false")
           << ",\"armed\":" << (snap.armed ? "true" : "false")
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
    // S-2026-07-13 campaign books (13j §2.11): same leg field contract as the
    // companions so the desk panel renders them with zero GUI changes.
    // parent_tag=CAMPAIGN-SELF marks the class; armed = parent stop protected
    // at-or-above entry; sublegs carries the ONE open parent lot (if any).
    for (const auto* mgr : g_campaigns) {
        for (const auto& s : mgr->snapshots()) {
            if (!first) js << ",";
            first = false;
            js << "{\"sym\":\"" << s.sym << "\",\"tag\":\"" << s.tag << "\",\"cell\":\"" << s.cell << "\""
               << ",\"det_w\":" << s.det_w
               << std::setprecision(1) << ",\"det_thr_pct\":" << s.det_thr * 100.0
               << ",\"parent_tag\":\"CAMPAIGN-SELF\",\"parent_w\":" << s.det_w
               << ",\"parent_thr_pct\":" << s.det_thr * 100.0
               << ",\"canonical\":true"
               << ",\"armed\":" << (s.armed ? "true" : "false")
               << std::setprecision(4) << ",\"peak_mfe_pct\":" << s.peak_mfe_pct
               << ",\"bars_since_high\":" << s.bars_since_high
               << ",\"clips\":" << s.clips
               << std::setprecision(2) << ",\"bank_bp\":" << s.bank_bp
               << ",\"bank_bp_real\":" << s.bank_bp_real
               << ",\"bank_bp_real_w\":" << s.bank_bp_real_w
               << ",\"mult\":" << s.size_mult
               << ",\"retired\":" << (s.retired ? "true" : "false")
               << ",\"sublegs\":[";
            if (s.open)
                js << "{\"id\":\"P\",\"armed\":" << (s.armed ? "true" : "false")
                   << std::setprecision(4) << ",\"peak_mfe_pct\":" << s.peak_mfe_pct
                   << ",\"bars_since_high\":" << s.bars_since_high << "}";
            js << "]}";
        }
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
    // Phase-4 item 20: honest connected-engine count + registry from the REAL
    // reconciled graph (not a hardcoded banner). connected_engines supersedes the
    // legacy engine_count (which is g_all_wired and reads 0 with the legacy layer culled).
    js << "\"connected_engines\":" << g_registry.connected_count() << ",";
    js << "\"engine_registry\":" << g_registry.state_json() << ",";
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
    // Phase-1 review: bind to localhost by default (was INADDR_ANY = public IP).
    // Set CHIMERA_HTTP_BIND=0.0.0.0 (or a specific IP) to override; the mutating
    // endpoints remain token-guarded regardless. nginx proxies to 127.0.0.1:8080.
    if (g_http_bind_addr == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else if (inet_pton(AF_INET, g_http_bind_addr.c_str(), &addr.sin_addr) != 1) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // safe fallback
    }
    addr.sin_port        = htons(port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }
    listen(server_fd, 16);
    std::printf("[HTTP] GUI on %s:%d | root: %s | control-auth=%s\n",
                g_http_bind_addr.c_str(), port, gui_root.c_str(),
                g_ctrl_token.empty() ? "DENY-ALL(no token)" : "token");
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

        // Phase-1 review: guard the state-mutating control endpoints
        // (kill / ratchet_reset / daily_kill_clear / session_reset). They must
        // be POST and carry the control token (X-Auth-Token header or ?token=).
        bool req_handled = false;
        if (chimera::http_is_mutating_control(req)) {
            if (!chimera::http_request_is_post(req)) {
                status = 405; body = "{\"error\":\"method not allowed — use POST\"}";
                req_handled = true;
            } else if (!chimera::http_control_authorized(req, g_ctrl_token)) {
                status = 401; body = "{\"error\":\"unauthorized — missing/invalid control token\"}";
                std::fprintf(stderr, "[HTTP] REJECTED unauthenticated control request\n");
                req_handled = true;
            }
        }

        if (req_handled) {
            /* security pre-check already set status + body */
        } else if (strstr(req, "GET /api/bars/")) {
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
// Phase-8A Stage-2: allocator enforcement stage (config "portfolio_alloc_mode").
enum class AllocMode { OFF, HARDCAP, FULL };

struct LiveRuntimeConfig {
    std::string credentials_file = "config/binance_credentials.json";
    bool        shadow_mode      = true;
    double      max_position_usd = 10000.0;
    double      min_edge_bps     = 10.0;
    // Phase-2 review (2026-07-11): total portfolio cash the ExchangeLedger
    // reserves against. >0 => cash reservation ENFORCES (rejects/resizes the
    // cross-sleeve overbook). 0 (default) => track-only: the ledger records
    // position/attribution/cash but never blocks, preserving the existing SHADOW
    // research record until the operator opts in.
    double      portfolio_cash_usd = 0.0;
    // Phase-3 review (2026-07-11): SpotPortfolioAllocator enforcement. false
    // (default) => TRACK-ONLY: the allocator merges/caps/nets every sleeve's
    // target and LOGS the unified vector ([ALLOC-TRACK]) so the whole layer is
    // exercised, but does NOT emit — the per-sleeve shadow books + the 32-cell
    // UpJump threshold GRID keep their own records. true => ENFORCE: the allocator
    // emits the netted deltas and the raw per-sleeve orders defer to it.
    // (Back-compat only — superseded by portfolio_alloc_mode below.)
    bool        portfolio_alloc_enforce = false;
    // Phase-8A Stage-2 (2026-07-11): three-way allocator enforcement stage.
    //   off     (default) => TRACK-ONLY, identical to portfolio_alloc_enforce=false
    //   hardcap           => Stage-2 SAFETY CAPS: engines still propose qty, but the
    //                        allocator can REDUCE/REJECT a BUY that breaches the
    //                        symbol / momentum-factor / drawdown caps. In-limit
    //                        orders pass byte-identical to track-only.
    //   full              => Stage-3+ plan()-emit (== portfolio_alloc_enforce=true)
    // If the string key is absent, falls back to the bool above.
    AllocMode   portfolio_alloc_mode = AllocMode::OFF;
    // S44d: testing-mode protection bypass. When TRUE + shadow_mode=true,
    // the portfolio ratchet does NOT halt entries (lets testing flow
    // through). Going live (shadow_mode=false) with this set = FATAL abort.
    bool        protection_disabled_for_testing = false;
    // Phase-1 review (2026-07-11): optional explicit "mode" string
    // (disabled|shadow|paper|live). Cross-checked against shadow_mode + the
    // credentials file at startup; a disagreement is a HARD abort.
    bool               mode_key_present = false;
    chimera::RuntimeMode mode_key       = chimera::RuntimeMode::SHADOW;
    // Phase-1 review: control-API hardening. bind defaults to localhost; a
    // token (env CHIMERA_CTRL_TOKEN overrides) guards the mutating endpoints.
    std::string http_bind     = "127.0.0.1";
    std::string control_token = "";
    // Phase-8G review (2026-07-11): user-data-stream heartbeat-lapse AUTO-HALT
    // threshold (ms). If the LIVE stream goes silent longer than this, new
    // entries auto-halt until a clean reconcile (exits always pass). Inert in
    // shadow (no live stream to lapse). Default 45s.
    double      user_stream_halt_ms = 45000.0;
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
    cfg.portfolio_cash_usd = lrc_extract_double(content, "portfolio_cash_usd", 0.0);
    cfg.portfolio_alloc_enforce = lrc_extract_bool(content, "portfolio_alloc_enforce", false);
    // Phase-8A: three-way mode string wins if present; else fall back to the bool.
    {
        std::string m = lrc_extract_string(content, "portfolio_alloc_mode");
        for (auto& ch : m) if (ch >= 'A' && ch <= 'Z') ch += 32;
        if      (m == "hardcap")               cfg.portfolio_alloc_mode = AllocMode::HARDCAP;
        else if (m == "full" || m == "enforce") cfg.portfolio_alloc_mode = AllocMode::FULL;
        else if (m == "off" || m == "track")   cfg.portfolio_alloc_mode = AllocMode::OFF;
        else cfg.portfolio_alloc_mode = cfg.portfolio_alloc_enforce ? AllocMode::FULL : AllocMode::OFF;
    }
    cfg.protection_disabled_for_testing =
        lrc_extract_bool(content, "protection_disabled_for_testing", false);
    // Phase-1: optional explicit runtime mode + control-API bind/token.
    {
        std::string mkey = lrc_extract_string(content, "mode");
        if (!mkey.empty()) {
            chimera::RuntimeMode mk;
            if (!chimera::parse_runtime_mode(mkey, mk)) {
                std::fprintf(stderr,
                    "[STARTUP] FATAL: live_config.json mode=\"%s\" is not one of "
                    "disabled|shadow|paper|live\n", mkey.c_str());
                std::exit(1);
            }
            cfg.mode_key_present = true;
            cfg.mode_key = mk;
        }
        std::string hb = lrc_extract_string(content, "http_bind");
        if (!hb.empty()) cfg.http_bind = hb;
        std::string ct = lrc_extract_string(content, "control_token");
        if (!ct.empty()) cfg.control_token = ct;
    }
    // Phase-8G: user-data-stream heartbeat-lapse auto-halt threshold (ms).
    cfg.user_stream_halt_ms = lrc_extract_double(content, "user_stream_halt_ms", 45000.0);
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
    std::printf("[MACRO] init: BTC 200d-MA=%.0f last_close=%.0f -> %s "
                "(TELEMETRY ONLY — gates no slot entry; NO-200DMA rule, S-2026-07-11)\n",
                ma, last_close, last_close > ma ? "BULL" : "BEAR");
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

    // ── Phase-1 review (2026-07-11): resolve ONE immutable runtime mode and
    // cross-check every mode-bearing config file. A disagreement is a HARD abort
    // (previously the mode was spread across live_config.json + the credentials
    // file with no consistency check — a mismatch could route real orders while
    // the system believed it was in shadow).
    auto _mode_res = chimera::resolve_runtime_mode(runtime_cfg.shadow_mode,
                                                   runtime_cfg.mode_key_present,
                                                   runtime_cfg.mode_key);
    if (!_mode_res.ok) {
        std::fprintf(stderr,
            "\n[STARTUP] FATAL mode conflict (live_config.json): %s\n"
            "Refusing to start.\n\n", _mode_res.error.c_str());
        std::exit(1);
    }
    if (exec_ok) {
        auto _xchk = chimera::cross_check_credentials_shadow(_mode_res.mode,
                                                             executor.is_shadow());
        if (!_xchk.ok) {
            std::fprintf(stderr,
                "\n[STARTUP] FATAL mode conflict (config files disagree): %s\n"
                "Refusing to start.\n\n", _xchk.error.c_str());
            std::exit(1);
        }
    }
    const chimera::RuntimeMode g_runtime_mode = _mode_res.mode;
    std::printf("[STARTUP] RUNTIME MODE = %s (live_config.shadow_mode=%d creds_shadow=%d)\n",
                chimera::runtime_mode_str(g_runtime_mode),
                runtime_cfg.shadow_mode ? 1 : 0,
                exec_ok ? (executor.is_shadow() ? 1 : 0) : -1);
    std::fflush(stdout);

    // ── Phase-1 review: THE single order chokepoint. Every strategy order goes
    // through gateway.submit(); SpotExecutor::execute() is private + befriends the
    // gateway, so a direct executor.execute() from strategy code will not compile.
    // The gateway applies mode + kill-switch (daily-loss / emergency halt) +
    // exchange filters; risk-reducing EXITS are never blocked.
    chimera::ExecutionGatewayT<chimera::SpotExecutor> gateway(executor, g_runtime_mode);

    // ════════════════════════════════════════════════════════════════════
    // Phase-2 review (2026-07-11) — EXCHANGE TRUTH. Attach the authoritative
    // ledger + exchange filters + clock-sync + deterministic-id registry +
    // user-data-stream path to the ONE gateway, so every order the sleeves
    // already route through submit() now: reserves cash before it books,
    // normalizes to exchange filters, and drives the ledger from the resulting
    // execution report (holdings from truth, not intent). SHADOW: the shadow
    // fill's OrderResult IS the report, so the exact live code path is exercised.
    // ════════════════════════════════════════════════════════════════════
    static chimera::ExchangeLedger   g_ledger;
    static chimera::ExchangeFilters  g_filters;
    static chimera::ExchangeTimeSync g_clock;
    static chimera::OrderIdRegistry  g_idreg;
    static chimera::UserDataStream   g_userstream;
    static chimera::UserStreamHaltGuard g_stream_halt;   // Phase-8G auto-halt latch
    {
        // Cash: portfolio_cash_usd>0 => ENFORCE reservation (rejects/resizes the
        // cross-sleeve overbook); 0 (default) => track-only, preserving the SHADOW
        // research record. LIVE deploys seed this from the real USDT balance.
        double seed_cash = runtime_cfg.portfolio_cash_usd;
        bool enforce = runtime_cfg.portfolio_cash_usd > 0.0;
        g_ledger.configure(seed_cash, enforce, /*fee*/0.001);

        // The user-data stream drives the ledger from every execution report.
        // LIVE: real executionReport events. SHADOW: the simulated fill fed by the
        // gateway. Either way the same handler updates the one ledger.
        g_userstream.set_handler([](const chimera::ExecReport& r){ g_ledger.apply_report(r); });
        g_userstream.set_shadow_driven(runtime_cfg.shadow_mode);

        // Phase-8G: user-stream heartbeat-lapse AUTO-HALT threshold. Inert in
        // shadow (the stream is shadow-driven => never lapses); arms on the live
        // path the instant the real WS user-stream goes silent past this gap.
        g_stream_halt.set_threshold_ms((int64_t)runtime_cfg.user_stream_halt_ms);

        // Clock (item 6): sync to Binance server time via the public /time probe
        // (works in shadow too). The gateway only HALTS signed (LIVE) orders on
        // drift; in shadow it never blocks. Fall back to synced-0 if the probe
        // fails so the shadow path stays open.
        g_clock.set_threshold_ms(1000);
        if (exec_ok) {
            int64_t local0 = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            int64_t srv = executor.server_time();
            if (srv > 0) { g_clock.record_pair(srv, local0);
                std::printf("[CLOCK] Binance server-time synced: offset=%lldms\n",
                            (long long)g_clock.offset_ms()); }
            else         { g_clock.record_offset(0);
                std::fprintf(stderr, "[CLOCK] server-time probe failed — assuming synced (shadow-safe)\n"); }
        } else {
            g_clock.record_offset(0);
        }

        // Filters (item 5): cache real exchangeInfo LOT_SIZE/MIN_NOTIONAL/step so
        // every order is normalized before submit. Public endpoint — loaded in
        // shadow too; a failure leaves the cache empty (gateway passes through).
        if (exec_ok) {
            std::string info = executor.exchange_info();
            int nf = info.empty() ? 0 : g_filters.load_from_json(info);
            std::printf("[FILTERS] exchangeInfo cached: %d symbols%s\n",
                        nf, info.empty() ? " (probe failed — pass-through)" : "");
            std::fflush(stdout);
        }

        gateway.set_ledger(&g_ledger);
        gateway.set_filters(&g_filters);   // empty until exchangeInfo is loaded (pass-through)
        gateway.set_clock(&g_clock);
        gateway.set_id_registry(&g_idreg);
        gateway.set_stream(&g_userstream);

        // Phase-4 item 22: attach the ADDITIVE realistic-fill observer. Every
        // gateway-routed fill (XSec / RipRider / UpJump-parent — NOT the grid
        // companions, which never route here) is mirrored into a PARALLEL book:
        // one leg at signal price (= current record), one leg at a realistic
        // price (spread+slippage+fee+queue). The signal-price shadow ledger and
        // the 32-cell grid are untouched — this is a side metric the operator
        // compares. realistic_pnl <= signal_pnl by construction.
        gateway.on_fill_observer = [](const chimera::OrderIntent& in, double filled_qty, double ref_px){
            if (filled_qty <= 0.0 || ref_px <= 0.0) return;
            g_fill_realism.on_fill(in.source ? in.source : "?", in.symbol,
                                   in.is_buy, ref_px, filled_qty);
        };

        // Startup reconciliation (item 9): the ledger must agree with the exchange
        // before trading. SHADOW clean-boot => empty snapshot reconciles trivially;
        // the hard-block on mismatch/fetch-failure is proven by the regression test
        // and ACTIVATES LIVE once a full account snapshot is wired to the reconciler.
        chimera::StartupReconciler reconciler;
        chimera::ExchangeSnapshot snap; snap.ok = true;   // clean shadow boot: no working orders
        auto rec = reconciler.reconcile(snap, g_ledger, &g_idreg,
            (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        std::printf("[LEDGER] Phase-2 exchange-truth wired: cash=$%.2f enforce=%d | reconcile: %s (%s)\n",
                    seed_cash, enforce ? 1 : 0, rec.passed ? "PASS" : "BLOCK", rec.detail.c_str());
        std::fflush(stdout);
        // Phase-8G: a clean boot reconcile also clears any pre-armed auto-halt
        // (defensive; latch starts clear). The live periodic reconcile below is
        // what auto-clears an in-session heartbeat-lapse halt.
        g_stream_halt.on_reconcile(rec.passed);
    }

    // ── Phase-8G (2026-07-11): USER-STREAM HEARTBEAT AUTO-HALT (go-live blocker).
    // Wire the auto-halt latch into the ONE gateway kill-switch — REUSING the
    // existing entry chokepoint (entries blocked while true; EXITS always pass).
    // The lambda POLLS the latch against the live user-stream every time entries
    // are gated: if the live WS heartbeat has lapsed > threshold, g_stream_halt
    // latches ON and new entries halt until a clean reconcile clears it. In
    // shadow the stream is shadow-driven, heartbeat_lapsed() is always false, so
    // poll() never arms and this is a pure no-op (identical to prior behaviour).
    gateway.kill_switch_active = []() -> bool {
        int64_t now = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        bool stream_halted = g_stream_halt.poll(g_userstream, now);   // AUTO-arm (no-op in shadow)
        // Do NOT honour the testing bypass here: the gateway is the last line of
        // defence for the XSec/RipRider sleeves that bypassed ALL risk before.
        return stream_halted
            || now < g_daily_kill_until_ms.load(std::memory_order_relaxed)
            || now < g_emergency_halt_until_ms.load(std::memory_order_relaxed);
    };

    // ════════════════════════════════════════════════════════════════════
    // Phase-3 review (2026-07-11) — PORTFOLIO UNIFICATION. "Strategies produce
    // TARGETS, not orders." Each production sleeve DECLARES a desired (symbol,
    // target_usd, factor, family) with the allocator; the allocator MERGES the
    // overlapping per-symbol targets, applies the FAMILY regime exposure + global
    // drawdown scale + AGGREGATE momentum-factor cap + portfolio vol/cluster/beta
    // risk scale, caps ONCE at the symbol level, nets the delta vs the exchange-
    // truth ledger (actual + pending), and would emit the netted order.
    //
    // TRACK-ONLY by default (portfolio_alloc_enforce=false): the allocator COMPUTES
    // + LOGS the unified vector ([ALLOC-TRACK]) so the whole layer is exercised on
    // live targets, but does NOT emit — the per-sleeve shadow books and the 32-cell
    // UpJump threshold GRID keep their own records untouched (the grid cells never
    // register a target, so they are preserved by construction). Set enforce=true
    // (go-live) to make the netted deltas the real orders. The allocator NEVER
    // edits a validated sleeve's signal/exit logic.
    // ════════════════════════════════════════════════════════════════════
    static chimera::SpotPortfolioAllocator g_allocator;
    static std::mutex                      g_alloc_mtx;
    {
        double nav = runtime_cfg.max_position_usd > 0.0 ? runtime_cfg.max_position_usd : 10000.0;
        // Research starting caps (track-only; operator tunes before enforce). One
        // symbol cap = 1 NAV unit; aggregate momentum cap = 4 NAV units (XSec +
        // UpJump + RipRider are ONE factor); vol/beta OFF until the rolling
        // covariance warms (apply_risk returns 1.0 cold); cluster cap 50%.
        chimera::EnforceMode emode =
            runtime_cfg.portfolio_alloc_mode == AllocMode::FULL    ? chimera::EnforceMode::FULL :
            runtime_cfg.portfolio_alloc_mode == AllocMode::HARDCAP ? chimera::EnforceMode::HARDCAP :
                                                                     chimera::EnforceMode::OFF;
        g_allocator.configure(/*enforce(FULL)*/emode == chimera::EnforceMode::FULL,
                              /*symbol_cap*/nav, /*momentum_cap*/4.0*nav,
                              /*target_vol*/0.0, /*cluster_frac*/0.50, /*beta*/0.0);
        g_allocator.set_enforce_mode(emode);   // Phase-8A: OFF | HARDCAP | FULL
        g_allocator.regime().configure(/*hysteresis*/0.05);
        g_allocator.drawdown().configure();
        g_allocator.risk().configure(/*window*/30, /*shrink*/0.30);
        g_allocator.ref_px = [](const std::string& usym) -> double {
            std::string lc = usym; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
            int sid = chimera::sym_id(lc); if (sid < 0) return 0.0;
            return load_dbl_atomic(g_last_spot_px_bits[sid]);
        };
        g_allocator.cluster_of = [](const std::string& usym) -> int {
            std::string lc = usym; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
            return symbol_cluster(chimera::symbol_to_id(lc));
        };
        // ENFORCE sink (never called in track-only): route the netted delta through
        // the ONE Phase-1/2 gateway (mode + kill-switch + filters + cash + ledger).
        g_allocator.emit = [&gateway](const chimera::AllocDelta& d, chimera::Factor, chimera::Family){
            gateway.submit({ d.symbol, d.is_buy, d.qty, d.usd>0.0 ? d.usd/d.qty : 0.0,
                             /*is_exit*/ !d.is_buy, "ALLOC" });
        };
        std::printf("[ALLOC] Phase-3/8A portfolio allocator wired: mode=%s | symbol_cap=$%.0f "
                    "momentum_cap=$%.0f cluster<=50%% (regime+DD+factor overlays ON)%s\n",
                    chimera::enforce_mode_str(emode), nav, 4.0*nav,
                    emode == chimera::EnforceMode::HARDCAP
                        ? " [Stage-2: reduce/reject over-cap BUYs; in-limit unchanged; grid+shadow preserved]"
                        : emode == chimera::EnforceMode::FULL ? " [Stage-3+: plan() emits]"
                        : " (TRACK-ONLY: shadow record + grid preserved)");
        std::fflush(stdout);
    }

    // ── Phase-8A Stage-2: governed submit for the PROMOTED sleeves only ──────────
    // The books that feed the allocator (XSec v1 BTC/BR, XSec2, RipRider, the
    // EdgeEngine/UpJump intent path) route their BUY entries through here. In
    // OFF/FULL mode, or for any exit/sell, this is a straight passthrough —
    // byte-identical to the pre-8A gateway.submit — so track-only behaviour is
    // preserved. In HARDCAP it asks the allocator whether the proposed BUY breaches
    // a hard cap: an in-limit order passes unchanged; a genuine breach is REDUCED to
    // the headroom or REJECTED. Observation books (P6/P7) and the 32-cell grid do
    // NOT call this and are wholly unaffected. Cash is not enforced here (shadow).
    auto governed_submit = [&](const chimera::OrderIntent& in, chimera::Factor factor)
            -> chimera::OrderResult {
        if (runtime_cfg.portfolio_alloc_mode != AllocMode::HARDCAP || !in.is_buy || in.is_exit)
            return gateway.submit(in);
        chimera::CapDecision cd;
        {
            std::lock_guard<std::mutex> lk(g_alloc_mtx);
            cd = g_allocator.govern_entry(in.symbol, in.qty, in.ref_px, factor, &g_ledger);
        }
        if (!cd.approved) {
            std::fprintf(stderr, "[ALLOC-HARDCAP] REJECT src=%s %s proposed=$%.2f — %s\n",
                         in.source ? in.source : "?", in.symbol.c_str(), cd.proposed_usd, cd.reason);
            chimera::OrderResult r; r.error = std::string("hardcap:") + cd.reason; return r;
        }
        if (cd.reduced) {
            std::fprintf(stderr,
                "[ALLOC-HARDCAP] REDUCE src=%s %s proposed=$%.2f -> $%.2f (qty %.8f -> %.8f) — %s\n",
                in.source ? in.source : "?", in.symbol.c_str(), cd.proposed_usd,
                cd.approved_usd, in.qty, cd.approved_qty, cd.reason);
            chimera::OrderIntent adj = in; adj.qty = cd.approved_qty;
            return gateway.submit(adj);
        }
        return gateway.submit(in);   // in-limit: byte-identical to track-only
    };
    (void)governed_submit;   // referenced by the promoted-sleeve callbacks below

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
    // ── Phase-5 (S-2026-07-11): XSec 2.0 — a SEPARATE SHADOW comparison book ──
    // Runs ALONGSIDE v1 (does NOT replace it) so the operator sees v1 vs v2.0
    // forward. Composite score (vol-adj 7/30/90d + accel − liq/corr penalty),
    // adaptive rebalance + hysteresis, core+challenger sizing, POINT-IN-TIME
    // dynamic universe. Regime gate = BREADTH (participation ratio), NO 200DMA.
    // BACKTEST verdict (backtest/xsec2_bt.cpp; long-only spot SITS OUT bears, so
    // 2022 is SHOWN-not-gated per standing rule feedback-crypto-omit-2022-longonly):
    // EX-2022 v2.0 +3354% Sh1.28 maxDD53% is a risk-adjusted DEAD HEAT with v1
    // (Sh1.29 maxDD57%; v1 higher raw net +4112%). Clears the corrected gate — WF
    // both halves +, 2×-cost +, broad plateau, beats 95% random, point-in-time.
    // KEEP as a complementary DIVERSIFIER: 50/50 v1+v2 Sh1.51 (>v1 1.29), DD 47%,
    // corr 0.44. Both sleeves run via the allocator (ONE MOMENTUM/XSEC factor —
    // aggregate momentum cap bounds the combined risk). See [[ChimeraReviewPhase5]].
    chimera::XSec2Config xsec2_cfg;
    chimera::CrossSectionalMomentum2Engine xsec2(xsec2_cfg);
    {
        std::vector<std::string> xu = {
            "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
            "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
            "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"};
        xsec_btc.set_universe(xu); xsec_br.set_universe(xu);
        // Phase-4 item 23: structural data-quality gate for the committed daily
        // warm-seed CSVs. Refuse a CORRUPTED seed (invalid price / duplicate /
        // out-of-order timestamp / bad schema) — do NOT seed the engines on it.
        // Staleness + gap rejection are OFF: these files are static history that
        // may legitimately be old and span exchange-downtime gaps.
        g_dq_gate.configure(/*step*/ 86400000LL, /*max_stale*/ 0, /*schema*/ 1);
        g_dq_gate.set_reject_on_gap(false);
        int xseeded = 0, xrejected = 0;
        for (const auto& s : xu) {
            std::string xpath = "data/xsec_seed/" + s + "USDT_1d.csv";
            {
                auto dq = g_dq_gate.validate_csv(xpath, /*now*/ 0);
                if (dq.rows > 0 && !dq.ok) {
                    std::printf("[DATA-QUALITY] REFUSE seed %s — %s (rows=%d dup=%d ooo=%d invalid=%d)\n",
                                xpath.c_str(), dq.reason.c_str(), dq.rows,
                                dq.duplicates, dq.out_of_order, dq.invalid);
                    ++xrejected; continue;   // refuse to seed on malformed history
                }
            }
            std::ifstream xf(xpath);
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
        if (xrejected > 0)
            std::printf("[DATA-QUALITY] XSEC seed: %d symbol(s) REFUSED for corrupt history\n", xrejected);
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
        auto fire = [&exec_ok, &governed_submit](const char* tag, double sleeve_nav,
                    std::map<std::string,double>& hold, int64_t day,
                    const std::map<std::string,double>& tw, bool bull,
                    double breadth, double dispersion) {
            std::string picks; for (const auto& kv : tw) if (kv.second > 0.0) { char b[48];
                std::snprintf(b, sizeof b, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); picks += b; }
            std::printf("[%s] REBALANCE day=%lld bull=%d -> %s\n", tag, (long long)day, bull?1:0,
                        bull ? (picks.empty()?"(none)":picks.c_str()) : "CASH (gate)"); std::fflush(stdout);
            if (!exec_ok) return;
            for (const auto& kv : tw) {
                std::string lc = kv.first; for (auto& ch : lc) if (ch >= 'A' && ch <= 'Z') ch += 32;
                int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                double tgt = kv.second * sleeve_nav, dusd = tgt - hold[kv.first];
                if (std::fabs(dusd) < 25.0) continue;
                // Phase-1: route through the gateway. A rebalance SELL (dusd<0) is a
                // risk-reducing exit -> never blocked by halts.
                // Phase-8A: BUYs pass through the hard-cap governor (MOMENTUM/XSEC);
                // in-limit unchanged, over-cap reduced/rejected. SELLs pass through.
                governed_submit({ kv.first + "USDT", dusd > 0.0, std::fabs(dusd)/px, px,
                                 /*is_exit*/ dusd < 0.0, tag }, chimera::Factor::MOMENTUM);
                hold[kv.first] = tgt;
                // DESK export (S-2026-07-12): rebalance leg = notional shift,
                // not a realized round-trip -> net 0.00; the $ delta rides in
                // the reason column so the desk row is honest, not fake pnl.
                {
                    int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    char why[48];
                    std::snprintf(why, sizeof why, "REBAL%s$%.0f", dusd > 0 ? "+" : "-", std::fabs(dusd));
                    export_desk_trade(now_ms, now_ms, kv.first, tag,
                                      dusd > 0.0 ? "BUY" : "SELL", px, px, 0.0, why);
                }
            }
            // Phase-3 TRACK-ONLY: declare this sleeve's DESIRED per-symbol targets to
            // the portfolio allocator and log the merged/capped/netted unified vector.
            // Does NOT alter the gateway.submit above (shadow book preserved).
            {
                std::lock_guard<std::mutex> lk(g_alloc_mtx);
                g_allocator.set_regime_inputs(breadth, dispersion, /*severe*/false);
                for (const auto& kv : tw) {
                    std::string usym = kv.first + "USDT";
                    double tgt = bull ? kv.second * sleeve_nav : 0.0;
                    g_allocator.set_target(tag, usym, tgt, chimera::Factor::MOMENTUM,
                                           chimera::Family::XSEC);
                }
                g_allocator.plan(&g_ledger);
            }
        };
        xsec_btc.set_rebalance_callback([fire, nav_btc, &xsec_btc](int64_t d, const std::map<std::string,double>& tw, bool b){
            static std::map<std::string,double> hold;
            fire("XSEC-BTC", nav_btc, hold, d, tw, b, xsec_btc.breadth_latest(), xsec_btc.dispersion_latest()); });
        xsec_br.set_rebalance_callback([fire, nav_br, &xsec_br](int64_t d, const std::map<std::string,double>& tw, bool b){
            static std::map<std::string,double> hold;
            fire("XSEC-BR", nav_br, hold, d, tw, b, xsec_br.breadth_latest(), xsec_br.dispersion_latest()); });
    }

    // ── Phase-5: XSec 2.0 SHADOW comparison book (own tag XSEC2, own NAV) ─────
    // Seeds close + DOLLAR-VOLUME (close*base_vol) from the SAME daily CSVs (its
    // point-in-time universe eligibility needs the volume). Its callback logs
    // [XSEC2] REBALANCE, mirrors a SHADOW order per target (tag XSEC2 -> its own
    // shadow ledger, for the v1-vs-v2 forward comparison) AND declares targets to
    // the portfolio allocator TRACK-ONLY (one MOMENTUM/XSEC factor with v1). The
    // 32-cell UpJump grid + the existing shadow record are untouched (new tag).
    xsec2.set_universe({
        "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
        "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
        "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"});
    {
        std::vector<std::string> xu = {
            "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
            "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
            "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"};
        int x2seeded = 0, x2rejected = 0;
        for (const auto& s : xu) {
            std::string xpath = "data/xsec_seed/" + s + "USDT_1d.csv";
            {   // same Phase-4 data-quality gate as v1
                auto dq = g_dq_gate.validate_csv(xpath, /*now*/ 0);
                if (dq.rows > 0 && !dq.ok) { std::printf("[DATA-QUALITY] REFUSE XSEC2 seed %s — %s\n",
                        xpath.c_str(), dq.reason.c_str()); ++x2rejected; continue; }
            }
            std::ifstream xf(xpath); if (!xf) continue;
            std::string xl; std::getline(xf, xl); int xr = 0;
            while (std::getline(xf, xl)) {
                char* p = xl.data(); char* e; long long ts = strtoll(p, &e, 10); if (*e != ',') continue;
                const char* q = e + 1;
                for (int k = 0; k < 3 && q; ++k) { q = std::strchr(q, ','); if (q) ++q; }
                if (!q) continue; char* e2; double c = strtod(q, &e2);        // col4 = close
                double vol = 0.0; if (*e2 == ',') vol = strtod(e2 + 1, nullptr); // col5 = base volume
                if (c > 0) { long long d = ts / 86400000LL;
                    xsec2.seed_daily(s, d, c, c * vol); ++xr; }
            }
            if (xr > 0) ++x2seeded;
        }
        double nav2 = (runtime_cfg.max_position_usd > 0.0 ? runtime_cfg.max_position_usd : 10000.0);
        std::printf("[XSEC2] Phase-5 shadow book installed: warm-seeded %d/%zu symbols, %zu days; "
                    "composite/adaptive/core+challenger, BREADTH-gate (NO 200DMA) nav=$%.0f SHADOW (allocator TRACK-ONLY)\n",
                    x2seeded, xu.size(), xsec2.num_days(), nav2);
        if (x2rejected > 0) std::printf("[DATA-QUALITY] XSEC2 seed: %d symbol(s) REFUSED for corrupt history\n", x2rejected);
        { size_t i2 = xsec2.num_days()?xsec2.num_days()-1:0; bool b2;
          auto w2 = xsec2.compute_target_weights(i2, b2);
          std::string p; for (auto& kv : w2) if (kv.second > 0) { char z[48];
              std::snprintf(z, sizeof z, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); p += z; }
          std::printf("[XSEC2] startup: bull=%d breadth=%.2f -> %s\n",
                      b2?1:0, xsec2.breadth_latest(), b2 ? (p.empty()?"(none)":p.c_str()) : "CASH (breadth gate)"); }
        std::fflush(stdout);
        xsec2.set_rebalance_callback([&gateway, &exec_ok, nav2, &xsec2, &governed_submit](int64_t day,
                                     const std::map<std::string,double>& tw, bool bull){
            static std::map<std::string,double> hold;
            std::string picks; for (auto& kv : tw) if (kv.second > 0) { char b[48];
                std::snprintf(b, sizeof b, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); picks += b; }
            std::printf("[XSEC2] REBALANCE day=%lld bull=%d -> %s\n", (long long)day, bull?1:0,
                        bull ? (picks.empty()?"(none)":picks.c_str()) : "CASH (breadth gate)"); std::fflush(stdout);
            if (exec_ok) for (auto& kv : tw) {
                std::string lc = kv.first; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
                int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                double tgt = kv.second * nav2, dusd = tgt - hold[kv.first];
                if (std::fabs(dusd) < 25.0) continue;
                // Phase-8A: governed BUY (MOMENTUM/XSEC); SELL passes through.
                governed_submit({ kv.first + "USDT", dusd > 0.0, std::fabs(dusd)/px, px, /*is_exit*/ dusd < 0.0, "XSEC2" }, chimera::Factor::MOMENTUM);
                hold[kv.first] = tgt;
                // DESK export (S-2026-07-12): rebalance leg (see XSEC fire()).
                {
                    int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    char why[48];
                    std::snprintf(why, sizeof why, "REBAL%s$%.0f", dusd > 0 ? "+" : "-", std::fabs(dusd));
                    export_desk_trade(now_ms, now_ms, kv.first, "XSEC2",
                                      dusd > 0.0 ? "BUY" : "SELL", px, px, 0.0, why);
                }
            }
            // also zero-out held names no longer targeted (rebalance to CASH/out)
            if (exec_ok) for (auto& kv : hold) if (kv.second > 0 && !tw.count(kv.first)) {
                std::string lc = kv.first; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
                int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                gateway.submit({ kv.first + "USDT", false, (kv.second)/px, px, /*is_exit*/ true, "XSEC2" });
                // DESK export (S-2026-07-12): full exit of a dropped name.
                {
                    int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    char why[48];
                    std::snprintf(why, sizeof why, "REBAL-$%.0f", kv.second);
                    export_desk_trade(now_ms, now_ms, kv.first, "XSEC2",
                                      "SELL", px, px, 0.0, why);
                }
                kv.second = 0.0;
            }
            // Phase-3 TRACK-ONLY: declare XSEC2 targets to the allocator (one
            // MOMENTUM/XSEC factor with v1). Does NOT emit (shadow record preserved).
            {
                std::lock_guard<std::mutex> lk(g_alloc_mtx);
                g_allocator.set_regime_inputs(xsec2.breadth_latest(), xsec2.dispersion_latest(), /*severe*/false);
                for (auto& kv : tw) g_allocator.set_target("XSEC2", kv.first + "USDT",
                        bull ? kv.second * nav2 : 0.0, chimera::Factor::MOMENTUM, chimera::Family::XSEC);
                g_allocator.plan(&g_ledger);
            }
        });
    }

    // ── Phase-6 (S-2026-07-11): NEW long-only families — SHADOW OBSERVATION-ONLY
    // Three daily long-only spot families (trend-pullback/reclaim, compression
    // breakout, bull-regime mean-reversion). Long-only, NO shorts, NO 200DMA
    // (regime = smoothed BREADTH participation, never a price MA).
    // BACKTEST VERDICT (backtest/phase6_families_bt.cpp; corrected long-only gate,
    // 2022 shown-not-gated): ALL THREE FAIL the exposure-matched random-pick
    // control — ex-2022 Sharpe 0.71/0.69/0.51 vs a breadth-gated RANDOM basket's
    // MEDIAN 1.20 (beat 0% of 200 draws). Their apparent edge is breadth TIMING,
    // not entry SELECTION. Trend-pullback full-WF FAILS (H2 −1.7%); compression
    // ex-2022 WF H2 −5%; bull-MR plateau FAILS (edge only at RSI-os=35: os30→−0.4%,
    // os40→−21%) + barely trades (43 days). The 80/20 "improvement" is mechanical
    // de-levering (a lower-vol sleeve trims combined DD), and two are momentum-
    // correlated (~0.4 → they LOAD the momentum factor the backlog says to CAP,
    // not diversify). NONE is a promotion candidate. Wired OBSERVATION-ONLY: own
    // shadow tag + forward record, NOT fed to the SpotPortfolioAllocator, NOT in
    // the promoted-sleeve track. The 32-cell UpJump grid + shadow record untouched.
    // See [[ChimeraReviewPhase6]]. (Phase-6b: the other 6 families + two-stage
    // ignition — NOT built, noted for a future session.)
    chimera::TrendPullbackReclaimEngine     p6_tpr;
    chimera::CompressionBreakoutDailyEngine p6_cbd;
    chimera::BullRegimeMeanReversionEngine  p6_bmr;
    {
        std::vector<std::string> xu = {
            "BTC","ETH","SOL","BNB","AVAX","LINK","XRP","DOGE","NEAR","HBAR",
            "INJ","ADA","TRX","ATOM","FIL","AAVE","UNI","DOT","ICP","GRT",
            "SAND","MANA","CRV","COMP","BCH","LTC","ETC","XLM","VET","RUNE","FET","LDO"};
        double nav6 = (runtime_cfg.max_position_usd > 0.0 ? runtime_cfg.max_position_usd : 10000.0);
        auto install6 = [&](chimera::LongOnlyDailyBase& eng, const char* tag) {
            eng.set_universe(xu);
            int seeded = 0, rejected = 0;
            for (const auto& s : xu) {
                std::string path = "data/xsec_seed/" + s + "USDT_1d.csv";
                {   auto dq = g_dq_gate.validate_csv(path, /*now*/ 0);
                    if (dq.rows > 0 && !dq.ok) { std::printf("[DATA-QUALITY] REFUSE %s seed %s — %s\n",
                            tag, path.c_str(), dq.reason.c_str()); ++rejected; continue; } }
                std::ifstream f(path); if (!f) continue;
                std::string l; std::getline(f, l); int r = 0;   // header
                while (std::getline(f, l)) {
                    char* p = l.data(); char* e; long long ts = strtoll(p, &e, 10); if (*e != ',') continue;
                    double o  = strtod(e+1, &e); double h  = strtod(e+1, &e);
                    double lo = strtod(e+1, &e); double c  = strtod(e+1, &e);
                    double v  = strtod(e+1, &e);
                    if (c > 0) { long long d = ts / 86400000LL; eng.seed_daily(s, d, o, h, lo, c, v); ++r; }
                }
                if (r > 0) ++seeded;
            }
            std::printf("[%s] Phase-6 OBSERVATION-ONLY book installed: seeded %d/%zu symbols, %zu days; "
                        "long-only daily, BREADTH-gate (NO 200DMA); NOT allocator-fed (failed pick-edge control) "
                        "nav=$%.0f SHADOW\n", tag, seeded, xu.size(), eng.num_days(), nav6);
            if (rejected > 0) std::printf("[DATA-QUALITY] %s seed: %d symbol(s) REFUSED for corrupt history\n", tag, rejected);
            { size_t i6 = eng.num_days()?eng.num_days()-1:0; bool b6; auto w6 = eng.compute_target_weights(i6, b6);
              std::string p; for (auto& kv : w6) if (kv.second > 0) { char z[48];
                  std::snprintf(z, sizeof z, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); p += z; }
              std::printf("[%s] startup: bull=%d breadth=%.2f -> %s\n",
                          tag, b6?1:0, eng.breadth_latest(), b6 ? (p.empty()?"(none)":p.c_str()) : "CASH (breadth gate)"); }
            std::fflush(stdout);
            eng.set_rebalance_callback([&gateway, &exec_ok, nav6, tag](int64_t day,
                                        const std::map<std::string,double>& tw, bool bull){
                static std::map<std::string,std::map<std::string,double>> holds;  // per-tag hold
                auto& hold = holds[tag];
                std::string picks; for (auto& kv : tw) if (kv.second > 0) { char b[48];
                    std::snprintf(b, sizeof b, "%s:%.0f%% ", kv.first.c_str(), kv.second*100.0); picks += b; }
                std::printf("[%s] REBALANCE day=%lld bull=%d -> %s\n", tag, (long long)day, bull?1:0,
                            bull ? (picks.empty()?"(none)":picks.c_str()) : "CASH (breadth gate)"); std::fflush(stdout);
                if (exec_ok) {
                    for (auto& kv : tw) { std::string lc = kv.first; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
                        int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                        double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                        double tgt = kv.second * nav6, dusd = tgt - hold[kv.first];
                        if (std::fabs(dusd) < 25.0) continue;
                        gateway.submit({ kv.first + "USDT", dusd > 0.0, std::fabs(dusd)/px, px, /*is_exit*/ dusd < 0.0, tag });
                        hold[kv.first] = tgt; }
                    for (auto& kv : hold) if (kv.second > 0 && !tw.count(kv.first)) {
                        std::string lc = kv.first; for (auto& ch : lc) if (ch>='A'&&ch<='Z') ch += 32;
                        int sid = chimera::sym_id(lc + "usdt"); if (sid < 0) continue;
                        double px = load_dbl_atomic(g_last_spot_px_bits[sid]); if (px <= 0.0) continue;
                        gateway.submit({ kv.first + "USDT", false, (kv.second)/px, px, /*is_exit*/ true, tag });
                        kv.second = 0.0; }
                }
                // OBSERVATION-ONLY: deliberately NOT declared to g_allocator (this
                // family failed the pick-edge control — no promotion track).
            });
        };
        install6(p6_tpr, "P6-TPR");   // trend-pullback / reclaim
        install6(p6_cbd, "P6-CBD");   // compression breakout
        install6(p6_bmr, "P6-BMR");   // bull-regime mean-reversion
    }

    // ── Phase-7 (S-2026-07-11): DERIVATIVES-DATA-AS-SIGNAL — OBSERVATION-ONLY recorder
    // Derivatives + microstructure data as a QUALITY FILTER / SIZE modifier on the
    // EXISTING spot-long entries — the data is NEVER traded; every executed trade
    // stays SPOT-LONG. Long-only, NO shorts, NO 200DMA.
    // BACKTEST VERDICT (backtest/phase7_derivsignals_bt.cpp; faithful live UpJump
    // per-coin H1 W/thr, ride-to-flip, 20bp RT; 8 sym × 2025-05..2026-05 = the ENTIRE
    // derivatives history available; gate-attribution + quartile monotonicity;
    // cost-invariant to 2×): ALL THREE data-supported filters REJECTED —
    //   A funding-extreme  : NEUTRAL   (no separation; funding-pct quartiles non-monotonic).
    //   B spot-vs-perp CVD : WEAK/NON-MONOTONIC (perp-led Q1 worst but spot-led Q4 also
    //                        worst; the veto 'help' is the Q1 tail only, does not rank
    //                        quality → not a robust filter). Strongest of the three.
    //   C basis-extreme    : SUSPECT   (high-basis entries were WINNERS → veto suppresses winners).
    // DEFERRED — NO HISTORICAL DATA (won't fabricate): OI change (no OI file; Binance
    // REST OI hist ~30d only), real order-book liquidity cost / expected impact
    // (DepthManager live-only, no historical depth), liquidation clusters
    // (LiquidationWSFeed live-only), stablecoin/exchange flows, event risk (unlocks).
    // Because NO filter earned wiring — and the ONLY real blocker is the 1-year data
    // window — Phase 7 ships as a pure OBSERVATION-ONLY RECORDER: it computes the real
    // funding-pct / spot-vs-perp CVD / basis context at boot and stamps it, so a
    // forward derivative dataset accrues to RE-JUDGE Phase 7 with more history. It
    // changes NOTHING — no order, no size, no veto, no allocator, no feed plumbing.
    // The 32-cell UpJump grid + every shadow book are untouched. See [[ChimeraReviewPhase7]].
    static chimera::DerivativesSignalBook g_deriv_book;
    {
        const std::vector<std::string> dsyms = {
            "BTCUSDT","ETHUSDT","SOLUSDT","DOGEUSDT","BNBUSDT","LINKUSDT","XRPUSDT","AVAXUSDT"};
        // bounded-tail H1 CVD seed (last ~33h) so boot stays light (no full-year 1m parse)
        auto seed_cvd_tail = [&](const std::string& path, bool perp, const std::string& sym){
            FILE* f = std::fopen(path.c_str(), "r"); if (!f) return;
            std::fseek(f, 0, SEEK_END); long sz = std::ftell(f);
            long back = sz > 220000 ? 220000 : sz; std::fseek(f, sz - back, SEEK_SET);
            char line[1024]; if (back < sz) std::fgets(line, sizeof line, f); // drop partial first line
            struct Bar { double c=0, v=0, tbb=0; };
            std::map<int64_t, Bar> agg; // hour -> {close,vol,tbb}
            while (std::fgets(line, sizeof line, f)) {
                char* p = line; char* e; int64_t t = strtoll(p, &e, 10); if (e == p || *e != ',') continue;
                strtod(e+1,&e); strtod(e+1,&e); strtod(e+1,&e); double c = strtod(e+1,&e);
                double v = strtod(e+1,&e); strtoll(e+1,&e,10); strtod(e+1,&e); strtoll(e+1,&e,10);
                double tbb = strtod(e+1,&e); if (c <= 0) continue;
                int64_t hh = (t/3600000LL)*3600000LL; auto& a = agg[hh]; a.c=c; a.v+=v; a.tbb+=tbb;
            }
            std::fclose(f);
            for (auto& kv : agg) { if (perp) g_deriv_book.on_perp_h1(sym, kv.first, kv.second.c, kv.second.v, kv.second.tbb);
                                   else       g_deriv_book.on_spot_h1(sym, kv.first, kv.second.c, kv.second.v, kv.second.tbb); }
        };
        int seeded = 0;
        for (const auto& s : dsyms) {
            // funding (full, tiny)
            std::ifstream ff("data/funding/" + s + ".csv");
            if (ff) { std::string l; std::getline(ff, l); int fr = 0;
                while (std::getline(ff, l)) { auto c1 = l.find(','); if (c1 == std::string::npos) continue;
                    char* e; int64_t t = strtoll(l.c_str()+c1+1, &e, 10); double r = strtod(e+1, &e);
                    g_deriv_book.on_funding(s, t, r); ++fr; }
                if (fr > 0) ++seeded; }
            seed_cvd_tail("data/klines_spot/" + s + "_1m.csv", false, s);
            seed_cvd_tail("data/klines_perp/" + s + "_1m.csv", true,  s);
        }
        std::printf("[P7-DERIV] Phase-7 DERIVATIVES-DATA-AS-SIGNAL — OBSERVATION-ONLY recorder installed: "
                    "seeded %d/%zu symbols (funding + spot/perp CVD + basis); ALL 3 filters REJECTED by BT "
                    "(funding NEUTRAL / CVD non-monotonic / basis SUSPECT); OI+depth+liq+flows DEFERRED (no history); "
                    "changes NOTHING (no order/size/veto). SHADOW.\n", seeded, dsyms.size());
        for (const auto& s : dsyms) {
            auto d = g_deriv_book.eval(s);
            if (!d.ready) { std::printf("[P7-DERIV] %-8s context: (not ready)\n", s.c_str()); continue; }
            std::printf("[P7-DERIV] %-8s context: funding=%.4f%% pct=%.2f | CVD_div=%+.3f (%s) | basis=%+.3f%% "
                        "| would-be size_mult=%.2f (INERT — recorder)\n",
                        s.c_str(), d.funding_rate*100.0, d.funding_pct, d.cvd_div,
                        d.spot_led ? "spot-led" : "perp-led", d.basis_pct*100.0,
                        d.size_mult(g_deriv_book.params()));
        }
        std::fflush(stdout);
    }

    // ── Phase-6b (S-2026-07-11): REMAINING long-only families + two-stage ignition
    //    — SALVAGE CHECK, ALL REJECTED, NOTHING WIRED (documentation stamp only).
    // Screened the other 6 families (breakout-retest, relative-strength acceleration,
    // BTC-lead alt confirmation, breadth-thrust, capitulation-recovery, liquidity-
    // sweep-reversal) + young-liquid-coin momentum + item 28 two-stage ignition.
    // Long-only spot, NO shorts, NO 200DMA. Led with Phase-6's exposure-matched
    // pick-edge control (a breadth-gated RANDOM basket ~ ex-2022 Sharpe 1.20).
    // VERDICT (backtest/phase6b_families_bt.cpp): ALL 7 FAIL the control — ex-2022
    // Sharpe breakout-retest 0.73 / RS-accel 0.75 / BTC-lead 0.80 / breadth-thrust
    // 0.90 / capitulation 0.44 / liq-sweep −0.16 / young-coin 0.45, each beating
    // 0-4% of 200 random draws. Their edge is breadth TIMING, not entry SELECTION;
    // the momentum-flavoured ones carry HIGH corr (0.55-0.58) to XSec v1 so they
    // LOAD the momentum factor, not diversify; capitulation/liq-sweep/young also
    // FAIL walk-forward outright. ITEM 28 (backtest/phase6b_twostage_bt.cpp): at
    // IDENTICAL per-signal capital, NO tranche/split variant beats the immediate-
    // only RipRider parent (net/alloc +23351%, Sharpe 1.76) — pullback tranches
    // fill cheaper (+2.5-6%) and lift win-rate, but UNDER-DEPLOY on the non-
    // pullback runners that carry the fat tail, so capital-matched net is equal-to-
    // worse across every depth/ratio; confirmation tranches (buy higher) are
    // strictly worse. Parent EXIT logic unchanged. NONE promoted, NONE wired — no
    // engine instance, no on_tick, no allocator target. Engine headers + both
    // backtests + tests/run_phase6b_tests.sh are the documented salvage record.
    // See [[ChimeraReviewPhase6b]]. The 32-cell UpJump grid + every shadow book
    // are untouched; MODE=SHADOW.
    std::printf("[P6b] SALVAGE CHECK — 6 remaining families + young-coin + item-28 two-stage: "
                "ALL REJECTED (fail exposure-matched pick-edge control = breadth timing not selection; "
                "two-stage under-deploys vs immediate-only RipRider at equal capital). NONE wired. "
                "See ChimeraReviewPhase6b. SHADOW.\n");
    std::fflush(stdout);

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
        int rseeded = 0, rrejected = 0;
        for (const auto& s : ru) {
            std::string rpath = "data/xsec_seed/" + s + "USDT_1d.csv";
            {   // Phase-4 item 23: refuse a corrupted seed (same gate as XSEC).
                auto dq = g_dq_gate.validate_csv(rpath, /*now*/ 0);
                if (dq.rows > 0 && !dq.ok) {
                    std::printf("[DATA-QUALITY] REFUSE RIP seed %s — %s\n", rpath.c_str(), dq.reason.c_str());
                    ++rrejected; continue;
                }
            }
            std::ifstream rf(rpath);
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
        if (rrejected > 0)
            std::printf("[DATA-QUALITY] RIP seed: %d symbol(s) REFUSED for corrupt history\n", rrejected);
        std::fflush(stdout);
        riprider.set_entry_callback([&exec_ok, rip_nav, &governed_submit](const std::string& sym, double px, int64_t ts){
            (void)ts;
            std::printf("[RIP] ENTRY %s @ %.6f\n", sym.c_str(), px); std::fflush(stdout);
            // Phase-8A: governed BUY (MOMENTUM/RIPRIDER); over aggregate momentum cap -> reduce/reject.
            if (exec_ok && px > 0)
                governed_submit({ sym + "USDT", true, (rip_nav/8.0)/px, px, /*is_exit*/false, "RIP" }, chimera::Factor::MOMENTUM);
            // Phase-3 TRACK-ONLY: declare the RipRider target (one factor with XSec/
            // UpJump); the allocator caps aggregate momentum. Shadow book unchanged.
            if (px > 0) {
                std::lock_guard<std::mutex> lk(g_alloc_mtx);
                g_allocator.set_target("RIP", sym + "USDT", rip_nav/8.0,
                                       chimera::Factor::MOMENTUM, chimera::Family::RIPRIDER);
                g_allocator.plan(&g_ledger);
            }
        });
        riprider.set_close_callback([&gateway, &exec_ok](const chimera::RipClose& t){
            double ret = t.entryPrice>0 ? (t.exitPrice/t.entryPrice-1)*100.0 : 0.0;
            std::printf("[RIP] EXIT %s entry=%.6f exit=%.6f ret=%+.1f%% %s\n",
                        t.symbol.c_str(), t.entryPrice, t.exitPrice, ret, t.exitReason.c_str()); std::fflush(stdout);
            // Phase-2 (item 4): close the EXCHANGE-CONFIRMED remaining qty from the
            // ledger — NOT rip_nav/exitPrice (which mis-sizes because exit!=entry
            // price and ignores partial fills, leaving a residual or overselling).
            // is_exit=true => risk-reducing, never blocked by halts.
            std::string sym = t.symbol + "USDT";
            double held = g_ledger.attributed_qty("RIP", sym);
            if (exec_ok && t.exitPrice > 0 && held > 0.0)
                gateway.submit({ sym, false, held, t.exitPrice, /*is_exit*/true, "RIP" });
            // Phase-3 TRACK-ONLY: RipRider exited this coin -> clear its target.
            {
                std::lock_guard<std::mutex> lk(g_alloc_mtx);
                g_allocator.clear_target("RIP", sym);
                g_allocator.plan(&g_ledger);
            }
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
                // Phase-1: route through the single gateway (EdgeEngine SELLs are exits).
                // Phase-8A: UPJUMP/TSMOM tags are the MOMENTUM factor -> governed by the
                // aggregate momentum cap; other EdgeEngines are per-symbol EDGE (OTHER,
                // symbol-cap only). SELLs pass through. (This intent path is legacy-gated.)
                bool _im = intent.tag.find("UPJUMP") != std::string::npos
                        || intent.tag.find("TSMOM")  != std::string::npos;
                auto result = governed_submit({ intent.symbol, intent.is_buy, qty, intent.ref_px,
                                               /*is_exit*/ !intent.is_buy, intent.tag.c_str() },
                                               _im ? chimera::Factor::MOMENTUM : chimera::Factor::OTHER);
                if (!result.ok) {
                    std::fprintf(stderr,
                        "[ORDER-INTENT] execute failed tag=%s symbol=%s err=%s\n",
                        intent.tag.c_str(), intent.symbol.c_str(), result.error.c_str());
                }
                // Phase-3 TRACK-ONLY: declare the EdgeEngine/UpJump-parent target to
                // the allocator (a BUY sets the target notional; a SELL/exit clears
                // it). UPJUMP tags are the momentum factor + UPJUMP family (weaker
                // macro, reduced size); other EdgeEngines are per-symbol EDGE books.
                // Grid CLIP companions never reach here (own book) — preserved.
                {
                    std::string usym = intent.symbol;
                    for (auto& ch : usym) if (ch>='a'&&ch<='z') ch -= 32;
                    bool is_uj = intent.tag.find("UPJUMP") != std::string::npos;
                    bool is_mom = is_uj || intent.tag.find("TSMOM") != std::string::npos;
                    std::lock_guard<std::mutex> lk(g_alloc_mtx);
                    if (intent.is_buy)
                        g_allocator.set_target(intent.tag, usym, qty * intent.ref_px,
                            is_mom ? chimera::Factor::MOMENTUM : chimera::Factor::OTHER,
                            is_uj  ? chimera::Family::UPJUMP   : chimera::Family::EDGE);
                    else
                        g_allocator.clear_target(intent.tag, usym);
                    g_allocator.plan(&g_ledger);
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
                // Phase-1: route the pyramid ADD (a buy/entry) through the gateway.
                auto result = gateway.submit({ engine.cfg().symbol, true, qty, price,
                                               /*is_exit*/false, tag.c_str() });
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
    // ── KILL_UPJUMP_PARENTS (operator 2026-07-13, both systems) ──────────────────
    // NO up-jump on ANY engine. Every StrategyKind::UPJUMP parent is taken OFF the live
    // path: the g_slots push (the tick-loop driver — the ONLY thing that actually trades,
    // since wire_engine no-ops without CHIMERA_WIRE_LEGACY) is skipped, and the wire_engine
    // calls are guarded too (future-proof: a CHIMERA_WIRE_LEGACY run can't resurrect up-jump).
    // The EdgeEngine OBJECTS remain constructed (GridCoin feed refs need them; the grid is
    // already dead via KILL_UPJUMP_CLIPS) — an un-slotted/un-wired engine never ticks, never
    // trades (its boot "[..] ARMED" line is just a constructor log, not a live signal). Parents
    // are tracked only by the aggregate EDGE-SLOTS registry bucket, which stays wired via the
    // 24 remaining TSMOM/ICHI slots (incl. BTC-TSMOM-D1) -> no per-engine registry abort.
    // Re-enable = flip false + rebuild. KEEP btc_tsmom_d1 (TSMOM) — NOT an up-jump engine.
    const bool KILL_UPJUMP_PARENTS = true;
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
    if (!KILL_UPJUMP_PARENTS) {   // no up-jump on any engine (2026-07-13)
    wire_engine(btc_upjump_h1);
    wire_engine(eth_upjump_h1);
    wire_engine(sol_upjump_h1);
    wire_engine(doge_upjump_h1);
    wire_engine(bnb_upjump_h1);
    }
    // UPJUMP-H1 remaining 5 legs — S-2026-07-03b: feeds already subscribed (all 62
    // SYM_FULL). Per-coin thr from Crypto 437337c. OP = parent-only (no companion).
    chimera::EdgeEngine::Config ada_upjump_cfg  = make_upjump("adausdt",  "ADA-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine::Config near_upjump_cfg = make_upjump("nearusdt", "NEAR-UPJUMP-H1", 4, 0.02);
    chimera::EdgeEngine::Config xrp_upjump_cfg  = make_upjump("xrpusdt",  "XRP-UPJUMP-H1",  4, 0.02);
    chimera::EdgeEngine ada_upjump_h1(ada_upjump_cfg);
    chimera::EdgeEngine near_upjump_h1(near_upjump_cfg);
    chimera::EdgeEngine xrp_upjump_h1(xrp_upjump_cfg);
    if (!KILL_UPJUMP_PARENTS) {   // no up-jump on any engine (2026-07-13)
    wire_engine(ada_upjump_h1);
    wire_engine(near_upjump_h1);
    wire_engine(xrp_upjump_h1);
    }

    // ── FULL BULL ROSTER (operator-approved 2026-07-10/11) — 8 ADDITIVE up-jump cells, SHADOW ──
    // On TOP of the fat-tail roster above (never replace it). Validated STANDALONE
    // ([[CompanionDominanceError]] — never vs-WIDE) under the corrected long-only gate (net>0,
    // PF>=1.3, both WF halves>0, 2x-cost>0; 2022-bear NOT gated — long-only spot can't short a
    // crash). Parity via Crypto/backtest/upjump_earlyarm_bt (single: `live`; cascade: `stagger`,
    // driving THIS header). NO 200DMA anywhere; parent ENTRY thr DISTINCT from mimic ARM.
    //   ETH-UPJUMP2  1h/+2% BE-CASCADE{BE,+2,+3,+4,+6,+8}    : +3337% PF2.95 WF+1832/+1504 2x+3000 DD15731
    //   BTC-UPJUMP4  2h/+4% BE-CASCADE{3,4,6,8,10,12}        : +658%  PF1.93 WF+228/+430  2x+605  DD15117
    //   BNB-UPJUMP3  1h/+3% BE-CASCADE{3..16 N8}             : +3759% PF3.41 WF+2494/+1265 2x+3614 DD19322
    //   SOL-UPJUMP5  1h/+5% BE-CASCADE{BE..+12 N8}           : +3721% PF4.24 WF+2846/+875 2x+3602 DD23545
    //   DOGE-UPJUMP4 4h/+4% single : +3098/1.42 · ADA-UPJUMP5 1h/+5% single : +1471/1.52 (1h beats 6h)
    //   XRP-UPJUMP4  1h/+4% single : +1483/1.46 · TRX-UPJUMP5 1h/+5% single x0.5 (thin ~23 win) : +1964/2.72
    // (all 4 cascades hold the ≤1-un-BE'd guarantee; smoke-tested N6+N8, both arm shapes.)
    chimera::EdgeEngine::Config eth_upjump2_cfg  = make_upjump("ethusdt",  "ETH-UPJUMP2-H1",  1, 0.02);
    chimera::EdgeEngine::Config btc_upjump4_cfg  = make_upjump("btcusdt",  "BTC-UPJUMP4-H2",  2, 0.04);
    chimera::EdgeEngine::Config bnb_upjump3_cfg  = make_upjump("bnbusdt",  "BNB-UPJUMP3-H1",  1, 0.03);
    chimera::EdgeEngine::Config sol_upjump5_cfg  = make_upjump("solusdt",  "SOL-UPJUMP5-H1",  1, 0.05);
    chimera::EdgeEngine::Config doge_upjump4_cfg = make_upjump("dogeusdt", "DOGE-UPJUMP4-H4", 4, 0.04);
    chimera::EdgeEngine::Config ada_upjump5_cfg  = make_upjump("adausdt",  "ADA-UPJUMP5-H1",  1, 0.05);
    chimera::EdgeEngine::Config xrp_upjump4_cfg  = make_upjump("xrpusdt",  "XRP-UPJUMP4-H1",  1, 0.04);
    chimera::EdgeEngine::Config trx_upjump5_cfg  = make_upjump("trxusdt",  "TRX-UPJUMP5-H1",  1, 0.05);
    chimera::EdgeEngine eth_upjump2_h1(eth_upjump2_cfg);
    chimera::EdgeEngine btc_upjump4_h1(btc_upjump4_cfg);
    chimera::EdgeEngine bnb_upjump3_h1(bnb_upjump3_cfg);
    chimera::EdgeEngine sol_upjump5_h1(sol_upjump5_cfg);
    chimera::EdgeEngine doge_upjump4_h4(doge_upjump4_cfg);
    chimera::EdgeEngine ada_upjump5_h1(ada_upjump5_cfg);
    chimera::EdgeEngine xrp_upjump4_h1(xrp_upjump4_cfg);
    chimera::EdgeEngine trx_upjump5_h1(trx_upjump5_cfg);
    if (!KILL_UPJUMP_PARENTS) {   // no up-jump on any engine (2026-07-13)
    wire_engine(eth_upjump2_h1);
    wire_engine(btc_upjump4_h1);
    wire_engine(bnb_upjump3_h1);
    wire_engine(sol_upjump5_h1);
    wire_engine(doge_upjump4_h4);
    wire_engine(ada_upjump5_h1);
    wire_engine(xrp_upjump4_h1);
    wire_engine(trx_upjump5_h1);
    }

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
    // STAGGERED-OPEN companion (S-2026-07-11). Custom base-tier arms open in order under a
    // BE-cascade: the next leg releases ONLY once every released leg is open AND BE'd (mfe >=
    // stagger_be_bp=20bp) => at most ONE un-BE'd leg at a time. reclip OFF + cap==#tiers (no
    // self-funding ladder) preserve the guarantee. gb 0.50 + MTM window-exit flush = the
    // DRAWDOWN-CANCEL protection (200DMA banned). Parity-verified on THIS header via
    // Crypto/backtest/upjump_earlyarm_bt `stagger` mode; ≤1-un-BE smoke-tested N6+N8.
    auto make_stagger_companion = [&unretired](const char* ptag, const char* ctag, const char* sym,
                                     int det_w, double det_thr, const std::vector<double>& arms,
                                     int stagger_mode, int stagger_k, double size_mult, double retire_bp) {
        chimera::UpJumpLadderCompanion::Config c;
        c.parent_tag = ptag; c.tag = ctag; c.symbol = sym;
        c.tight = {arms[0], 0, 0.50, 0};
        c.wide  = {arms[1], 0, 0.50, 0};
        for (size_t k = 2; k < arms.size(); ++k) c.extra_base.push_back({arms[k], 0, 0.50, 0});
        c.reclip_pct = 0.0;                                // OFF (cascade guarantee)
        c.cap = (int)arms.size();                          // == #tiers -> no self-funding ladder
        c.cost_gate_bp = 0.0; c.confirm_bp = 0.0; c.be_floor = false;
        c.det_w = det_w; c.det_thr = det_thr; c.tf_secs = 3600; c.round_trip_bp = 20.0;
        c.size_mult = size_mult; c.rank_out = false; c.retire_bp = retire_bp;
        c.retire_override = unretired(ctag);
        c.stagger_mode = stagger_mode; c.stagger_k = stagger_k; c.stagger_be_bp = 20.0;
        // S-2026-07-13 operator HARD REVERSAL STOP: cut ANY leg (parent + every mimic) at 50bp
        // below entry, per-tick on the reversal signal. Backtest (upjump_earlyarm_bt coldcut,
        // REAL column): worst clip -900..-1800bp -> -70bp on ALL coins, net preserved within
        // ~10% (some higher), PF 2.5 -> ~4.5. Long-only spot: cutting below entry is edge-neutral.
        c.loss_cut_bp = 50.0;
        return c;
    };
    using LTier = chimera::UpJumpLadderCompanion::Tier;
    (void)sizeof(LTier);   // LTier retained for make_lad_companion callers; grid uses make_stagger_companion
    // ── THRESHOLD-COMPARISON GRID (S-2026-07-11, operator) ───────────────────────
    // ONE canonical set: per coin, 4 SEPARATE shadow companion books at thr {2,3,4,5%},
    // SAME BE-cascade mimic + detect window, so the operator watches the real-time forward
    // difference per threshold and lets live real-fills pick the winner (extra bp cost accepted).
    // RETIRES the older fat-tail *-UPJUMP-CLIP + single *-UPJUMP{2..5}-CLIP companions (no
    // 3-overlapping-sets). 8 coins x 4 thr = 32 BE-cascade cells (NEAR excluded — stays cut).
    // ALL shadow, long-only, NO 200DMA; each cell's cascade mimic carries the ≤1-un-BE'd DD-cut
    // (reclip OFF, cap=#tiers). Parity (Crypto/backtest/upjump_earlyarm_bt `grid`): tuned-thr
    // cells reproduce the deployed singles EXACTLY (ETH@2%+3336, BTC@4%+658, BNB@3%+3759,
    // SOL@5%+3721); 31/32 clear the corrected long-only gate (BTC@5% soft WF-H1, kept for the
    // forward comparison). Feeds = the 8 tuned parents (already g_slots'd/ARMED/SEEDED, right
    // window per coin: ETH/rest 1h, BTC 2h, DOGE 4h); the fat-tail *-UPJUMP-H1 parent legs are
    // retired (g_slots below). retire_bp = −2× the coin's worst-thr BT maxDD.
    // ── S-2026-07-12 DAILY UP-JUMP WINNERS (universe scan) — PARENT engines, declared here so
    // the grid below can feed them the BE-cascade MIMIC overlay. Each backtested long-only spot,
    // 2x-cost robust, 2022 omitted; the daily BE-cascade MIMIC dominates the parent ride
    // (upjump_earlyarm_bt UJW_TF=1d grid): NEAR PF7.4-8.0 · AVAX 13.9-16.9 · LINK 5.4-10.6 ·
    // BCH 2.6-3.5 · UNI 11.3-15.2 · LDO 4.5-5.9 (parent ride PF was 1.6-3.7). Parent rides WIDE,
    // mimic clips safe — BOTH run, additive (operator: not one or the other).
    auto make_uj = [](const char* sym, const char* tag, int64_t tf, int w, double thr) {
        return chimera::EdgeEngine::Config{
            .symbol=sym, .tag=tag, .kind=chimera::StrategyKind::UPJUMP, .tf_secs=tf,
            .atr_period=14, .upjump_w=w, .upjump_thr=thr, .ride_to_flip=true,
            .round_trip_bp=20.0, .max_history=96 };
    };
    // KILL_UPJUMP_PARENTS (2026-07-13): objects kept (GridCoin feed refs need them; grid is dead)
    // but NOT wired — no up-jump on any engine. wire_engine is a live no-op anyway (CHIMERA_WIRE_LEGACY).
    chimera::EdgeEngine near_uj8_d1 (make_uj("nearusdt","NEAR-UPJUMP8-D1", 86400,24,0.08));
    chimera::EdgeEngine avax_uj5_d1 (make_uj("avaxusdt","AVAX-UPJUMP5-D1", 86400,24,0.05));
    chimera::EdgeEngine link_uj8_d1 (make_uj("linkusdt","LINK-UPJUMP8-D1", 86400,24,0.08));
    chimera::EdgeEngine bch_uj4_d1  (make_uj("bchusdt", "BCH-UPJUMP4X48-D1",86400,48,0.04));
    chimera::EdgeEngine uni_uj8_d1  (make_uj("uniusdt", "UNI-UPJUMP8-D1",  86400,24,0.08));
    chimera::EdgeEngine ldo_uj3_d1  (make_uj("ldousdt", "LDO-UPJUMP3-D1",  86400,24,0.03));
    chimera::EdgeEngine op_uj3_h4   (make_uj("opusdt",  "OP-UPJUMP3-H4",   14400,24,0.03));
    // S-2026-07-13c operator: XLM/GRT/AAVE passed the thrfloor up-jump BE-cascade study at
    // every threshold (0.5-3%, PF 2.4-2.8, both WF halves +, y2022 +). They were Keltner-only
    // (greyed on the desk = no mimic). Give them the SAME up-jump grid + mimics as the other
    // daily coins; the Keltner engines above stay as separate ADDITIVE books.
    chimera::EdgeEngine xlm_uj5_d1  (make_uj("xlmusdt", "XLM-UPJUMP5-D1",  86400,24,0.05));   // KILL_UPJUMP_PARENTS: not wired
    chimera::EdgeEngine grt_uj5_d1  (make_uj("grtusdt", "GRT-UPJUMP5-D1",  86400,24,0.05));   // KILL_UPJUMP_PARENTS: not wired
    chimera::EdgeEngine aave_uj5_d1 (make_uj("aaveusdt","AAVE-UPJUMP5-D1", 86400,24,0.05));   // KILL_UPJUMP_PARENTS: not wired

    struct GridCoin { const char* pfx; const char* sym; chimera::EdgeEngine* feed; int det_w;
                      std::vector<double> arms; double retire_bp; };
    std::vector<GridCoin> _gcoins = {
        {"ETH", "ethusdt", &eth_upjump2_h1, 1, {0.2,2,3,4,6,8},        -31500.0},   // BE-cascade N6
        {"BTC", "btcusdt", &btc_upjump4_h1, 2, {3,4,6,8,10,12},        -43000.0},   // arm>=3 N6, 2h window
        {"BNB", "bnbusdt", &bnb_upjump3_h1, 1, {3,4,6,8,10,12,14,16},  -38500.0},   // arm>=3 N8
        {"SOL", "solusdt", &sol_upjump5_h1, 1, {0.2,2,3,4,6,8,10,12}, -100000.0},   // BE-cascade N8
        {"DOGE","dogeusdt",&doge_upjump4_h4,4, {0.2,2,3,4,6,8},        -40500.0},   // BE-cascade N6, 4h window
        {"ADA", "adausdt", &ada_upjump5_h1, 1, {0.2,2,3,4,6,8},        -45000.0},   // BE-cascade N6
        {"XRP", "xrpusdt", &xrp_upjump4_h1, 1, {0.2,2,3,4,6,8},        -33000.0},   // BE-cascade N6
        {"TRX", "trxusdt", &trx_upjump5_h1, 1, {0.2,2,3,4,6,8},        -25500.0},   // BE-cascade N6
        // S-2026-07-12 daily/4h up-jump winners — MIMIC overlay on the parents above (additive).
        {"NEAR","nearusdt",&near_uj8_d1, 1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 7.4-8.0
        {"AVAX","avaxusdt",&avax_uj5_d1, 1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 13.9-16.9
        {"LINK","linkusdt",&link_uj8_d1, 1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 5.4-10.6
        {"BCH", "bchusdt", &bch_uj4_d1,  1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 2.6-3.5
        {"UNI", "uniusdt", &uni_uj8_d1,  1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 11.3-15.2
        {"LDO", "ldousdt", &ldo_uj3_d1,  1, {0.2,2,3,4,6,8}, -50000.0},   // daily, mimic PF 4.5-5.9
        {"OP",  "opusdt",  &op_uj3_h4,   1, {0.2,2,3,4,6,8}, -50000.0},   // 4h, mimic overlay on OP parent
        // S-2026-07-13c: Keltner coins promoted to full up-jump grid+mimic (thrfloor PASS).
        {"XLM", "xlmusdt", &xlm_uj5_d1,  1, {0.2,2,3,4,6,8}, -50000.0},
        {"GRT", "grtusdt", &grt_uj5_d1,  1, {0.2,2,3,4,6,8}, -50000.0},
        {"AAVE","aaveusdt",&aave_uj5_d1, 1, {0.2,2,3,4,6,8}, -50000.0},
    };
    // stable tag storage: make_stagger_companion copies the char* into std::string, but keep
    // the backing strings alive for the whole run anyway (main never returns).
    std::vector<std::string> _grid_ptags, _grid_ctags;
    std::vector<chimera::UpJumpLadderCompanion> _grid; _grid.reserve(96);   // reserve => &_grid[i] stable (15 coins x4 = 60 cells; 96 headroom)
    std::vector<chimera::EdgeEngine*> _grid_feeds;
    // ── KILL_UPJUMP_CLIPS (operator 2026-07-13, both systems) ────────────────────
    // The ENTIRE UpJumpLadderCompanion clip layer (the 2/3/4/5% threshold grid + the
    // 0.5% UJH low-thr family) is DISABLED. Every cell enters IMMEDIATELY on the up-jump
    // detection (det_thr) and books a -70bp REVERSAL_CUT when price reverses -- an
    // immediate-entry structure that "trades into a loss" (forbidden). The same-day 50bp
    // loss_cut (2330a8a) was insufficient (clips still booked the reversal cut), so the
    // clips are removed ENTIRELY rather than cut-protected. Skipping population keeps
    // the up-jump cells out of _grid/_all_clips (Phase-3 BE-entry mimics below DO
    // populate _grid — they are NOT this failure class: they open only past BE).
    // The PARENT EdgeEngine UPJUMP legs (wire_engine +
    // g_slots, ride_to_flip) are UNTOUCHED -- the core crypto strategy keeps riding to flip.
    // NOT the clip failure class. StallCompanion (retired-Mac python) is not native here, so
    // there is no at-BE mimic to preserve. Re-enable = flip this flag back to false + rebuild.
    const bool KILL_UPJUMP_CLIPS = true;
    for (auto& gc : _gcoins)
        for (int thr : {2, 3, 4, 5}) {
            if (KILL_UPJUMP_CLIPS) continue;   // immediate-entry clip layer disabled (see above)
            _grid_ptags.push_back(std::string(gc.pfx) + "-UJ" + std::to_string(thr));           // distinct map key
            _grid_ctags.push_back(std::string(gc.pfx) + "-UJ" + std::to_string(thr) + "-CLIP"); // desk book tag
            _grid.emplace_back(make_stagger_companion(
                _grid_ptags.back().c_str(), _grid_ctags.back().c_str(), gc.sym,
                gc.det_w, thr / 100.0, gc.arms, /*BE_CASCADE*/1, 0, 1.0, gc.retire_bp));
            _grid_feeds.push_back(gc.feed);
        }
    // ── S-2026-07-13 LOW-THRESHOLD (UJH) family, SEPARATE tag from the 2-5% grid.
    // Operator (2026-07-13b): run the low-thr study on ALL traded crypto, not just the 8 grid
    // coins — no reason to do half. thrfloor mode (Crypto repo) re-run over EVERY coin with 1h
    // data: ALL PASS at 0.5% (PF 2.4-2.8, both WF halves +, y2022 POSITIVE, maxDD shrinks at
    // low thr). So the UJH cell is added to every _gcoins coin that has passing evidence:
    //   8 grid (BTC/ETH/BNB/SOL/DOGE/ADA/XRP/TRX) + daily-cascade NEAR/AVAX/LINK/BCH/UNI/OP.
    // LDO EXCLUDED — no 1h data to backtest 0.5% on it (add when data exists; do not wire blind).
    // XLM/GRT/AAVE also pass but are Keltner engines (not in _gcoins) — separate feed wiring.
    // BE-N6 arms {0.2,2,3,4,6,8}, det_w = coin's window. Chop protection structural.
    {
        static const std::vector<double> _lowarms = {0.2, 2, 3, 4, 6, 8};
        for (auto& gc : _gcoins) {
            if (KILL_UPJUMP_CLIPS) continue;   // immediate-entry UJH 0.5% clip family disabled (see above)
            const std::string pfx = gc.pfx;
            if (pfx == "LDO") continue;   // no 1h thrfloor evidence — not wired blind
            _grid_ptags.push_back(pfx + "-UJH");
            _grid_ctags.push_back(pfx + "-UJH-CLIP");
            _grid.emplace_back(make_stagger_companion(
                _grid_ptags.back().c_str(), _grid_ctags.back().c_str(), gc.sym,
                gc.det_w, 0.005, _lowarms, /*BE_CASCADE*/1, 0, 1.0, gc.retire_bp));
            _grid_feeds.push_back(gc.feed);
        }
    }
    // ── PHASE 3 (2026-07-13): REGIME_SWITCH parents + BE-ENTRY MIMIC ─────────────
    // The replacement for the killed up-jump family (operator: "replace with our mimic
    // engine", SESSION_HANDOFF_2026-07-13b). Parent kind decided by the Phase-1 scan
    // (backtest/parent_scan_bt.cpp, 54 coins, gate-certified data, random-entry control):
    // REGIME_SWITCH = 30/54 net+ @2x cost, ~59% time-past-BE, flat in chop/bear.
    // Config mirrors backtest/companion_be_mimic_bt.cpp EXACTLY (Phase-2 parity) —
    // these slots are EXEMPT from the tier-preset / S44N / vol_filter override loops
    // below, because this config IS the backtested protection verdict.
    // ADVERSE-PROTECTION (parent): staged BE-ratchet (start=20bp, be_arm=30bp,
    // lock 75/85/90/95%) + 3-ATR stop + ride_to_flip; Phase-1 worst coin-year −7..−21%.
    auto make_regime = [](const char* sym, const char* tag) {
        chimera::EdgeEngine::Config c{};
        c.symbol=sym; c.tag=tag; c.kind=chimera::StrategyKind::REGIME_SWITCH;
        c.tf_secs=86400; c.lookback=20; c.hold_bars=12; c.sl_atr_mult=3.0;
        c.atr_period=14; c.ride_to_flip=true;
        c.round_trip_bp=20.0; c.max_history=64;
        c.hard_floor_bp=0.0; c.early_kill_bp=0.0; c.early_kill_mfe=0.0;
        c.early_kill_min_hold_ms=0; c.giveback_arm_bp=0.0; c.signal_confirm_bars=1;
        c.ratchet_start_bp=20.0; c.be_arm_bp=30.0; c.ratchet_lock_pct=0.75;
        c.prog_lock_pct_2=0.85; c.prog_lock_pct_3=0.90; c.prog_lock_pct_4=0.95;
        c.trail_arm_atr=1.0; c.trail_dist_atr=0.4;
        c.trail_tighten_atr=3.0; c.trail_tighten_dist_atr=0.25;
        // realistic_gap_fill left at default (true): live fills are real ticks, not
        // bar-path replay — the backtest's false was its coarse-OHLC fill model.
        return c;
    };
    chimera::EdgeEngine near_regime_d1 (make_regime("nearusdt", "NEAR-REGIME_SWITCH"));
    chimera::EdgeEngine theta_regime_d1(make_regime("thetausdt","THETA-REGIME_SWITCH"));
    chimera::EdgeEngine sushi_regime_d1(make_regime("sushiusdt","SUSHI-REGIME_SWITCH"));
    chimera::EdgeEngine ada_regime_d1  (make_regime("adausdt",  "ADA-REGIME_SWITCH"));
    chimera::EdgeEngine dot_regime_d1  (make_regime("dotusdt",  "DOT-REGIME_SWITCH"));
    // BE-ENTRY MIMIC factory — reuses UpJumpLadderCompanion in LADDER mode: det_w=0
    // observes the EXTERNAL parent above (never self-detects), confirm_bp=20 keeps
    // every leg PENDING (booking nothing, paying no cost) until the parent's move has
    // already cleared +20bp (== BE == RT cost) — it can never open underwater, the
    // exact property the killed immediate-entry clips lacked. Tight peak-giveback
    // trail once armed, reversal exit, re-clips on +5% continuation.
    // Standalone additive shadow book; judged STANDALONE, never vs the parent
    // (feedback-companion-independent-engine).
    // ADVERSE-PROTECTION (mimic): loss_cut_bp=60 — backtested verdict
    // (companion_be_mimic_bt, certified data): improves PF on 4/5 basket coins
    // (DOT 1.70→2.43, ADA 21.2→36.9) and bounds the tail; per-coin auto-retire below.
    auto make_be_mimic = [&unretired](const char* ptag, const char* ctag, const char* sym,
                                      double retire_bp) {
        chimera::UpJumpLadderCompanion::Config c;
        c.parent_tag = ptag; c.tag = ctag; c.symbol = sym;
        c.tight = {0.30, 0, 0.40, 0.0};   // arm 0.30%  giveback 40% (banks fast)
        c.wide  = {0.80, 0, 0.55, 0.0};   // arm 0.80%  giveback 55% (rides far)
        c.reclip_pct = 0.05;              // re-enter on +5% new peak after a clip
        c.confirm_bp = 20.0;              // BE-ENTRY: open ONLY once fav >= BE (== RT cost)
        c.cap        = 2;                 // 2 base tiers, NO self-funding ladder (BE book)
        c.cost_gate_bp = 0.0; c.be_floor = false;   // LADDER honest-MTM (be_floor family retired)
        c.det_w = 0; c.det_thr = 0.0;     // observe the EXTERNAL parent, not self-detect
        c.tf_secs = 86400; c.round_trip_bp = 20.0;
        c.loss_cut_bp = 60.0;
        c.retire_bp = retire_bp;          // per-coin auto-retire on negative real bank
        c.retire_override = unretired(ctag);
        return c;
    };
    // Core basket = coins whose standalone mimic book cleared the Phase-2 gate at
    // confirm=20/losscut=60 (net+, PF>=1.3, 2x-cost robust; scratchpad/
    // bemimic_regime_results.csv). AVAX (PF 0.62) + KSM (PF 0.39) FAILED — left out.
    // XTZ (PF 1.48) + CRV (PF 1.82) pass net/PF but their FIRST WF half is negative
    // (−2759 / −3794) — left out until a half-split passes (do not wire soft-WF coins).
    // retire_bp ≈ −1.5–2x the coin's Phase-2 worst clip.
    struct RegimeParent { const char* pfx; const char* sym; chimera::EdgeEngine* eng; double retire_bp; };
    const std::vector<RegimeParent> _regime_basket = {
        {"NEAR",  "nearusdt",  &near_regime_d1,  -2000.0},   // mimic PF 1.65, worst −978
        {"THETA", "thetausdt", &theta_regime_d1, -1000.0},   // mimic PF 33.7, worst −421
        {"SUSHI", "sushiusdt", &sushi_regime_d1, -2000.0},   // mimic PF 1.78, worst −1063
        {"ADA",   "adausdt",   &ada_regime_d1,   -1000.0},   // mimic PF 36.9, worst −260
        {"DOT",   "dotusdt",   &dot_regime_d1,   -1800.0},   // mimic PF 2.43, worst −878
    };
    for (const auto& rp : _regime_basket) {
        _grid_ptags.push_back(std::string(rp.pfx) + "-REGIME_SWITCH");
        _grid_ctags.push_back(std::string(rp.pfx) + "-REGIME-BEMIMIC");
        _grid.emplace_back(make_be_mimic(_grid_ptags.back().c_str(), _grid_ctags.back().c_str(),
                                         rp.sym, rp.retire_bp));
        _grid_feeds.push_back(rp.eng);    // det_w=0 => driven off THIS parent's bar closes
    }
    // ── S-2026-07-13 SWEET-SPOT CONFIRMED-ENTRY MIMIC CELLS (operator: "wire all 3") ──
    // Full 18-coin × 0.5%-step threshold grid, evaluated on 2023-26 windows ONLY (2022 is
    // FULLY IRRELEVANT for long-only spot — operator final ruling, feedback-crypto-omit-
    // 2022-longonly). Survivors of the complete validation stack (WF halves + 0.5%-plateau
    // + exbestEpi>0 + 2x-cost re-sim + 20-seed random-entry z), harness
    // Crypto/backtest/upjump_earlyarm_bt confirmcut/confirmrand with CC_FROMYEAR=2023:
    //   NEAR 4.0%: +603%/PF3.34 n=168, 2x +570/PF3.02, z=+7.4, plateau 3.5-5.5%
    //   UNI  3.5%: +278%/PF1.93 n=235, 2x +231/PF1.70, z=+2.9, plateau 3.5-4.5%
    //   BNB  4.0%: +125%/PF3.91 n=34 (THIN), 2x +119/PF3.51, z=+4.4, plateau 3.5-5.0%
    // CONFIRMED entry (confirm_bp=20): every leg stays PENDING — books nothing, pays no
    // cost — until the move has already covered BE. It can never open underwater; this is
    // the permitted mimic class, NOT the killed immediate-entry clips above (those entered
    // ON the jump). Self-detect det_w=1(h1)/det_thr windows: the feed objects below are
    // symbol/tag holders ONLY — never g_slots'd, never wire_engine'd, never ticked — so
    // KILL_UPJUMP_PARENTS stands (observe() ignores parent state for det_w>0 books; the
    // per-tick companion driver matches on the feed's symbol and drives self-detection).
    // NEAR is a SEPARATE book from NEAR-REGIME-BEMIMIC (operator instruction; distinct
    // parent_tag key, distinct desk tag, own bank).
    // ADVERSE-PROTECTION (mimic): loss_cut_bp=50 per-tick hard stop (factory) + gb 0.50
    // peak-giveback + MTM window-exit flush + BE-cascade ≤1-un-BE'd leg; retire_bp = −1500
    // ≈ 2.3–3x the worst 2023-26 BT clip (BNB −503 / UNI −664 / NEAR −656 bp).
    chimera::EdgeEngine bnb_sweet_feed (make_uj("bnbusdt",  "BNB-UJ4-SWEETFEED",  3600, 1, 0.040));
    chimera::EdgeEngine uni_sweet_feed (make_uj("uniusdt",  "UNI-UJ35-SWEETFEED", 3600, 1, 0.035));
    chimera::EdgeEngine near_sweet_feed(make_uj("nearusdt", "NEAR-UJ4-SWEETFEED", 3600, 1, 0.040));
    // ── S-2026-07-13 WINDOW×THRESHOLD FULL GRID (operator: "give me the sweet spot for the
    // other crypto") — the 13i grid fixed W per coin (mostly 1h); this pass swept the WINDOW
    // dimension too: 19 coins × W{1,2,3,4,6,8,12,24}h × thr{0.5..8, 0.5-step} = 2432 cells,
    // 2023-26 ONLY, same stack (gate PF>=1.3/n>=30/H1>0/H2>0/exbestEpi>0 + plateau[isolated
    // cells REJECTED: ETH/XLM/GRT/OP/XRP neighbors flip deep negative] + 2x-cost re-sim +
    // 20-seed random-entry z>=2). NEW survivors (base / 2x / z):
    //   TRX  8h/+3.5%: +612%/PF5.38 n=175 / +577/PF4.59 / z=+10.4  plateau W6-12 x 3-5.5%
    //   UNI  2h/+4.0%: +667%/PF2.39 n=371 / +592/PF2.13 / z=+5.3   (2nd UNI book — overlap
    //        with UNI-UJ35-SWEET accepted in shadow; forward real-fills pick the winner)
    //   AAVE 3h/+4.5%: +348%/PF1.66 n=358 / +277/PF1.48 / z=+4.4
    //   ADA  8h/+3.5%: +440%/PF1.45 n=833 / +274/PF1.25 / z=+2.5   (separate book from
    //        ADA-REGIME-BEMIMIC — different parent family, companions are additive)
    //   DOGE 1h/+5.5%: +188%/PF7.19 n=45 THIN / +179/PF6.24 / z=+4.1  plateau 4.5-6%
    //   LINK 8h/+4.5%: +312%/PF1.51 n=509 / +211/PF1.31 / z=+2.5   (W24/+7.5% alternate also
    //        passed z=+2.5 but H1-heavy; one LINK book only)
    //   LDO  8h/+7.0%: +174%/PF1.53 n=262 / +122/PF1.33 / z=+2.2 MARGINAL (weakest wire)
    // DEAD — NO cell survives the stack on ANY window: BTC (rand long placement earns +110%
    // = drift not edge, z<=0.9), ETH, SOL, XRP, XLM, GRT, OP, BCH, AVAX, LTC.
    // ADVERSE-PROTECTION unchanged (lc50 per-tick + gb 0.50 + BE-cascade); retire_bp per cell
    // ≈ 3x its worst 2x-cost clip. W8 detector warms in 9 H1 closes (cold-start honest).
    chimera::EdgeEngine trx_sweet_feed (make_uj("trxusdt",  "TRX-UJ35W8-SWEETFEED",  3600, 8, 0.035));
    chimera::EdgeEngine uni2_sweet_feed(make_uj("uniusdt",  "UNI-UJ4W2-SWEETFEED",   3600, 2, 0.040));
    chimera::EdgeEngine aave_sweet_feed(make_uj("aaveusdt", "AAVE-UJ45W3-SWEETFEED", 3600, 3, 0.045));
    chimera::EdgeEngine ada_sweet_feed (make_uj("adausdt",  "ADA-UJ35W8-SWEETFEED",  3600, 8, 0.035));
    chimera::EdgeEngine doge_sweet_feed(make_uj("dogeusdt", "DOGE-UJ55-SWEETFEED",   3600, 1, 0.055));
    chimera::EdgeEngine link_sweet_feed(make_uj("linkusdt", "LINK-UJ45W8-SWEETFEED", 3600, 8, 0.045));
    chimera::EdgeEngine ldo_sweet_feed (make_uj("ldousdt",  "LDO-UJ7W8-SWEETFEED",   3600, 8, 0.070));
    {
        const std::vector<double> _sw_arms = {0.2, 2, 3, 4, 6, 8};   // BE-N6 (the validated cell form)
        struct SweetCell { const char* pfx; const char* tagsfx; const char* sym;
                           chimera::EdgeEngine* feed; int det_w; double thr; double retire_bp; };
        const std::vector<SweetCell> _sweet_cells = {
            {"BNB",  "UJ4-SWEET",    "bnbusdt",  &bnb_sweet_feed,  1, 0.040, -1500.0},
            {"UNI",  "UJ35-SWEET",   "uniusdt",  &uni_sweet_feed,  1, 0.035, -1500.0},
            {"NEAR", "UJ4-SWEET",    "nearusdt", &near_sweet_feed, 1, 0.040, -1500.0},
            // S-2026-07-13 window-sweep survivors (validation stack in the comment above)
            {"TRX",  "UJ35W8-SWEET", "trxusdt",  &trx_sweet_feed,  8, 0.035, -1200.0},
            {"UNI",  "UJ4W2-SWEET",  "uniusdt",  &uni2_sweet_feed, 2, 0.040, -2400.0},
            {"AAVE", "UJ45W3-SWEET", "aaveusdt", &aave_sweet_feed, 3, 0.045, -2300.0},
            {"ADA",  "UJ35W8-SWEET", "adausdt",  &ada_sweet_feed,  8, 0.035, -1600.0},
            {"DOGE", "UJ55-SWEET",   "dogeusdt", &doge_sweet_feed, 1, 0.055, -1200.0},
            {"LINK", "UJ45W8-SWEET", "linkusdt", &link_sweet_feed, 8, 0.045, -2000.0},
            {"LDO",  "UJ7W8-SWEET",  "ldousdt",  &ldo_sweet_feed,  8, 0.070, -1800.0},
        };
        for (const auto& sc : _sweet_cells) {
            _grid_ptags.push_back(std::string(sc.pfx) + "-" + sc.tagsfx + "FEED");
            _grid_ctags.push_back(std::string(sc.pfx) + "-" + sc.tagsfx);
            auto c = make_stagger_companion(
                _grid_ptags.back().c_str(), _grid_ctags.back().c_str(), sc.sym,
                sc.det_w, sc.thr, _sw_arms, /*BE_CASCADE*/1, 0, 1.0, sc.retire_bp);
            c.confirm_bp = 20.0;   // CONFIRMED entry — the defining property of this class
            _grid.emplace_back(c);
            _grid_feeds.push_back(sc.feed);
        }
    }
    // ── S-2026-07-14 PER-COIN JUMP-FLOOR CELLS (operator: "revisit… pull every lever…
    // where it becomes viable for each coin" -> then "add these to our trades as you
    // have them here"). EXPLICIT operator override of the immediate-entry ban for
    // THESE cells only (feedback-no-immediate-entry-upjump-mimic-only stands for
    // everything else; KILL_UPJUMP_CLIPS above stays true — that grid remains dead).
    // NOT the retired be_floor family: jump_floor enters ON the jump (pays cost, can
    // lose pre-BE — the exposure IS in the BT verdict), floors at BE once a close
    // covers cost, exits on reversal. Harness: Crypto/backtest/upjump2pct_be_bt.cpp
    // `percoin` (936 lever combos/coin, corrected long-only gate + plateau check,
    // 2x-cost re-sim, n>=30). 17/19 coins VIABLE; LDO + LTC NO-CELL (not wired).
    // S-2026-07-14 STOP RE-SWEEP CULL (operator: "remove all and only keep those 4
    // that are viable"): pre-BE-stop stopsweep (upjump2pct_be_bt `stopsweep`, s in
    // {0.25..5}% at each wired W/thr/g) found only FOUR cells where a hard per-trade
    // cap keeps the gate: ETH s=4% (free — net +18575 vs +18498, maxDD improves),
    // AAVE s=1% (-3.6% net, maxDD halved), GRT s=5% (-6.5% net), DOGE s=4% (-13% net).
    // Everywhere else a stop churns the pre-BE dip that precedes the big riders
    // (60-96% of entries stopped; SOL flips +18938 -> -9445). The 13 stop-incompatible
    // cells (incl. LINK's own s=2 cell) were REMOVED per the same operator call.
    // retire_bp = -2x the STOPPED config's BT maxDD. Vault: UpJump2pctSpotParent.
    // ADVERSE-PROTECTION: backtested per cell (floor + bracket levers swept);
    // every surviving cell carries a hard pre-BE stop + BE-floor.
    {
        struct PJCell { const char* pfx; const char* cell; const char* sym; int W;
                        double thr; double s_bp; double g; double retire_bp; };
        static const std::vector<PJCell> _pj_cells = {
            {"AAVE", "PJ4W1",   "aaveusdt", 1,  0.040, 100.0, 1.0, -10300.0},
            {"DOGE", "PJ3W12",  "dogeusdt", 12, 0.030, 400.0, 1.0, -57300.0},
            {"ETH",  "PJ7W24",  "ethusdt",  24, 0.070, 400.0, 1.0,  -6700.0},
            {"GRT",  "PJ5W1",   "grtusdt",  1,  0.050, 500.0, 1.0,  -9900.0},
        };
        // feed objects: symbol/tag holders ONLY (SWEET pattern) — never g_slots'd,
        // never wire_engine'd; the per-tick companion driver matches on feed symbol.
        static std::vector<chimera::EdgeEngine> _pj_feeds; _pj_feeds.reserve(_pj_cells.size());
        for (const auto& pc : _pj_cells) {
            _grid_ptags.push_back(std::string(pc.pfx) + "-" + pc.cell + "-FEED");
            _grid_ctags.push_back(std::string(pc.pfx) + "-" + pc.cell);
            _pj_feeds.emplace_back(make_uj(pc.sym, _grid_ptags.back().c_str(), 3600, pc.W, pc.thr));
            chimera::UpJumpLadderCompanion::Config c;
            c.parent_tag = _grid_ptags.back(); c.tag = _grid_ctags.back(); c.symbol = pc.sym;
            c.det_w = pc.W; c.det_thr = pc.thr;
            c.jump_floor = true; c.jf_giveback = pc.g; c.jf_prebe_stop_bp = pc.s_bp;
            c.tf_secs = 3600; c.round_trip_bp = 20.0;
            c.confirm_bp = 0.0;   // IMMEDIATE entry — the backtested lever (jf path never reads it)
            c.loss_cut_bp = 0.0;  // pre-BE stop handled by jf_prebe_stop_bp, not the leg cut
            c.be_floor = false; c.reclip_pct = 0.0; c.cap = 1; c.cost_gate_bp = 0.0;
            c.size_mult = 1.0;
            c.retire_bp = pc.retire_bp;
            c.retire_override = unretired(_grid_ctags.back().c_str());
            _grid.emplace_back(std::move(c));
            _grid_feeds.push_back(&_pj_feeds.back());
        }
    }
    std::vector<chimera::UpJumpLadderCompanion*> _all_clips;
    std::vector<chimera::EdgeEngine*>            _all_clip_parents;
    for (size_t i = 0; i < _grid.size(); ++i) { _all_clips.push_back(&_grid[i]); _all_clip_parents.push_back(_grid_feeds[i]); }
    {
        std::lock_guard<std::mutex> lk(g_companion_mtx);
        auto _clip_totals = load_companion_clip_totals();
        const int _NCLIP = (int)_all_clips.size();   // Phase-3: 5 BE-entry mimics (up-jump grid killed)
        g_grid_clip_count = _NCLIP;   // Phase-4 item 20: real grid-cell count for the honest registry
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
                if (cc.jump_floor)
                    std::printf("[CLIP-INIT] %s -> det=%dh/%+.2f%% (self)  JUMP-FLOOR imm-entry preBEstop=%.0fbp gb=%.2f rt=%.0fbp mult=x%.1f retire@%.0fbp%s shadow=1\n",
                        cc.tag.c_str(), cc.det_w, cc.det_thr * 100,
                        cc.jf_prebe_stop_bp, cc.jf_giveback, cc.round_trip_bp,
                        cc.size_mult, cc.retire_bp,
                        _all_clips[i]->is_retired() ? " [RETIRED]" : (cc.rank_out ? " [RANK-OUT]" : ""));
                else if (cc.be_floor)
                    std::printf("[CLIP-INIT] %s -> price %s  BE-FLOOR be=%.0fbp trail(T%.0f/W%.0f)bp det=%dh/%+.2f%% cap=%d shadow=1\n",
                        cc.tag.c_str(), cc.parent_tag.c_str(), cc.be_bp,
                        cc.tight.trail_bp, cc.wide.trail_bp, cc.det_w, cc.det_thr * 100, cc.cap);
                else
                    std::printf("[CLIP-INIT] %s -> det=%dh/%+.2f%% (self)  TIGHT(a%.0f/s%d/g%.2f) WIDE(a%.0f/s%d/g%.2f) +%d stacked-arm(s) reclip=%.2f cap=%d cg=%.0f confirm=%.0fbp mult=x%.1f retire@%.0fbp%s NO-FLOOR shadow=1\n",
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

    // ── S-2026-07-13 CAMPAIGN ARCHITECTURE books (13j §2.11 task 3) ─────────
    // The 4 PASS parent cells from CAMPAIGN_LEVERS_2026-07-13.md, re-verified
    // this session (net/PF/worst @20bp: UNI-W1 +74%/2.53/-155, UNI-W2
    // +156%/2.65/-236, TRX-W8 +92%/4.88/-131, LDO-W8 +68%/1.39/-431; all pass
    // 30/40bp re-sims + 1-bar delay + random-entry z 3.66/5.50/4.73/2.02).
    // ONE campaign per symbol: UNI W1+W2 are two detectors on one campaign
    // slot. Size tiers via mult: UNI x1.0, TRX x0.5 (episode-concentration
    // flag), LDO x0.25 (borderline z). retire_bp ≈ -3x worst 2x-cost clip.
    // MIMIC LOTS OFF (no standalone H1 edge — tick-granularity revisit).
    // ADVERSE-PROTECTION: backtested structural stops + fee-BE floor +
    // net-lock + HWM trail per cell (see CryptoCampaignManager.hpp header).
    chimera::CryptoCampaignManager uni_campaign(
        {"uniusdt", "UNI", 3600, /*mimic*/ false, {
            {"UNI-CAMP-W1", "CW1-3.5", 1, 0.035, 20.0, 135.0, 270.0, 38.0, 1.0,  -550.0, 40.0},
            {"UNI-CAMP-W2", "CW2-4.0", 2, 0.040, 20.0, 216.0, 270.0, 38.0, 1.0,  -800.0, 40.0},
        }}, &g_camp_cost_ledger, &g_camp_gate);
    chimera::CryptoCampaignManager trx_campaign(
        {"trxusdt", "TRX", 3600, /*mimic*/ false, {
            {"TRX-CAMP-W8", "CW8-3.5", 8, 0.035, 20.0, 111.0, 0.0,   13.0, 0.5,  -450.0, 40.0},
        }}, &g_camp_cost_ledger, &g_camp_gate);
    chimera::CryptoCampaignManager ldo_campaign(
        {"ldousdt", "LDO", 3600, /*mimic*/ false, {
            {"LDO-CAMP-W8", "CW8-7.0", 8, 0.070, 20.0, 411.0, 342.0, 48.0, 0.25, -1400.0, 40.0},
        }}, &g_camp_cost_ledger, &g_camp_gate);
    {
        std::lock_guard<std::mutex> lk(g_companion_mtx);
        g_camp_cost_ledger.configure("uniusdt", 20.0, 3.0, 2.0);
        g_camp_cost_ledger.configure("trxusdt", 20.0, 3.0, 2.0);
        g_camp_cost_ledger.configure("ldousdt", 20.0, 3.0, 2.0);
        g_campaigns = { &uni_campaign, &trx_campaign, &ldo_campaign };
        auto _camp_totals = load_companion_clip_totals();
        for (auto* m : g_campaigns) {
            g_campaign_cell_count += m->cell_count();
            for (int ci = 0; ci < m->cell_count(); ++ci) {
                auto ct = _camp_totals.find(m->config().cells[ci].tag);
                if (ct != _camp_totals.end())
                    m->rehydrate_cell(ci, ct->second.n, ct->second.net,
                                      ct->second.net_real, ct->second.net_real_w);
            }
            m->set_on_clip(persist_companion_clip);
            for (const auto& cc : m->config().cells)
                std::printf("[CAMP-INIT] %s W=%d thr=%+.1f%% conf=%.0fbp stop=%.0fbp trail=%s "
                            "mult=x%.2f retire@%.0fbp maxRT=%.0fbp SHADOW mimic=OFF\n",
                            cc.tag.c_str(), cc.W, cc.thr * 100, cc.confirm_bp, cc.pstop_bp,
                            cc.ptrail_bp > 0 ? std::to_string((int)cc.ptrail_bp).c_str() : "RIDE",
                            cc.size_mult, cc.retire_bp, cc.max_validated_rt_bp);
        }
        restore_campaign_state();   // verbatim window/campaign restore (stale >24 bars discarded)
        emit_companion_state();     // include campaign legs in the startup emit
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

    // ── S-2026-07-12 MAC-FOLD (crypto consolidation onto josgp1) ───────────────
    // ADA daily Keltner-breakout — folds the validated Mac ibkrcrypto ADA Kelt mimic
    // (faithful --protect-sweep: OOS_23-26 PF 4.60, DD 6.6%, 2x-cost-robust). KELTNER_BREAK
    // = upper-band breakout TREND (close>EMA20+2*ATR20 -> long), ride to lower-band flip,
    // NO trade-level stops (ride_to_flip) — NOT KELTNER_REVERT (opposite lower-band revert).
    // Params matched to Mac Kelt(20,2.0): keltner_ema_len=20, keltner_atr_mult=2.0, atr20.
    chimera::EdgeEngine::Config ada_kelt_d1_cfg{
        // keltner_ema_len(20)/keltner_atr_mult(2.0) left at struct defaults = Mac Kelt(20,2.0);
        // atr_period=20 sets the ATR window to match. (Explicit keltner_* designators would
        // violate GCC's declaration-order rule vs trail_* — defaults are identical anyway.)
        .symbol="adausdt", .tag="ADA-KELT-D1", .kind=chimera::StrategyKind::KELTNER_BREAK,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=20,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ada_kelt_d1(ada_kelt_d1_cfg);
    wire_engine(ada_kelt_d1);

    // ADA daily Regime-switch — 2nd folded Mac mimic (Mac ibkrcrypto ADA Regime OOS PF 1.67).
    // REGIME_SWITCH = ER>0.40 trending->momentum long / ER<0.25 chop->IBS mean-rev long; ride_to_flip.
    chimera::EdgeEngine::Config ada_reg_d1_cfg{
        .symbol="adausdt", .tag="ADA-REGIME-D1", .kind=chimera::StrategyKind::REGIME_SWITCH,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=14,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine ada_reg_d1(ada_reg_d1_cfg);
    wire_engine(ada_reg_d1);

    // AAVE daily Keltner-breakout — folds the Mac ibkrcrypto AAVE Kelt mimic (OOS PF 1.56,
    // DD 11.5%, 2x-robust; the ONLY viable AAVE mimic — operator hard-no on the 62%-DD UpJump).
    // aaveusdt already fed (SYM_AAVE=26 < MAX_SYMBOLS=62). Same KELTNER_BREAK as ADA.
    chimera::EdgeEngine::Config aave_kelt_d1_cfg{
        .symbol="aaveusdt", .tag="AAVE-KELT-D1", .kind=chimera::StrategyKind::KELTNER_BREAK,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=20,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine aave_kelt_d1(aave_kelt_d1_cfg);
    wire_engine(aave_kelt_d1);

    // ── S-2026-07-12 UNIVERSE SCAN winners (Kelt cells; deployed settings, 2x-cost robust) ──
    // XLM daily Keltner-break — the STRONGEST candidate in the whole universe (OOS +250%/PF11.9,
    // 2x-cost PF 11.3). XRP daily Kelt (OOS +163/PF6.0, 2x 5.76) — stronger than its grid up-jump.
    // Both fed (SYM_XLM=60, SYM_XRP=6). KELTNER_BREAK = upper-band breakout, ride-to-flip.
    chimera::EdgeEngine::Config xlm_kelt_d1_cfg{
        .symbol="xlmusdt", .tag="XLM-KELT-D1", .kind=chimera::StrategyKind::KELTNER_BREAK,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=20,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine xlm_kelt_d1(xlm_kelt_d1_cfg);
    wire_engine(xlm_kelt_d1);

    chimera::EdgeEngine::Config xrp_kelt_d1_cfg{
        .symbol="xrpusdt", .tag="XRP-KELT-D1", .kind=chimera::StrategyKind::KELTNER_BREAK,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=20,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine xrp_kelt_d1(xrp_kelt_d1_cfg);
    wire_engine(xrp_kelt_d1);

    // GRT — the only fed coin with NO engine (operator gap). Backtested this session with the
    // deployed settings: GRT 1d Kelt OOS +48.6/PF2.93 (2x 2.78); UpJump4 even stronger (PF3.12,
    // -> joins the daily up-jump mimic). Kelt cell closes the gap now. Fed SYM_GRT.
    chimera::EdgeEngine::Config grt_kelt_d1_cfg{
        .symbol="grtusdt", .tag="GRT-KELT-D1", .kind=chimera::StrategyKind::KELTNER_BREAK,
        .tf_secs=86400, .lookback=20, .hold_bars=12, .sl_atr_mult=3.0, .atr_period=20,
        .ride_to_flip=true, .bb_k=2.0, .rsi_threshold=30.0, .round_trip_bp=22, .max_history=64,
        .trail_arm_atr=1.0, .trail_dist_atr=0.4, .trail_tighten_atr=3.0, .trail_tighten_dist_atr=0.25,
    };
    chimera::EdgeEngine grt_kelt_d1(grt_kelt_d1_cfg);
    wire_engine(grt_kelt_d1);

    // NOTE: the S-2026-07-12 universe-scan up-jump PARENT winners (NEAR/AVAX/LINK/BCH/UNI/LDO
    // daily + OP 4h) are declared EARLIER (just before the _gcoins grid) so the grid can feed
    // them the BE-cascade mimic overlay. See the "DAILY UP-JUMP WINNERS" block above the grid.

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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_d1,   "bnbusdt",  86400, "BNB-TSMOM-D1",   3.16, 2.91,  90,  32, 14});

    // UPJUMP-H1 fat-tail parent legs — RETIRED S-2026-07-11 (threshold-comparison grid supersedes;
    // no 3-overlapping-sets). Their companion books were retired above; these parent legs are now
    // un-slotted so they no longer trade/clutter the desk. Objects stay declared+wired (untraded,
    // no g_slot => never ticked). The grid's 8 feeds are the tuned parents slotted just below.
    // g_slots.push_back({chimera::SYM_BTC,  &btc_upjump_h1,  "btcusdt",  3600, "BTC-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_ETH,  &eth_upjump_h1,  "ethusdt",  3600, "ETH-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_SOL,  &sol_upjump_h1,  "solusdt",  3600, "SOL-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_DOGE, &doge_upjump_h1, "dogeusdt", 3600, "DOGE-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_BNB,  &bnb_upjump_h1,  "bnbusdt",  3600, "BNB-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_ADA,  &ada_upjump_h1,  "adausdt",  3600, "ADA-UPJUMP-H1",  0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_NEAR, &near_upjump_h1, "nearusdt", 3600, "NEAR-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    // g_slots.push_back({chimera::SYM_XRP, &xrp_upjump_h1, "xrpusdt", 3600, "XRP-UPJUMP-H1", 0.0, 0.0, 0, 0, 57});
    // FULL BULL ROSTER parents (S-2026-07-11) — price feed + ARM+SEED for the 8 new companion cells
    // (companions self-detect their own window; parent position not read for det_w books). SHADOW.
    // KILL_UPJUMP_PARENTS (2026-07-13): the 8 UPJUMP2-5 legs were the ONLY live-driven up-jump
    // engines (tick loop iterates g_slots). Skip them -> no up-jump trades. EDGE-SLOTS drops 32->24
    // (TSMOM/ICHI incl BTC-TSMOM-D1 remain) so the registry bucket stays wired (no abort).
    if (!KILL_UPJUMP_PARENTS) {
    g_slots.push_back({chimera::SYM_ETH,  &eth_upjump2_h1,  "ethusdt",  3600, "ETH-UPJUMP2-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_BTC,  &btc_upjump4_h1,  "btcusdt",  3600, "BTC-UPJUMP4-H2",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_BNB,  &bnb_upjump3_h1,  "bnbusdt",  3600, "BNB-UPJUMP3-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_SOL,  &sol_upjump5_h1,  "solusdt",  3600, "SOL-UPJUMP5-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_DOGE, &doge_upjump4_h4, "dogeusdt", 3600, "DOGE-UPJUMP4-H4", 0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_ADA,  &ada_upjump5_h1,  "adausdt",  3600, "ADA-UPJUMP5-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_XRP,  &xrp_upjump4_h1,  "xrpusdt",  3600, "XRP-UPJUMP4-H1",  0.0, 0.0, 0, 0, 57});
    g_slots.push_back({chimera::SYM_TRX,  &trx_upjump5_h1,  "trxusdt",  3600, "TRX-UPJUMP5-H1",  0.0, 0.0, 0, 0, 57});
    }

    // ── PHASE 3 (2026-07-13): REGIME_SWITCH D1 trend parents — the live book ─────
    // Phase-1 winner kind (parent_scan_bt, 54 coins, certified data, random-entry
    // control). Stats = today's certified-data rerun (parent_scan_bt REGIME_SWITCH
    // cost=20 seed=20): PF / trades per coin. These feed the *-REGIME-BEMIMIC
    // companions (registered above) via on_bar_callback. EXEMPT from tier-preset/
    // S44N/vol_filter overrides — config is the Phase-2 backtest parity config.
    g_slots.push_back({chimera::SYM_NEAR,  &near_regime_d1,  "nearusdt",  86400, "NEAR-REGIME_SWITCH",  1.98, 0.0, 0, 33, 60});
    g_slots.push_back({chimera::SYM_THETA, &theta_regime_d1, "thetausdt", 86400, "THETA-REGIME_SWITCH", 6.98, 0.0, 0, 12, 60});
    g_slots.push_back({chimera::SYM_SUSHI, &sushi_regime_d1, "sushiusdt", 86400, "SUSHI-REGIME_SWITCH", 4.00, 0.0, 0, 23, 60});
    g_slots.push_back({chimera::SYM_ADA,   &ada_regime_d1,   "adausdt",   86400, "ADA-REGIME_SWITCH",   5.78, 0.0, 0, 30, 60});
    g_slots.push_back({chimera::SYM_DOT,   &dot_regime_d1,   "dotusdt",   86400, "DOT-REGIME_SWITCH",   2.58, 0.0, 0, 24, 60});

    // H12 engines (3)
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h12,  "btcusdt",  43200, "BTC-TSMOM-H12",  3.63, 3.40,  96,  31, 14});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h12, "dogeusdt", 43200, "DOGE-TSMOM-H12", 2.78, 3.66, 100,  82, 14});

    // H6 engines (8) — NEW Session 15
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h6,   "xrpusdt",  21600, "XRP-TSMOM-H6",   2.68, 4.41, 100, 120, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC,  &btc_tsmom_h6,    "btcusdt",  21600, "BTC-TSMOM-H6",   2.59, 5.16, 100, 169, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ETH,  &eth_tsmom_h6,    "ethusdt",  21600, "ETH-TSMOM-H6",   2.07, 3.70, 100, 151, 15});
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h6,    "solusdt",  21600, "SOL-TSMOM-H6",   2.07, 3.25, 100, 127, 15});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h6,    "bnbusdt",  21600, "BNB-TSMOM-H6",   2.07, 2.76, 100,  95, 15});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h6,   "linkusdt", 21600, "LINK-TSMOM-H6",  1.33, 1.12, 100, 549, 15});  // AUDIT-2026 revived: bvr PF=1.33 n=549 Sh=1.12
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h6,   "dogeusdt", 21600, "DOGE-TSMOM-H6",  1.72, 2.24,  77,  91, 15});
// S44-CULL:     g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h6,   "avaxusdt", 21600, "AVAX-TSMOM-H6",  1.48, 2.03, 100, 1157, 15});  // AUDIT-2026 revived: bvr PF=1.48 n=1157 Sh=2.03 (top dark engine)

    // H4 engines (7)
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_XRP,  &xrp_tsmom_h4,   "xrpusdt",  14400, "XRP-TSMOM-H4",   2.43, 5.80, 100, 267, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB,  &bnb_tsmom_h4,    "bnbusdt",  14400, "BNB-TSMOM-H4",   1.91, 3.79, 100, 291, 14});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h4,   "linkusdt", 14400, "LINK-TSMOM-H4",  1.91, 4.07,  95, 205, 14});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_SOL,  &sol_tsmom_h4,    "solusdt",  14400, "SOL-TSMOM-H4",   1.89, 3.82, 100, 208, 14});  // AUDIT-2026-S35 revived: protected-bvr PF=19.26 Sharpe=15.6 worst=-70bp MDD/cum=0.1%
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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h12, "nearusdt", 43200, "NEAR-TSMOM-H12", 1.92, 3.03,  95, 126, 20});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h6,  "nearusdt", 21600, "NEAR-TSMOM-H6",  1.85, 3.62, 100, 257, 20});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h4,  "nearusdt", 14400, "NEAR-TSMOM-H4",  2.17, 3.59, 100, 209, 20});  // AUDIT-2026-S35 revived: protected-bvr PF=22.66 Sharpe=13.6 worst=-72bp MDD/cum=0.1%
    // DISABLED-AUDIT2026: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h3,  "nearusdt", 10800, "NEAR-TSMOM-H3",  1.75, 3.65,  87, 351, 20});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h6,   "suiusdt",  21600, "SUI-TSMOM-H6",   1.80, 3.22, 100, 129, 20});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_SUI,  &sui_tsmom_h4,   "suiusdt",  14400, "SUI-TSMOM-H4",   1.44, 2.11,  88, 169, 20});  // AUDIT-2026-S35 revived: protected-bvr PF=16.46 Sharpe=16.6 worst=-72bp MDD/cum=0.2%
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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_NEAR, &near_tsmom_h8, "nearusdt", 28800, "NEAR-TSMOM-H8", 2.10, 3.79, 97, 171, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h8, "bnbusdt", 28800, "BNB-TSMOM-H8", 2.86, 3.67, 100, 138, 21});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h8, "dogeusdt", 28800, "DOGE-TSMOM-H8", 2.02, 2.54, 100, 107, 21});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_AVAX, &avax_tsmom_h8, "avaxusdt", 28800, "AVAX-TSMOM-H8", 1.38, 1.05, 100, 609, 21});  // AUDIT-2026 revived: bvr PF=1.38 n=609 Sh=1.05
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_SUI, &sui_tsmom_h8, "suiusdt", 28800, "SUI-TSMOM-H8", 2.27, 2.50, 81, 62, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_ARB, &arb_tsmom_h8, "arbusdt", 28800, "ARB-TSMOM-H8", 2.01, 2.84, 50, 86, 21});
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BTC, &btc_tsmom_h16, "btcusdt", 57600, "BTC-TSMOM-H16", 5.16, 4.01, 100, 22, 21});
// S44-CULL:     g_slots.push_back({chimera::SYM_SOL, &sol_tsmom_h16, "solusdt", 57600, "SOL-TSMOM-H16", 3.47, 3.77, 100, 54, 21});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_XRP, &xrp_tsmom_h16, "xrpusdt", 57600, "XRP-TSMOM-H16", 4.72, 4.14, 100, 55, 21});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h16, "linkusdt", 57600, "LINK-TSMOM-H16", 1.37, 0.85, 100, 272, 21});  // AUDIT-2026 revived: bvr PF=1.37 n=272 Sh=0.85
    // DISABLED-AUDIT2026-P7: g_slots.push_back({chimera::SYM_BNB, &bnb_tsmom_h16, "bnbusdt", 57600, "BNB-TSMOM-H16", 2.76, 2.70, 100, 61, 21});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_DOGE, &doge_tsmom_h16, "dogeusdt", 57600, "DOGE-TSMOM-H16", 2.16, 2.33, 92, 54, 21});
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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_h8,   "fetusdt",  28800, "FET-TSMOM-H8",   68.23, 13.71, 100, 1479, 35});  // S35 new: PF=68 Sh=13.7 worst=-72bp MDD=-144bp
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_h12,  "fetusdt",  43200, "FET-TSMOM-H12", 120.29, 11.57, 100,  980, 35});  // S35 new: PF=120 Sh=11.6
// S44-CULL:     g_slots.push_back({chimera::SYM_FET,  &fet_tsmom_d1,   "fetusdt",  86400, "FET-TSMOM-D1",  331.33,  8.66, 100,  451, 35});  // S35 new: PF=331 Sh=8.7
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_TIA,  &tia_tsmom_h8,   "tiausdt",  28800, "TIA-TSMOM-H8",   56.38, 12.73, 100,  631, 35});  // S35 new: PF=56 Sh=12.7
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_TIA,  &tia_tsmom_h12,  "tiausdt",  43200, "TIA-TSMOM-H12",  61.89, 10.53, 100,  425, 35});  // S35 new: PF=62 Sh=10.5
// S44-CULL:     g_slots.push_back({chimera::SYM_ONDO, &ondo_tsmom_h12, "ondousdt", 43200, "ONDO-TSMOM-H12", 33.54, 11.08, 100,  180, 35});  // S35 new: PF=33 Sh=11.1 (shortest history, ~1yr)

    // AUDIT-2026-S35 WAVE 2: HBAR / INJ / ADA / TRX / SEI TSMOM engines.
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_h8,  "hbarusdt", 28800, "HBAR-TSMOM-H8",  34.94, 10.00, 100, 1458, 35});  // S35 w2: PF=35 Sh=10.0
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_h12, "hbarusdt", 43200, "HBAR-TSMOM-H12", 47.04,  8.36, 100,  934, 35});  // S35 w2: PF=47 Sh=8.4
// S44-CULL:     g_slots.push_back({chimera::SYM_HBAR, &hbar_tsmom_d1,  "hbarusdt", 86400, "HBAR-TSMOM-D1",  72.48,  7.40, 100,  430, 35});  // S35 w2: PF=72 Sh=7.4
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_h8,   "injusdt",  28800, "INJ-TSMOM-H8",   42.00, 13.00, 100, 1546, 35});  // S35 w2: PF=42 Sh=13.0
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_h12,  "injusdt",  43200, "INJ-TSMOM-H12",  80.88, 11.17, 100, 1043, 35});  // S35 w2: PF=81 Sh=11.2
// S44-CULL:     g_slots.push_back({chimera::SYM_INJ,  &inj_tsmom_d1,   "injusdt",  86400, "INJ-TSMOM-D1",  146.79,  8.81, 100,  479, 35});  // S35 w2: PF=147 Sh=8.8
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_h8,   "adausdt",  28800, "ADA-TSMOM-H8",   27.17, 11.48, 100, 1438, 35});  // S35 w2: PF=27 Sh=11.5
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_h12,  "adausdt",  43200, "ADA-TSMOM-H12",  41.00, 10.27, 100,  954, 35});  // S35 w2: PF=41 Sh=10.3
// S44-CULL:     g_slots.push_back({chimera::SYM_ADA,  &ada_tsmom_d1,   "adausdt",  86400, "ADA-TSMOM-D1",   72.66,  7.20, 100,  423, 35});  // S35 w2: PF=73 Sh=7.2
// S44-CULL:     g_slots.push_back({chimera::SYM_TRX,  &trx_tsmom_d1,   "trxusdt",  86400, "TRX-TSMOM-D1",   23.00,  6.61, 100,  612, 35});  // S35 w2: PF=23 Sh=6.6 (stable carry — slow TF only)
// S44-CULL:     g_slots.push_back({chimera::SYM_TRX,  &trx_tsmom_d2,   "trxusdt", 172800, "TRX-TSMOM-D2",   42.34,  5.62, 100,  305, 35});  // S35 w2: PF=42 Sh=5.6
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_SEI,  &sei_tsmom_h8,   "seiusdt",  28800, "SEI-TSMOM-H8",   49.69, 13.39, 100,  722, 35});  // S35 w2: PF=50 Sh=13.4
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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_LINK, &link_tsmom_h12, "linkusdt", 43200, "LINK-TSMOM-H12", 1.34, 1.24, 100, 643, 24});  // AUDIT-2026 revived: bvr PF=1.34 n=643 Sh=1.24
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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_SOL,  &sol_ichi_h8,  "solusdt",  28800, "SOL-ICHI-H8",  5.92, 6.20,  94, 188, 37});
// S44-CULL:     g_slots.push_back({chimera::SYM_LINK, &link_keltner_h6, "linkusdt", 21600, "LINK-KELTNER-H6", 6.12, 2.69, 91, 35, 37});
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_NEAR, &near_ichi_h8, "nearusdt", 28800, "NEAR-ICHI-H8", 2.92, 4.67, 89, 192, 37});  // S37: bear-stress passed all 4 windows PF>=2.35

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
    // PHASE3-DEAD-TSMOM (2026-07-13): Phase-1 parent scan = TSMOM 0/54 vs random, book replaced by REGIME_SWITCH: g_slots.push_back({chimera::SYM_APT,  &apt_tsmom_h8,   "aptusdt",   28800, "APT-TSMOM-H8",  1.49, 1.42,  85, 404, 31});
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
            // PHASE3: REGIME_SWITCH parents keep their make_regime config verbatim —
            // it IS the Phase-1/2 backtested protection (staged ratchet 20/30,
            // lock 75/85/90/95, no floor/kill/giveback). Presets would break parity.
            if (slot.engine->cfg().kind == chimera::StrategyKind::REGIME_SWITCH) continue;
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
            // PHASE3: parity exemption (see tier loop above).
            if (slot.engine->cfg().kind == chimera::StrategyKind::REGIME_SWITCH) continue;
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
        // CHIMERA→OMEGA DESK export (S-2026-07-12): slot engines (TSMOM/ICHI/
        // BOLL + UPJUMP parents) had NO on_trade wired — wire_engine() is
        // legacy-gated (CHIMERA_WIRE_LEGACY) and this loop only set on_bar —
        // so every close died in journalctl and the operator's desk was blind
        // (SUI-TSMOM-H4 +12.79bp on 11-07 never left the box). EXPORT-ONLY
        // hook: appends to data/chimera_inbound.csv for the desk relay. It
        // does NOT feed g_trade_log / PER / streak-halt / live-tiers — wiring
        // slot closes into those protections would be a risk-behavior change
        // needing its own backtested verdict (finding logged, not smuggled).
        if (slot.engine) slot.engine->set_on_trade(
            [](const chimera::EdgeEngine::TradeRecord& rec) {
                export_desk_trade(rec.entry_ts_ms, rec.exit_ts_ms, rec.symbol,
                    rec.tag, "BUY", rec.entry_px, rec.exit_px,
                    rec.net_bp + rec.pyramid_bp, rec.reason);
            });
    }

    // ── Activate vol_filter + mtf_gate on all counter-trend engines ──────
    // Counter-trend = RSI_REVERT, BOLLINGER, KELTNER_REVERT
    // These benefit from suppression during chaos and bearish D1 trends.
    {
        int vol_count = 0, mtf_count = 0;
        for (auto& slot : g_slots) {
            if (!slot.engine) continue;
            // PHASE3: REGIME_SWITCH is trend-riding (not counter-trend) but isn't in
            // is_trend_following(); exempt it from the counter-trend filters (parity).
            if (slot.engine->cfg().kind == chimera::StrategyKind::REGIME_SWITCH) continue;
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

    // ── S-2026-07-14 companion detector warm-seed (operator: NEVER wait for warmup) ──
    // restore_companion_det_state() above only covers RESTARTS. A brand-new det_w
    // cell (first deploy) — or one whose persisted ring went stale across an outage —
    // cold-started its W-bar window and sat blind up to det_w hours (ETH-PJ7W24 ≈25h
    // on the 14-07 deploy). Fill every cold/stale ring from Binance REST 1h klines
    // (public endpoint, same source as engine seeding). State-only: entries can only
    // fire on LIVE ticks at real prices (see seed_det_ring_hist header note).
    {
        struct DetSeedJob { chimera::UpJumpLadderCompanion* comp; std::string sym; int det_w; };
        std::vector<DetSeedJob> jobs;
        {
            std::lock_guard<std::mutex> lk(g_companion_mtx);
            for (auto& kv : g_companion_by_parent) {
                chimera::UpJumpLadderCompanion* comp = kv.second.second;
                if (!comp) continue;
                const auto& cc = comp->config();
                if (cc.det_w <= 0) continue;
                if (cc.tf_secs != 3600) {   // all det_w books are H1 today; guard future drift
                    std::printf("[CLIP-SEED] %s tf=%llds != 1h — det ring warm-seed skipped\n",
                        cc.tag.c_str(), (long long)cc.tf_secs);
                    continue;
                }
                jobs.push_back({comp, cc.symbol, cc.det_w});
            }
        }
        if (!jobs.empty()) {
            chimera::BinanceREST det_rest;   // fetch OUTSIDE the companion lock (ticks may already flow)
            const int64_t now_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            std::map<std::string, int> need;                      // symbol -> max W among its cells
            for (auto& j : jobs) { int& n = need[j.sym]; if (j.det_w > n) n = j.det_w; }
            std::map<std::string, std::vector<double>> closes_by_sym;
            std::map<std::string, int64_t> lastbar_by_sym;
            for (auto& kv : need) {
                auto kl = det_rest.fetch_klines(kv.first, "1h", kv.second + 6);
                while (!kl.empty() && kl.back().open_ts_ms + 3600000LL > now_ms)
                    kl.pop_back();                                // drop the in-progress bar: FINALIZED closes only
                if ((int)kl.size() < 2) {
                    std::fprintf(stderr, "[CLIP-SEED] %s: fetch_klines(1h) gave %d closed bars — "
                        "det ring stays cold (honest warmup)\n", kv.first.c_str(), (int)kl.size());
                    continue;
                }
                std::vector<double> cs; cs.reserve(kl.size());
                for (const auto& k : kl) cs.push_back(k.c);
                closes_by_sym[kv.first] = std::move(cs);
                lastbar_by_sym[kv.first] = kl.back().open_ts_ms / 3600000LL;
            }
            std::lock_guard<std::mutex> lk(g_companion_mtx);
            int warmed = 0;
            for (auto& j : jobs) {
                auto it = closes_by_sym.find(j.sym);
                if (it == closes_by_sym.end()) continue;
                j.comp->seed_det_ring_hist(it->second, lastbar_by_sym[j.sym]);
                warmed++;
            }
            std::printf("[CLIP-SEED] det-ring warm-seed pass: %d det_w book(s) over %zu symbol fetch(es) — no cold-start wait\n",
                warmed, closes_by_sym.size());
            std::fflush(stdout);
        }
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

    // ── Phase-1 review: control-API hardening. Bind localhost by default; require
    // a token on the mutating endpoints. Precedence: env > live_config.json.
    // If no token is configured a random one is generated + printed (so the
    // mutating endpoints fail closed to anyone without it).
    if (const char* b = std::getenv("CHIMERA_HTTP_BIND")) g_http_bind_addr = b;
    else                                                  g_http_bind_addr = runtime_cfg.http_bind;
    if (g_http_bind_addr.empty()) g_http_bind_addr = "127.0.0.1";
    if (const char* t = std::getenv("CHIMERA_CTRL_TOKEN")) g_ctrl_token = t;
    else                                                   g_ctrl_token = runtime_cfg.control_token;
    if (g_ctrl_token.empty()) {
        std::random_device rd;
        static const char* hex = "0123456789abcdef";
        std::string tok; for (int i = 0; i < 24; ++i) tok += hex[rd() & 0xF];
        g_ctrl_token = tok;
        std::printf("[STARTUP] no control_token configured — generated one for this run: %s\n"
                    "          (set CHIMERA_CTRL_TOKEN or control_token in live_config.json to fix it)\n",
                    g_ctrl_token.c_str());
    }
    std::printf("[STARTUP] HTTP server starting on %s:8080...\n", g_http_bind_addr.c_str());
    std::fflush(stdout);
    // CH-06 (audit 2026-07-13): static-storage http thread — a detached local could be
    // running while main's locals/statics tear down. Static outlives them; process exits
    // right after the shutdown snapshot. (Full stop-flag+socket-close = larger, tracked.)
    static std::thread s_http_thread(http_server_thread, 8080);

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
          xsec2.on_tick(xss, mid, xnow);          // Phase-5 XSec 2.0 shadow book
          riprider.on_tick(xss, mid, xnow);
          p6_tpr.on_tick(xss, mid, xnow);         // Phase-6 observation-only books
          p6_cbd.on_tick(xss, mid, xnow);
          p6_bmr.on_tick(xss, mid, xnow); }

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
                    // S-2026-07-11 NO-200DMA (bounce RCA): the S54 "BTC<200d-MA halts
                    // longs" macro veto is REMOVED from ALL slot entry gating. It was
                    // already cut for UPJUMP on 2026-07-05 by explicit, repeated operator
                    // directive; the standing rule (feedback-no-200dma-crypto) bans a
                    // 200DMA bull-gate ANYWHERE in crypto. During the Jul-8..10 bounce
                    // (all 8 majors +1.7..4.7% off the low) EVERY TSMOM/ICHI/BOLL signal
                    // was suppressed by this veto while the log mislabelled the block as
                    // "CLUSTER_GATE" (GateAttribution scored it SUSPECT — killed winners).
                    // g_macro_bull is still COMPUTED for telemetry (/status JSON,
                    // macro-base NAV, S55 grid sleeve) but gates NO slot entry.
                    // S-2026-07-05 UPJUMP rationale retained: the W-bar up-jump trigger
                    // IS the signal — broad market-direction vetoes are irrelevant to it.
                    // Genuine RISK caps stay in force: per-symbol + per-cluster
                    // concurrency, cluster 24h loss circuit-breaker, and the (non-200DMA)
                    // BTC/symbol regime chop-halt for non-UPJUMP kinds.
                    bool direction_ok    = (e->cfg().kind == chimera::StrategyKind::UPJUMP)
                                               ? true
                                               : (btc_regime_ok && sym_regime_ok);
                    // Honest gate attribution — pre-fix every suppression printed
                    // "CLUSTER_GATE: correlated exposure cap hit" regardless of cause.
                    const char* gate_name = nullptr; const char* gate_why = nullptr;
                    if      (!concurrency_ok)  { gate_name = "CLUSTER_GATE"; gate_why = "correlated exposure cap hit"; }
                    else if (!cluster_loss_ok) { gate_name = "CLUSTER_GATE"; gate_why = "cluster 24h loss circuit-breaker"; }
                    else if (!direction_ok)    { gate_name = "REGIME_GATE";
                                                 gate_why  = !btc_regime_ok ? "BTC regime below entry floor (chop-halt)"
                                                                            : "symbol regime below entry floor (chop-halt)"; }
                    e->set_cluster_gate(concurrency_ok && cluster_loss_ok && direction_ok,
                                        gate_name, gate_why);
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
                // S-2026-07-13 campaign books: same per-tick drive (H1 closes roll
                // internally; structural stops fire per tick like the BT's bar-low).
                for (auto* mgr : g_campaigns) {
                    if (chimera::symbol_to_id(mgr->config().symbol) != id) continue;
                    mgr->on_tick(mid, now_ms);
                }
                // Refresh the desk snapshot when a clip fired this tick, or on a
                // light 5s time-throttle so armed/peak/bank track intra-bar (the
                // desk panel polls ~15s). Avoids per-tick file I/O.
                static int64_t last_cc_emit_ms = 0;
                if (clips_after != clips_before || now_ms - last_cc_emit_ms >= 5000) {
                    last_cc_emit_ms = now_ms;
                    emit_companion_state();
                    save_campaign_state();   // durable window/campaign state (tiny file, throttled)
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

    // ── Phase-4 item 20: HONEST engine registry — reconcile declared lifecycle
    // vs the ACTUAL wired+connected graph, then generate the status count from
    // connected_count() (NOT the old hardcoded/aspirational "94 TSMOM …" banner
    // that overstated a graph of ~30 slots). Startup ABORTS on any mismatch
    // (e.g. a "LIVE" engine whose callback is disconnected). ── begin
    {
        bool wire_legacy = std::getenv("CHIMERA_WIRE_LEGACY") != nullptr;
        // Programmatic defaults (authoritative fallback) …
        g_registry.declare("EDGE-SLOTS", chimera::Lifecycle::SHADOW,
                           "REGIME_SWITCH D1 parents (g_slots) — TSMOM/ICHI book culled Phase-3 2026-07-13");
        g_registry.declare("LEGACY-EDGE", chimera::Lifecycle::DISABLED,
                           "285 per-symbol EdgeEngines — CULLED unless CHIMERA_WIRE_LEGACY");
        g_registry.declare("XSEC-BTC",   chimera::Lifecycle::SHADOW, "cross-sectional momentum, BTC-gated sleeve");
        g_registry.declare("XSEC-BR",    chimera::Lifecycle::SHADOW, "cross-sectional momentum, breadth-gated sleeve");
        g_registry.declare("RIPRIDER",   chimera::Lifecycle::SHADOW, "RipRider next-open sleeve");
        // KILL_UPJUMP_CLIPS (2026-07-13): the immediate-entry clip grid is disabled (g_grid_clip_count==0),
        // so declare it DISABLED to keep the honest registry consistent (an ACTIVE decl with no wired
        // callback aborts startup). Re-enabling the grid restores g_grid_clip_count>0 -> SHADOW again.
        g_registry.declare("UPJUMP-GRID",
                           g_grid_clip_count > 0 ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::DISABLED,
                           "companion grid — Phase-3 BE-entry mimics (REGIME_SWITCH parents) + S-2026-07-13 sweet-spot confirmed-entry cells BNB/UNI/NEAR (up-jump immediate-entry clips killed 2026-07-13)");
        // S-2026-07-13 campaign architecture (13j §2.11): guarded by the real
        // cell count so a zero-instance build declares DISABLED instead of
        // aborting validate() (same pattern as UPJUMP-GRID below).
        g_registry.declare("CAMPAIGN-MGR",
                           g_campaign_cell_count > 0 ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::DISABLED,
                           "virtual-lot parent campaigns (CostLedger+OpportunityGate+CampaignManager) — 4 PASS cells UNI-W1/W2 fused, TRX-W8, LDO-W8; mimic lots OFF");
        // EXECUTOR surfaces order-routing readiness HONESTLY without ever aborting
        // the shadow desk: SHADOW when the executor is ready, HALTED when creds
        // failed (sleeves still compute signals+books; only routing is a no-op).
        g_registry.declare("EXECUTOR", chimera::Lifecycle::SHADOW, "SpotExecutor order-routing readiness");
        // … operator override (config/engine_registry.json, if present) …
        int loaded = g_registry.load_from_json("config/engine_registry.json");
        // … then the env truth for the legacy layer wins regardless of the json.
        g_registry.set_state("LEGACY-EDGE",
                             wire_legacy ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::DISABLED);
        // PHASE3 (2026-07-13): runtime truth wins for the companion grid too. Its
        // population is a COMPILE-TIME fact (KILL_UPJUMP_CLIPS / Phase-3 mimic
        // registration), so a stale json override can only abort the desk — which it
        // did twice today (json SHADOW vs killed grid crash-loop on the old binary;
        // json DISABLED vs Phase-3 mimics abort on the new one). Same pattern as
        // LEGACY-EDGE above: the json may annotate, the wiring decides the state.
        g_registry.set_state("UPJUMP-GRID",
                             g_grid_clip_count > 0 ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::DISABLED);
        // Campaign books: wiring truth wins over any stale json, same pattern.
        g_registry.set_state("CAMPAIGN-MGR",
                             g_campaign_cell_count > 0 ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::DISABLED);
        // Executor-readiness reflected as a non-aborting HALTED when not ready.
        g_registry.set_state("EXECUTOR", exec_ok ? chimera::Lifecycle::SHADOW : chimera::Lifecycle::HALTED);
        // Runtime wiring truth. The XSec/RipRider/grid sleeves are installed +
        // seeded UNCONDITIONALLY (their blocks run regardless of exec_ok — only
        // order routing checks exec_ok), so they are wired+connected in shadow
        // independent of executor readiness. EDGE-SLOTS/UPJUMP-GRID reflect the
        // real container sizes.
        g_registry.mark_wired("EDGE-SLOTS",  !g_slots.empty(),                    (int)g_slots.size());
        g_registry.mark_wired("LEGACY-EDGE", wire_legacy && !g_all_wired.empty(), (int)g_all_wired.size());
        g_registry.mark_wired("XSEC-BTC",    true, 1);
        g_registry.mark_wired("XSEC-BR",     true, 1);
        g_registry.mark_wired("RIPRIDER",    true, 1);
        g_registry.mark_wired("UPJUMP-GRID", g_grid_clip_count > 0, g_grid_clip_count);
        g_registry.mark_wired("CAMPAIGN-MGR", g_campaign_cell_count > 0, g_campaign_cell_count);
        g_registry.mark_wired("EXECUTOR",    exec_ok, 1);
        std::string reg_err;
        if (!g_registry.validate(reg_err)) {
            std::fprintf(stderr, "[REGISTRY] STARTUP ABORT — declared vs actual mismatch: %s\n",
                         reg_err.c_str());
            return 1;
        }
        std::printf("[REGISTRY] loaded %d override(s) from config/engine_registry.json; reconcile PASS\n", loaded);
        g_registry.print_summary("[REGISTRY]");
        // Phase-4 item 21/22: configure the observability sinks (SHADOW, additive).
        g_gate_attr.configure(/*horizon*/ (int64_t)6 * 3600 * 1000, /*tp_bp*/ 0.0, /*sl_bp*/ 0.0);
        // Bound the per-signal store (oldest-first eviction) so a long shadow run
        // can't grow the counterfactual record set without limit (~15MB/month
        // unbounded). 20k records keeps a deep rolling window while the aggregated
        // per-gate stats are retained across eviction. Additive/observational.
        g_gate_attr.set_capacity(/*max_records*/ 20000);
        g_fill_realism.configure(chimera::FillModelParams{});   // Binance-spot-like defaults
        // Attach the gate-attribution sink to every CONNECTED EdgeEngine (g_slots
        // + any legacy-wired). Observational only — records each raw signal's
        // per-gate suppression reason + counterfactual + correlation-ID.
        {
            std::lock_guard<std::mutex> lk(g_engine_mtx);
            int attached = 0;
            for (auto& s : g_slots) if (s.engine) { s.engine->set_gate_sink(&g_gate_attr); ++attached; }
            for (auto* e : g_all_wired) if (e) { e->set_gate_sink(&g_gate_attr); ++attached; }
            std::printf("[GATE-ATTR] attached to %d connected engines (counterfactual horizon=6h, SHADOW)\n", attached);
        }
    }
    std::printf("[STARTUP] ════════════════════════════════════════════════════════\n");
    std::printf("[STARTUP] ✓ CHIMERA READY — %d engines connected (from the real graph; shadow_mode=true)\n",
                g_registry.connected_count());
    std::printf("[STARTUP]   EDGE-SLOTS=%d  LEGACY-EDGE=%d(wired)  XSEC=2  RIPRIDER=1  UPJUMP-GRID=%d\n",
                (int)g_slots.size(), (int)g_all_wired.size(), g_grid_clip_count);
    std::printf("[STARTUP]   Gates observed (item 21): portfolio+cluster+confirm+funding+vol_regime+corr+session+volume+vol_filter+mtf+adx\n");
    std::printf("[STARTUP]   spot-long-only | NO 200DMA | SHADOW | counts are RECONCILED (not aspirational)\n");
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
            // ── Phase-4 item 21: feed forward prices to resolve gate-attribution
            // counterfactuals (6h horizon; 60s cadence is ample), and print a
            // per-gate helpfulness summary every 10 min. Observational only.
            {
                std::lock_guard<std::mutex> lk(g_engine_mtx);
                for (auto& s : g_slots) {
                    if (!s.engine) continue;
                    double spot = load_dbl_atomic(g_last_spot_px_bits[s.symbol_id]);
                    if (spot > 0.0) g_gate_attr.on_price(s.symbol_str, spot, now_ms);
                }
            }
            static int64_t last_obs_summary_ms = 0;
            if (now_ms - last_obs_summary_ms >= 600000) {   // 10 min
                last_obs_summary_ms = now_ms;
                g_gate_attr.print_summary("[GATE-ATTR]");
                g_fill_realism.print_summary("[FILL-REALISM]");
            }
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
                // CH-06: owned funding worker — join the prior fetch (8h old, long done)
                // before launching the next; joined at shutdown. No detached overlap.
                if (g_funding_thread.joinable()) g_funding_thread.join();
                g_funding_thread = std::thread([](){ g_funding_filter.fetch_all(); });
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

                // ── Phase-8G: auto-CLEAR the user-stream heartbeat halt on
                // stream-resume + clean reconcile. Runs ONLY while the auto-halt
                // is armed — which happens ONLY on the live path (shadow never
                // arms, heartbeat_lapsed()==false), so this is a no-op in shadow.
                // When the live WS user-stream has resumed (heartbeat fresh
                // again) AND a reconcile re-agrees the ledger with the exchange,
                // the latch clears and entries resume. The full-account snapshot
                // fetch is the same LIVE-activation surface as the Phase-2 boot
                // reconcile (empty clean snapshot until the account fetch is wired).
                if (g_stream_halt.halted()) {
                    int64_t now_hb = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    if (!g_userstream.heartbeat_lapsed(now_hb, g_stream_halt.threshold_ms())) {
                        chimera::StartupReconciler rec8g;
                        chimera::ExchangeSnapshot snap8g; snap8g.ok = true;   // LIVE: fill from account fetch
                        auto rr8g = rec8g.reconcile(snap8g, g_ledger, &g_idreg, now_hb);
                        g_stream_halt.on_reconcile(rr8g.passed);
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
                // CH-03 (audit 2026-07-13): this gate needs an active BTC *D1 trend* slot as
                // its reference. Every D1 BTC slot is currently culled, so btc_momentum stays
                // 0 and the gate NEVER fires — a protection that LOOKS enabled but is inert.
                // ONE-SHOT visibility so this dead state is on the record (fixing the trap the
                // audit flagged). ENABLING the gate (rebuild reference + backtest the alt
                // suppression) is a behaviour change owed a separate verdict — NOT done here.
                {
                    static bool corr_state_announced = false;
                    if (!corr_state_announced) {
                        bool have_btc_d1_ref = false;
                        for (auto& s : g_slots)
                            if (s.engine && s.symbol_id == chimera::SYM_BTC && s.tf_secs == 86400
                                && s.engine->is_trend_following()) { have_btc_d1_ref = true; break; }
                        std::printf("[CORR-GATE] reference %s — alt-suppression %s\n",
                                    have_btc_d1_ref ? "LIVE (BTC-D1 slot present)" : "INACTIVE (no BTC-D1 slot; gate cannot fire)",
                                    have_btc_d1_ref ? "armed" : "OFF");
                        std::fflush(stdout);
                        corr_state_announced = true;
                    }
                }
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
                            try { btc_momentum = std::stod(state.substr(pos + 15, 14)); }  // CH-04 (audit 2026-07-13): key "momentum_pct": is 15 chars; +16 dropped the first digit, flipping -6.2 -> 6.2
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
                                try { sym_mom = std::stod(state.substr(pos + 15, 14)); }  // CH-04 (audit 2026-07-13): +16 -> +15 (see BTC site)
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
    if (g_funding_thread.joinable()) g_funding_thread.join();  // CH-06: join owned worker before teardown

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
