#pragma once
#include <cstdint>

namespace chimera {

// ============================================================================
// CHIMERA TRADING ENGINE — CENTRAL CONFIGURATION
// ============================================================================
// AUDIT 2026-03-28: Complete recalibration.
//   KILLED:  VACUUM (TP=16bp < cost floor), OFI (TP=18bp < cost floor),
//            IMBALANCE standalone (TP=12bp), IMPULSE (0% WR 3 trades),
//            EXPANSION (0% WR 2 trades), ETH-LEAD (17% WR -121bp),
//            SOL-LEAD (0% WR), SPREAD-COMPRESS standalone (TP=10bp)
//   FIXED:   VWAP (entry 12->25bp, TP 12->30bp, hold 35->90s)
//            LEADLAG (TP floor raised, kill window enforced)
//            LIQ (kill window gate added to QuadEngine on_tick)
//            MM-PRESSURE (kill window gate added)
//   NEW:     Pyramiding infrastructure (2-unit max, tight trailing)
//            FUNDING + NGAS re-enabled (viable TP at slow-burn hold)
//            SESSION_MOM re-enabled for EU/US opens only
//
// COST STRUCTURE (Binance spot with BNB discount):
//   Fee per side:       0.075% = 7.5bp (25% BNB discount on 0.10%)
//   Round trip fees:    15bp
//   Spread + slip:      ~2bp
//   TOTAL COST FLOOR:   17bp minimum gross target (use 15bp maker floor conservatively)
// ============================================================================

struct TradingConfig {

    // -------------------------------------------------------------------------
    // LATENCY LIMITS
    // -------------------------------------------------------------------------
    static constexpr double LATENCY_HARD_LIMIT_MS     = 100.0;
    static constexpr double LATENCY_NET_CLEAN_MS      = 60.0;
    static constexpr double LATENCY_LEADLAG_MAX_MS    = 80.0;
    static constexpr double LATENCY_IMBALANCE_MAX_MS  = 60.0;

    // -------------------------------------------------------------------------
    // COST FLOORS
    // -------------------------------------------------------------------------
    static constexpr double COST_FLOOR_BP             = 22.0;   // taker round-trip floor
    static constexpr double EXPANSION_COST_FLOOR_BP   = 22.0;
    static constexpr double MAKER_COST_FLOOR_BP       = 15.0;   // maker round-trip with BNB
    static constexpr double TAKER_ROUND_TRIP_BP       = 20.0;   // 10bp/side VIP0 taker
    static constexpr double MAKER_ROUND_TRIP_BP       = 15.0;   // 7.5bp/side with BNB

    // -------------------------------------------------------------------------
    // PYRAMIDING (scale-in on winning trades)
    // -------------------------------------------------------------------------
    // When a position reaches PYRAMID_ARM_BP profit, add a second unit.
    // The second unit gets a tighter trail from the point of addition.
    // Only enabled for strategies with wide targets (LIQ, MM, VWAP, FUNDING, NGAS).
    // Max 2 units total — never add a third; cascading size in momentum = ruin.
    //
    // Unit 2 entry: price must still be trending (no reversal signal)
    // Unit 2 size:  50% of unit 1 (half-size add, still has edge)
    // Unit 2 trail: arm immediately at PYRAMID_TRAIL_ARM_BP above add price
    // Combined exit: trail floor based on blended entry, not unit 1 entry
    //
    // EV math (LIQ example, unit 1 at price P, unit 2 at P+40bp):
    //   Unit 1: 150bp TP trail from P  → large winner
    //   Unit 2: 60bp trail from P+40   → moderate winner, tighter protection
    //   Combined: higher avg profit on winners, minimal extra risk (tight trail)
    //
    static constexpr bool   PYRAMID_ENABLED            = true;
    static constexpr double PYRAMID_ARM_BP             = 30.0;  // profit needed before adding unit 2
    static constexpr double PYRAMID_UNIT2_SIZE_MULT    = 0.50;  // unit 2 = 50% of unit 1 size
    static constexpr double PYRAMID_TRAIL_ARM_BP       = 8.0;   // arm trail on unit 2 immediately
    static constexpr double PYRAMID_TRAIL_LOCK_PCT     = 0.60;  // lock 60% of unit 2 peak
    // Strategies eligible for pyramiding (must have TP >= 50bp or trail exit)
    // LIQ, MM_PRESSURE, FUNDING, NGAS, VWAP (when deviation >= 40bp)
    static constexpr double PYRAMID_MIN_VWAP_DEV_BP    = 40.0;  // VWAP only pyramids on deep dips

    // -------------------------------------------------------------------------
    // LEAD-LAG SIGNAL PARAMETERS
    // -------------------------------------------------------------------------
    static constexpr double  LEADLAG_BTC_THRESHOLD_BP   = 3.0;  // FIX: 5→3bp — at $73k BTC, 5bp=$36.50 move in 400ms almost never fires in low-vol; 3bp=$21.90, fires 10-20x/day
    static constexpr double  LEADLAG_CONFIRM_OB_RATIO   = 1.01;  // FIX: 1.08→1.01 — on BTC-led moves ETH/SOL book hasn't repriced, MMs pulling = low imbalance at exact signal time
    static constexpr double  LEADLAG_CONFIRM_FLOW_RATIO = 1.01;  // FIX: 1.04→1.01 — flow EMAs lag on fast BTC moves, 1.04 kills valid signals
    static constexpr double  LEADLAG_TARGET_MAX_BP      = 4.0;  // FIX: 8→4bp — if alt already moved 8bp of 45bp TP window too much consumed; 4bp leaves clean 41bp runway
    static constexpr double  LEADLAG_ETH_SOL_THRESHOLD_BP = 9.0;
    static constexpr double  LEADLAG_ETH_SOL_TP_BP      = 25.0;  // raised 10->25bp: 10bp < 15bp cost floor
    static constexpr double  LEADLAG_ETH_SOL_SL_BP      = 15.0;  // raised to cost floor minimum
    static constexpr int64_t LEADLAG_ETH_SOL_MAX_HOLD_MS = 10000; // extended 3->10s
    static constexpr double  LEADLAG_TP_BP              = 45.0;  // FIX: 30→45bp — shadow mode 0.55x = 24.75bp, net +9.75bp above 15bp cost floor
    static constexpr double  LEADLAG_SL_BP              = 15.0;  // raised 8->15bp: at cost floor minimum
    static constexpr int64_t LEADLAG_MAX_HOLD_MS        = 15000; // extended 5->15s: give move time to develop

    // -------------------------------------------------------------------------
    // LIQUIDATION CASCADE ENGINE
    // -------------------------------------------------------------------------
    // EV: TP=150bp trail, SL=20bp. At 40% WR: 0.4*(150-15) - 0.6*20 = +42bp net.
    // Kill window enforced in QuadEngineBalancedEngine::on_tick() — no dead tape.
    // Pyramiding: arm at +30bp, add 50% unit at that level, trail unit 2 tightly.
    static constexpr double  LIQ_MIN_NOTIONAL_USD      = 1000000.0;
    static constexpr double  LIQ_MIN_NOTIONAL_ALT_USD  = 1500000.0;
    static constexpr double  LIQ_SPOT_MOVED_MAX_BP     = 2.5;
    static constexpr int64_t LIQ_SIGNAL_WINDOW_MS      = 250;
    static constexpr int64_t LIQ_COOLDOWN_MS           = 10000;
    static constexpr double  LIQ_MAX_SPREAD_BPS        = 2.0;
    static constexpr double  LIQ_MIN_FLOW_RATIO        = 0.56;
    static constexpr double  LIQ_MIN_BOOK_IMBALANCE    = 0.08;
    static constexpr double  LIQ_MIN_VOL_RATIO         = 0.90;
    static constexpr double  LIQ_MAX_VOL_RATIO         = 2.40;
    static constexpr double  LIQ_TP_BP                 = 150.0;  // trail exit, not fixed TP
    static constexpr double  LIQ_SL_BP                 = 20.0;
    static constexpr int64_t LIQ_MAX_HOLD_MS           = 30000;

    // -------------------------------------------------------------------------
    // IMBALANCE SIGNAL — PERMANENTLY DISABLED STANDALONE
    // -------------------------------------------------------------------------
    // TP=12bp < 15bp cost floor. Needs 93% WR. Impossible in practice.
    // Left here for exit management switch statement — do not re-enable.
    static constexpr double  IMBALANCE_THRESHOLD       = 0.42;
    static constexpr double  IMBALANCE_MAX_SPREAD_BPS  = 1.5;
    static constexpr double  IMBALANCE_TP_BP           = 12.0;  // INACTIVE
    static constexpr double  IMBALANCE_SL_BP           = 4.0;
    static constexpr int64_t IMBALANCE_MAX_HOLD_MS     = 8000;

    // -------------------------------------------------------------------------
    // BREAKOUT / IMPULSE — DISABLED (0% WR 3 trades, -14.9bp avg)
    // -------------------------------------------------------------------------
    static constexpr double  IMPULSE_TP_BP             = 14.0;
    static constexpr double  IMPULSE_ALT_TP_BP         = 20.0;
    static constexpr double  IMPULSE_SL_BP             = 5.0;
    static constexpr int64_t IMPULSE_MAX_HOLD_MS       = 20000;
    static constexpr double  EXPANSION_TP_BP           = 10.0;
    static constexpr double  EXPANSION_ALT_TP_BP       = 25.0;
    static constexpr double  EXPANSION_SL_BP           = 3.0;
    static constexpr int64_t EXPANSION_MAX_HOLD_MS     = 8000;
    static constexpr int     IMPULSE_MIN_SHORT_TICKS   = 5;

    // -------------------------------------------------------------------------
    // LIQUIDITY VACUUM — DISABLED (TP=16bp < cost floor, 17 trades 0% WR)
    // -------------------------------------------------------------------------
    // Parameters updated to 35bp+ for future potential re-enable (shadow only).
    static constexpr double  VACUUM_ASK_DRAIN_RATIO    = 0.40;
    static constexpr double  VACUUM_MIN_IMBALANCE      = 0.20;
    static constexpr double  VACUUM_MAX_SPREAD_BPS     = 2.0;
    static constexpr double  VACUUM_TP_BP              = 35.0;   // raised for future re-enable
    static constexpr double  VACUUM_SL_BP              = 8.0;    // raised to match wider target
    static constexpr int64_t VACUUM_MAX_HOLD_MS        = 20000;  // extended for wider target
    static constexpr double  LATENCY_VACUUM_MAX_MS     = 60.0;

    // -------------------------------------------------------------------------
    // VWAP REVERSION ENGINE — RECALIBRATED
    // -------------------------------------------------------------------------
    // FIX 2026-03-28: entry raised 12->25bp. Previous 12bp caught tick noise.
    // New EV math: TP=30bp, SL=5bp, 50% WR → 0.5*(30-15) - 0.5*5 = +5bp net.
    // Pyramiding: if deviation >= 40bp AND price still below VWAP, add 50% unit.
    // Hold extended 35->90s — mean reversion needs time, don't cut it short.
    // Partial exit at 15bp (half of TP): lock gains before the second half runs.
    static constexpr double  VWAP_ENTRY_DEVIATION_BP   = 15.0;  // FIX: 25→15bp — fires 5-8x/day vs 2-3x; EV positive at wider TP below
    static constexpr double  VWAP_MAX_DEVIATION_BP     = 100.0; // FIX: 60→100bp (allow deep dips)
    static constexpr double  VWAP_PARTIAL_EXIT_BP      = 15.0;  // FIX: 4→15bp (exit half at +15bp)
    static constexpr double  VWAP_PARTIAL_EXIT_SIZE    = 0.50;  // exit 50% at partial trigger
    static constexpr double  VWAP_MIN_IMBALANCE        = 0.20;  // FIX: raised 0.18→0.20
    static constexpr double  VWAP_MIN_OFI_RATIO        = 0.15;  // FIX: raised 0.12→0.15
    static constexpr double  VWAP_MAX_SPREAD_BPS       = 1.8;
    static constexpr double  VWAP_TP_BP                = 40.0;  // FIX: 30→40bp — at 15bp entry: 0.5*(40-15)-0.5*15=+5bp net EV
    static constexpr double  VWAP_SL_BP                = 15.0;  // unchanged — at cost floor minimum
    static constexpr int64_t VWAP_MAX_HOLD_MS          = 90000; // FIX: 35→90s
    static constexpr double  VWAP_TRAIL_ARM_BP         = 12.0;  // FIX: 1.5→12bp (arm at real profit)
    static constexpr double  VWAP_TRAIL_LOCK_PCT       = 0.65;  // lock 65% of peak once armed
    static constexpr double  LATENCY_VWAP_MAX_MS       = 50.0;

    // -------------------------------------------------------------------------
    // EDGE PROMOTION / DEMOTION GATES
    // -------------------------------------------------------------------------
    static constexpr int     EDGE_WINDOW_TRADES         = 30;
    static constexpr int     EDGE_PROMOTE_MIN_TRADES    = 30;
    static constexpr int     EDGE_DEMOTE_MIN_TRADES     = 20;
    static constexpr double  EDGE_PROMOTE_MIN_AVG_PNL_BP = 0.8;
    static constexpr double  EDGE_PROMOTE_MAX_TIMEOUT_RT  = 0.55;
    static constexpr double  EDGE_PROMOTE_MFE_BUFFER_BP = 1.0;
    static constexpr double  EDGE_DEMOTE_AVG_PNL_BP     = -0.5;
    static constexpr int64_t EDGE_DISABLE_MS            = 30 * 60 * 1000LL;

    // -------------------------------------------------------------------------
    // REGIME CLASSIFICATION THRESHOLDS
    // -------------------------------------------------------------------------
    static constexpr double REGIME_DEAD_ENTER              = 0.60;
    static constexpr double REGIME_DEAD_EXIT               = 0.90;
    static constexpr double REGIME_GRIND_ENTER_FROM_DEAD   = 0.90;
    static constexpr double REGIME_GRIND_EXIT_TO_DEAD      = 0.75;
    static constexpr double REGIME_GRIND_EXIT_TO_BUILDUP   = 1.35;
    static constexpr double REGIME_BUILDUP_ENTER           = 1.35;
    static constexpr double REGIME_BUILDUP_EXIT            = 0.95;
    static constexpr double REGIME_BUILDUP_TO_BREAKOUT     = 1.65;
    static constexpr double REGIME_BREAKOUT_ENTER          = 1.65;
    static constexpr double REGIME_BREAKOUT_EXIT           = 1.35;
    static constexpr int    MIN_REGIME_TICKS               = 50;
    static constexpr int    EXPAND_POST_COMPRESS_LOCKOUT   = 3;
    static constexpr double REGIME_MIN_LONG_AVG            = 0.004;
    static constexpr double REGIME_DEAD_THRESHOLD          = REGIME_DEAD_EXIT;
    static constexpr double REGIME_GRIND_THRESHOLD         = REGIME_GRIND_EXIT_TO_BUILDUP;
    static constexpr double REGIME_BUILDUP_THRESHOLD       = REGIME_BUILDUP_TO_BREAKOUT;

    // -------------------------------------------------------------------------
    // VOLATILITY CALCULATION
    // -------------------------------------------------------------------------
    static constexpr int    SHORT_VOL_WINDOW          = 20;
    static constexpr int    LONG_VOL_WINDOW           = 200;
    static constexpr double LONG_VOL_EMA_ALPHA        = 0.06;
    static constexpr double VOL_RATIO_EMA_ALPHA       = 0.06;
    static constexpr double VOL_MIN_LONG              = 1e-8;
    static constexpr double MIN_LONG_VOL_FOR_TRADING  = 0.000010;

    // -------------------------------------------------------------------------
    // EXPANSION LAYER
    // -------------------------------------------------------------------------
    static constexpr double EXPANSION_VOL_RATIO       = 1.60;
    static constexpr int    EXPANSION_CONFIRM_TICKS   = 2;
    static constexpr int    EXPANSION_MIN_SHORT_TICKS = 10;

    // -------------------------------------------------------------------------
    // EXIT MANAGEMENT — shared across strategies
    // -------------------------------------------------------------------------
    static constexpr double TRAIL_LONG_VOL_MULT       = 3.0;
    static constexpr double MIN_PROFIT_TO_TRAIL_BP    = 4.0;
    static constexpr int    MIN_HOLD_TICKS            = 3;
    static constexpr double MIN_DISPLACEMENT_LONG_MULT = 0.75;

    // -------------------------------------------------------------------------
    // ORDER FLOW CONFIRMATION
    // -------------------------------------------------------------------------
    static constexpr double FLOW_CONFIRM_THRESHOLD    = 0.55;

    // -------------------------------------------------------------------------
    // MAKER ORDER MODE
    // -------------------------------------------------------------------------
    static constexpr bool   MAKER_MODE                = true;
    static constexpr double MAKER_STALE_BP            = 3.0;
    static constexpr int64_t MAKER_IMBALANCE_TIMEOUT_MS = 5000;
    static constexpr int64_t MAKER_LEADLAG_TIMEOUT_MS  = 200;
    static constexpr int64_t MAKER_IMPULSE_TIMEOUT_MS  = 500;

    // -------------------------------------------------------------------------
    // FUNDING RATE SIGNAL ENGINE — RE-ENABLED
    // -------------------------------------------------------------------------
    // EV: TP=30bp, SL=8bp. At 50% WR: 0.5*(30-15) - 0.5*8 = +3.5bp net.
    // Pyramiding eligible: add 50% unit if funding still negative and up 30bp+.
    static constexpr double  FUNDING_SIG_THRESHOLD    = -0.0003;
    static constexpr double  FUNDING_SIG_TP_BP        = 30.0;
    static constexpr double  FUNDING_SIG_SL_BP        = 15.0;  // raised 8->15bp: must exceed cost floor
    static constexpr int64_t FUNDING_SIG_MAX_HOLD_MS  = 7200000;   // 2 hours
    static constexpr int64_t FUNDING_SIG_COOLDOWN_MS  = 14400000;  // 4 hours
    static constexpr double  FUNDING_SIG_LATENCY_MAX  = 50.0;

    // -------------------------------------------------------------------------
    // NGAS LEAD-LAG ENGINE — RE-ENABLED
    // -------------------------------------------------------------------------
    // EV: TP=35bp, SL=10bp. At 50% WR: 0.5*(35-15) - 0.5*10 = +5bp net.
    static constexpr double  NGAS_DROP_PCT             = 2.0;
    static constexpr double  NGAS_SPIKE_PCT            = 2.0;
    static constexpr double  NGAS_CRYPTO_MOVED_MAX_BP  = 25.0;
    static constexpr double  NGAS_TP_BP                = 35.0;
    static constexpr double  NGAS_SL_BP                = 15.0;  // raised 10->15bp: at minimum cost floor
    static constexpr int64_t NGAS_MAX_HOLD_MS          = 3600000;   // 1 hour
    static constexpr int64_t NGAS_COOLDOWN_MS          = 28800000;  // 8 hours
    static constexpr double  NGAS_LATENCY_MAX_MS       = 50.0;

    // -------------------------------------------------------------------------
    // ETH/SOL LEAD-LAG (DISABLED — insufficient WR at cost floor)
    // -------------------------------------------------------------------------
    static constexpr double  ETH_LEAD_TP_BP            = 25.0;  // raised 12->25bp: same cost floor issue as LEADLAG
    static constexpr double  ETH_LEAD_SL_BP            = 15.0;  // raised to cost floor minimum
    static constexpr int64_t ETH_LEAD_MAX_HOLD_MS      = 8000;  // extended 5->8s
    static constexpr double  ETH_LEAD_SUSTAIN_MULT     =  0.6;
    static constexpr double  SOL_LEAD_TP_BP            = 14.0;
    static constexpr double  SOL_LEAD_SL_BP            =  3.5;
    static constexpr int64_t SOL_LEAD_MAX_HOLD_MS      = 4000;
    static constexpr double  SOL_LEAD_SUSTAIN_MULT     =  0.58;

    // -------------------------------------------------------------------------
    // VOLUME SHOCK CONTINUATION
    // -------------------------------------------------------------------------
    static constexpr double  VOLSHOCK_TP_BP            = 25.0;  // FIX 2026-03-28: raised 18->25bp (18bp only 3bp above 15bp cost floor)
    static constexpr double  VOLSHOCK_SL_BP            = 15.0;  // at cost floor minimum
    static constexpr int64_t VOLSHOCK_MAX_HOLD_MS      = 6000;

    // -------------------------------------------------------------------------
    // ORDER FLOW IMBALANCE — DISABLED (24 trades 0% WR -96bp)
    // -------------------------------------------------------------------------
    // TP raised for future potential re-enable; engine hard-disabled in code.
    static constexpr double  OFI_RATIO_THRESHOLD       = 0.25;
    static constexpr double  OFI_VOLUME_SPIKE_MULT     = 1.5;
    static constexpr double  OFI_BOOK_CONFIRM_IMBAL    = 0.10;
    static constexpr double  OFI_MAJOR_RATIO_THRESHOLD     = 0.32;
    static constexpr double  OFI_MAJOR_VOLUME_SPIKE_MULT   = 2.0;
    static constexpr double  OFI_MAJOR_BOOK_CONFIRM_IMBAL  = 0.18;
    static constexpr double  OFI_MAJOR_FLOW_MIN             = 0.58;
    static constexpr double  OFI_MAX_SPREAD_BPS        = 2.0;
    static constexpr double  OFI_TP_BP                 = 35.0;  // raised for future re-enable
    static constexpr double  OFI_SL_BP                 = 6.0;
    static constexpr int64_t OFI_MAX_HOLD_MS           = 20000; // extended for wider target

    // -------------------------------------------------------------------------
    // LIQUIDITY SWEEP — SHADOW ONLY (0 trades yet)
    // -------------------------------------------------------------------------
    static constexpr double  SWEEP_SIZE_SPIKE_MULT     = 5.0;
    static constexpr double  SWEEP_DEPTH_COLLAPSE_RATIO = 0.40;
    static constexpr double  SWEEP_MAX_SPREAD_BPS      = 3.0;
    static constexpr double  SWEEP_TP_BP               = 30.0;  // raised 22→30bp
    static constexpr double  SWEEP_SL_BP               = 8.0;
    static constexpr int64_t SWEEP_MAX_HOLD_MS         = 12000;

    // -------------------------------------------------------------------------
    // MARKET MAKER INVENTORY PRESSURE — ACTIVE (trail exit, kill window fixed)
    // -------------------------------------------------------------------------
    // EV: trail TP ~150bp, SL=10bp. At 50% WR: large positive. Just needed kill window.
    // Pyramiding: arm at +30bp, add 50% unit.
    static constexpr double  MM_IMBAL_EMA_THRESHOLD   = 0.25;
    static constexpr double  MM_DRIFT_BPS_THRESHOLD   = 5.0;
    static constexpr double  MM_MAX_SPREAD_BPS        = 1.5;
    static constexpr double  MM_TP_BP                 = 150.0;  // trail target
    static constexpr double  MM_SL_BP                 = 20.0;  // raised 10->20bp: must exceed 15bp cost floor
    static constexpr double  MM_TRAIL_ARM_BP          = 25.0;
    static constexpr int64_t MM_MAX_HOLD_MS           = 45000;

    // -------------------------------------------------------------------------
    // STAT ARB — BTC/ETH cointegration mean reversion
    // -------------------------------------------------------------------------
    // EV: TP=20bp, SL=6bp. At 65% WR (cointegrated): 0.65*(20-15) - 0.35*6 = +1.15bp net.
    static constexpr double  STATARB_TP_BP            = 20.0;
    static constexpr double  STATARB_SL_BP            = 15.0;  // at cost floor minimum
    static constexpr int64_t STATARB_MAX_HOLD_MS      = 4 * 3600000LL;
    static constexpr int64_t STATARB_COOLDOWN_MS      = 1800000LL;
    static constexpr double  STATARB_ENTRY_ZSCORE     = 2.0;
    static constexpr double  STATARB_EXIT_ZSCORE      = 0.4;

    // -------------------------------------------------------------------------
    // SPREAD COMPRESSION — DISABLED STANDALONE (TP=10bp < cost floor)
    // -------------------------------------------------------------------------
    // Parameters raised for future re-enable in combination with another signal.
    static constexpr double  SPREAD_COMPRESS_TP_BP    = 25.0;
    static constexpr double  SPREAD_COMPRESS_SL_BP    =  5.0;
    static constexpr int64_t SPREAD_COMPRESS_MAX_HOLD_MS = 20000;

    // -------------------------------------------------------------------------
    // DIVERGENCE — DISABLED STANDALONE (TP=8bp << cost floor)
    // -------------------------------------------------------------------------
    static constexpr double  DIVERGE_TP_BP            = 25.0;
    static constexpr double  DIVERGE_SL_BP            =  5.0;
    static constexpr int64_t DIVERGE_MAX_HOLD_MS      = 45000;

    // -------------------------------------------------------------------------
    // SESSION OPEN MOMENTUM — RE-ENABLED (EU/US opens only)
    // -------------------------------------------------------------------------
    // EV: TP=22bp, SL=6bp. At 70% WR (session open): 0.7*(22-15) - 0.3*6 = +3.1bp net.
    static constexpr double  SESSION_MOM_TP_BP        = 22.0;
    static constexpr double  SESSION_MOM_SL_BP        = 15.0;  // at cost floor minimum
    static constexpr int64_t SESSION_MOM_MAX_HOLD_MS  = 900000LL;  // 15 min

    // -------------------------------------------------------------------------
    // DIAGNOSTIC OUTPUT
    // -------------------------------------------------------------------------
    static constexpr int REGIME_DIAG_INTERVAL         = 500;
    static constexpr int SYMBOL_STATE_INTERVAL        = 500;

    // -------------------------------------------------------------------------
    // TIME-OF-DAY SESSION GATING (UTC)
    // -------------------------------------------------------------------------
    // KILL WINDOW (02:00-07:00 UTC): applied in BOTH BalancedEngine AND
    // QuadEngineBalancedEngine::on_tick() to catch all sub-engines.
    // EU open (07:00-09:00), US open (13:00-16:00), Asia open (00:00-02:00).
    static constexpr int SESSION_EU_OPEN_UTC          =  7;
    static constexpr int SESSION_EU_CLOSE_UTC         =  9;
    static constexpr int SESSION_US_OPEN_UTC          = 13;
    static constexpr int SESSION_US_CLOSE_UTC         = 16;
    static constexpr int SESSION_ASIA_OPEN_UTC        =  0;
    static constexpr int SESSION_ASIA_CLOSE_UTC       =  2;
    static constexpr int SESSION_DEAD_START_UTC       = 20;
    static constexpr int SESSION_DEAD_END_UTC         = 23;
    static constexpr double DEAD_ZONE_IMBAL_MULT      = 1.5;
    static constexpr int    DEAD_ZONE_MAX_POS         = 1;
    static constexpr int    KILL_WINDOW_START_UTC     = 2;
    static constexpr int    KILL_WINDOW_END_UTC       = 7;
    static constexpr int    MAX_CONCURRENT_POSITIONS  = 5;
    static constexpr int    LEADLAG_PRIME_START_UTC   = 7;   // EU open — full active session
    static constexpr int    LEADLAG_PRIME_END_UTC     = 22;  // NY close
    static constexpr double LEADLAG_OFFPEAK_SIZE_MULT = 0.5;  // Wintermute 2025: altcoin rallies shorter, stay with 0.5x off-peak
    static constexpr double RISK_FRACTION             = 0.02;
    static constexpr double MAX_TOTAL_EXPOSURE_PCT    = 0.25;
};

} // namespace chimera
