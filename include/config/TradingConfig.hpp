#pragma once
#include <cstdint>

namespace chimera {

// ============================================================================
// CHIMERA TRADING ENGINE  CENTRAL CONFIGURATION
// ============================================================================
//
// ALL constants calibrated to measured VPS performance.
// DO NOT change latency values without re-running tools/ping_binance.sh.
//
// MEASURED LATENCY (Tokyo VPS 154.45.251.118  Binance AWS Tokyo):
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
    // LATENCY LIMITS  calibrated to Tokyo VPS measurements
    // -------------------------------------------------------------------------
    // Never change these without re-running ping_binance.sh on the VPS.
    // All strategies check latency before firing. No strategy fires above HARD_LIMIT.

    // Hard cutoff: above this, all signals blocked
    // = WS p95 (25ms) + 2x buffer for spikes. Any higher = genuine congestion.
    static constexpr double LATENCY_HARD_LIMIT_MS  = 100.0; // data age: block only genuine feed lag (>100ms)

    // Net-clean threshold: below = full size trades allowed
    // = WS p95 upper bound + small buffer. Above = reduce size or skip.
    static constexpr double LATENCY_NET_CLEAN_MS   = 60.0;  // data age p95=36ms, was cutting size on every normal tick

    // Lead-lag maximum: BTCETH/SOL propagation window shrinks with latency
    // At 35ms we still have 15-165ms of the 50-200ms propagation window remaining
    // Above 35ms the edge window is too uncertain to trade reliably
    static constexpr double LATENCY_LEADLAG_MAX_MS = 80.0;  // data age: p95=36ms was permanently blocking all leadlag

    // Imbalance signal max: requires very fresh book data to be reliable
    // Only fire imbalance trades when latency is well below 25ms p95
    static constexpr double LATENCY_IMBALANCE_MAX_MS = 60.0; // data age: p50=25ms was blocking ~50% of imbalance ticks


    // -------------------------------------------------------------------------
    // COST FLOOR  minimum edge to beat round-trip cost
    // -------------------------------------------------------------------------
    // Taker fees (8bp) + spread (1bp) + slippage (1bp) = ~10bp per round trip.
    // Any signal with expected edge below this is a guaranteed loser.
    static constexpr double COST_FLOOR_BP = 12.0;        // taker round trip floor (LEADLAG/IMPULSE etc)
    static constexpr double EXPANSION_COST_FLOOR_BP = 8.0; // BUG10: EXPANSION net-negative at 4bp floor, raise to 8bp


    // -------------------------------------------------------------------------
    // LEAD-LAG SIGNAL PARAMETERS
    // -------------------------------------------------------------------------
    // Edge: BTC moves, ETH/SOL follow 50-200ms later.
    // Our WS latency is 18-25ms p95  we receive BTC move with 25ms delay.
    // Remaining edge window: 25-175ms. Conservative target: 75ms usable window.
    //
    // TP/SL calibrated so expected value is positive after 10bp cost:
    //   TP = +14bp, SL = -5bp
    //   If win rate >= 44%: EV = 0.44*14 - 0.56*5 = 6.16 - 2.8 = +3.36bp net
    //   (after deducting 10bp costs from TP: net_win = 14-10 = +4bp)
    //   Minimum win rate for positive EV: ~72% (accounting for full costs)
    //    Only fire when BTC move >= 12bp AND target hasn't moved yet

    // BTC must move at least this many bp in the lookback window to signal
    // Raised 810bp: at 8bp too many weak moves were triggering. 10bp filters
    // for genuine momentum that ETH/SOL reliably follows.
    static constexpr double LEADLAG_BTC_THRESHOLD_BP  = 5.0;   // crypto spot needs more signals; still gated by flow/book confirmation
    // LEADLAG confirmation gates (added Mar 2026)
    // OB ratio: bid_size/ask_size must exceed this — filters neutral/bearish book
    static constexpr double LEADLAG_CONFIRM_OB_RATIO   = 1.08; // bid 8% > ask
    // Flow ratio: buy_vol_ema/sell_vol_ema must exceed this — confirms buy aggression
    static constexpr double LEADLAG_CONFIRM_FLOW_RATIO  = 1.04; // buy flow 4% > sell

    // Target already moved this much  edge consumed, don't enter
    // Tightened 32bp: if target already moved 2bp the propagation is done
    static constexpr double LEADLAG_TARGET_MAX_BP      = 3.0;

    // Take-profit for lead-lag trades (gross, before costs)
    // Net profit after 10bp costs = +4bp. Worth it if win rate > 70%.
    // ETH  SOL LEAD-LAG
    // ETH leads SOL by ~30-80ms. Smaller move threshold than BTCETH/SOL.
    // Long-only, spot-valid. Fires on SOL only.
    // Raised ETH threshold 812bp: at 8bp too many small ETH moves that SOL ignores.
    // Hold tightened 25001500ms: if SOL hasn't moved in 1.5s, the propagation is done.
    // Flow confirm added: requires SOL buy pressure to confirm ETH signal not yet absorbed.
    static constexpr double  LEADLAG_ETH_SOL_THRESHOLD_BP = 9.0; // min ETH move to signal
    static constexpr double  LEADLAG_ETH_SOL_TP_BP        = 10.0; // if we pay taker cost the move must be worth taking
    static constexpr double  LEADLAG_ETH_SOL_SL_BP        = 3.0;
    static constexpr int64_t LEADLAG_ETH_SOL_MAX_HOLD_MS  = 3000;

    static constexpr double LEADLAG_TP_BP              = 12.0;   // fast momentum must clear taker cost, not just maker cost

    // Stop loss for lead-lag trades
    // Tight stop  if ETH/SOL doesn't follow BTC within 5s, exit
    static constexpr double LEADLAG_SL_BP              = 3.0;

    // Maximum hold time for lead-lag before forced flat
    // Extended: trades need time to reach 8bp TP
    static constexpr int64_t LEADLAG_MAX_HOLD_MS       = 5000;

    // -------------------------------------------------------------------------
    // LIQUIDATION CASCADE ENGINE  spot long on short liquidations from perp
    // -------------------------------------------------------------------------
    // Short liquidation on perp = forced buy on perp = spot follows up 50-200ms later
    // Min notional filters noise  only meaningful liquidations move spot
    static constexpr double  LIQ_MIN_NOTIONAL_USD   = 350000.0; // stricter: ignore small liquidation noise
    static constexpr double  LIQ_MIN_NOTIONAL_ALT_USD = 500000.0; // alts are noisier than BTC/ETH/SOL; require larger cascade
    static constexpr double  LIQ_SPOT_MOVED_MAX_BP  = 2.5;      // tighter anti-chase window
    static constexpr int64_t LIQ_SIGNAL_WINDOW_MS   = 500;      // reclaim-style maker entry needs a slightly wider validation window
    static constexpr int64_t LIQ_COOLDOWN_MS        = 10000;    // avoid repeated liq stabs in chop
    static constexpr double  LIQ_MAX_SPREAD_BPS     = 2.0;      // don't chase liquidation when book widens
    static constexpr double  LIQ_RECLAIM_MIN_BP     = 0.35;     // require an initial local reclaim before posting maker
    static constexpr double  LIQ_MIN_FLOW_RATIO     = 0.56;     // require aggressive buy flow confirmation
    static constexpr double  LIQ_MIN_BOOK_IMBALANCE = 0.08;     // require at least mild bid pressure
    static constexpr double  LIQ_MIN_VOL_RATIO      = 0.90;     // avoid dead tape
    static constexpr double  LIQ_MAX_VOL_RATIO      = 2.40;     // avoid panic blow-off fills
    static constexpr double  LIQ_TP_BP              = 10.0;     // smaller target to reduce timeout drift
    static constexpr double  LIQ_SL_BP              = 3.0;      // tighter loss cap: reduce -12bp style hits
    static constexpr int64_t LIQ_MAX_HOLD_MS        = 3000;     // faster invalidation if continuation doesn't appear


    // -------------------------------------------------------------------------
    // IMBALANCE SIGNAL PARAMETERS (GRIND regime)
    // -------------------------------------------------------------------------
    // Edge: strong order book imbalance predicts 1-3 tick direction.
    // Only valid when latency < 25ms (fresh data) and spread is tight.
    //
    // TP/SL calibrated for positive EV after 10bp cost:
    //   TP = +12bp, SL = -3bp
    //   Need win rate >= 79%: 0.79*2 - 0.21*3 = 1.58 - 0.63 = +0.95bp net
    //    Only fire on very strong imbalance (0.45 threshold)
    //    Only fire when spread < 1.5bp (tight market = good fills)

    // Minimum |book_imbalance| to fire: (bid_size - ask_size)/(bid_size + ask_size)
    // 0.42 = bids must be 2.45x asks. Restored toward original 0.45  at 0.30 the
    // signal fired on noise and WR dropped to ~50%. Need 79%+ WR at this TP/SL.
    static constexpr double IMBALANCE_THRESHOLD        = 0.34;

    // Reject if spread too wide  wide spread means fills are poor
    // BTC normal spread: 0.1-0.5bp. Above 1.5bp = unusually wide, skip.
    static constexpr double IMBALANCE_MAX_SPREAD_BPS   = 1.5;

    // Take-profit for imbalance trades (gross)
    // Net after 10bp costs = +2bp. Needs very high win rate.
    static constexpr double IMBALANCE_TP_BP            = 12.0;

    // Stop loss for imbalance trades
    // 3bp was too tight  getting stopped by tick noise. 4bp gives trade more room
    // while still exiting genuine reversals quickly.
    static constexpr double IMBALANCE_SL_BP            = 4.0;

    // Max hold time for imbalance trade
    static constexpr int64_t IMBALANCE_MAX_HOLD_MS     = 8000;


    // -------------------------------------------------------------------------
    // BREAKOUT / IMPULSE SIGNAL PARAMETERS
    // -------------------------------------------------------------------------
    // Edge: vol_ratio > 1.95 signals genuine breakout move.
    // Our latency (18-25ms) is fast enough to catch early breakouts.
    //
    // TP wide enough to justify being a taker: net 10bp+ after costs
    static constexpr double IMPULSE_TP_BP              = 14.0;  // BTC/SOL: shallow moves, 10bp is ceiling
    static constexpr double IMPULSE_ALT_TP_BP          = 20.0;  // AVAX/LINK/POL: thin books, moves run further (AVAX hit 10.5bp with zero resistance)
    static constexpr double IMPULSE_SL_BP              = 5.0;   // tightened 75bp: 9 SL hits all -7bp, 7/9 had MFE<0.6 (never moved right)  bad entries not noise. Cuts 2bp per SL hit.
    static constexpr int64_t IMPULSE_MAX_HOLD_MS       = 20000; // extended 1520s: timeouts suggest moves need more time

    // EXPANSION has own tighter parameters  weaker edge than IMPULSE
    static constexpr double EXPANSION_TP_BP             = 10.0;   // BTC: moves are shallow, 6bp is correct ceiling
    static constexpr double EXPANSION_ALT_TP_BP         = 25.0;  // LINK/AVAX/POL: thin books, moves run 11-22bp consistently (3 trades: +22, +11, +22bp)
    static constexpr double EXPANSION_SL_BP             = 3.0;   // tightened 5->3bp: avg SL loss was -8bp, tighter with higher vol filter
    static constexpr int64_t EXPANSION_MAX_HOLD_MS      = 8000;  // tightened 12->8s: cut dead trades faster, timeout losses were drift

    // Minimum ticks in short window before impulse fires
    static constexpr int IMPULSE_MIN_SHORT_TICKS       = 5;

    // -------------------------------------------------------------------------
    // LIQUIDITY VACUUM ENGINE
    // Edge: ask-side depth drains >40% in 2 ticks  price gaps up through vacuum
    // Spot-only long: buy when ask wall disappears before price moves
    // -------------------------------------------------------------------------
    static constexpr double VACUUM_ASK_DRAIN_RATIO     = 0.30;  // ask depth drops 30%+
    static constexpr double VACUUM_MIN_IMBALANCE       = 0.12;  // bid must be present
    static constexpr double VACUUM_MAX_SPREAD_BPS      = 2.0;   // don't enter wide spreads
    static constexpr double VACUUM_TP_BP               = 16.0;
    static constexpr double VACUUM_SL_BP               = 6.0;
    static constexpr int64_t VACUUM_MAX_HOLD_MS        = 12000;
    static constexpr double LATENCY_VACUUM_MAX_MS      = 60.0;  // data age calibrated: old 30ms blocked p95 ticks

    // -------------------------------------------------------------------------
    // VWAP REVERSION ENGINE
    // Edge: in GRIND regime, price >20bp below session VWAP + bid imbalance = buy
    // Mean reversion back toward VWAP. High win rate in ranging markets.
    // -------------------------------------------------------------------------
    static constexpr double VWAP_ENTRY_DEVIATION_BP    = 16.0;  // min distance below VWAP
    static constexpr double VWAP_MAX_DEVIATION_BP      = 80.0;  // too far = trending, skip
    static constexpr double VWAP_MIN_IMBALANCE         = 0.10;  // bid pressure must confirm
    static constexpr int    VWAP_MIN_TRADE_SAMPLES     = 8;     // require real trade prints before VWAP is tradable
    static constexpr double VWAP_MAX_SPREAD_BPS        = 2.0;
    static constexpr double VWAP_TP_BP                 = 12.0;
    static constexpr double VWAP_SL_BP                 = 4.5;
    static constexpr int64_t VWAP_MAX_HOLD_MS          = 12000; // avoid long timeout bleed
    static constexpr double LATENCY_VWAP_MAX_MS        = 50.0;  // not latency sensitive

    // -------------------------------------------------------------------------
    // EDGE PROMOTION / DEMOTION GATES
    // -------------------------------------------------------------------------
    // Layers start either enabled or parked (set in BalancedEngine).
    // Parked layers are only promoted after proving edge on rolling shadow sample.
    static constexpr int    EDGE_WINDOW_TRADES           = 30;
    static constexpr int    EDGE_PROMOTE_MIN_TRADES      = 30;
    static constexpr int    EDGE_DEMOTE_MIN_TRADES       = 20;
    static constexpr double EDGE_PROMOTE_MIN_AVG_PNL_BP  = 0.8;
    static constexpr double EDGE_PROMOTE_MAX_TIMEOUT_RT  = 0.55;
    static constexpr double EDGE_PROMOTE_MFE_BUFFER_BP   = 1.0; // p50 MFE must exceed cost + buffer
    static constexpr double EDGE_DEMOTE_AVG_PNL_BP       = -0.5;
    static constexpr int64_t EDGE_DISABLE_MS             = 30 * 60 * 1000LL; // 30 minutes


    // -------------------------------------------------------------------------
    // REGIME CLASSIFICATION THRESHOLDS
    // -------------------------------------------------------------------------
    // vol_ratio = short_vol / long_vol (both are log-return stddev)
    // Hysteresis bands prevent thrashing at boundaries.

    static constexpr double REGIME_DEAD_ENTER               = 0.60;
    static constexpr double REGIME_DEAD_EXIT                = 0.90;
    static constexpr double REGIME_GRIND_ENTER_FROM_DEAD    = 0.90;
    static constexpr double REGIME_GRIND_EXIT_TO_DEAD       = 0.75;
    static constexpr double REGIME_GRIND_EXIT_TO_BUILDUP    = 1.35;
    static constexpr double REGIME_BUILDUP_ENTER            = 1.35;
    static constexpr double REGIME_BUILDUP_EXIT             = 0.95;
    static constexpr double REGIME_BUILDUP_TO_BREAKOUT      = 1.65;
    static constexpr double REGIME_BREAKOUT_ENTER           = 1.65;
    static constexpr double REGIME_BREAKOUT_EXIT            = 1.35;
    static constexpr int    MIN_REGIME_TICKS                = 12;
    static constexpr int    EXPAND_POST_COMPRESS_LOCKOUT  = 3;   // block EXPAND for N ticks after COMPRESSIONBREAKOUT transition (regime lag filter)
    static constexpr double REGIME_MIN_LONG_AVG             = 0.004;

    // Aliases used by RegimeClassifier::classify_regime()
    // Maps to the hysteresis enter thresholds above
    static constexpr double REGIME_DEAD_THRESHOLD           = REGIME_DEAD_EXIT;           // 0.90
    static constexpr double REGIME_GRIND_THRESHOLD          = REGIME_GRIND_EXIT_TO_BUILDUP; // 1.35
    static constexpr double REGIME_BUILDUP_THRESHOLD        = REGIME_BUILDUP_TO_BREAKOUT; // 1.65


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
    // Raised vol_ratio 1.1251.55: at 1.125 fires on weak BUILDUP noise.
    // 1.55 = genuine expansion, not just minor vol uptick.
    // BREAKOUT-only enforced in check_expansion (removed BUILDUP).
    // -------------------------------------------------------------------------
    static constexpr double EXPANSION_VOL_RATIO       = 1.60;  // slightly earlier breakout participation on thin alts
    static constexpr int    EXPANSION_CONFIRM_TICKS   = 2;
    static constexpr int    EXPANSION_MIN_SHORT_TICKS = 10;


    // -------------------------------------------------------------------------
    // EXIT MANAGEMENT  shared across strategies
    // -------------------------------------------------------------------------

    // Trailing stop multiplier for non-micro strategies:
    // trail_distance = TRAIL_MULT * long_vol * price  (correct price units)
    // At BTC=$85k, long_vol=0.0003: trail = 2.5 * 0.0003 * 85000 = $63.75 = ~7.5bp
    static constexpr double TRAIL_LONG_VOL_MULT        = 3.0;  // loosened  was cutting 12bp winner at 4bp

    // Minimum profit before trailing stop activates
    // IMPULSE TP=20bp  trail must not arm until trade has real room.
    // Previous 1.5bp was cutting winners at 1-3bp; 0 TP hits in 24 trades.
    // Raise to 10bp: let IMPULSE run, hard SL=4bp protects the downside.
    static constexpr double MIN_PROFIT_TO_TRAIL_BP     = 4.0;  // arm at 4bp  IMPULSE TP=10bp, SL=7bp: protect gains once we clear 40% of TP

    // Minimum ticks to hold before any exit allowed (prevent instant exits)
    static constexpr int    MIN_HOLD_TICKS             = 3;

    // Minimum displacement from regime anchor before entry allowed
    static constexpr double MIN_DISPLACEMENT_LONG_MULT = 0.75;


    // -------------------------------------------------------------------------
    // ORDER FLOW CONFIRMATION
    // -------------------------------------------------------------------------
    // agg_buy_volume / (agg_buy + agg_sell) must exceed this to confirm a long.
    // 0.55 = slight buy majority. 0.60 = clear buy pressure.
    // Filters breakouts where smart money is actually selling into retail buyers.
    // Set to 0.0 to disable (useful when testing new signals).
    static constexpr double FLOW_CONFIRM_THRESHOLD = 0.55;

    // Secondary continuation confirmations reused from the old book-only stack.
    // These are now context filters, not standalone entry engines.
    static constexpr int    CONTINUATION_CONFIRM_MIN_COUNT      = 1;
    static constexpr int    FAST_CONTINUATION_CONFIRM_MIN_COUNT = 2;
    static constexpr int    VOLSHOCK_CONFIRM_MIN_COUNT          = 1;
    static constexpr int    OVERLAY_CONFIRM_MIN_COUNT           = 1;
    static constexpr int    EXPERIMENTAL_CONFIRM_MIN_COUNT      = 2;
    static constexpr double CONTINUATION_OFI_RATIO_MIN          = 0.10;
    static constexpr double CONTINUATION_FLOW_MIN               = 0.53;
    static constexpr double CONTINUATION_BOOK_IMBAL_MIN         = 0.05;
    static constexpr double CONTINUATION_VWAP_MAX_UNDERWATER_BP = 12.0;
    static constexpr double CONTINUATION_VACUUM_DRAIN_RATIO     = 0.25;
    static constexpr double CONTINUATION_DEPTH_RATIO_MIN        = 0.80;
    static constexpr double CONTINUATION_DEPTH_IMBAL_MIN        = 0.08;
    static constexpr double CONTINUATION_DEPTH_FLOW_MIN         = 0.53;

    // Default engine mix for the live-data paper stack.
    static constexpr bool ENABLE_PRIMARY_VOLSHOCK   = true;
    static constexpr bool ENABLE_PRIMARY_IMPULSE    = true;
    static constexpr bool ENABLE_PRIMARY_LEADLAG    = true;
    static constexpr bool ENABLE_STANDALONE_EXPAND  = false;
    static constexpr bool ENABLE_FUNDING_OVERLAY    = true;
    static constexpr bool ENABLE_NGAS_OVERLAY       = false;
    static constexpr bool ENABLE_LIQ_PAPER_ONLY     = true;

    // Paper-only research mode:
    // after a prolonged dry spell, relax the continuation stack modestly and
    // allow the experimental fallback stack to sample trades. This only applies
    // in shadow mode and leaves live behavior unchanged.
    static constexpr int64_t PAPER_RESEARCH_IDLE_MS                = 2 * 60 * 1000LL;
    static constexpr double  PAPER_RESEARCH_FLOW_CONFIRM_THRESHOLD = 0.51;
    static constexpr double  PAPER_RESEARCH_DISPLACEMENT_MULT      = 0.55;
    static constexpr double  PAPER_RESEARCH_EXPANSION_VOL_RATIO    = 1.45;
    static constexpr double  PAPER_RESEARCH_LEADLAG_OB_RATIO       = 1.04;
    static constexpr double  PAPER_RESEARCH_LEADLAG_FLOW_RATIO     = 1.01;
    static constexpr double  PAPER_RESEARCH_DEPTH_RATIO_MIN        = 0.70;
    static constexpr double  PAPER_RESEARCH_DEPTH_IMBAL_MIN        = 0.05;

    // -------------------------------------------------------------------------
    // MAKER ORDER MODE
    // -------------------------------------------------------------------------
    // true  = post limit orders (maker rebate ~1bp/side = ~4bp round trip)
    // false = market orders    (taker fee  ~4bp/side = ~10bp round trip)
    // Saving: ~6bp per trade. This is the single biggest lever available.
    static constexpr bool MAKER_MODE = true;

    // -------------------------------------------------------------------------
    // MAKER LIMIT ORDER PARAMETERS
    // -------------------------------------------------------------------------

    // Cancel if ask rises this many bp above our limit (move happened without us)
    static constexpr double MAKER_STALE_BP = 3.0;

    // Timeouts per strategy  lead-lag has the tightest window
    // IMBALANCE: 5s    buy pressure can persist, patient fill is fine
    // LEAD-LAG:  200ms  edge window is ~75ms remaining, must fill fast or skip
    // IMPULSE:   500ms  breakout fills fast or not at all
    static constexpr int64_t MAKER_IMBALANCE_TIMEOUT_MS = 5000;
    static constexpr int64_t MAKER_LEADLAG_TIMEOUT_MS   =  200;
    static constexpr int64_t MAKER_IMPULSE_TIMEOUT_MS   =  500;

    // Cost floor recalibrated for maker fees:
    // Maker fee: ~1bp/side rebate = -2bp total
    // Spread: 0bp (we are the spread, not crossing it)
    // Slippage: ~0.5bp (limit fills at our price, minimal slip)
    // Total maker round-trip cost: ~3-4bp
    // Use 4bp as conservative floor.
    static constexpr double MAKER_COST_FLOOR_BP = 4.0;
    // Round-trip costs used in exit() net PnL calculation (BUG4 FIX)
    static constexpr double TAKER_ROUND_TRIP_BP = 8.0;  // 4bp/side VIP0 taker fee (IMPULSE/ETH-LEAD when active)
    static constexpr double MAKER_ROUND_TRIP_BP = 4.0;  // ~1bp rebate/side + ~2bp spread
    // Research mode now assumes maker entry with market-style exit until a
    // full maker-exit lifecycle is implemented.
    static constexpr double MAKER_ENTRY_MARKET_EXIT_BP = 6.5;
    static constexpr double MAKER_ENTRY_MARKET_EXIT_COST_FLOOR_BP = 8.0;

    // -------------------------------------------------------------------------
    // FUNDING RATE SIGNAL ENGINE  spot long when shorts crowded on perp
    // -------------------------------------------------------------------------
    // When funding rate is deeply negative, shorts are paying longs.
    // Shorts are overstretched  spot gets bought as longs collect carry.
    // This is a slow-burn multi-hour directional trade, not a scalp.
    //
    // Entry: funding < -0.0003 (-30bp/8h) AND no position open on that symbol
    // Exit:  trailing TP 30bp, SL 8bp, max hold 2 hours
    // Only fires on BTC (id=0) and ETH (id=1)  most liquid, most reliable
    // Only fires once per 4-hour window per symbol  funding changes slowly
    //
    static constexpr double  FUNDING_SIG_THRESHOLD     = -0.0003; // -30bp/8h = shorts very crowded
    static constexpr double  FUNDING_SIG_TP_BP         = 30.0;    // wider TP  slow-burn move
    static constexpr double  FUNDING_SIG_SL_BP         = 8.0;     // wider SL  slow-burn noise
    static constexpr int64_t FUNDING_SIG_MAX_HOLD_MS   = 7200000; // 2 hours
    static constexpr int64_t FUNDING_SIG_COOLDOWN_MS   = 14400000;// 4 hours per symbol
    static constexpr double  FUNDING_SIG_LATENCY_MAX   = 50.0;    // not latency sensitive

    // -------------------------------------------------------------------------
    // ETH LEAD-LAG (Tier 2) ENGINE
    static constexpr double  ETH_LEAD_TP_BP         = 12.0;  // slightly lower than BTC->alts, ETH moves less
    static constexpr double  ETH_LEAD_SL_BP         =  3.0;  // tight -- same mechanism, same timing edge
    static constexpr int64_t ETH_LEAD_MAX_HOLD_MS   = 5000;  // 5s -- ETH propagation is ~100ms slower
    static constexpr double  ETH_LEAD_SUSTAIN_MULT  =  0.6;  // sustain filter: 60% of threshold

    // SOL LEAD-LAG (Tier 3) ENGINE
    static constexpr double  SOL_LEAD_TP_BP         = 14.0;  // SOL->alts moves ~8-12bp
    static constexpr double  SOL_LEAD_SL_BP         =  3.5;  // slightly wider: SOL is more volatile
    static constexpr int64_t SOL_LEAD_MAX_HOLD_MS   = 4000;  // 4s -- SOL propagation is fastest
    static constexpr double  SOL_LEAD_SUSTAIN_MULT  =  0.58; // sustain: 7/12bp threshold

    // VOLUME SHOCK CONTINUATION ENGINE
    static constexpr double  VOLSHOCK_TP_BP         = 14.0;  // conservative: new engine, no data yet
    static constexpr double  VOLSHOCK_SL_BP         =  4.0;  // 2.5:1 gross R:R
    static constexpr int64_t VOLSHOCK_MAX_HOLD_MS   = 6000;  // 6s -- let continuation develop

    // NGAS LEAD-LAG ENGINE  Natural Gas macro signal leads crypto risk rotation
    // -------------------------------------------------------------------------
    // NGAS price drop  (< -NGAS_DROP_PCT over 15min)  = risk-on   LONG BTC/ETH
    // NGAS price spike (> +NGAS_SPIKE_PCT over 15min) = risk-off  SHORT (skipped, spot-only)
    //
    // Data: stooq.com (NGO.F CME front-month), polled every 5 minutes
    // Fallback: Yahoo Finance NG=F
    //
    // Signal window: 15-60 min after NGAS move (macro rotation is slow)
    // Only BTC (id=0) and ETH (id=1)  strongest NGAS/crypto correlation
    // 8h cooldown per symbol  NGAS price normalises over several hours
    //
    static constexpr double  NGAS_DROP_PCT              = 2.0;     // min NGAS drop  % to trigger risk-on LONG
    static constexpr double  NGAS_SPIKE_PCT             = 2.0;     // min NGAS spike % (informational  no short)
    static constexpr double  NGAS_CRYPTO_MOVED_MAX_BP   = 25.0;    // if crypto already moved 25bp, signal absorbed
    static constexpr double  NGAS_TP_BP                 = 35.0;    // wider TP  macro slow-burn move
    static constexpr double  NGAS_SL_BP                 = 10.0;    // wider SL   macro noise is higher
    static constexpr int64_t NGAS_MAX_HOLD_MS           = 3600000; // 1 hour max hold
    static constexpr int64_t NGAS_COOLDOWN_MS           = 28800000;// 8 hours between entries per symbol
    static constexpr double  NGAS_LATENCY_MAX_MS        = 50.0;    // not latency-sensitive (macro signal)

    // -------------------------------------------------------------------------
    // ORDER FLOW IMBALANCE (LAYER_OFI)
    // -------------------------------------------------------------------------
    // Edge: 33-tick OFI ratio > 0.25 + volume spike + book confirmation
    // Maker entry. TP=18bp, SL=6bp → EV +4.8bp at 45% WR after ~4bp cost.
    // Only fires in GRIND/BUILDUP to avoid chasing breakout momentum.
    static constexpr double OFI_RATIO_THRESHOLD    = 0.18;  // (buy_ema - sell_ema) / total > 0.18
    static constexpr double OFI_VOLUME_SPIKE_MULT  = 1.25;  // current volume > 1.25x EMA baseline
    static constexpr double OFI_BOOK_CONFIRM_IMBAL = 0.06;  // book_imbalance > 0.06 (bid-heavy)
    // BTC/ETH are deeper books; require stronger confirmation to avoid chop entries.
    static constexpr double OFI_MAJOR_RATIO_THRESHOLD    = 0.24;
    static constexpr double OFI_MAJOR_VOLUME_SPIKE_MULT  = 1.6;
    static constexpr double OFI_MAJOR_BOOK_CONFIRM_IMBAL = 0.12;
    static constexpr double OFI_MAJOR_FLOW_MIN           = 0.55;
    static constexpr double OFI_MAX_SPREAD_BPS     = 2.0;   // skip if spread too wide
    static constexpr double OFI_TP_BP              = 18.0;  // conservative: new engine, calibrate later
    static constexpr double OFI_SL_BP              = 6.0;   // 3:1 gross R:R
    static constexpr int64_t OFI_MAX_HOLD_MS       = 10000; // 10s: OFI moves are medium-speed

    // -------------------------------------------------------------------------
    // LIQUIDITY SWEEP (LAYER_SWEEP)
    // -------------------------------------------------------------------------
    // Edge: trade size spike >5x + ask depth collapse >40% + buyer aggression
    // Taker entry (edge decays fast, <200ms window). TP=22bp, SL=8bp.
    // EV at 55% WR: 0.55*22 - 0.45*8 = +8.5bp net after ~8bp taker cost.
    static constexpr double SWEEP_SIZE_SPIKE_MULT     = 5.0;  // trade qty > 5x EMA baseline
    static constexpr double SWEEP_DEPTH_COLLAPSE_RATIO = 0.40; // ask depth drops >40% vs prev tick
    static constexpr double SWEEP_MAX_SPREAD_BPS      = 3.0;  // wider allowed: sweeps happen in volatile books
    static constexpr double SWEEP_TP_BP               = 22.0; // sweeps produce 30-120bp raw, 22bp is conservative
    static constexpr double SWEEP_SL_BP               = 8.0;  // wider SL: sweep entry is fast/volatile
    static constexpr int64_t SWEEP_MAX_HOLD_MS        = 8000; // 8s: momentum continuation fades fast

    // -------------------------------------------------------------------------
    // MARKET MAKER INVENTORY PRESSURE (LAYER_MM_PRESSURE)
    // -------------------------------------------------------------------------
    // Edge: persistent slow book imbalance + upward price drift + OFI confirmation
    // Maker entry. TP=20bp, SL=7bp → EV +6.5bp at 50% WR after ~4bp maker cost.
    // Only fires in GRIND regime — MM rebalancing is a ranging-market phenomenon.
    static constexpr double MM_IMBAL_EMA_THRESHOLD  = 0.14;  // slow imbal EMA > 0.14 (bid-heavy)
    static constexpr double MM_DRIFT_BPS_THRESHOLD  = 2.0;   // cumulative drift > 2bp over window
    static constexpr double MM_MAX_SPREAD_BPS       = 2.0;   // tight spread required
    static constexpr double MM_TP_BP                = 20.0;  // slow drift captured 20-80bp raw
    static constexpr double MM_SL_BP               = 7.0;   // wider SL: slower signal, more noise
    static constexpr int64_t MM_MAX_HOLD_MS         = 20000; // 20s: inventory pressure resolves slowly

    // -------------------------------------------------------------------------
    // DIAGNOSTIC OUTPUT
    // -------------------------------------------------------------------------
    static constexpr int REGIME_DIAG_INTERVAL     = 500;
    static constexpr int SYMBOL_STATE_INTERVAL    = 500;

    // -------------------------------------------------------------------------
    // TIME-OF-DAY SESSION GATING (UTC hours)
    // Prime sessions: EU open 07:00-09:00, US open 13:00-16:00, Asia 00:00-02:00
    // Dead zone: 20:00-23:00 UTC (low volume, high spread, poor fill quality)
    // During dead zone: raise thresholds, reduce max positions to 1
    // -------------------------------------------------------------------------
    static constexpr int SESSION_EU_OPEN_UTC      =  7;
    static constexpr int SESSION_EU_CLOSE_UTC     =  9;
    static constexpr int SESSION_US_OPEN_UTC      = 13;
    static constexpr int SESSION_US_CLOSE_UTC     = 16;
    static constexpr int SESSION_ASIA_OPEN_UTC    =  0;
    static constexpr int SESSION_ASIA_CLOSE_UTC   =  2;
    static constexpr int SESSION_DEAD_START_UTC   = 20;
    static constexpr int SESSION_DEAD_END_UTC     = 23;
    // In dead zone: max 1 position, raise imbalance threshold by this factor
    static constexpr double DEAD_ZONE_IMBAL_MULT  = 1.5;
    static constexpr int    DEAD_ZONE_MAX_POS     = 1;
    static constexpr int    MAX_CONCURRENT_POSITIONS = 5;
    static constexpr int    LEADLAG_PRIME_START_UTC   = 1;
    static constexpr int    LEADLAG_PRIME_END_UTC     = 5;
    static constexpr double LEADLAG_OFFPEAK_SIZE_MULT = 0.5;
    static constexpr double RISK_FRACTION             = 0.02;
    static constexpr double MAX_TOTAL_EXPOSURE_PCT    = 0.25;
};

} // namespace chimera
