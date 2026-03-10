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
    static constexpr double LATENCY_HARD_LIMIT_MS  = 50.0;

    // Net-clean threshold: below = full size trades allowed
    // = WS p95 upper bound + small buffer. Above = reduce size or skip.
    static constexpr double LATENCY_NET_CLEAN_MS   = 30.0;

    // Lead-lag maximum: BTCETH/SOL propagation window shrinks with latency
    // At 35ms we still have 15-165ms of the 50-200ms propagation window remaining
    // Above 35ms the edge window is too uncertain to trade reliably
    static constexpr double LATENCY_LEADLAG_MAX_MS = 35.0;

    // Imbalance signal max: requires very fresh book data to be reliable
    // Only fire imbalance trades when latency is well below 25ms p95
    static constexpr double LATENCY_IMBALANCE_MAX_MS = 25.0;


    // -------------------------------------------------------------------------
    // COST FLOOR  minimum edge to beat round-trip cost
    // -------------------------------------------------------------------------
    // Taker fees (8bp) + spread (1bp) + slippage (1bp) = ~10bp per round trip.
    // Any signal with expected edge below this is a guaranteed loser.
    static constexpr double COST_FLOOR_BP = 12.0;  // raised from 10  kills zero P&L entries (25% of trades)


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
    static constexpr double LEADLAG_BTC_THRESHOLD_BP  = 8.0;   // lowered from 10bp  catch more valid moves, sustain filter handles fakes

    // Target already moved this much  edge consumed, don't enter
    // Tightened 32bp: if target already moved 2bp the propagation is done
    static constexpr double LEADLAG_TARGET_MAX_BP      = 2.0;

    // Take-profit for lead-lag trades (gross, before costs)
    // Net profit after 10bp costs = +4bp. Worth it if win rate > 70%.
    // ETH  SOL LEAD-LAG
    // ETH leads SOL by ~30-80ms. Smaller move threshold than BTCETH/SOL.
    // Long-only, spot-valid. Fires on SOL only.
    // Raised ETH threshold 812bp: at 8bp too many small ETH moves that SOL ignores.
    // Hold tightened 25001500ms: if SOL hasn't moved in 1.5s, the propagation is done.
    // Flow confirm added: requires SOL buy pressure to confirm ETH signal not yet absorbed.
    static constexpr double  LEADLAG_ETH_SOL_THRESHOLD_BP = 12.0; // min ETH move to signal
    static constexpr double  LEADLAG_ETH_SOL_TP_BP        = 8.0;   // lowered: MFE shows 8bp reachable, 20bp was never hit
    static constexpr double  LEADLAG_ETH_SOL_SL_BP        = 3.0;  // tightened: less loss when wrong
    static constexpr int64_t LEADLAG_ETH_SOL_MAX_HOLD_MS  = 2500; // extended: need more time to reach 20bp TP

    static constexpr double LEADLAG_TP_BP              = 15.0;  // raised: trail exits show moves go 10-15bp

    // Stop loss for lead-lag trades
    // Tight stop  if ETH/SOL doesn't follow BTC within 5s, exit
    static constexpr double LEADLAG_SL_BP              = 3.0;   // tightened: timing edge  wrong = exit immediately

    // Maximum hold time for lead-lag before forced flat
    // Propagation completes within ~200ms. 3s is generous timeout.
    static constexpr int64_t LEADLAG_MAX_HOLD_MS       = 5000;

    // -------------------------------------------------------------------------
    // LIQUIDATION CASCADE ENGINE  spot long on short liquidations from perp
    // -------------------------------------------------------------------------
    // Short liquidation on perp = forced buy on perp = spot follows up 50-200ms later
    // Min notional filters noise  only meaningful liquidations move spot
    static constexpr double  LIQ_MIN_NOTIONAL_USD   = 200000.0; // 00k min  smaller liq don't move spot
    static constexpr double  LIQ_SPOT_MOVED_MAX_BP  = 4.0;      // if spot already moved 4bp, we're chasing  skip
    static constexpr int64_t LIQ_SIGNAL_WINDOW_MS   = 400;      // signal expires after 400ms  propagation window
    static constexpr int64_t LIQ_COOLDOWN_MS        = 3000;     // 3s between liq trades per symbol  no stacking
    static constexpr double  LIQ_TP_BP              = 12.0;     // TP: slightly tighter than LEADLAG (liq moves ~8-15bp)
    static constexpr double  LIQ_SL_BP              = 4.0;      // SL: slightly wider than LEADLAG (more volatile entry)
    static constexpr int64_t LIQ_MAX_HOLD_MS        = 5000;     // max hold 5s same as LEADLAG


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
    static constexpr double IMBALANCE_THRESHOLD        = 0.42;

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
    static constexpr double IMPULSE_TP_BP              = 10.0;  // lowered 2010bp: 0 TP hits at 20bp, moves not extending
    static constexpr double IMPULSE_SL_BP              = 5.0;   // tightened 75bp: 9 SL hits all -7bp, 7/9 had MFE<0.6 (never moved right)  bad entries not noise. Cuts 2bp per SL hit.
    static constexpr int64_t IMPULSE_MAX_HOLD_MS       = 20000; // extended 1520s: timeouts suggest moves need more time

    // EXPANSION has own tighter parameters  weaker edge than IMPULSE
    static constexpr double EXPANSION_TP_BP             = 6.0;   // tightened 18->6bp: MFE data shows moves peak at 5-7bp, 18bp never hit
    static constexpr double EXPANSION_SL_BP             = 3.0;   // tightened 5->3bp: avg SL loss was -8bp, tighter with higher vol filter
    static constexpr int64_t EXPANSION_MAX_HOLD_MS      = 8000;  // tightened 12->8s: cut dead trades faster, timeout losses were drift

    // Minimum ticks in short window before impulse fires
    static constexpr int IMPULSE_MIN_SHORT_TICKS       = 5;

    // -------------------------------------------------------------------------
    // LIQUIDITY VACUUM ENGINE
    // Edge: ask-side depth drains >40% in 2 ticks  price gaps up through vacuum
    // Spot-only long: buy when ask wall disappears before price moves
    // -------------------------------------------------------------------------
    static constexpr double VACUUM_ASK_DRAIN_RATIO     = 0.40;  // ask depth drops 40%+
    static constexpr double VACUUM_MIN_IMBALANCE       = 0.20;  // bid must be present
    static constexpr double VACUUM_MAX_SPREAD_BPS      = 2.0;   // don't enter wide spreads
    static constexpr double VACUUM_TP_BP               = 16.0;
    static constexpr double VACUUM_SL_BP               = 6.0;
    static constexpr int64_t VACUUM_MAX_HOLD_MS        = 12000;
    static constexpr double LATENCY_VACUUM_MAX_MS      = 30.0;  // tight  edge decays fast

    // -------------------------------------------------------------------------
    // VWAP REVERSION ENGINE
    // Edge: in GRIND regime, price >20bp below session VWAP + bid imbalance = buy
    // Mean reversion back toward VWAP. High win rate in ranging markets.
    // -------------------------------------------------------------------------
    static constexpr double VWAP_ENTRY_DEVIATION_BP    = 20.0;  // min distance below VWAP
    static constexpr double VWAP_MAX_DEVIATION_BP      = 80.0;  // too far = trending, skip
    static constexpr double VWAP_MIN_IMBALANCE         = 0.15;  // bid pressure must confirm
    static constexpr double VWAP_MAX_SPREAD_BPS        = 2.0;
    static constexpr double VWAP_TP_BP                 = 18.0;
    static constexpr double VWAP_SL_BP                 = 7.0;
    static constexpr int64_t VWAP_MAX_HOLD_MS          = 45000; // slower mean reversion
    static constexpr double LATENCY_VWAP_MAX_MS        = 50.0;  // not latency sensitive


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
    static constexpr int    MIN_REGIME_TICKS                = 30;
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
    static constexpr double EXPANSION_VOL_RATIO       = 1.75;  // raised 1.55->1.75: only enter strong expansions, reduces false breakouts
    static constexpr int    EXPANSION_CONFIRM_TICKS   = 3;   // raised 2->3: extra confirmation reduces noise entries
    static constexpr int    EXPANSION_MIN_SHORT_TICKS = 12;  // was 8  need more confirmation ticks


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
};

} // namespace chimera
