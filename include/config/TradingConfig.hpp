#pragma once
#include <cstdint>

namespace chimera {

// ============================================================================
// CHIMERA TRADING ENGINE — CENTRAL CONFIGURATION
// ============================================================================
//
// ALL constants calibrated to measured VPS performance.
// DO NOT change latency values without re-running tools/ping_binance.sh.
//
// MEASURED LATENCY (Tokyo VPS 154.45.251.118 → Binance AWS Tokyo):
//   ICMP api.binance.com:   2ms
//   TCP connect:            4-15ms
//   REST RTT p95:           36-38ms  (one 58ms spike)
//   WS first message:       371ms    (TLS handshake only)
//   WS ongoing feed p95:    18-25ms  (real data latency)
//   One-way clock offset:   ~18ms
//   Route:                  4 hops via Cogent Tokyo (optimal)
//
// COST STRUCTURE (Binance spot taker):
//   Fee per side:           0.04% = 4bp
//   Round trip fees:        8bp
//   Spread (BTC):           ~0.5bp each side = 1bp
//   Slippage estimate:      1-2bp
//   TOTAL ROUND TRIP COST:  ~10-12bp
//
//   Minimum gross edge to be profitable: 10bp per trade
//
// ============================================================================

struct TradingConfig {

    // -------------------------------------------------------------------------
    // LATENCY LIMITS — calibrated to Tokyo VPS measurements
    // -------------------------------------------------------------------------
    // Never change these without re-running ping_binance.sh on the VPS.
    // All strategies check latency before firing. No strategy fires above HARD_LIMIT.

    // Hard cutoff: above this, all signals blocked
    // = WS p95 (25ms) + 2x buffer for spikes. Any higher = genuine congestion.
    static constexpr double LATENCY_HARD_LIMIT_MS  = 50.0;

    // Net-clean threshold: below = full size trades allowed
    // = WS p95 upper bound + small buffer. Above = reduce size or skip.
    static constexpr double LATENCY_NET_CLEAN_MS   = 30.0;

    // Lead-lag maximum: BTC→ETH/SOL propagation window shrinks with latency
    // At 35ms we still have 15-165ms of the 50-200ms propagation window remaining
    // Above 35ms the edge window is too uncertain to trade reliably
    static constexpr double LATENCY_LEADLAG_MAX_MS = 35.0;

    // Imbalance signal max: requires very fresh book data to be reliable
    // Only fire imbalance trades when latency is well below 25ms p95
    static constexpr double LATENCY_IMBALANCE_MAX_MS = 25.0;


    // -------------------------------------------------------------------------
    // COST FLOOR — minimum edge to beat round-trip cost
    // -------------------------------------------------------------------------
    // Taker fees (8bp) + spread (1bp) + slippage (1bp) = ~10bp per round trip.
    // Any signal with expected edge below this is a guaranteed loser.
    static constexpr double COST_FLOOR_BP = 10.0;


    // -------------------------------------------------------------------------
    // LEAD-LAG SIGNAL PARAMETERS
    // -------------------------------------------------------------------------
    // Edge: BTC moves, ETH/SOL follow 50-200ms later.
    // Our WS latency is 18-25ms p95 → we receive BTC move with 25ms delay.
    // Remaining edge window: 25-175ms. Conservative target: 75ms usable window.
    //
    // TP/SL calibrated so expected value is positive after 10bp cost:
    //   TP = +14bp, SL = -5bp
    //   If win rate >= 44%: EV = 0.44*14 - 0.56*5 = 6.16 - 2.8 = +3.36bp net
    //   (after deducting 10bp costs from TP: net_win = 14-10 = +4bp)
    //   Minimum win rate for positive EV: ~72% (accounting for full costs)
    //   → Only fire when BTC move >= 12bp AND target hasn't moved yet

    // BTC must move at least this many bp in the lookback window to signal
    static constexpr double LEADLAG_BTC_THRESHOLD_BP  = 12.0;

    // Target already moved this much → edge consumed, don't enter
    static constexpr double LEADLAG_TARGET_MAX_BP      = 4.0;

    // Take-profit for lead-lag trades (gross, before costs)
    // Net profit after 10bp costs = +4bp. Worth it if win rate > 70%.
    static constexpr double LEADLAG_TP_BP              = 14.0;

    // Stop loss for lead-lag trades
    // Tight stop — if ETH/SOL doesn't follow BTC within 5s, exit
    static constexpr double LEADLAG_SL_BP              = 5.0;

    // Maximum hold time for lead-lag before forced flat
    // Propagation completes within ~200ms. 3s is generous timeout.
    static constexpr int64_t LEADLAG_MAX_HOLD_MS       = 3000;


    // -------------------------------------------------------------------------
    // IMBALANCE SIGNAL PARAMETERS (GRIND regime)
    // -------------------------------------------------------------------------
    // Edge: strong order book imbalance predicts 1-3 tick direction.
    // Only valid when latency < 25ms (fresh data) and spread is tight.
    //
    // TP/SL calibrated for positive EV after 10bp cost:
    //   TP = +12bp, SL = -3bp
    //   Need win rate >= 79%: 0.79*2 - 0.21*3 = 1.58 - 0.63 = +0.95bp net
    //   → Only fire on very strong imbalance (0.45 threshold)
    //   → Only fire when spread < 1.5bp (tight market = good fills)

    // Minimum |book_imbalance| to fire: (bid_size - ask_size)/(bid_size + ask_size)
    // 0.45 = bids must be 2.6x asks. Very strong pressure. Filters noise well.
    static constexpr double IMBALANCE_THRESHOLD        = 0.45;

    // Reject if spread too wide — wide spread means fills are poor
    // BTC normal spread: 0.1-0.5bp. Above 1.5bp = unusually wide, skip.
    static constexpr double IMBALANCE_MAX_SPREAD_BPS   = 1.5;

    // Take-profit for imbalance trades (gross)
    // Net after 10bp costs = +2bp. Needs very high win rate.
    static constexpr double IMBALANCE_TP_BP            = 12.0;

    // Stop loss for imbalance trades
    // Wrong-side imbalance reverses fast. Get out at -3bp.
    static constexpr double IMBALANCE_SL_BP            = 3.0;

    // Max hold time for imbalance trade
    static constexpr int64_t IMBALANCE_MAX_HOLD_MS     = 8000;


    // -------------------------------------------------------------------------
    // BREAKOUT / IMPULSE SIGNAL PARAMETERS
    // -------------------------------------------------------------------------
    // Edge: vol_ratio > 1.95 signals genuine breakout move.
    // Our latency (18-25ms) is fast enough to catch early breakouts.
    //
    // TP wide enough to justify being a taker: net 10bp+ after costs
    static constexpr double IMPULSE_TP_BP              = 20.0;
    static constexpr double IMPULSE_SL_BP              = 8.0;
    static constexpr int64_t IMPULSE_MAX_HOLD_MS       = 30000;

    // Minimum ticks in short window before impulse fires
    static constexpr int IMPULSE_MIN_SHORT_TICKS       = 5;


    // -------------------------------------------------------------------------
    // REGIME CLASSIFICATION THRESHOLDS
    // -------------------------------------------------------------------------
    // vol_ratio = short_vol / long_vol (both are log-return stddev)
    // Hysteresis bands prevent thrashing at boundaries.

    static constexpr double REGIME_DEAD_ENTER               = 0.60;
    static constexpr double REGIME_DEAD_EXIT                = 0.90;
    static constexpr double REGIME_GRIND_ENTER_FROM_DEAD    = 0.90;
    static constexpr double REGIME_GRIND_EXIT_TO_DEAD       = 0.75;
    static constexpr double REGIME_GRIND_EXIT_TO_BUILDUP    = 1.55;
    static constexpr double REGIME_BUILDUP_ENTER            = 1.55;
    static constexpr double REGIME_BUILDUP_EXIT             = 0.95;
    static constexpr double REGIME_BUILDUP_TO_BREAKOUT      = 1.95;
    static constexpr double REGIME_BREAKOUT_ENTER           = 1.95;
    static constexpr double REGIME_BREAKOUT_EXIT            = 1.55;
    static constexpr int    MIN_REGIME_TICKS                = 30;
    static constexpr double REGIME_MIN_LONG_AVG             = 0.004;


    // -------------------------------------------------------------------------
    // VOLATILITY CALCULATION
    // -------------------------------------------------------------------------
    static constexpr int    SHORT_VOL_WINDOW        = 20;
    static constexpr int    LONG_VOL_WINDOW         = 200;  // deprecated, using EMA
    static constexpr double LONG_VOL_EMA_ALPHA      = 0.06;
    static constexpr double VOL_RATIO_EMA_ALPHA     = 0.12;
    static constexpr double VOL_MIN_LONG            = 1e-8;

    // Absolute volatility floor: below this, no trade is profitable
    // log-return stddev < 0.00001 means market is not moving enough to beat costs
    static constexpr double MIN_LONG_VOL_FOR_TRADING = 0.000010;


    // -------------------------------------------------------------------------
    // EXPANSION LAYER
    // -------------------------------------------------------------------------
    static constexpr double EXPANSION_VOL_RATIO       = 1.125;
    static constexpr int    EXPANSION_MIN_SHORT_TICKS = 8;


    // -------------------------------------------------------------------------
    // EXIT MANAGEMENT — shared across strategies
    // -------------------------------------------------------------------------

    // Trailing stop multiplier for non-micro strategies:
    // trail_distance = TRAIL_MULT * long_vol * price  (correct price units)
    // At BTC=$85k, long_vol=0.0003: trail = 2.5 * 0.0003 * 85000 = $63.75 = ~7.5bp
    static constexpr double TRAIL_LONG_VOL_MULT        = 2.5;

    // Minimum profit before trailing stop activates (don't trail below costs)
    static constexpr double MIN_PROFIT_TO_TRAIL_BP     = 5.0;

    // Minimum ticks to hold before any exit allowed (prevent instant exits)
    static constexpr int    MIN_HOLD_TICKS             = 3;

    // Minimum displacement from regime anchor before entry allowed
    static constexpr double MIN_DISPLACEMENT_LONG_MULT = 0.75;


    // -------------------------------------------------------------------------
    // DIAGNOSTIC OUTPUT
    // -------------------------------------------------------------------------
    static constexpr int REGIME_DIAG_INTERVAL     = 500;
    static constexpr int SYMBOL_STATE_INTERVAL    = 500;
};

} // namespace chimera
