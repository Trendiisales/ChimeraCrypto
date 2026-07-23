// ============================================================================
// EdgeEngine.hpp — Tier-2 long-only longer-timeframe edges
//
// Replaces SwingEngine + FundingWindowEngine + BasisMomentumEngine +
// OrderbookImbalanceEngine. A single configurable header-only class instantiated
// once per (symbol, strategy, timeframe) keeper edge from the backtest pipeline.
//
// Backtest provenance (run 2026-05-11 on 2022-01-01 -> 2026-05-11, 10bp cost):
//
//   instance              symbol     strat        tf    trades  OOS_PF  OOS_bp
//   ----------------------------------------------------------------------------
//   link_rsi_h6           LINKUSDT   RSI_REVERT   H6        64    2.82   +7828
//   eth_bb_h6             ETHUSDT    BOLLINGER    H6       219    1.31   +4258
//   sol_donch_h6          SOLUSDT    DONCHIAN     H6       108    1.24   +2900
//   xrp_donch_h1          XRPUSDT    DONCHIAN     H1       608    1.20   +4547
//   btc_tsmom_d1          BTCUSDT    TSMOM        D1        86    1.19   +1039
//
// Exit logic (every strategy):
//   - Entry at next-bar OPEN after signal close (no look-ahead)
//   - Hard SL at entry - sl_atr_mult * ATR14(at signal bar)
//   - Trailing stop: arms at 1.0x ATR profit, trails at peak - 0.5x ATR
//     (ratchets up only, never down). Once armed, the effective stop is
//     max(hard_sl, trail_stop) — so the trail only helps, never hurts.
//   - Time exit at hold_bars after entry
//
// All instances are LONG-only spot (per ChimeraCrypto SPOT-ONLY guardrail).
// Shadow mode default = true; promote to live only after 4 weeks of paper
// trades match backtest expectations.
//
// Bar synthesis is internal: each engine accumulates ticks into its own
// timeframe bars (no shared bar bus required).
//
// Cold-start mitigation: seed_bars() pre-populates the closed-bar deques from
// historical OHLC pulled by main.cpp (BinanceREST::fetch_klines), so an engine
// can evaluate signals on bar 1 instead of waiting ~lookback bars for live
// ticks to build the history (which would take ~20 days for BTC-TSMOM-D1).
//
// Time-gated strategies (added 2026-05-16):
//   OVERNIGHT  — buy at 21:00 UTC bar close (H1) when trend is positive.
//                Captures the documented overnight premium (21-23 UTC window).
//   WEEKDAY    — buy on Monday D1 bar close when close > SMA(5).
//                Captures the Monday effect (+0.51%/day avg).
//
// Session 28 additions (2026-05-17):
//   KELTNER_REVERT — mean reversion using EMA + ATR bands (more robust than
//                    Bollinger's std-dev bands for crypto's fat tails).
//   DUAL_THRUST    — range breakout: enter when close > open + K * range
//                    where range = max(HH-LC, HC-LL) over prior N bars.
//   Volatility regime filter — ATR(14)/ATR(50) ratio gate that suppresses
//                    counter-trend entries (RSI/BOLL/KELTNER) during chaos
//                    (ratio > 1.6) and elevated vol (ratio > 1.2).
//
// Session 29 additions (2026-05-17):
//   ADX regime filter — suppresses trend-following entries (TSMOM/DONCHIAN/
//                    DUAL_THRUST) when ADX(14) < 25 (no directional trend).
//                    Prevents whipsaw entries in ranging/choppy markets.
//   Volume regime filter — counts ticks per bar as volume proxy. Suppresses
//                    ALL entries when tick count < 30% of rolling average
//                    (detects weekend dead zones and exchange outages).
//   ICHIMOKU       — Cloud breakout + Tenkan/Kijun cross (trend-following).
//   SUPERTREND     — ATR-based trailing trend indicator, enters on flip
//                    from bearish to bullish.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <functional>
#include "live/GateAttribution.hpp"   // Phase-4 item 21: observability sink (dep-free, header-only)

namespace chimera {

// S-2026-07-23 LIVE-ONLY CULL (operator: viable-only rebuild step 2). Every EdgeEngine
// prints an "[TAG] ARMED ... shadow=%d" line at construction. Most constructed engines
// are the culled non-roster shadow zoo (never g_slots'd, wire_engine is a live no-op) —
// their ARMED lines are dead-engine boot noise that VIOLATES never-display-dead-engines.
// When g_edge_arm_quiet is set (main.cpp sets it true before building the zoo), the ctor
// suppresses the per-engine ARMED line and instead bumps g_edge_arm_suppressed, so the
// aggregate [LIVE-ONLY-GATE] boot line can report a COUNT with no individual dead names.
// Default false = byte-identical behavior for every other build (backtests etc.).
inline bool g_edge_arm_quiet = false;
inline int  g_edge_arm_suppressed = 0;

enum class StrategyKind {
    TSMOM,          // 20-bar return > 0
    DONCHIAN,       // close > prior 20-bar high
    BOLLINGER,      // bar pierces lower BB(20,2) then closes back above
    RSI_REVERT,     // RSI(14) crosses up from <= 30
    OVERNIGHT,      // H1 bar at 21:00 UTC + uptrend filter
    WEEKDAY,        // D1 bar on Monday + SMA(5) filter
    KELTNER_REVERT, // bar pierces lower Keltner (EMA-ATR) band then closes above
    DUAL_THRUST,    // range breakout: close > open + K * range(N)
    ICHIMOKU,       // cloud breakout + Tenkan/Kijun cross (Session 29)
    SUPERTREND,     // ATR-based trailing trend flip (Session 29)
    WILLIAMS_R,     // Williams %R cross up from oversold (Session 29b)
    STOCH_RSI,      // Stochastic RSI cross up from oversold (Session 29b)
    BREAKOUT_PULLBACK, // S38: N-bar high breakout, enter on pullback that holds the breakout level
    MIMIC,         // S-2026-07-03: wide W-bar mimic, ride to symmetric down-jump flip; NO trade-level stops (ride_to_flip)
    KELTNER_BREAK,  // S-2026-07-12: upper-Keltner breakout TREND (close>EMA+M*ATR -> long), ride to lower-band flip; NO stops. Folds the Mac ibkrcrypto Kelt(20,2.0). NOT KELTNER_REVERT (that's the opposite lower-band mean-revert).
    REGIME_SWITCH,  // S-2026-07-12: efficiency-ratio regime switch (ER>hi trending->momentum long; ER<lo chop->IBS mean-rev long; else flat). Folds the Mac ibkrcrypto Regime(20,0.40,0.25); ride_to_flip (exit when signal != long).
    EMAX,           // S-2026-07-21 (DirectionalTrendRoster port): fast-EMA(F) > slow-EMA(S) -> long, ride until ef<=es. Folds the Mac ibkrcrypto EMAx(20,50). Research-faithful EMA seed (4*p). ride_to_flip long-only. THE WORKHORSE (~55% of book P&L).
    ROC,            // S-2026-07-21 (port): N-bar rate-of-change > thr -> long, ride until roc<=thr. Folds the Mac ibkrcrypto Roc(20,0.0). ride_to_flip long-only. momentum satellite.
    IBS             // S-2026-07-21 (port): standalone Internal-Bar-Strength (c-l)/(h-l) < lo -> long oversold close, exit when v>=lo. Folds the Mac ibkrcrypto IBS(0.15,0.85). mean-rev satellite (NOT the REGIME_SWITCH chop sub-branch).
};

inline const char* strategy_name(StrategyKind k) {
    switch (k) {
        case StrategyKind::TSMOM:          return "TSMOM";
        case StrategyKind::DONCHIAN:       return "DONCHIAN";
        case StrategyKind::BOLLINGER:      return "BOLLINGER";
        case StrategyKind::RSI_REVERT:     return "RSI_REVERT";
        case StrategyKind::OVERNIGHT:      return "OVERNIGHT";
        case StrategyKind::WEEKDAY:        return "WEEKDAY";
        case StrategyKind::KELTNER_REVERT: return "KELTNER_REVERT";
        case StrategyKind::DUAL_THRUST:    return "DUAL_THRUST";
        case StrategyKind::ICHIMOKU:       return "ICHIMOKU";
        case StrategyKind::SUPERTREND:     return "SUPERTREND";
        case StrategyKind::WILLIAMS_R:     return "WILLIAMS_R";
        case StrategyKind::STOCH_RSI:      return "STOCH_RSI";
        case StrategyKind::BREAKOUT_PULLBACK: return "BREAKOUT_PULLBACK";
        case StrategyKind::MIMIC:         return "MIMIC";
        case StrategyKind::KELTNER_BREAK:  return "KELTNER_BREAK";
        case StrategyKind::REGIME_SWITCH:  return "REGIME_SWITCH";
        case StrategyKind::EMAX:           return "EMAX";
        case StrategyKind::ROC:            return "ROC";
        case StrategyKind::IBS:            return "IBS";
    }
    return "UNK";
}

// S54: trend/breakout kinds need a REAL trend to have edge — they churn losses
// in chop. Mean-reversion + session kinds earn in chop (or are regime-neutral).
// Used by the entry gate to require BULL_TREND for trend kinds but allow
// BULL_CHOP for the rest (the "TSMOM off / mean-revert on in chop" design).
inline bool is_trend_kind(StrategyKind k) {
    switch (k) {
        case StrategyKind::TSMOM:
        case StrategyKind::DONCHIAN:
        case StrategyKind::DUAL_THRUST:
        case StrategyKind::ICHIMOKU:
        case StrategyKind::SUPERTREND:
        case StrategyKind::BREAKOUT_PULLBACK:
        case StrategyKind::MIMIC:
        case StrategyKind::KELTNER_BREAK:  // upper-band breakout = trend kind
        case StrategyKind::EMAX:           // EMA cross = trend kind
        case StrategyKind::ROC:            // rate-of-change momentum = trend kind
            return true;
        default:               // BOLLINGER/RSI_REVERT/KELTNER_REVERT/WILLIAMS_R/
            return false;      // STOCH_RSI/OVERNIGHT/WEEKDAY/IBS — ok in chop
    }
}

class EdgeEngine {
public:
    struct Config {
        std::string  symbol;        // "btcusdt"
        std::string  tag;           // short label e.g. "BTC-TSMOM-D1"
        StrategyKind kind;
        int64_t      tf_secs    = 21600;   // bar timeframe (1h=3600, 6h=21600, 1d=86400)
        int          lookback   = 20;
        int          hold_bars  = 12;
        double       sl_atr_mult = 2.5;
        int          atr_period  = 14;
        // MIMIC (S-2026-07-03): wide W-bar mimic, ride to symmetric down-jump
        // flip. ride_to_flip => NO trade-level price stops (vault
        // UpMoveTrailLossMitigation: stops destroy the up-move edge; protection =
        // the separate companion clip only).
        int          mimic_w    = 24;
        double       mimic_thr  = 0.08;
        bool         ride_to_flip = false;
        // BOLLINGER:
        double       bb_k        = 2.0;
        // RSI_REVERT:
        double       rsi_threshold = 30.0;
        // Optional cost-bp deducted from logged net P&L (display only — does not
        // affect signal):
        double       round_trip_bp = 10.0;
        // Max bar buffer history kept (must be >= max(lookback, bb_len, atr_period)+5)
        int          max_history = 64;

        // ── P0/S46: gap-honest exit fill ──────────────────────────────────
        // A stop guarantees you EXIT, never the PRICE. If a tick gaps BELOW the
        // effective stop, the real fill is at that penetrating price, not the
        // stop level. Recording the stop level was the optimism that booked SEI
        // at -170 when it actually traded -422. Default true = fill honestly at
        // the breaching price. gap_extra_slip_bp adds book-depth slippage on top.
        // Harness --legacy-stop-fill sets this false to reproduce old numbers.
        bool         realistic_gap_fill = true;
        double       gap_extra_slip_bp  = 0.0;

        // ── Trailing stop parameters ──────────────────────────────────────
        // trail_arm_atr: profit (in ATR multiples) required before trail
        //   activates. E.g. 1.0 means price must reach entry + 1.0*ATR.
        // trail_dist_atr: once armed, trail sits this far below peak price
        //   (in ATR multiples). E.g. 0.5 means trail_stop = peak - 0.5*ATR.
        // Trail only ratchets UP. Effective stop = max(hard_sl, trail_stop).
        double       trail_arm_atr  = 1.0;
        double       trail_dist_atr = 0.5;

        // ── Staged ratcheting trail (Session 24) ─────────────────────────
        // When unrealised profit exceeds trail_tighten_atr * ATR, the trail
        // distance tightens from trail_dist_atr to trail_tighten_dist_atr.
        // Set trail_tighten_atr = 0.0 to disable (default — no tightening).
        double       trail_tighten_atr      = 0.0;   // 0 = disabled; e.g. 3.0
        double       trail_tighten_dist_atr = 0.3;   // tighter trail once threshold hit

        // ── BE-Lock (Session 31) ────────────────────────────────────────
        // When trail arms, the trail stop is GUARANTEED to be at or above
        // breakeven (entry_px + round_trip_bp). This prevents parameter
        // combos where trail_dist > trail_arm from leaving the trail below
        // entry. A winner can NEVER become a loser once the trail arms.
        // Always on — no toggle needed. Uses round_trip_bp for the BE level.

        // ── BP-based staged ratchet (Session 32b — tighter profit lock) ────
        // Three-stage profit-protection runs in parallel with ATR trail.
        // Whichever stop is tighter wins. Stages:
        //
        // 1. HARD FLOOR (mfe < ratchet_start_bp):
        //    sl = max(atr_sl, entry * (1 + hard_floor_bp/1e4))
        //
        // 2. EARLY RAMP (ratchet_start_bp <= mfe < be_arm_bp):
        //    Linear ramp from -50bp to 0bp (BE) as MFE traverses
        //    [ratchet_start_bp, be_arm_bp]. Rescues "almost made it" trades
        //    that previously died at -100 floor.
        //
        // 3. FULL LOCK (mfe >= be_arm_bp):
        //    locked_bp = round_trip_bp + (mfe - be_arm_bp) * ratchet_lock_pct
        //
        // PLUS first-bar reversal kill (independent of stage): if MFE
        // never crossed early_kill_mfe AND unrealised < early_kill_bp,
        // exit immediately (catches dead-on-arrival dumps).
        double       hard_floor_bp     = -100.0;  // absolute per-position loss cap
        // S44L G: MFE-trail standalone. Exit when current_bp / mfe_bp < retain
        // AND mfe_bp >= min_mfe. 0 = off.
        double       mfe_trail_retain  = 0.0;   // e.g., 0.50 = exit if give back 50% of MFE
        double       mfe_trail_min_bp  = 50.0;  // only active once MFE crosses this
        // S44L H: swing-low SL — set SL = max(atr_sl, swing_low_N). 0 = off.
        int          swing_low_bars    = 0;     // lookback bars for swing-low
        // S44M #2: low-vol entry filter. If current ATR < ratio × avg_ATR_N,
        // skip entry (chop suppression). 0 = off.
        double       low_vol_skip_ratio = 0.0;
        int          low_vol_avg_bars   = 20;
        double       ratchet_start_bp  = 15.0;    // earliest partial protection (Stage 2 begins)
        double       be_arm_bp         = 50.0;    // BE lock threshold (Stage 3 begins)
        double       ratchet_lock_pct  = 0.75;    // base lock_pct (mfe 50-100 band)
        double       early_kill_bp     = -50.0;   // exit if unrealised < this AND mfe < early_kill_mfe
        double       early_kill_mfe    = 10.0;    // MFE threshold below which early-kill arms
        // S35-cluster: minimum hold time before EARLY_KILL can fire (ms).
        // Lets fresh entries breathe past initial spread/noise. hard_floor
        // still catches catastrophe. Tape showed kills at 0-2m post-entry
        // wiping high-correlation alt baskets — this throttles that.
        int64_t      early_kill_min_hold_ms = 0;  // 0 = disabled (back-compat)

        // ── BIG WINNER PROTECTION (Session 32c) ─────────────────────────
        // Two extra layers stacked on top of staged ratchet, protecting
        // trades that reach big MFE from giving back too much.
        //
        // A. PROGRESSIVE LOCK_PCT — lock fraction grows with MFE.
        //    mfe band   lock_pct
        //    50-100     ratchet_lock_pct (base, default 0.75)
        //    100-200    prog_lock_pct_2 (default 0.85)
        //    200-300    prog_lock_pct_3 (default 0.90)
        //    300+       prog_lock_pct_4 (default 0.95)
        double       prog_lock_pct_2 = 0.85;  // mfe 100-200 lock fraction
        double       prog_lock_pct_3 = 0.90;  // mfe 200-300 lock fraction
        double       prog_lock_pct_4 = 0.95;  // mfe 300+    lock fraction
        //
        // B. GIVEBACK CAP — once MFE crosses giveback_arm_bp, force exit
        //    when current unrealised drops by giveback_pct * peak_mfe.
        //    Catches sharp reversals that ratchet doesn't keep up with.
        //    Set giveback_arm_bp = 0 to disable.
        double       giveback_arm_bp = 100.0;  // arm at peak MFE >= 100bp
        double       giveback_pct    = 0.30;   // exit if pullback >= 30% of peak

        // ── Smart Pyramid (Session 31) ──────────────────────────────────
        // Adds to position ONLY after trail is armed (BE locked) and profit
        // exceeds pyramid_arm_atr. Each add is pyramid_size_mult * base size.
        // Subsequent adds fire every pyramid_step_atr above the previous add.
        // All pyramid adds exit with the base trade (shared trail stop).
        // Pyramid P&L is tracked separately and reported on exit.
        bool         pyramid_enabled   = false;    // master switch

        // ── S34: confirmation bar gate ──────────────────────────────────────
        // Number of consecutive bars with signal required before entering.
        // 1 = no confirmation (legacy). 2 = wait 1 extra bar for follow-through.
        // Filters DOA setups (signal fires on noise spike then reverts).
        int          signal_confirm_bars = 1;
        double       pyramid_arm_atr   = 2.5;      // first add at +2.5 ATR profit
        double       pyramid_step_atr  = 1.5;      // subsequent adds every +1.5 ATR after
        double       pyramid_size_mult = 0.5;      // 50% of base size per add
        int          pyramid_max_adds  = 2;         // max pyramid additions per trade

        // ── OVERNIGHT strategy parameters ────────────────────────────────
        // entry_hour_utc: the UTC hour at which the H1 bar must close for
        //   signal to fire. Default 21 = the 21:00-22:00 bar close.
        int          entry_hour_utc = 21;

        // ── WEEKDAY strategy parameters ──────────────────────────────────
        // entry_dow: day-of-week for entry (0=Sunday, 1=Monday, ..., 6=Saturday)
        int          entry_dow = 1;  // Monday
        // sma_len: SMA length for the momentum filter (close > SMA to enter)
        int          sma_len = 5;

        // ── KELTNER_REVERT parameters (Session 28) ──────────────────────
        // keltner_ema_len: EMA period for Keltner channel midline (default 20)
        int          keltner_ema_len = 20;
        // keltner_atr_mult: ATR multiplier for channel width (default 2.0)
        double       keltner_atr_mult = 2.0;
        // S-2026-07-21 (crypto-keltner-pool-fix): faithful-Keltner exit selector.
        // The original KELTNER_BREAK (S-2026-07-12) rides a long until close falls
        // through the LOWER band — a DIFFERENT engine from the validated research
        // Kelt (which goes FLAT the moment close re-enters the band, i.e. close no
        // longer > upper band). The ride-to-lower exit is a divergence from the
        // stated "folds ibkrcrypto Kelt(20,2.0)" intent (verified: SOL Kelt was
        // +1775% vs validated +315%). When TRUE, KELTNER_BREAK exits on re-entry
        // of the band (close <= upper) = the validated research Kelt long-only path.
        // DEFAULT FALSE preserves the existing (divergent) behavior for the already-
        // wired main.cpp g_slots (ADA/AAVE/XLM/XRP/GRT-KELT-D1) so nothing changes
        // for them; the DirectionalTrendRoster Keltner legs opt IN via make_config.
        bool         keltner_exit_reenter_band = false;

        // ── EMAX / ROC / IBS parameters (S-2026-07-21 DirectionalTrendRoster port) ──
        // Research-faithful to Mac ibkrcrypto_bt.cpp EMAx(F,S)/Roc(N,thr)/IBS(lo,hi).
        // EMAX uses ema_fast/ema_slow with the 4*p seed window (research EMA); ROC
        // uses `lookback` as N with roc_thr; IBS uses ibs_lo/ibs_hi on the last bar.
        int          ema_fast   = 20;    // EMAX fast EMA period
        int          ema_slow   = 50;    // EMAX slow EMA period
        double       roc_thr    = 0.0;   // ROC threshold (fraction, e.g. 0.0)
        double       ibs_lo     = 0.15;  // IBS oversold entry level
        double       ibs_hi     = 0.85;  // IBS overbought (research short leg; long-only spot ignores)

        // ── RSI_REVERT faithful LEVEL-revert selector (S-2026-07-21 final-closeout) ──
        // The original RSI_REVERT (Session 19/21) fires only on a CROSS-UP of the
        // threshold (r_prev<=thr && r_now>thr) using an EXPONENTIAL RSI, and holds
        // for a fixed hold_bars — a DIFFERENT engine from the validated research
        // RSIrev, which is a LEVEL-revert: long WHENEVER RSI<oversold (SMA RSI over
        // N=atr_period bars), ride_to_flip exit the moment RSI>=oversold. The
        // cross-up construction fires ~0 trades on the NDX daily roster leg (+0.0%
        // vs research +54.6%). When TRUE, signal_rsi_revert_ uses the research
        // level-entry + a ride_to_flip flip-out (RSI>=thr). DEFAULT FALSE preserves
        // the existing (cross-up, timed-hold) behavior for every live Session 19/21
        // RSI_REVERT g_slot byte-identically; the DirectionalTrendRoster NDX RSIrev
        // leg opts IN via make_config. Uses rsi_threshold as the oversold level.
        bool         rsi_level_revert = false;

        // ── RSIREV intraday BE-floor mode (S-2026-07-23, SOL+XRP RSIrev port) ──────
        // OPT-IN, DEFAULT FALSE. Only meaningful when rsi_level_revert is ALSO true.
        // When true the leg runs the CERTIFIED honest-intraday BE-floor management from
        // backtest/rsirev_intraday_verify_full_bt.cpp instead of ride_to_flip:
        //   • ride_to_flip=false so check_exits_ stays active (intraday floor lives there);
        //   • the RSI>=thr mean-revert flip-out is evaluated in close_bar_ EVEN THOUGH the
        //     leg is not ride_to_flip (decoupled exit — see close_bar_);
        //   • check_exits_ bypasses the staged-ratchet/early-kill/trail/giveback-cap block
        //     and runs the harness-exact arm+BE-floor+g-giveback stop with honest worse-of
        //     fill (see check_exits_).
        // DEFAULT FALSE ⇒ ZERO behavior change for every existing leg (the block is fully
        // guarded on this flag; the one pre-existing TIME-exit conditional gains
        // `&& !rsi_revert_intraday_floor`, which is `&& true` when the flag is off ⇒ identical).
        bool         rsi_revert_intraday_floor = false;
        // Profit-lock giveback for the RSIREV floor (g0.9 = give back 10% of peak MFE). The
        // armed stop = entry*(1 + peak*(1-g)), floored at BE. Certified g=0.9 (cert config).
        double       rsirev_giveback_g = 0.9;

        // ── VOL-TARGET sizing (S-2026-07-21 crypto-keltner-pool-fix) ────────────
        // Ported from Crypto/src/ibkrcrypto_bt.cpp (Cfg.vt_target/vt_lb/vt_min/vt_max)
        // and crypto_oos_engine_port.sizer. Per-trade size multiplier set AT ENTRY:
        //   size = clamp(vt_target / realized_daily_vol(vt_lb), vt_min, vt_max)
        // vt_target=0 -> size=1.0 (off; the live-engine default, unchanged). The
        // DirectionalTrendRoster + VolTargetPool layer set vt_target=0.020 on the
        // trend/Kelt/Regime/Roc legs (IBS + NDX legs stay 0). vt_max MUST be 1.50 to
        // match the C++ ref (the Python port's 1.0 undersized every leg — corrected).
        double       vt_target  = 0.0;   // target daily notional vol (0 = off)
        int          vt_lb      = 20;    // realized-vol lookback (days)
        double       vt_min     = 0.10;  // min size multiplier
        double       vt_max     = 1.50;  // max size multiplier (== C++ Cfg.vt_max)

        // ── DUAL_THRUST parameters (Session 28) ─────────────────────────
        // dt_k1: multiplier for range to compute upper trigger (default 0.5)
        double       dt_k1 = 0.5;
        // dt_range_bars: number of prior bars to compute the range (default 4)
        int          dt_range_bars = 4;

        // ── BREAKOUT_PULLBACK parameters (S38) ──────────────────────────
        // Wait for a close above the prior `lookback`-bar high (the breakout),
        // then enter on a later bar that pulls back to the breakout level
        // and reclaims it (low <= level, close > level, close > open).
        // bp_max_age: max bars since the breakout to still accept the pullback
        // entry. Stops you chasing breakouts that have already extended too far.
        int          bp_max_age = 5;

        // ── Volatility regime filter (Session 28) ────────────────────────
        // When enabled, suppresses counter-trend entries (RSI/BOLL/KELTNER)
        // during elevated volatility as measured by ATR(14)/ATR(50) ratio.
        // Set vol_filter = false to disable (default for trend-following).
        bool         vol_filter = false;
        double       vol_chaos_threshold    = 2.0;  // ratio above this = suppress ALL (raised from 1.6 — shadow tuning: let engines trade in elevated vol)
        double       vol_elevated_threshold = 1.8;  // ratio above this = suppress counter-trend only (raised from 1.5 — shadow tuning)

        // ── Multi-timeframe gate (Session 28) ────────────────────────────
        // When enabled, suppresses counter-trend entries (RSI/BOLL/KELTNER)
        // when the D1 TSMOM trend for this symbol is bearish. Prevents
        // mean-reversion entries against a strong daily downtrend.
        // The D1 state is fed externally via set_d1_bullish().
        bool         mtf_gate = false;

        // ── ADX regime filter (Session 29) ───────────────────────────────
        // When enabled, suppresses TREND-FOLLOWING entries (TSMOM/DONCHIAN/
        // DUAL_THRUST/ICHIMOKU/SUPERTREND) when ADX(14) < adx_threshold.
        // Prevents whipsaw entries in ranging/choppy markets.
        // Does NOT affect counter-trend strategies (they want low ADX).
        bool         adx_filter = false;
        int          adx_period = 14;
        double       adx_threshold = 12.0;  // ADX must be >= this for trend entry (lowered from 20 — shadow tuning: 20 blocks most ranging markets)

        // ── Volume regime filter (Session 29) ────────────────────────────
        // Counts ticks per bar as a volume proxy. If current bar's tick count
        // is below vol_tick_ratio * rolling_avg(vol_tick_lookback bars), suppress
        // ALL entries (detects weekend dead zones and exchange outages).
        // Requires vol_tick_warmup bars of history before activation.
        bool         volume_gate = false;
        double       vol_tick_ratio = 0.15;       // suppress if ticks < 15% of avg (lowered from 30% — shadow tuning: 30% blocks normal quiet periods)
        int          vol_tick_lookback = 10;      // rolling average over N bars
        int          vol_tick_warmup = 5;         // don't activate until N bars seen

        // ── BTC correlation regime filter (Session 29b) ───────────────────
        // When enabled on non-BTC engines, suppresses ALL entries when the
        // symbol's rolling correlation with BTC returns is above threshold.
        // During extreme correlation (herding), individual alpha vanishes.
        // State is fed externally via set_corr_high().
        bool         corr_filter = false;
        double       corr_threshold = 0.90;  // suppress when rolling corr > this

        // ── Time-of-day session filter (Session 29b) ─────────────────────
        // Suppresses entries during low-activity sessions. Default suppresses
        // Asian session (00:00-08:00 UTC) for sub-H6 timeframes where the
        // low-liquidity creates adverse fills and false breakouts.
        bool         session_filter = false;
        int          session_suppress_start = 0;   // UTC hour start (inclusive)
        int          session_suppress_end   = 8;   // UTC hour end (exclusive)

        // ── Portfolio-level gate (Session 29b) ───────────────────────────
        // External gate set by main.cpp when max concurrent positions reached
        // or drawdown circuit breaker triggered. Engine will not enter when false.
        // This is NOT a config toggle — it's dynamic state managed externally.
        // (Included in Config section for documentation only; actual state below)

        // ── ICHIMOKU parameters (Session 29) ─────────────────────────────
        // Tenkan-sen (conversion line): midpoint of highest high & lowest low
        //   over tenkan_period bars (default 9 → mapped to crypto: 20)
        // Kijun-sen (base line): midpoint over kijun_period bars (default 26 → 60)
        // Senkou Span A: midpoint of Tenkan & Kijun (no displacement in our use)
        // Senkou Span B: midpoint of highest/lowest over senkou_b_period (default 52 → 120)
        // Signal: price closes above the cloud AND Tenkan > Kijun
        int          ichi_tenkan_period  = 20;
        int          ichi_kijun_period   = 60;
        int          ichi_senkou_b_period = 120;

        // ── SUPERTREND parameters (Session 29) ──────────────────────────
        // SuperTrend is computed as: HL2 +/- multiplier * ATR(period).
        // Signal fires when SuperTrend flips from bearish to bullish (price
        // crosses above the upper band after being below).
        double       st_multiplier = 3.0;
        int          st_atr_period = 10;

        // ── WILLIAMS_R parameters (Session 29b) ─────────────────────────
        // Williams %R = (Highest High - Close) / (Highest High - Lowest Low) * -100
        // Range: -100 (oversold) to 0 (overbought). Signal: cross up from below threshold.
        int          willr_period = 14;
        double       willr_threshold = -80.0;  // oversold level (buy when crosses up from here)

        // ── STOCH_RSI parameters (Session 29b) ──────────────────────────
        // Stochastic RSI = (RSI - minRSI(N)) / (maxRSI(N) - minRSI(N))
        // Range: 0 to 100. Signal: crosses up from below threshold.
        // Faster oscillator than raw RSI — catches reversals sooner.
        int          stochrsi_rsi_period = 14;
        int          stochrsi_stoch_period = 14;
        double       stochrsi_threshold = 20.0;  // oversold level
    };

    // -----------------------------------------------------------------------
    // SeedBar — one historical OHLC bar supplied to seed_bars().
    // Decoupled from any specific REST client so EdgeEngine.hpp stays free
    // of curl/openssl includes. main.cpp converts BinanceREST::Kline to this.
    // -----------------------------------------------------------------------
    struct SeedBar {
        int64_t open_ts_ms = 0;
        double  o = 0.0;
        double  h = 0.0;
        double  l = 0.0;
        double  c = 0.0;
    };

    // -----------------------------------------------------------------------
    // TradeRecord — emitted via on_trade callback after every exit.
    // main.cpp persists these to disk for the dashboard trade history.
    // -----------------------------------------------------------------------
    struct TradeRecord {
        std::string tag;
        std::string symbol;
        std::string strategy;
        std::string reason;      // "SL", "TRAIL", "TIME", "KILL"
        int64_t     entry_ts_ms  = 0;
        int64_t     exit_ts_ms   = 0;
        double      entry_px     = 0.0;
        double      exit_px      = 0.0;
        double      sl_px        = 0.0;
        double      gross_bp     = 0.0;
        double      net_bp       = 0.0;
        double      mfe_bp       = 0.0;  // max favourable excursion
        int         trade_num    = 0;    // sequential trade number
        bool        shadow       = true;
        // Pyramid P&L (Session 31) — combined result of all pyramid adds
        int         pyramid_adds = 0;    // how many pyramid adds were executed
        double      pyramid_bp   = 0.0;  // combined net P&L from all pyramid adds
        double      total_net_bp = 0.0;  // base net_bp + pyramid_bp (full trade result)
    };

    using TradeCallback = std::function<void(const TradeRecord&)>;

    // Pyramid callback — fired when a pyramid add should be executed.
    // main.cpp receives (tag, price, size_mult, add_number) and places the order.
    using PyramidCallback = std::function<void(const std::string& tag, double price, double size_mult, int add_num)>;

    // -----------------------------------------------------------------------
    // BarRecord — emitted via on_bar callback after every completed bar.
    // main.cpp persists these to disk for warm-start and audit trail.
    // -----------------------------------------------------------------------
    struct BarRecord {
        std::string tag;
        int64_t     open_ts_ms  = 0;
        int64_t     tf_secs     = 0;
        double      o           = 0.0;
        double      h           = 0.0;
        double      l           = 0.0;
        double      c           = 0.0;
        double      atr         = 0.0;
        double      momentum_pct = 0.0;
        bool        signal_ready = false;
        bool        signal_fired = false;
        bool        in_position  = false;
        int         bars_in_buffer = 0;
    };

    using BarCallback = std::function<void(const BarRecord&)>;

    // OrderIntentRecord — emitted on entry and exit transitions so main.cpp
    // can mirror intents into SpotExecutor for shadow-mode paper broker
    // behavior. Fires BEFORE on_trade_ (which only fires on exit).
    struct OrderIntentRecord {
        std::string tag;
        std::string symbol;
        bool        is_buy = true;
        double      ref_px = 0.0;
        int64_t     ts_ms  = 0;
        // P1/S46: safety size multiplier (DD-throttle x vol-overlay), carried to
        // main.cpp so the live qty calc finally applies them. Was previously
        // computed into sizing_mult_ and discarded (never reached order qty).
        double      risk_mult = 1.0;
        // Phase-4 item 21: correlation-ID threaded from the raw signal so the
        // whole chain (signal -> gate -> target -> order -> fill -> pnl) is
        // resolvable in the GateAttribution store. 0 => no sink attached.
        uint64_t    corr_id = 0;
    };

    using OrderIntentCallback = std::function<void(const OrderIntentRecord&)>;

    bool shadow_mode = true;  // public for main.cpp init parity with old engines

    // Phase-4 item 21: attach a gate-attribution sink (observational only —
    // records each raw signal's per-gate suppression reason + counterfactual +
    // correlation-ID). Never alters signal/exit logic. null => no-op.
    void set_gate_sink(GateAttribution* s) { gate_sink_ = s; }

    // Set a callback to receive trade records on each exit.
    void set_on_trade(TradeCallback cb) { on_trade_ = std::move(cb); }

    // Set a callback to receive bar records on each bar close.
    void set_on_bar(BarCallback cb) { on_bar_ = std::move(cb); }

    // Set a callback for pyramid add events (main.cpp executes additional buy).
    void set_on_pyramid(PyramidCallback cb) { on_pyramid_ = std::move(cb); }

    // Set a callback for entry/exit order intents (paper broker mirror).
    void set_on_order_intent(OrderIntentCallback cb) { on_order_intent_ = std::move(cb); }

    // Read-only access to the engine's config (used by main.cpp pyramid callback).
    const Config& cfg() const { return cfg_; }

    explicit EdgeEngine(const Config& cfg) : cfg_(cfg) {
        // CH-09 (audit 2026-07-13): zero/negative timeframe divides-by-zero in on_tick.
        // Audit suggested std::abort() — REJECTED (a config typo must not crash-loop a live
        // book). Clamp to safe defaults + log LOUD so boot review catches it.
        if (cfg_.tf_secs   <= 0) { std::fprintf(stderr, "[EDGE][CONFIG-GUARD] %s tf_secs=%lld -> clamp 3600\n", cfg_.tag.c_str(), (long long)cfg_.tf_secs); cfg_.tf_secs = 3600; }
        if (cfg_.atr_period <= 0) cfg_.atr_period = 14;
        if (cfg_.lookback   <= 0) cfg_.lookback   = 20;
        if (cfg_.max_history < cfg_.lookback + 5)  cfg_.max_history = cfg_.lookback + 5;
        if (cfg_.max_history < cfg_.atr_period + 5) cfg_.max_history = cfg_.atr_period + 5;
        if (cfg_.max_history < cfg_.sma_len + 5)    cfg_.max_history = cfg_.sma_len + 5;
        // Keltner needs EMA history
        if (cfg_.max_history < cfg_.keltner_ema_len + 5) cfg_.max_history = cfg_.keltner_ema_len + 5;
        // EMAX (S-2026-07-21 port) needs the research 4*slow EMA seed window
        if (cfg_.kind == StrategyKind::EMAX && cfg_.max_history < 4 * cfg_.ema_slow + 5)
            cfg_.max_history = 4 * cfg_.ema_slow + 5;
        // ROC needs lookback+1 bars
        if (cfg_.kind == StrategyKind::ROC && cfg_.max_history < cfg_.lookback + 5)
            cfg_.max_history = cfg_.lookback + 5;
        // DUAL_THRUST needs range_bars + 1
        if (cfg_.max_history < cfg_.dt_range_bars + 5) cfg_.max_history = cfg_.dt_range_bars + 5;
        // Vol filter needs ATR(50) which needs 51 bars
        if (cfg_.vol_filter && cfg_.max_history < 56) cfg_.max_history = 56;
        // ADX filter needs adx_period + 2 bars
        if (cfg_.adx_filter && cfg_.max_history < cfg_.adx_period + 5) cfg_.max_history = cfg_.adx_period + 5;
        // Ichimoku needs senkou_b_period + 1 bars
        if (cfg_.kind == StrategyKind::ICHIMOKU && cfg_.max_history < cfg_.ichi_senkou_b_period + 5)
            cfg_.max_history = cfg_.ichi_senkou_b_period + 5;
        // SuperTrend needs st_atr_period + 2
        if (cfg_.kind == StrategyKind::SUPERTREND && cfg_.max_history < cfg_.st_atr_period + 5)
            cfg_.max_history = cfg_.st_atr_period + 5;
        if (g_edge_arm_quiet) {
            // LIVE-ONLY CULL: suppress the per-engine ARMED line (dead-engine boot noise);
            // count it for the aggregate [LIVE-ONLY-GATE] line instead (no dead names shown).
            ++g_edge_arm_suppressed;
        } else {
            std::printf("[%s] ARMED  symbol=%s strat=%s tf=%llds lookback=%d hold=%d sl=%.2f*atr trail_arm=%.1f*atr trail_dist=%.1f*atr  shadow=%d\n",
                cfg_.tag.c_str(), cfg_.symbol.c_str(),
                strategy_name(cfg_.kind),
                (long long)cfg_.tf_secs, cfg_.lookback, cfg_.hold_bars, cfg_.sl_atr_mult,
                cfg_.trail_arm_atr, cfg_.trail_dist_atr,
                shadow_mode ? 1 : 0);
            std::fflush(stdout);
        }
    }

    // -----------------------------------------------------------------------
    // seed_bars — pre-populate the closed-bar history from REST klines.
    //
    // Called once at startup before the live tick stream begins. Bars must
    // arrive OLDEST-FIRST (which is how Binance returns them). After seeding,
    // cur_bar_id_ is set to the last seeded bar's id so the next live tick
    // either extends the current (partial) bar or starts a fresh one cleanly.
    //
    // Returns the number of bars actually inserted (after max_history trim).
    // Safe to call with an empty vector (no-op).
    // -----------------------------------------------------------------------
    int seed_bars(const std::vector<SeedBar>& bars) {
        if (bars.empty()) return 0;

        int64_t prev_ts = bar_ts_ms_.empty() ? 0 : bar_ts_ms_.back();
        for (const auto& b : bars) {
            if (b.o <= 0.0 || b.h <= 0.0 || b.l <= 0.0 || b.c <= 0.0) continue;
            // CH-10 (audit 2026-07-13): reject NaN/inf, non-monotonic ts, malformed OHLC.
            if (!(std::isfinite(b.o) && std::isfinite(b.h) && std::isfinite(b.l) && std::isfinite(b.c))) continue;
            if (b.open_ts_ms <= prev_ts) continue;
            if (b.h < b.l || b.h < b.o || b.h < b.c || b.l > b.o || b.l > b.c) continue;
            prev_ts = b.open_ts_ms;
            opens_.push_back(b.o);
            highs_.push_back(b.h);
            lows_.push_back(b.l);
            closes_.push_back(b.c);
            bar_ts_ms_.push_back(b.open_ts_ms);
            // Seed bars get a synthetic tick count (use average = 100 so volume
            // gate doesn't suppress on startup)
            tick_counts_.push_back(100);
        }

        // Trim to max_history (drop oldest first).
        while ((int)closes_.size() > cfg_.max_history) {
            opens_.pop_front();
            highs_.pop_front();
            lows_.pop_front();
            closes_.pop_front();
            bar_ts_ms_.pop_front();
            tick_counts_.pop_front();
        }

        if (!closes_.empty()) {
            last_close_ = closes_.back();

            // Anchor cur_bar_id_ to the most recent SEEDED bar so the first
            // live tick after seeding doesn't fire close_bar_() with garbage.
            // open_ts_ms is the bar OPEN timestamp; the corresponding bar id
            // is (open_ts_ms / 1000) / tf_secs.
            int64_t last_open_ts_ms = bar_ts_ms_.back();
            cur_bar_id_     = last_open_ts_ms / 1000 / cfg_.tf_secs;
            cur_open_ts_ms_ = cur_bar_id_ * cfg_.tf_secs * 1000;

            // Initialise the in-flight bar at the last close so the first
            // live tick either updates the high/low/close of this same bar
            // (if still within its window) or rolls forward via the normal
            // gap-fill path in on_tick().
            cur_open_  = last_close_;
            cur_high_  = last_close_;
            cur_low_   = last_close_;
            cur_close_ = last_close_;
        }

        std::printf("[%s] SEED   bars_in=%d closes_kept=%d last_close=%.6f\n",
            cfg_.tag.c_str(),
            (int)bars.size(), (int)closes_.size(), last_close_);
        std::fflush(stdout);

        return (int)closes_.size();
    }

    // Called on every spot tick for this symbol. Builds bars internally.
    void on_tick(double price, int64_t ts_ms) {
        if (price <= 0.0) return;
        int64_t bar_id = ts_ms / 1000 / cfg_.tf_secs;
        if (cur_bar_id_ == 0) {
            // First tick — begin a new bar at this id.
            cur_bar_id_ = bar_id;
            cur_open_ = cur_high_ = cur_low_ = cur_close_ = price;
            cur_open_ts_ms_ = bar_id * cfg_.tf_secs * 1000;
            cur_tick_count_ = 1;
        } else if (bar_id < cur_bar_id_) {
            // CH-01 (audit 2026-07-13, test_edge_bar_chronology.cpp): a LATE / out-of-order
            // tick lands in a bar that has already closed. The old code fell straight into
            // the boundary branch below, rewound cur_bar_id_ backwards and re-closed the
            // already-closed period — corrupting indicator history on ONE delayed event.
            // Drop it (and count it for telemetry); a closed bar is immutable.
            stale_tick_count_++;
            return;
        } else if (bar_id != cur_bar_id_) {
            // Bar boundary crossed — close out the previous bar then open new ones
            // for every full bar gap (in case of feed silence).
            close_bar_();
            int64_t gap = bar_id - cur_bar_id_;
            // CH-01: cap synthetic filler bars — a single bogus far-future timestamp
            // must not spin millions of empty closes. Beyond the cap, jump straight to
            // the new bar (still forward, still monotonic).
            if (gap > 1000) { stale_tick_count_++; gap = 1; }
            for (int64_t i = 1; i < gap; ++i) {
                // synthesise an empty filler bar at last close (rare in crypto;
                // happens during exchange outages)
                cur_bar_id_ += 1;
                cur_open_ts_ms_ = cur_bar_id_ * cfg_.tf_secs * 1000;
                cur_open_ = cur_high_ = cur_low_ = cur_close_ = last_close_;
                cur_tick_count_ = 0;  // zero-tick filler bar
                close_bar_();
            }
            cur_bar_id_ = bar_id;
            cur_open_ts_ms_ = bar_id * cfg_.tf_secs * 1000;
            cur_open_ = cur_high_ = cur_low_ = cur_close_ = price;
            cur_tick_count_ = 1;
        } else {
            if (price > cur_high_) cur_high_ = price;
            if (price < cur_low_)  cur_low_  = price;
            cur_close_ = price;
            cur_tick_count_++;
        }

        // Intra-bar exit check (so we don't miss the stop until the next bar boundary)
        if (in_position_) {
            check_exits_(price, ts_ms);
        }

        // ── MIMIC intra-bar ENTRY (S-2026-07-05, operator: no boundary wait) ──
        // The mimic is a PRICE event, not a bar-close event. Fire the instant
        // the live price crosses the jump threshold vs the close W bars back —
        // do NOT wait for the H1 boundary. Full gate chain still applies inside
        // evaluate_signal_ (cluster/regime already cut for MIMIC; funding/vol/
        // confirmation still run). Guarded to at most one attempt per forming
        // bar so a persistent jump doesn't spam entry-eval every tick; the H1
        // close path (close_bar_) remains as the backstop.
        // Fire on EITHER a fresh live jump (intrabar_mimic_fires_, cheap O(1))
        // OR an already-standing mimic regime (mimic_state_()==1) — a coin
        // that entered mimic regime on a prior bar and is flat only because
        // entry hadn't re-evaluated must open NOW, not wait for the H1 close.
        if (!in_position_ && !halted_ && cfg_.kind == StrategyKind::MIMIC
            && intrabar_fired_bar_ != cur_bar_id_
            && (intrabar_mimic_fires_(cur_close_) || mimic_state_() == 1)) {
            intrabar_fired_bar_ = cur_bar_id_;
            evaluate_signal_intrabar_(cur_close_, ts_ms);
        }
    }

    // Force-flatten any open paper position at the given price.
    void kill_all(double price, int64_t ts_ms) {
        if (in_position_ && price > 0.0) {
            exit_position_(price, ts_ms, "KILL");
        }
        halted_ = true;
    }

    // Graceful close — flatten open position at given price without halting.
    // Used during orderly shutdown to capture unrealised profits in the journal.
    void graceful_close(double price, int64_t ts_ms) {
        if (in_position_ && price > 0.0) {
            exit_position_(price, ts_ms, "SHUTDOWN");
        }
    }

    // ── Position resume (Session 28) ────────────────────────────────────────
    // Injects a previously-saved position back into the engine after restart.
    // Called from main.cpp after reading data/open_positions.json.
    // The engine then manages the trade (trail, SL, time-exit) as if it was
    // never interrupted.
    struct ResumeState {
        double  entry_px        = 0.0;
        double  sl_px           = 0.0;
        double  atr_at_entry    = 0.0;
        int64_t entry_ts_ms     = 0;
        int64_t time_exit_ts_ms = 0;
        int     bars_held       = 0;
        bool    trail_armed     = false;
        double  trail_stop_px   = 0.0;
        double  trail_arm_px    = 0.0;
        double  mfe_px          = 0.0;
        double  mfe_bp          = 0.0;
        // Pyramid resume (Session 31)
        int     pyramid_count   = 0;
        double  pyramid_next_atr = 0.0;
        std::vector<std::pair<double, double>> pyramid_entries;  // {entry_px, size_mult}
    };

    bool resume_position(const ResumeState& rs) {
        if (in_position_) return false;  // already in a trade somehow
        if (rs.entry_px <= 0.0) return false;
        // CH-10 (audit 2026-07-13): corrupt crash-state must not resurrect a position.
        auto bad = [](double v){ return !std::isfinite(v); };
        if (bad(rs.entry_px) || bad(rs.sl_px) || bad(rs.atr_at_entry) || bad(rs.trail_stop_px) ||
            bad(rs.trail_arm_px) || bad(rs.mfe_px) || bad(rs.mfe_bp)) {
            std::fprintf(stderr, "[EDGE][RESUME-REJECT] %s non-finite state\n", cfg_.tag.c_str());
            return false;
        }
        if (rs.sl_px < 0.0 || rs.sl_px >= rs.entry_px * 2.0 ||
            rs.bars_held < 0 || rs.pyramid_count < 0 || rs.pyramid_count > 16) {
            std::fprintf(stderr, "[EDGE][RESUME-REJECT] %s implausible sl/bars/pyramid\n", cfg_.tag.c_str());
            return false;
        }

        in_position_     = true;
        entry_px_        = rs.entry_px;
        sl_px_           = rs.sl_px;
        atr_at_entry_    = rs.atr_at_entry;
        entry_ts_ms_     = rs.entry_ts_ms;
        time_exit_ts_ms_ = rs.time_exit_ts_ms;
        bars_held_       = rs.bars_held;
        trail_armed_     = rs.trail_armed;
        trail_stop_px_   = rs.trail_stop_px;
        trail_arm_px_    = rs.trail_arm_px;
        mfe_px_          = rs.mfe_px;
        mfe_bp_          = rs.mfe_bp;

        // Resume pyramid state (Session 31)
        pyramid_count_    = rs.pyramid_count;
        pyramid_next_atr_ = rs.pyramid_next_atr;
        pyramid_adds_.clear();
        for (const auto& pe : rs.pyramid_entries) {
            PyramidAdd pa;
            pa.entry_px  = pe.first;
            pa.size_mult = pe.second;
            pyramid_adds_.push_back(pa);
        }

        std::printf("[%s] RESUME  entry=%.6f  sl=%.6f  trail_armed=%d  trail_stop=%.6f  bars_held=%d  mfe=+%.1fbp  pyramids=%d\n",
            cfg_.tag.c_str(), entry_px_, sl_px_, (int)trail_armed_,
            trail_stop_px_, bars_held_, mfe_bp_, pyramid_count_);
        return true;
    }

    // Returns a JSON object string describing the open position, or "" if flat.
    // Contains ALL fields needed for resume_position() after crash recovery.
    std::string position_snapshot_json(double spot_px) const {
        if (!in_position_) return "";
        double unreal_bp = (spot_px > 0.0 && entry_px_ > 0.0)
            ? (spot_px / entry_px_ - 1.0) * 1e4
            : 0.0;
        std::ostringstream js;
        js << std::fixed;
        js << "{";
        js << "\"tag\":\"" << cfg_.tag << "\",";
        js << "\"symbol\":\"" << cfg_.symbol << "\",";
        js << "\"strategy\":\"" << strategy_name(cfg_.kind) << "\",";
        js << std::setprecision(8);
        js << "\"entry_px\":" << entry_px_ << ",";
        js << "\"sl_px\":" << sl_px_ << ",";
        js << "\"atr_at_entry\":" << atr_at_entry_ << ",";
        js << "\"spot_px\":" << spot_px << ",";
        js << std::setprecision(2);
        js << "\"unreal_bp\":" << unreal_bp << ",";
        js << "\"mfe_bp\":" << mfe_bp_ << ",";
        js << "\"entry_ts\":" << entry_ts_ms_ << ",";
        js << "\"time_exit_ts\":" << time_exit_ts_ms_ << ",";
        js << "\"bars_held\":" << bars_held_ << ",";
        js << "\"trail_armed\":" << (trail_armed_ ? "true" : "false") << ",";
        js << std::setprecision(8);
        js << "\"trail_stop_px\":" << trail_stop_px_ << ",";
        js << "\"trail_arm_px\":" << trail_arm_px_ << ",";
        js << "\"mfe_px\":" << mfe_px_ << ",";
        // Pyramid state (Session 31)
        js << "\"pyramid_count\":" << pyramid_count_ << ",";
        js << std::setprecision(2);
        js << "\"pyramid_next_atr\":" << pyramid_next_atr_ << ",";
        js << "\"pyramid_adds\":[";
        for (int i = 0; i < (int)pyramid_adds_.size(); ++i) {
            if (i > 0) js << ",";
            js << std::setprecision(8);
            js << "{\"entry_px\":" << pyramid_adds_[i].entry_px;
            js << ",\"size_mult\":" << std::setprecision(2) << pyramid_adds_[i].size_mult << "}";
        }
        js << "]";
        js << "}";
        return js.str();
    }

    // JSON state line for /api/state (one object per engine; main.cpp wraps in array).
    std::string state_json() const {
        std::ostringstream js;
        js << "{";
        js << "\"tag\":\""    << cfg_.tag    << "\",";
        js << "\"symbol\":\"" << cfg_.symbol << "\",";
        js << "\"strategy\":\"" << strategy_name(cfg_.kind) << "\",";
        js << "\"tf_secs\":" << cfg_.tf_secs << ",";
        js << "\"shadow\":"  << (shadow_mode ? "true" : "false") << ",";
        js << "\"halted\":"  << (halted_     ? "true" : "false") << ",";
        js << "\"in_position\":" << (in_position_ ? "true" : "false") << ",";
        js << std::fixed << std::setprecision(6);
        js << "\"entry_px\":"  << (in_position_ ? entry_px_ : 0.0)  << ",";
        js << "\"sl_px\":"     << (in_position_ ? effective_stop_() : 0.0) << ",";
        js << "\"last_close\":" << last_close_ << ",";
        js << "\"trades\":"    << trades_ << ",";
        js << "\"wins\":"      << wins_   << ",";
        js << std::setprecision(2);
        js << "\"total_bp\":"  << total_bp_      << ",";
        js << "\"last_bp\":"   << last_trade_bp_ << ",";
        js << "\"bars_in_buffer\":" << (int)closes_.size() << ",";
        // Trailing stop state for GUI
        js << "\"trail_armed\":" << (trail_armed_ ? "true" : "false") << ",";
        js << std::setprecision(6);
        js << "\"mfe_px\":" << (in_position_ ? mfe_px_ : 0.0) << ",";
        js << std::setprecision(2);
        js << "\"mfe_bp\":" << (in_position_ ? mfe_bp_ : 0.0) << ",";
        js << std::setprecision(6);
        js << "\"trail_stop_px\":" << (trail_armed_ ? trail_stop_px_ : 0.0) << ",";

        // ── Session 29 filter state for diagnostics ─────────────────────
        js << "\"adx_filter\":" << (cfg_.adx_filter ? "true" : "false") << ",";
        js << std::setprecision(1);
        js << "\"adx_value\":" << adx_(cfg_.adx_period) << ",";
        js << "\"volume_gate\":" << (cfg_.volume_gate ? "true" : "false") << ",";
        js << "\"cur_tick_count\":" << cur_tick_count_ << ",";
        js << "\"avg_tick_count\":" << avg_tick_count_() << ",";

        // ── Diagnostic fields (read-only, no effect on trading logic) ────
        js << "\"lookback\":" << cfg_.lookback << ",";
        js << "\"hold_bars_cfg\":" << cfg_.hold_bars << ",";
        js << "\"sl_atr_mult\":" << std::setprecision(1) << cfg_.sl_atr_mult << ",";
        js << "\"round_trip_bp\":" << std::setprecision(1) << cfg_.round_trip_bp << ",";
        js << "\"trail_arm_atr\":" << std::setprecision(1) << cfg_.trail_arm_atr << ",";
        js << "\"trail_dist_atr\":" << std::setprecision(1) << cfg_.trail_dist_atr << ",";
        js << "\"trail_tighten_atr\":" << std::setprecision(1) << cfg_.trail_tighten_atr << ",";
        js << "\"trail_tighten_dist_atr\":" << std::setprecision(1) << cfg_.trail_tighten_dist_atr << ",";
        // Pyramid state (Session 31)
        js << "\"pyramid_enabled\":" << (cfg_.pyramid_enabled ? "true" : "false") << ",";
        js << "\"pyramid_count\":" << pyramid_count_ << ",";
        js << "\"pyramid_max_adds\":" << cfg_.pyramid_max_adds << ",";
        js << "\"pyramid_arm_atr\":" << std::setprecision(2) << cfg_.pyramid_arm_atr << ",";
        js << "\"pyramid_step_atr\":" << cfg_.pyramid_step_atr << ",";
        js << "\"pyramid_size_mult\":" << cfg_.pyramid_size_mult << ",";
        // S44d audit: expose ALL filter flags + safety preset values so
        // dashboard/API can verify overlays per-engine.
        js << "\"vol_filter\":" << (cfg_.vol_filter ? "true" : "false") << ",";
        js << "\"mtf_gate\":" << (cfg_.mtf_gate ? "true" : "false") << ",";
        js << "\"corr_filter\":" << (cfg_.corr_filter ? "true" : "false") << ",";
        js << "\"ratchet_start_bp\":" << std::setprecision(1) << cfg_.ratchet_start_bp << ",";
        js << "\"be_arm_bp\":" << cfg_.be_arm_bp << ",";
        js << "\"ratchet_lock_pct\":" << std::setprecision(2) << cfg_.ratchet_lock_pct << ",";

        // Momentum: close[now] vs close[now - lookback]
        bool signal_ready = ((int)closes_.size() >= cfg_.lookback + 1);
        double lb_close = 0.0;
        double momentum_pct = 0.0;
        if (signal_ready) {
            lb_close = closes_[closes_.size() - 1 - cfg_.lookback];
            if (lb_close > 0.0)
                momentum_pct = (closes_.back() / lb_close - 1.0) * 100.0;
        }
        js << "\"signal_ready\":" << (signal_ready ? "true" : "false") << ",";
        js << std::setprecision(4);
        js << "\"lookback_close\":" << lb_close << ",";
        js << "\"momentum_pct\":" << momentum_pct << ",";

        // Bar timing: when did the current bar open, when does it close?
        int64_t next_bar_close_ms = (cur_bar_id_ + 1) * cfg_.tf_secs * 1000;
        js << "\"cur_bar_open_ms\":" << cur_open_ts_ms_ << ",";
        js << "\"next_bar_close_ms\":" << next_bar_close_ms << ",";
        js << "\"bars_held\":" << bars_held_;

        js << "}";
        return js.str();
    }

    // Counters
    int trades() const { return trades_; }
    int wins() const { return wins_; }
    double total_bp() const { return total_bp_; }
    bool in_position() const { return in_position_; }
    int bars_in_buffer() const { return (int)closes_.size(); }

    // ── VOL-TARGET sizing (ported, S-2026-07-21) ────────────────────────────
    // realized daily vol over the last vt_lb close-to-close pct returns (population
    // stddev — matches Crypto/src/ibkrcrypto_bt.cpp realized_vol()).
    double realized_vol() const {
        int sz = (int)closes_.size();
        int lb = cfg_.vt_lb;
        if (sz < lb + 1) return 0.0;
        double m = 0.0; int k = 0;
        for (int j = sz - lb; j < sz; ++j) { m += (closes_[j] - closes_[j-1]) / closes_[j-1]; ++k; }
        m /= k;
        double s2 = 0.0;
        for (int j = sz - lb; j < sz; ++j) { double rr = (closes_[j] - closes_[j-1]) / closes_[j-1]; s2 += (rr - m) * (rr - m); }
        return std::sqrt(s2 / k);
    }
    // vol-target size multiplier = clamp(vt_target/realized_vol, vt_min, vt_max).
    // vt_target<=0 -> 1.0 (off). Faithful to research sizer() (vt_max=1.50).
    double vol_target_size() const {
        if (cfg_.vt_target <= 0.0) return 1.0;
        double rv = realized_vol();
        if (rv <= 0.0) return cfg_.vt_min;
        double z = cfg_.vt_target / rv;
        return std::max(cfg_.vt_min, std::min(cfg_.vt_max, z));
    }

    // Unrealised P&L (bp) at given spot price. Returns 0 if flat.
    // Used by main.cpp aggregate drawdown circuit (Session 32).
    double unrealised_bp(double spot_px) const {
        if (!in_position_ || entry_px_ <= 0.0 || spot_px <= 0.0) return 0.0;
        return (spot_px / entry_px_ - 1.0) * 1e4;
    }
    double entry_px() const { return entry_px_; }
    // Live-trade peak favourable price + entry ts — used by MimicCompanionEngine::seed_open()
    // to rehydrate the companion's peak-to-date on restart (S-2026-07-05).
    double  mfe_px()      const { return mfe_px_; }
    int64_t entry_ts_ms() const { return entry_ts_ms_; }
    double last_close() const { return last_close_; }
    int max_history_needed() const { return cfg_.max_history; }

    // Runtime filter activation (can be called after construction)
    void enable_vol_filter(bool b) { cfg_.vol_filter = b; }
    void enable_mtf_gate(bool b)   { cfg_.mtf_gate = b; }
    void enable_adx_filter(bool b) { cfg_.adx_filter = b; }
    void set_adx_threshold(double t) { cfg_.adx_threshold = t; }
    void enable_volume_gate(bool b) { cfg_.volume_gate = b; }
    void enable_corr_filter(bool b) { cfg_.corr_filter = b; }
    void enable_session_filter(bool b) { cfg_.session_filter = b; }

    // ── S34: uniform safety preset ──────────────────────────────────────
    // Force-applies tight protection across all active engines so each one
    // has identical: hard floor, BE lock, tight trail, giveback cap,
    // early-kill. Used by main.cpp at startup to override bespoke per-engine
    // configs that may have been wider than wanted.
    //
    // Spot-only constraint: this codebase only buys (long). No short path
    // exists in maybe_enter_/exit_position_/SpotExecutor.
    // ── S34: PROTECTION-ONLY preset (elite engines, PF >= 2.0) ──────────
    // Keep this engine's bespoke trail_arm/trail_dist/trail_tighten config
    // (those drove the validated PF). Override ONLY the per-trade loss
    // caps + BE lock + giveback so a winner can't turn into a loser and
    // a deadweight trade can't bleed.
    // ── S36 PRESETS — staged-ratchet ONLY ────────────────────────────────
    // Backtest matrix (2026-05-28) over 15 TSMOM engines × 5yr OOS proved:
    //   prod_tiered (giveback 10%@rt+10 + early_kill -25@<15mfe + hard_floor
    //   -50 + signal_confirm=2) = -753,182bp / 0 of 15 engines profitable.
    //   staged_only (BE-ratchet + progressive lock 75/85/90/95% ONLY) =
    //   +274,840bp / 15 of 15 profitable, avg PF 2.44.
    //
    // Per-layer bisection cost vs legacy ATR-trail baseline:
    //   giveback_cap @ rt+10/10%: -942k bp (catastrophic — exits winners at
    //     +24bp before trend extends)
    //   early_kill @ -25bp/<15mfe: -178k bp (kills DOA + valid early dips
    //     indiscriminately)
    //   hard_floor @ -50bp:        -28k bp (cuts winners too small)
    //   signal_confirm_bars=2:     marginally +ve (cuts noise entries) but
    //     does NOT compensate for the three above
    //   staged_ratchet (BE-lock + 75/85/90/95 prog lock): +86k bp (the ONE
    //     beneficial layer — locks profit at correct MFE thresholds without
    //     forcing exit on noise pullbacks)
    //
    // Both presets now DISABLE giveback / early_kill / hard_floor and KEEP
    // staged-ratchet only. signal_confirm_bars=1 (was 2 — marginal benefit
    // not worth code path complexity).
    void apply_protection_only_preset() {
        // Disable destructive layers
        cfg_.hard_floor_bp           =  0.0;   // < 0 = active → 0 = off
        cfg_.early_kill_bp           =  0.0;
        cfg_.early_kill_mfe          =  0.0;
        cfg_.early_kill_min_hold_ms  =  0;
        cfg_.giveback_arm_bp         =  0.0;   // > 0 = active → 0 = off
        cfg_.signal_confirm_bars     =  1;

        // KEEP staged BE-ratchet (the one beneficial layer)
        double rt = cfg_.round_trip_bp;
        cfg_.ratchet_start_bp  = rt;
        cfg_.be_arm_bp         = rt + 10.0;
        cfg_.ratchet_lock_pct  = 0.75;
        cfg_.prog_lock_pct_2   = 0.85;
        cfg_.prog_lock_pct_3   = 0.90;
        cfg_.prog_lock_pct_4   = 0.95;

        // INTENTIONALLY NOT touched: trail_arm_atr, trail_dist_atr,
        // trail_tighten_atr, trail_tighten_dist_atr — preserve bespoke
        // per-engine trail tuning that drove validated PF.
    }

    // ── S38b: enable_pyramid_xlow ─────────────────────────────────────────
    // Switches pyramid ON with aggressive arm threshold (0.5 ATR profit
    // triggers first add). Backtested across all 4 WF windows on 26k
    // configs: 99.2% of high-PF (>=1.5) candidates gain bp, 0 lose >500bp.
    // Mean lift ~+5-10% on net bp. Pyramid adds only after trail-armed
    // (BE locked) so worst case = adds give back to BE.
    void enable_pyramid_xlow() {
        cfg_.pyramid_enabled    = true;
        cfg_.pyramid_arm_atr    = 0.5;
        cfg_.pyramid_step_atr   = 0.3;
        cfg_.pyramid_size_mult  = 0.5;
        cfg_.pyramid_max_adds   = 1;   // S38b: conservative — 1 add = 1.5x max
    }

    // ── S44: enable_pyramid_elite — validated on 405-engine 180d OOS ──────
    // +2.8% portfolio bp vs xlow, DD -0.1%, 343/405 engines improved bp,
    // 405/405 retained PF >=90%. arm 0.5 ATR, step 0.3 ATR, mult 0.75, 4 adds.
    // Worst case = pyramid adds give back to BE (BE-locked before first add).
    void enable_pyramid_elite() {
        cfg_.pyramid_enabled    = true;
        cfg_.pyramid_arm_atr    = 0.5;
        cfg_.pyramid_step_atr   = 0.3;
        cfg_.pyramid_size_mult  = 0.75;
        cfg_.pyramid_max_adds   = 4;
    }
    // S44f: per-tier pyramid_max override (TOP_ELITE=3, STRONG=3, STANDARD=2)
    void set_pyramid_max_adds(int n) { cfg_.pyramid_max_adds = n; }
    // S44i: per-trade hard floor — tighten SL inward to cap loss at N bp.
    // Sign convention: pass negative bp (e.g., -50.0 for 50bp loss cap).
    // Acts as upper bound on per-position drawdown when atr_sl is wider.
    void set_hard_floor_bp(double bp) { cfg_.hard_floor_bp = bp; }
    void set_realistic_gap_fill(bool b) { cfg_.realistic_gap_fill = b; }
    void set_gap_extra_slip_bp(double bp) { cfg_.gap_extra_slip_bp = bp; }
    // S44k: tune profit-protection bands. be_arm = MFE bp at which BE locks.
    // lock_pct = fraction of MFE above be_arm that's protected.
    void set_be_arm_bp(double bp)     { cfg_.be_arm_bp = bp; }
    void set_ratchet_start_bp(double bp) { cfg_.ratchet_start_bp = bp; }
    void set_ratchet_lock_pct(double p) { cfg_.ratchet_lock_pct = p; }
    void set_mfe_trail(double retain, double min_bp) {
        cfg_.mfe_trail_retain = retain; cfg_.mfe_trail_min_bp = min_bp;
    }
    void set_swing_low_bars(int n) { cfg_.swing_low_bars = n; }
    void set_low_vol_filter(double ratio, int avg_bars) {
        cfg_.low_vol_skip_ratio = ratio; cfg_.low_vol_avg_bars = avg_bars;
    }
    void set_signal_confirm_bars(int n) { cfg_.signal_confirm_bars = n; }
    // S44L F: vol-adaptive SL. When set > 0, on entry compare ATR to its
    // rolling average; if ATR > avg × ratio_threshold, tighten SL by mult.
    double vol_adaptive_ratio_ = 0.0;    // 0 = off
    double vol_adaptive_mult_  = 1.0;    // SL multiplier when triggered
    void set_vol_adaptive(double ratio, double sl_mult) {
        vol_adaptive_ratio_ = ratio; vol_adaptive_mult_ = sl_mult;
    }

    void apply_safety_preset() {
        // Same layer logic as protection_only — destructive layers off,
        // staged ratchet on. Difference vs protection_only: this preset
        // ALSO overrides bespoke trail params with a single uniform set
        // (was tighter trail; now matches Session-14 baseline since trail
        // is dominated by BE-ratchet anyway under staged-only).
        cfg_.hard_floor_bp           =  0.0;
        cfg_.early_kill_bp           =  0.0;
        cfg_.early_kill_mfe          =  0.0;
        cfg_.early_kill_min_hold_ms  =  0;
        cfg_.giveback_arm_bp         =  0.0;
        cfg_.signal_confirm_bars     =  1;

        double rt = cfg_.round_trip_bp;
        cfg_.ratchet_start_bp  = rt;
        cfg_.be_arm_bp         = rt + 10.0;
        cfg_.ratchet_lock_pct  = 0.75;
        cfg_.prog_lock_pct_2   = 0.85;
        cfg_.prog_lock_pct_3   = 0.90;
        cfg_.prog_lock_pct_4   = 0.95;

        // Uniform trail (S14 baseline values — proven across roster)
        cfg_.trail_arm_atr          = 1.0;
        cfg_.trail_dist_atr         = 0.4;
        cfg_.trail_tighten_atr      = 0.0;   // disabled (default)
        cfg_.trail_tighten_dist_atr = 0.3;
    }

    // Correlation regime: set by main.cpp when rolling corr(symbol, BTC) > threshold
    void set_corr_high(bool b) { corr_high_ = b; }
    bool corr_high() const { return corr_high_; }

    // Portfolio gate: set by main.cpp when max positions reached or drawdown breaker fires
    void set_portfolio_gate(bool allowed) { portfolio_entry_allowed_ = allowed; }
    bool portfolio_entry_allowed() const { return portfolio_entry_allowed_; }
    // ── Correlation-cluster exposure gate (independent of portfolio_gate) ──
    // main.cpp recomputes this every tick from live per-symbol / per-cluster
    // open-position counts. Self-resetting: re-opens automatically when a
    // correlated position exits. ANDed with portfolio_entry_allowed_ at entry.
    // gate_name/reason: optional honest attribution of WHY the gate is closed
    // (S-2026-07-11). Must be string literals / static storage. Defaults keep
    // legacy call sites source-compatible.
    void set_cluster_gate(bool allowed, const char* gate_name = nullptr,
                          const char* reason = nullptr) {
        cluster_gate_        = allowed;
        cluster_gate_name_   = (!allowed) ? gate_name : nullptr;
        cluster_gate_reason_ = (!allowed) ? reason    : nullptr;
    }
    bool cluster_gate() const { return cluster_gate_; }

    // MTF gate: called externally when D1 TSMOM trend state changes.
    // true = D1 bullish (allow all entries), false = D1 bearish (suppress counter-trend).
    // Session 29 refinement: tracks bearish streak. MTF gate only suppresses
    // after 3 consecutive bearish D1 readings (prevents single-bar whipsaw blocks).
    void set_d1_bullish(bool b) {
        if (!b) {
            d1_bearish_streak_++;
        } else {
            d1_bearish_streak_ = 0;
        }
        d1_bullish_ = b;
    }
    bool d1_bullish() const { return d1_bullish_; }
    int d1_bearish_streak() const { return d1_bearish_streak_; }

    // Returns whether this engine's strategy is trend-following (for MTF gate logic in main.cpp)
    bool is_trend_following() const {
        return cfg_.kind == StrategyKind::TSMOM || cfg_.kind == StrategyKind::DONCHIAN ||
               cfg_.kind == StrategyKind::DUAL_THRUST || cfg_.kind == StrategyKind::ICHIMOKU ||
               cfg_.kind == StrategyKind::SUPERTREND ||
               cfg_.kind == StrategyKind::BREAKOUT_PULLBACK;
    }

    // Returns the TSMOM trend direction: true = bullish (close > close[lookback]).
    // Used by main.cpp to extract D1 trend state from D1 TSMOM engines.
    bool trend_bullish() const {
        if ((int)closes_.size() < cfg_.lookback + 1) return true; // default bullish if insufficient data
        return closes_.back() > closes_[closes_.size() - 1 - cfg_.lookback];
    }

    // ── Session 30: Funding rate tailwind (Edge 1) ──────────────────────────
    // When negative funding detected (shorts paying longs), spot-long has carry edge.
    // Effect: lowers ADX threshold by 5 and vol_elevated threshold by 0.1 when true.
    // Also flags entry as "high conviction" for position sizing (Edge 4).
    void set_funding_tailwind(bool b) { funding_tailwind_ = b; }
    bool funding_tailwind() const { return funding_tailwind_; }

    // When positive funding is extreme (longs paying heavily), suppress entries.
    void set_funding_headwind(bool b) { funding_headwind_ = b; }
    bool funding_headwind() const { return funding_headwind_; }

    // ── Session 30: Volatility regime (Edge 3) ──��───────────────────────────
    // 3-state regime: LOW=0, MEDIUM=1, HIGH=2
    // LOW → only trend/breakout engines active
    // HIGH → only counter-trend engines active
    // MEDIUM → all active
    // Set externally by main.cpp based on ATR(14)/ATR(50) of BTC D1.
    enum class VolRegime : uint8_t { LOW = 0, MEDIUM = 1, HIGH = 2 };
    void set_vol_regime(VolRegime r) { vol_regime_ = r; }
    VolRegime vol_regime() const { return vol_regime_; }

    // ── Session 30: Position sizing multiplier (Edge 4) ─────────────────────
    // Base = 1.0. Engines with strong backtest stats get sizing_mult > 1.0.
    // Reduced during high vol, boosted during funding tailwind.
    // Applied by main.cpp at execution time (not inside engine signal logic).
    void set_sizing_mult(double m) { sizing_mult_ = m; }
    // P1/S46: safety size mult (DD-throttle x vol-overlay) actually applied to qty
    void set_risk_mult(double m) { risk_mult_ = m; }
    double risk_mult() const { return risk_mult_; }
    double sizing_mult() const { return sizing_mult_; }

    // ── Session 30: Cross-TF momentum score (Edge 5) ────────────────────────
    // Normalized momentum score from D1+H6+H4 agreement.
    // Range: 0.0 (no agreement) to 1.0 (all TFs strongly bullish).
    // When > 0.7, engine is "high conviction" → sizing boost.
    void set_cross_tf_score(double s) { cross_tf_score_ = s; }
    double cross_tf_score() const { return cross_tf_score_; }

    // Returns true if this entry is "high conviction" (funding tailwind + cross-TF alignment)
    bool is_high_conviction() const {
        return (funding_tailwind_ && cross_tf_score_ > 0.5) || cross_tf_score_ > 0.7;
    }

    // Public accessor for vol_ratio (used by main.cpp for regime classification)
    double vol_ratio_public() const { return vol_ratio_(); }

private:
    Config cfg_;

    // Bar accumulator
    int64_t cur_bar_id_     = 0;
    int64_t cur_open_ts_ms_ = 0;
    double  cur_open_  = 0.0, cur_high_ = 0.0, cur_low_ = 0.0, cur_close_ = 0.0;
    double  last_close_ = 0.0;

    // Tick counter for volume proxy (Session 29)
    int     cur_tick_count_ = 0;
    int64_t stale_tick_count_ = 0;   // CH-01: dropped out-of-order ticks (telemetry)

    // Closed-bar history (back is most recent)
    std::deque<double> opens_;
    std::deque<double> highs_;
    std::deque<double> lows_;
    std::deque<double> closes_;
    std::deque<int64_t> bar_ts_ms_;
    std::deque<int>    tick_counts_;   // tick count per bar (volume proxy)

    // MTF gate state
    bool    d1_bullish_  = true;    // default true = allow all entries
    int     d1_bearish_streak_ = 0; // consecutive bearish D1 readings (S29: need 3+ to suppress)

    // Correlation filter state (fed by main.cpp)
    bool    corr_high_ = false;     // true = extreme BTC correlation, suppress altcoin entries

    // Portfolio gate state (fed by main.cpp)
    bool    portfolio_entry_allowed_ = true;  // false = max positions or drawdown breaker active
    bool    cluster_gate_            = true;   // false = entry vetoed by main.cpp combined gate
    // S-2026-07-11 honest gate attribution: main.cpp says WHICH term of the
    // combined gate blocked (cluster cap vs loss breaker vs regime chop-halt).
    // Pre-fix every suppression was mislabelled "CLUSTER_GATE" — this masked
    // the S54 200DMA macro veto through the Jul-8..10 bounce. String literals
    // / static storage only (pointers are stored, not copied).
    const char* cluster_gate_name_   = nullptr;
    const char* cluster_gate_reason_ = nullptr;

    // Phase-4 item 21: gate-attribution sink (observational; null => no-op).
    GateAttribution* gate_sink_  = nullptr;
    uint64_t         cur_corr_id_ = 0;   // corr-id of the signal under evaluation

    // Session 30: Funding filter state
    bool    funding_tailwind_ = false;  // negative funding = carry edge for longs
    bool    funding_headwind_ = false;  // extreme positive funding = suppress

    // Session 30: Volatility regime state
    VolRegime vol_regime_ = VolRegime::MEDIUM;

    // Session 30: Position sizing multiplier (base=1.0)
    double  sizing_mult_ = 1.0;
    double  risk_mult_   = 1.0;   // P1: DD-throttle x vol-overlay, applied to live qty

    // Session 30: Cross-TF momentum score (0.0-1.0)
    double  cross_tf_score_ = 0.0;

    // SuperTrend state (persists across bars)
    bool    st_bullish_      = true;   // current SuperTrend direction
    bool    st_prev_bullish_ = true;   // previous bar's direction (for flip detection)
    double  st_upper_band_   = 0.0;
    double  st_lower_band_   = 0.0;

    // Position state
    bool    in_position_ = false;
    double  entry_px_    = 0.0;
    // Intra-bar MIMIC entry (S-2026-07-05): when >0, evaluate_signal_ opens at
    // this live price/ts instead of the just-closed bar. intrabar_fired_bar_
    // caps entry attempts to one per forming bar (avoids per-tick eval spam).
    double  intrabar_entry_px_ = 0.0;
    int64_t intrabar_entry_ts_ = 0;
    int64_t intrabar_fired_bar_ = -1;
    double  sl_px_       = 0.0;     // hard stop-loss (never moves)
    int64_t entry_ts_ms_ = 0;
    int64_t time_exit_ts_ms_ = 0;
    double  atr_at_entry_ = 0.0;
    int     bars_held_ = 0;

    // Trailing stop state
    bool    trail_armed_    = false;
    bool    rsirev_flat_book_ = false;  // S-2026-07-23: one-shot — next exit_position_ books FLAT 0 (BE-ENTRY unarmed)
    double  trail_stop_px_  = 0.0;  // ratchets up, never down
    double  trail_arm_px_   = 0.0;  // price level that arms the trail
    double  mfe_px_         = 0.0;  // max favourable excursion (highest price seen)
    double  mfe_bp_         = 0.0;  // MFE in basis points from entry

    // ── Pyramid state (Session 31) ──────────────────────────────────────
    struct PyramidAdd {
        double entry_px  = 0.0;     // price at which this add was executed
        double size_mult = 0.0;     // fraction of base position size
    };
    int     pyramid_count_    = 0;        // number of adds executed this trade
    double  pyramid_next_atr_ = 0.0;      // ATR profit level for next pyramid add
    std::vector<PyramidAdd> pyramid_adds_; // history of adds for P&L calc

    // Stats
    int    trades_ = 0;
    int    wins_   = 0;
    double total_bp_ = 0.0;
    double last_trade_bp_ = 0.0;
    bool   halted_ = false;

    // Trade callback (set by main.cpp for persistence)
    TradeCallback on_trade_;
    // Bar callback (set by main.cpp for bar persistence + warm-start)
    BarCallback on_bar_;
    // Pyramid callback (set by main.cpp for pyramid order execution)
    PyramidCallback on_pyramid_;
    // Order intent callback (set by main.cpp to mirror into SpotExecutor)
    OrderIntentCallback on_order_intent_;

    // ── Effective stop: max(hard_sl, trail_stop) ─────────────────────────────
    double effective_stop_() const {
        if (trail_armed_ && trail_stop_px_ > sl_px_) return trail_stop_px_;
        return sl_px_;
    }

    // ── Helper: extract UTC hour from a millisecond epoch timestamp ──────────
    static int utc_hour_from_ms_(int64_t ts_ms) {
        time_t secs = static_cast<time_t>(ts_ms / 1000);
        struct tm utc;
        gmtime_r(&secs, &utc);
        return utc.tm_hour;
    }

    // ── Helper: extract day-of-week (0=Sun,1=Mon,...6=Sat) from ms epoch ────
    static int utc_dow_from_ms_(int64_t ts_ms) {
        time_t secs = static_cast<time_t>(ts_ms / 1000);
        struct tm utc;
        gmtime_r(&secs, &utc);
        return utc.tm_wday;
    }

    // ── Simple Moving Average of last n closes ──────────────────────────────
    double sma_(int n) const {
        if ((int)closes_.size() < n) return 0.0;
        double sum = 0.0;
        const int sz = (int)closes_.size();
        for (int i = sz - n; i < sz; ++i) sum += closes_[i];
        return sum / (double)n;
    }

    // ── Exponential Moving Average of last n closes ─────────────────────────
    double ema_(int n) const {
        if ((int)closes_.size() < n) return 0.0;
        double alpha = 2.0 / (n + 1.0);
        double result = closes_[0];
        for (int i = 1; i < (int)closes_.size(); ++i) {
            result = alpha * closes_[i] + (1.0 - alpha) * result;
        }
        return result;
    }

    // ── Average tick count over last N bars (volume proxy) ──────────────────
    double avg_tick_count_() const {
        int n = cfg_.vol_tick_lookback;
        if ((int)tick_counts_.size() < n) {
            if (tick_counts_.empty()) return 100.0;
            n = (int)tick_counts_.size();
        }
        double sum = 0.0;
        const int sz = (int)tick_counts_.size();
        for (int i = sz - n; i < sz; ++i) sum += tick_counts_[i];
        return sum / (double)n;
    }

    // ── ADX (Average Directional Index) ─────────────────────────────────────
    // FIXED Session 32d: prior version zeroed the smaller DM before computing
    // DX, which forced per-bar DX to always be 0 or 100. Real ADX keeps both
    // DMs and uses smoothed sums. We use simple averaging over n bars (not
    // full Wilder smoothing) but keep both DMs — gives a useful 0-100 range.
    // Returns 25.0 (neutral) if insufficient data.
    double adx_(int n) const {
        if ((int)closes_.size() < n + 2) return 25.0;
        const int sz = (int)closes_.size();

        // Accumulate +DM, -DM, TR over the window; compute DX from the sums.
        double sum_plus_dm = 0.0, sum_minus_dm = 0.0, sum_tr = 0.0;
        for (int i = sz - n; i < sz; ++i) {
            double hi      = highs_[i];
            double lo      = lows_[i];
            double prev_hi = highs_[i - 1];
            double prev_lo = lows_[i - 1];
            double prev_c  = closes_[i - 1];

            double up_move   = hi - prev_hi;
            double down_move = prev_lo - lo;

            double plus_dm  = (up_move > down_move && up_move > 0.0)   ? up_move   : 0.0;
            double minus_dm = (down_move > up_move && down_move > 0.0) ? down_move : 0.0;

            double tr = std::max({hi - lo,
                                  std::fabs(hi - prev_c),
                                  std::fabs(lo - prev_c)});

            sum_plus_dm  += plus_dm;
            sum_minus_dm += minus_dm;
            sum_tr       += tr;
        }
        if (sum_tr <= 0.0) return 25.0;

        double plus_di  = (sum_plus_dm  / sum_tr) * 100.0;
        double minus_di = (sum_minus_dm / sum_tr) * 100.0;
        double di_sum   = plus_di + minus_di;
        if (di_sum <= 0.0) return 0.0;
        return std::fabs(plus_di - minus_di) / di_sum * 100.0;
    }

    // ── Williams %R (Session 29b) ──────────────────────────────────────────
    // %R = (Highest High(N) - Close) / (Highest High(N) - Lowest Low(N)) * -100
    // Range: -100 (oversold) to 0 (overbought)
    double williams_r_(int n) const {
        if ((int)highs_.size() < n) return -50.0;
        const int sz = (int)highs_.size();
        double hh = 0.0, ll = 1e18;
        for (int i = sz - n; i < sz; ++i) {
            if (highs_[i] > hh) hh = highs_[i];
            if (lows_[i] < ll)  ll = lows_[i];
        }
        double range = hh - ll;
        if (range <= 0.0) return -50.0;
        return (hh - closes_.back()) / range * -100.0;
    }

    // Williams %R at one bar back (for cross-up detection)
    double williams_r_prev_(int n) const {
        if ((int)highs_.size() < n + 1) return -50.0;
        const int sz = (int)highs_.size() - 1;  // exclude last bar
        double hh = 0.0, ll = 1e18;
        for (int i = sz - n; i < sz; ++i) {
            if (highs_[i] > hh) hh = highs_[i];
            if (lows_[i] < ll)  ll = lows_[i];
        }
        double range = hh - ll;
        if (range <= 0.0) return -50.0;
        return (hh - closes_[sz - 1]) / range * -100.0;
    }

    // ── Stochastic RSI (Session 29b) ────────────────────────────────────────
    // StochRSI = (RSI - min(RSI, N)) / (max(RSI, N) - min(RSI, N)) * 100
    // We compute RSI for the last stoch_period bars, then find min/max of those RSI values.
    double stoch_rsi_(int rsi_period, int stoch_period) const {
        // Need enough bars to compute stoch_period RSI values
        if ((int)closes_.size() < rsi_period + stoch_period + 2) return 50.0;
        const int sz = (int)closes_.size();

        // Compute RSI values for the last stoch_period bars
        // Using a sliding window approach
        double min_rsi = 1e18, max_rsi = -1e18;
        double cur_rsi = 0.0;

        for (int offset = 0; offset < stoch_period; ++offset) {
            // Compute RSI at position (sz - stoch_period + offset)
            int end_pos = sz - stoch_period + offset + 1;
            double avg_up = 0.0, avg_dn = 0.0;
            const double alpha = 1.0 / (double)rsi_period;
            for (int i = 1; i < end_pos; ++i) {
                double d = closes_[i] - closes_[i - 1];
                double u = d > 0 ? d : 0.0;
                double dn = d < 0 ? -d : 0.0;
                if (i == 1) { avg_up = u; avg_dn = dn; }
                else { avg_up = (1 - alpha) * avg_up + alpha * u;
                       avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
            }
            double rsi_val = (avg_dn == 0.0) ? 100.0 : (100.0 - 100.0 / (1.0 + avg_up / avg_dn));

            if (rsi_val < min_rsi) min_rsi = rsi_val;
            if (rsi_val > max_rsi) max_rsi = rsi_val;
            if (offset == stoch_period - 1) cur_rsi = rsi_val;
        }

        double range = max_rsi - min_rsi;
        if (range <= 0.0) return 50.0;
        return (cur_rsi - min_rsi) / range * 100.0;
    }

    // Stochastic RSI at one bar back (for cross-up detection)
    double stoch_rsi_prev_(int rsi_period, int stoch_period) const {
        if ((int)closes_.size() < rsi_period + stoch_period + 3) return 50.0;
        const int sz = (int)closes_.size() - 1;  // pretend last bar doesn't exist

        double min_rsi = 1e18, max_rsi = -1e18;
        double cur_rsi = 0.0;

        for (int offset = 0; offset < stoch_period; ++offset) {
            int end_pos = sz - stoch_period + offset + 1;
            double avg_up = 0.0, avg_dn = 0.0;
            const double alpha = 1.0 / (double)rsi_period;
            for (int i = 1; i < end_pos; ++i) {
                double d = closes_[i] - closes_[i - 1];
                double u = d > 0 ? d : 0.0;
                double dn = d < 0 ? -d : 0.0;
                if (i == 1) { avg_up = u; avg_dn = dn; }
                else { avg_up = (1 - alpha) * avg_up + alpha * u;
                       avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
            }
            double rsi_val = (avg_dn == 0.0) ? 100.0 : (100.0 - 100.0 / (1.0 + avg_up / avg_dn));

            if (rsi_val < min_rsi) min_rsi = rsi_val;
            if (rsi_val > max_rsi) max_rsi = rsi_val;
            if (offset == stoch_period - 1) cur_rsi = rsi_val;
        }

        double range = max_rsi - min_rsi;
        if (range <= 0.0) return 50.0;
        return (cur_rsi - min_rsi) / range * 100.0;
    }

    // ── Ichimoku Cloud components (Session 29) ──────────────────────────────
    // Midpoint of highest high and lowest low over N bars
    double ichi_midpoint_(int period) const {
        if ((int)highs_.size() < period) return 0.0;
        const int sz = (int)highs_.size();
        double hh = 0.0, ll = 1e18;
        for (int i = sz - period; i < sz; ++i) {
            if (highs_[i] > hh) hh = highs_[i];
            if (lows_[i] < ll)  ll = lows_[i];
        }
        return (hh + ll) / 2.0;
    }

    // ── SuperTrend computation (Session 29) ─────────────────────────────────
    // Updates st_bullish_ state and returns whether a bullish flip just occurred.
    // Must be called once per bar close (in close_bar_ flow).
    bool supertrend_update_() {
        if ((int)closes_.size() < cfg_.st_atr_period + 2) return false;

        double a = atr_(cfg_.st_atr_period);
        if (a <= 0.0) return false;

        double hl2 = (highs_.back() + lows_.back()) / 2.0;
        double basic_upper = hl2 + cfg_.st_multiplier * a;
        double basic_lower = hl2 - cfg_.st_multiplier * a;

        // SuperTrend band logic: bands can only move in the trend direction
        double final_upper = basic_upper;
        double final_lower = basic_lower;

        // Upper band: can only go DOWN (tighten) during bearish trend
        if (st_upper_band_ > 0.0 && basic_upper > st_upper_band_ && closes_.size() >= 2) {
            double prev_close = closes_[closes_.size() - 2];
            if (prev_close <= st_upper_band_) {
                final_upper = std::min(basic_upper, st_upper_band_);
            }
        }
        // Lower band: can only go UP (tighten) during bullish trend
        if (st_lower_band_ > 0.0 && basic_lower < st_lower_band_ && closes_.size() >= 2) {
            double prev_close = closes_[closes_.size() - 2];
            if (prev_close >= st_lower_band_) {
                final_lower = std::max(basic_lower, st_lower_band_);
            }
        }

        st_upper_band_ = final_upper;
        st_lower_band_ = final_lower;

        // Determine direction
        st_prev_bullish_ = st_bullish_;
        double close = closes_.back();
        if (close > st_upper_band_) {
            st_bullish_ = true;
        } else if (close < st_lower_band_) {
            st_bullish_ = false;
        }
        // else: keep previous direction

        // Return true if just flipped to bullish (entry signal)
        return (st_bullish_ && !st_prev_bullish_);
    }

    // ── Bar close ────────────────────────────────────────────────────────────
    void close_bar_() {
        opens_.push_back(cur_open_);
        highs_.push_back(cur_high_);
        lows_.push_back(cur_low_);
        closes_.push_back(cur_close_);
        bar_ts_ms_.push_back(cur_open_ts_ms_);
        tick_counts_.push_back(cur_tick_count_);
        while ((int)closes_.size() > cfg_.max_history) {
            opens_.pop_front(); highs_.pop_front(); lows_.pop_front();
            closes_.pop_front(); bar_ts_ms_.pop_front(); tick_counts_.pop_front();
        }
        last_close_ = cur_close_;
        cur_tick_count_ = 0;  // reset for next bar

        // Update SuperTrend state (must happen before signal evaluation)
        bool st_flip = false;
        if (cfg_.kind == StrategyKind::SUPERTREND) {
            st_flip = supertrend_update_();
        }

        // First, check if a time-based exit just landed on this bar boundary.
        // MIMIC (ride_to_flip): skip TIME; exit only on symmetric down-jump flip.
        // RSIREV-FLOOR (S-2026-07-23): also skip TIME — this mode is not ride_to_flip
        // but the certified exit is the RSI>=thr flip (below) + the intraday BE-floor
        // (check_exits_), NOT a fixed hold_bars timeout. `&& !rsi_revert_intraday_floor`
        // is `&& true` for every existing leg (flag default false) ⇒ identical.
        if (!cfg_.ride_to_flip && !cfg_.rsi_revert_intraday_floor && in_position_ &&
            cur_open_ts_ms_ + cfg_.tf_secs * 1000 > time_exit_ts_ms_) {
            // exit at this bar's close
            exit_position_(cur_close_, cur_open_ts_ms_ + cfg_.tf_secs * 1000, "TIME");
        }
        // ── RSIREV-FLOOR decoupled mean-revert flip-out (S-2026-07-23) ──────────
        // This mode runs ride_to_flip=false (so check_exits_ stays active for the
        // intraday BE-floor), but the CERT exits a level-revert long the moment RSI
        // recovers to >= thr. Evaluate that flip here at the daily bar close — the
        // exact complement of the entry, mirroring the ride_to_flip block below but
        // for the non-ride RSIREV-floor leg. Fully guarded ⇒ no effect when the flag
        // is off. Books at this bar's close (≈ next-day open on a live feed).
        if (cfg_.rsi_revert_intraday_floor && cfg_.rsi_level_revert && !cfg_.ride_to_flip
            && in_position_ && bars_held_ >= 2 && rsi_level_flipped_out_()) {
            // BE-ENTRY: if the leg NEVER armed (fav never reached +confirm) it never really
            // opened — book FLAT 0 (no P&L, no cost, no SELL routed), the cert's
            // `(be_floor && !armed) ? 0 : (eret-cost)`. trail_armed_ is the arm flag.
            rsirev_flat_book_ = !trail_armed_;
            exit_position_(cur_close_, cur_open_ts_ms_ + cfg_.tf_secs * 1000, "FLIP");
        }
        if (cfg_.ride_to_flip && in_position_) {
            bool flip_out = false;
            if (cfg_.kind == StrategyKind::MIMIC)             flip_out = (mimic_state_() == 0);
            else if (cfg_.kind == StrategyKind::KELTNER_BREAK) flip_out = keltner_break_flipped_out_();
            else if (cfg_.kind == StrategyKind::REGIME_SWITCH) flip_out = regime_switch_flipped_out_();
            else if (cfg_.kind == StrategyKind::EMAX)          flip_out = emax_flipped_out_();
            else if (cfg_.kind == StrategyKind::ROC)           flip_out = roc_flipped_out_();
            else if (cfg_.kind == StrategyKind::IBS)           flip_out = ibs_flipped_out_();
            else if (cfg_.kind == StrategyKind::RSI_REVERT && cfg_.rsi_level_revert) flip_out = rsi_level_flipped_out_();  // research level-revert exits when RSI recovers >= thr
            else if (cfg_.kind == StrategyKind::TSMOM)         flip_out = !signal_tsmom_();  // research TSMom rides until L-bar return sign flips
            if (flip_out) {
                exit_position_(cur_close_, cur_open_ts_ms_ + cfg_.tf_secs * 1000, "FLIP");
            }
        }

        // Then, evaluate a new signal (only if flat and not halted).
        bool was_flat = !in_position_;
        if (!in_position_ && !halted_) {
            evaluate_signal_(st_flip);
        }
        bool signal_fired = was_flat && in_position_;  // we just entered

        bars_held_ = in_position_ ? (bars_held_ + 1) : 0;

        // ── Fire bar callback for persistence + audit trail ──────────────
        if (on_bar_) {
            BarRecord br;
            br.tag            = cfg_.tag;
            br.open_ts_ms     = bar_ts_ms_.back();
            br.tf_secs        = cfg_.tf_secs;
            br.o              = opens_.back();
            br.h              = highs_.back();
            br.l              = lows_.back();
            br.c              = closes_.back();
            br.atr            = atr_(cfg_.atr_period);
            br.bars_in_buffer = (int)closes_.size();
            br.signal_ready   = ((int)closes_.size() >= cfg_.lookback + 1);
            br.signal_fired   = signal_fired;
            br.in_position    = in_position_;
            br.momentum_pct   = 0.0;
            if (br.signal_ready) {
                double lb_c = closes_[closes_.size() - 1 - cfg_.lookback];
                if (lb_c > 0.0) br.momentum_pct = (closes_.back() / lb_c - 1.0) * 100.0;
            }
            on_bar_(br);
        }
    }

    // ── Indicators (all read from the closed-bar buffer) ─────────────────────
    double atr_(int n) const {
        // Need n+1 bars to compute n TRs (TR uses prev close).
        if ((int)closes_.size() < n + 1) return 0.0;
        double sum = 0.0;
        const int sz = (int)closes_.size();
        for (int i = sz - n; i < sz; ++i) {
            double prev_close = closes_[i - 1];
            double tr = std::max({
                highs_[i] - lows_[i],
                std::fabs(highs_[i] - prev_close),
                std::fabs(lows_[i]  - prev_close)
            });
            sum += tr;
        }
        return sum / (double)n;
    }

    double rsi_(int n) const {
        // Exponential RSI matching backtest (alpha = 1/n).
        if ((int)closes_.size() < n + 2) return 50.0;
        const int sz = (int)closes_.size();
        double avg_up = 0.0, avg_dn = 0.0;
        const double alpha = 1.0 / (double)n;
        for (int i = 1; i < sz; ++i) {
            double d = closes_[i] - closes_[i - 1];
            double u = d > 0 ? d : 0.0;
            double dn = d < 0 ? -d : 0.0;
            if (i == 1) { avg_up = u; avg_dn = dn; }
            else        { avg_up = (1 - alpha) * avg_up + alpha * u;
                          avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
        }
        if (avg_dn == 0.0) return 100.0;
        double rs = avg_up / avg_dn;
        return 100.0 - 100.0 / (1.0 + rs);
    }

    // RSI value at one bar back (for cross-up detection)
    double rsi_prev_(int n) const {
        if ((int)closes_.size() < n + 3) return 50.0;
        const int sz = (int)closes_.size() - 1;  // pretend last bar doesn't exist
        double avg_up = 0.0, avg_dn = 0.0;
        const double alpha = 1.0 / (double)n;
        for (int i = 1; i < sz; ++i) {
            double d = closes_[i] - closes_[i - 1];
            double u = d > 0 ? d : 0.0;
            double dn = d < 0 ? -d : 0.0;
            if (i == 1) { avg_up = u; avg_dn = dn; }
            else        { avg_up = (1 - alpha) * avg_up + alpha * u;
                          avg_dn = (1 - alpha) * avg_dn + alpha * dn; }
        }
        if (avg_dn == 0.0) return 100.0;
        double rs = avg_up / avg_dn;
        return 100.0 - 100.0 / (1.0 + rs);
    }

    double bb_lower_(int n, double k) const {
        if ((int)closes_.size() < n) return 0.0;
        const int sz = (int)closes_.size();
        double mean = 0.0;
        for (int i = sz - n; i < sz; ++i) mean += closes_[i];
        mean /= (double)n;
        double var = 0.0;
        for (int i = sz - n; i < sz; ++i) {
            double d = closes_[i] - mean;
            var += d * d;
        }
        var /= (double)(n - 1);
        return mean - k * std::sqrt(var);
    }

    // Research-faithful Keltner midline EMA (S-2026-07-21): seed = close N bars
    // back, N EMA iterations (N = keltner_ema_len) — EXACTLY the ibkrcrypto Kelt(N,M)
    // EMA (seed c[i-N]). Distinct from ema_() (full-buffer seed) and research_ema_()
    // (4*p seed for EMAx). Used ONLY when keltner_exit_reenter_band is set.
    double kelt_research_ema_() const {
        const int N = cfg_.keltner_ema_len; const int sz = (int)closes_.size();
        if (sz < N + 1) return 0.0;
        double a = 2.0 / (N + 1.0); double e = closes_[sz - 1 - N];
        for (int j = sz - N; j <= sz - 1; ++j) e = a * closes_[j] + (1.0 - a) * e;
        return e;
    }
    // Keltner midline + width. When keltner_exit_reenter_band (the validated research
    // Kelt mode) is set, use the faithful N-bar-seed EMA and ATR over keltner_ema_len
    // (= research N); otherwise keep the legacy ema_()/atr_(atr_period) so the existing
    // KELTNER_REVERT engines and legacy KELTNER_BREAK g_slots are byte-unchanged.
    double keltner_mid_()   const { return cfg_.keltner_exit_reenter_band ? kelt_research_ema_() : ema_(cfg_.keltner_ema_len); }
    double keltner_width_() const { return cfg_.keltner_exit_reenter_band ? atr_(cfg_.keltner_ema_len) : atr_(cfg_.atr_period); }
    // ── Keltner lower band: EMA(n) - mult * ATR ─────────────────────────────
    double keltner_lower_() const {
        double e = keltner_mid_(); double a = keltner_width_();
        if (e <= 0.0 || a <= 0.0) return 0.0;
        return e - cfg_.keltner_atr_mult * a;
    }
    // S-2026-07-12: upper Keltner band (EMA + M*ATR) for KELTNER_BREAK (trend breakout).
    double keltner_upper_() const {
        double e = keltner_mid_(); double a = keltner_width_();
        if (e <= 0.0 || a <= 0.0) return 0.0;
        return e + cfg_.keltner_atr_mult * a;
    }

    // ── Volatility regime ratio: ATR(14) / ATR(50) ──────────────────────────
    double vol_ratio_() const {
        double a14 = atr_(14);
        double a50 = atr_(50);
        if (a50 <= 0.0) return 1.0;
        return a14 / a50;
    }

    // ── Signal evaluation on the just-closed bar ─────────────────────────────
    bool signal_tsmom_() const { return signal_tsmom_at_(0); }
    bool signal_donchian_() const { return signal_donchian_at_(0); }

    // S34: signal at bar `back` bars ago (0 = current). Used by confirmation
    // gate to look backward through history (no time wait needed).
    bool signal_tsmom_at_(int back) const {
        int sz = (int)closes_.size();
        if (sz < cfg_.lookback + 1 + back) return false;
        double now = closes_[sz - 1 - back];
        double ref = closes_[sz - 1 - back - cfg_.lookback];
        return now > ref;
    }

    bool signal_donchian_at_(int back) const {
        int sz = (int)highs_.size();
        if (sz < cfg_.lookback + 1 + back) return false;
        double prior_high = 0.0;
        int start = sz - back - cfg_.lookback - 1;
        int end   = sz - back - 1;
        for (int i = start; i < end; ++i) {
            if (highs_[i] > prior_high) prior_high = highs_[i];
        }
        return closes_[sz - 1 - back] > prior_high;
    }

    // S34: dispatcher for backward signal check on supported strategies.
    // Returns -1 if strategy doesn't support back-check (caller falls back).
    int signal_at_back(int back) const {
        switch (cfg_.kind) {
            case StrategyKind::TSMOM:    return signal_tsmom_at_(back) ? 1 : 0;
            case StrategyKind::DONCHIAN: return signal_donchian_at_(back) ? 1 : 0;
            default: return -1;  // unsupported -> caller skips confirm check
        }
    }

    bool signal_bollinger_() const {
        if ((int)closes_.size() < cfg_.lookback) return false;
        double lower = bb_lower_(cfg_.lookback, cfg_.bb_k);
        // Pierce: bar's low touched the lower band; closed back above
        return (lows_.back() <= lower) && (closes_.back() > lower);
    }

    // Research-faithful SMA RSI (S-2026-07-21 final-closeout): simple mean of the
    // last N gains/losses (N=atr_period), EXACTLY the ibkrcrypto RSIrev computation
    // (Crypto/src/ibkrcrypto_bt.cpp sig_rsirev). Distinct from rsi_() (exponential,
    // whole-buffer) — used ONLY for the level-revert path so all legacy cross-up
    // RSI_REVERT g_slots keep the exponential rsi_() byte-for-byte.
    double research_rsi_() const {
        const int n = cfg_.atr_period; const int sz = (int)closes_.size();
        if (sz < n + 1) return 50.0;
        double g = 0.0, ll = 0.0;
        for (int j = sz - n; j <= sz - 1; ++j) {
            double d = closes_[j] - closes_[j - 1];
            if (d > 0) g += d; else ll -= d;
        }
        g /= (double)n; ll /= (double)n;
        double rs = ll > 0.0 ? g / ll : 999.0;
        return 100.0 - 100.0 / (1.0 + rs);
    }
    bool signal_rsi_revert_() const {
        if (cfg_.rsi_level_revert) {
            // Research LEVEL-revert: long whenever RSI(SMA,N) < oversold level.
            if ((int)closes_.size() < cfg_.atr_period + 1) return false;
            return research_rsi_() < cfg_.rsi_threshold;
        }
        // Legacy CROSS-UP (Session 19/21): fire on the bar RSI crosses above thr.
        if ((int)closes_.size() < cfg_.atr_period + 3) return false;
        double r_now  = rsi_(cfg_.atr_period);
        double r_prev = rsi_prev_(cfg_.atr_period);
        return (r_prev <= cfg_.rsi_threshold) && (r_now > cfg_.rsi_threshold);
    }
    // ride_to_flip flip-out for the research level-revert: FLAT the moment RSI
    // recovers to >= the oversold level (the exact complement of the entry). Only
    // consulted when rsi_level_revert is set (see close_bar_).
    bool rsi_level_flipped_out_() const {
        if ((int)closes_.size() < cfg_.atr_period + 1) return false;
        return !(research_rsi_() < cfg_.rsi_threshold);
    }

    // ── KELTNER_REVERT: bar pierces lower Keltner band, closes back above ───
    bool signal_keltner_revert_() const {
        if ((int)closes_.size() < std::max(cfg_.keltner_ema_len, cfg_.atr_period) + 1)
            return false;
        double lower = keltner_lower_();
        if (lower <= 0.0) return false;
        return (lows_.back() <= lower) && (closes_.back() > lower);
    }

    // ── KELTNER_BREAK (S-2026-07-12): upper-band breakout TREND, folds the Mac
    //    ibkrcrypto Kelt(N,M): long when close > EMA+M*ATR; ride until close <
    //    lower band (symmetric flip), NO trade-level stops (ride_to_flip). This
    //    is the OPPOSITE of KELTNER_REVERT — do not conflate. ────────────────
    bool signal_keltner_break_() const {
        if ((int)closes_.size() < std::max(cfg_.keltner_ema_len, cfg_.atr_period) + 1)
            return false;
        double upper = keltner_upper_();
        if (upper <= 0.0) return false;
        return closes_.back() > upper;         // entry: close breaks the upper band
    }
    // flip-out. Two exits, selected by cfg_.keltner_exit_reenter_band:
    //  • FALSE (default, legacy S-2026-07-12): long rides until close falls through
    //    the LOWER band (ride-to-lower). Kept for the already-wired g_slots.
    //  • TRUE (validated research Kelt, long-only): exit the moment close is no
    //    longer above the UPPER band (close re-enters the channel) — this is the
    //    exact complement of the entry signal (close>upper), reproducing the
    //    ibkrcrypto Kelt long-only want=1->0 transition. The DirectionalTrendRoster
    //    Keltner legs use this to penny-match the validated per-leg net%.
    bool keltner_break_flipped_out_() const {
        if ((int)closes_.size() < std::max(cfg_.keltner_ema_len, cfg_.atr_period) + 1)
            return false;
        if (cfg_.keltner_exit_reenter_band) {
            double upper = keltner_upper_();
            if (upper <= 0.0) return false;
            return !(closes_.back() > upper);   // FLAT when close re-enters the band
        }
        double lower = keltner_lower_();
        if (lower <= 0.0) return false;
        return closes_.back() < lower;
    }

    // ── REGIME_SWITCH (S-2026-07-12): efficiency-ratio regime switch, folds the
    //    Mac ibkrcrypto Regime(N,0.40,0.25). ER>0.40 trending -> 50-bar momentum
    //    long; ER<0.25 chop -> IBS<0.15 mean-rev long; else flat. Long-only. ─────
    int regime_switch_state_() const {
        const int N  = cfg_.lookback > 0 ? cfg_.lookback : 20;
        const int sz = (int)closes_.size();
        if (sz < N + 1) return 0;
        double net = std::fabs(closes_[sz - 1] - closes_[sz - 1 - N]);
        double vol = 0.0;
        for (int j = sz - N; j <= sz - 1; ++j) vol += std::fabs(closes_[j] - closes_[j - 1]);
        double er = vol > 0.0 ? net / vol : 0.0;
        if (er > 0.40) {                                    // trending -> momentum long
            int lag = (sz - 1 >= 50) ? 50 : (sz - 1);
            return (closes_[sz - 1] - closes_[sz - 1 - lag]) > 0.0 ? 1 : 0;
        }
        if (er < 0.25) {                                    // chop -> IBS mean-rev long
            double rng = highs_[sz - 1] - lows_[sz - 1];
            if (rng <= 0.0) return 0;
            double v = (closes_[sz - 1] - lows_[sz - 1]) / rng;
            if (v < 0.15) return 1;
        }
        return 0;
    }
    bool signal_regime_switch_() const      { return regime_switch_state_() == 1; }
    bool regime_switch_flipped_out_() const { return regime_switch_state_() != 1; }

    // ── EMAX (S-2026-07-21 DirectionalTrendRoster port): research-faithful EMA
    //    cross. Folds Mac ibkrcrypto_bt.cpp EMAx(F,S). Long when EMA(F) > EMA(S);
    //    ride_to_flip exits when EMA(F) <= EMA(S) (research want != long). EMA uses
    //    the research 4*p seed window (NOT the whole-buffer ema_()), so max_history
    //    must be >= 4*ema_slow + 1 (set by the roster). THE WORKHORSE. ────────────
    double research_ema_(int p, int back = 0) const {
        int sz = (int)closes_.size();
        int i  = sz - 1 - back;
        if (i < 0) return 0.0;
        int st = i - 4 * p; if (st < 0) st = 0;
        double a = 2.0 / (p + 1.0);
        double e = closes_[st];
        for (int j = st + 1; j <= i; ++j) e = a * closes_[j] + (1.0 - a) * e;
        return e;
    }
    bool signal_emax_() const {
        if ((int)closes_.size() < 4 * cfg_.ema_slow + 1) return false;   // research: i < 4*S -> 0
        return research_ema_(cfg_.ema_fast) > research_ema_(cfg_.ema_slow);
    }
    bool emax_flipped_out_() const {
        if ((int)closes_.size() < 4 * cfg_.ema_slow + 1) return false;
        return !(research_ema_(cfg_.ema_fast) > research_ema_(cfg_.ema_slow));
    }

    // ── ROC (port): research Roc(N,thr). N-bar % change > thr -> long; ride until
    //    roc <= thr. Long-only (research short leg clamped). N = cfg_.lookback. ───
    double roc_val_() const {
        int sz = (int)closes_.size();
        int N  = cfg_.lookback > 0 ? cfg_.lookback : 20;
        if (sz < N + 1) return 0.0;
        double base = closes_[sz - 1 - N];
        if (base == 0.0) return 0.0;
        return (closes_[sz - 1] - base) / base;
    }
    bool signal_roc_()       const { if ((int)closes_.size() < (cfg_.lookback>0?cfg_.lookback:20)+1) return false; return roc_val_() > cfg_.roc_thr; }
    bool roc_flipped_out_()  const { if ((int)closes_.size() < (cfg_.lookback>0?cfg_.lookback:20)+1) return false; return !(roc_val_() > cfg_.roc_thr); }

    // ── IBS (port): research IBS(lo,hi). v=(c-l)/(h-l); v<lo -> long oversold;
    //    exit when v>=lo. Long-only spot (research short leg v>hi ignored). ───────
    double ibs_val_() const {
        int sz = (int)closes_.size();
        if (sz < 1) return 0.5;
        double rng = highs_[sz - 1] - lows_[sz - 1];
        if (rng <= 0.0) return 0.5;
        return (closes_[sz - 1] - lows_[sz - 1]) / rng;
    }
    bool signal_ibs_()      const { return ibs_val_() < cfg_.ibs_lo; }
    bool ibs_flipped_out_() const { return !(ibs_val_() < cfg_.ibs_lo); }

    // ── BREAKOUT_PULLBACK (S38): N-bar high breakout, enter on pullback ────
    // Search the last [1..bp_max_age] bars for a prior bar whose close
    // exceeded the highest high over the `lookback` bars ending just before it
    // (the "breakout bar"). That window's high is the `breakout_level`.
    // Enter on the current bar only if it pulled back to or below that level
    // (low <= level), reclaimed it (close > level), and closed bullish
    // (close > open). Earliest qualifying breakout wins.
    bool signal_breakout_pullback_() const {
        // S38: BO_PB is bull-only. Suppress when externally-fed D1 trend is
        // bearish (set via set_d1_bullish() by main.cpp / sweep harness).
        // Without this, false-breakout-in-bear murders the strategy.
        if (!d1_bullish_) return false;

        int sz = (int)highs_.size();
        int lb = cfg_.lookback;
        int max_age = cfg_.bp_max_age > 0 ? cfg_.bp_max_age : 5;
        if (sz < lb + max_age + 2) return false;
        if ((int)opens_.size() != sz) return false;

        for (int b = 1; b <= max_age; ++b) {
            int bar_idx = sz - 1 - b;
            int win_end = bar_idx;            // exclusive
            int win_start = win_end - lb;
            if (win_start < 0) continue;
            double prior_high = 0.0;
            for (int i = win_start; i < win_end; ++i) {
                if (highs_[i] > prior_high) prior_high = highs_[i];
            }
            if (prior_high <= 0.0) continue;
            if (closes_[bar_idx] > prior_high) {
                double level = prior_high;
                return (lows_.back()   <= level)
                    && (closes_.back() >  level)
                    && (closes_.back() >  opens_.back());
            }
        }
        return false;
    }

    // ── DUAL_THRUST: range breakout entry ───────────────────────────────────
    bool signal_dual_thrust_() const {
        int n = cfg_.dt_range_bars;
        if ((int)closes_.size() < n + 1) return false;
        if ((int)opens_.size() < n + 1)  return false;

        const int sz = (int)closes_.size();
        double HH = 0.0, LC = 1e18, HC = 0.0, LL = 1e18;
        for (int i = sz - n - 1; i < sz - 1; ++i) {
            if (highs_[i] > HH) HH = highs_[i];
            if (lows_[i] < LL)  LL = lows_[i];
            if (closes_[i] > HC) HC = closes_[i];
            if (closes_[i] < LC) LC = closes_[i];
        }

        double range = std::max(HH - LC, HC - LL);
        double upper_trigger = opens_.back() + cfg_.dt_k1 * range;

        return closes_.back() > upper_trigger;
    }

    // ── ICHIMOKU: Cloud breakout + Tenkan/Kijun cross (Session 29) ─────────
    // Entry signal fires when:
    //   1. Price closes above both Senkou Span A and Senkou Span B (above cloud)
    //   2. Tenkan-sen > Kijun-sen (short-term momentum confirms)
    //   3. Close > Kijun-sen (price is above base line)
    bool signal_ichimoku_() const {
        if ((int)highs_.size() < cfg_.ichi_senkou_b_period) return false;

        double tenkan  = ichi_midpoint_(cfg_.ichi_tenkan_period);
        double kijun   = ichi_midpoint_(cfg_.ichi_kijun_period);
        double span_a  = (tenkan + kijun) / 2.0;
        double span_b  = ichi_midpoint_(cfg_.ichi_senkou_b_period);

        if (tenkan <= 0.0 || kijun <= 0.0 || span_a <= 0.0 || span_b <= 0.0)
            return false;

        double close = closes_.back();
        double cloud_top = std::max(span_a, span_b);

        // All three conditions must be met
        bool above_cloud   = (close > cloud_top);
        bool tenkan_cross  = (tenkan > kijun);
        bool above_kijun   = (close > kijun);

        return above_cloud && tenkan_cross && above_kijun;
    }

    // ── SUPERTREND: signal is the bullish flip (computed in close_bar_) ─────
    // The actual computation happens in supertrend_update_() which is called
    // before evaluate_signal_(). The flip result is passed in as a parameter.
    bool signal_supertrend_(bool st_flip) const {
        return st_flip;
    }

    // ── WILLIAMS_R: cross up from oversold (Session 29b) ────────────────────
    // Signal fires when Williams %R crosses up from below threshold.
    // Different timing than RSI: %R uses highest high/lowest low range
    // normalization which makes it more responsive to recent extremes.
    bool signal_williams_r_() const {
        if ((int)closes_.size() < cfg_.willr_period + 2) return false;
        double r_now  = williams_r_(cfg_.willr_period);
        double r_prev = williams_r_prev_(cfg_.willr_period);
        // Cross up: was at/below threshold, now above
        return (r_prev <= cfg_.willr_threshold) && (r_now > cfg_.willr_threshold);
    }

    // ── STOCH_RSI: cross up from oversold (Session 29b) ─────────────────────
    // Signal fires when Stochastic RSI crosses up from below threshold.
    // Faster than raw RSI — catches reversals sooner because it normalizes
    // RSI within its own range. Ideal for mean-reversion timing.
    bool signal_stoch_rsi_() const {
        if ((int)closes_.size() < cfg_.stochrsi_rsi_period + cfg_.stochrsi_stoch_period + 3)
            return false;
        double sr_now  = stoch_rsi_(cfg_.stochrsi_rsi_period, cfg_.stochrsi_stoch_period);
        double sr_prev = stoch_rsi_prev_(cfg_.stochrsi_rsi_period, cfg_.stochrsi_stoch_period);
        // Cross up from below threshold
        return (sr_prev <= cfg_.stochrsi_threshold) && (sr_now > cfg_.stochrsi_threshold);
    }

    // ── OVERNIGHT: buy at the entry_hour_utc H1 bar close when trend is up ──
    bool signal_overnight_() const {
        if ((int)closes_.size() < cfg_.lookback + 1) return false;

        int64_t bar_open_ms = bar_ts_ms_.back();
        int bar_hour = utc_hour_from_ms_(bar_open_ms);
        if (bar_hour != cfg_.entry_hour_utc) return false;

        double now = closes_.back();
        double ref = closes_[closes_.size() - 1 - cfg_.lookback];
        if (now <= ref) return false;

        if (closes_.back() <= opens_.back()) return false;

        std::printf("[%s] OVERNIGHT signal | bar_hour=%d(UTC) | trend_ret=+%.1fbp | bar_ret=+%.1fbp\n",
            cfg_.tag.c_str(), bar_hour,
            (now / ref - 1.0) * 1e4,
            (closes_.back() / opens_.back() - 1.0) * 1e4);
        std::fflush(stdout);

        return true;
    }

    // ── WEEKDAY: buy on Monday D1 bar close when close > SMA(sma_len) ───────
    bool signal_weekday_() const {
        if ((int)closes_.size() < cfg_.sma_len) return false;

        int64_t bar_open_ms = bar_ts_ms_.back();
        int bar_dow = utc_dow_from_ms_(bar_open_ms);
        if (bar_dow != cfg_.entry_dow) return false;

        double sma = sma_(cfg_.sma_len);
        if (sma <= 0.0) return false;
        if (closes_.back() <= sma) return false;

        std::printf("[%s] WEEKDAY signal | dow=%d | close=%.2f > sma(%d)=%.2f\n",
            cfg_.tag.c_str(), bar_dow, closes_.back(), cfg_.sma_len, sma);
        std::fflush(stdout);

        return true;
    }

    // MIMIC (S-2026-07-03): scan back for the most-recent W-bar symmetric jump
    // event. +thr => mimic (long), -thr => down-jump (flat). Returns 1=long, 0=flat.
    int mimic_state_() const {
        const int W  = cfg_.mimic_w > 0 ? cfg_.mimic_w : 24;
        const int sz = (int)closes_.size();
        if (sz < W + 1) return 0;
        for (int k = sz - 1; k >= W; --k) {
            double j = closes_[k] / closes_[k - W] - 1.0;
            if (j >=  cfg_.mimic_thr) return 1;   // most recent event = mimic -> long
            if (j <= -cfg_.mimic_thr) return 0;   // most recent event = down-jump -> flat
        }
        return 0;
    }

    // Intra-bar MIMIC test (S-2026-07-05, operator: no boundary wait). The
    // mimic is a PRICE event — a jump of live_px vs the close W bars back —
    // not a bar-close event. Returns true the instant the live (forming-bar)
    // price crosses +thr, so entry fires mid-hour instead of at the H1 close.
    bool intrabar_mimic_fires_(double live_px) const {
        const int W  = cfg_.mimic_w > 0 ? cfg_.mimic_w : 24;
        const int sz = (int)closes_.size();
        if (sz < W || live_px <= 0.0) return false;
        const double j = live_px / closes_[sz - W] - 1.0;   // vs price W bars ago
        return j >= cfg_.mimic_thr;                        // mimic NOW = most-recent event = long
    }

    // Drive an entry evaluation intra-bar at the live price/ts (MIMIC only).
    // Reuses the full evaluate_signal_ gate chain (cluster/funding/vol/etc);
    // the override members make it open at the live price NOW, not next-bar-open.
    void evaluate_signal_intrabar_(double live_px, int64_t ts_ms) {
        intrabar_entry_px_ = live_px;
        intrabar_entry_ts_ = ts_ms;
        evaluate_signal_(false);
        intrabar_entry_px_ = 0.0;
        intrabar_entry_ts_ = 0;
    }

    void evaluate_signal_(bool st_flip = false) {
        bool fire = false;
        switch (cfg_.kind) {
            case StrategyKind::TSMOM:          fire = signal_tsmom_();          break;
            case StrategyKind::DONCHIAN:       fire = signal_donchian_();       break;
            case StrategyKind::BOLLINGER:      fire = signal_bollinger_();      break;
            case StrategyKind::RSI_REVERT:     fire = signal_rsi_revert_();     break;
            case StrategyKind::OVERNIGHT:      fire = signal_overnight_();      break;
            case StrategyKind::WEEKDAY:        fire = signal_weekday_();        break;
            case StrategyKind::KELTNER_REVERT: fire = signal_keltner_revert_(); break;
            case StrategyKind::KELTNER_BREAK:  fire = signal_keltner_break_();  break;
            case StrategyKind::REGIME_SWITCH:  fire = signal_regime_switch_();  break;
            case StrategyKind::EMAX:           fire = signal_emax_();           break;
            case StrategyKind::ROC:            fire = signal_roc_();            break;
            case StrategyKind::IBS:            fire = signal_ibs_();            break;
            case StrategyKind::DUAL_THRUST:    fire = signal_dual_thrust_();    break;
            case StrategyKind::ICHIMOKU:       fire = signal_ichimoku_();       break;
            case StrategyKind::SUPERTREND:     fire = signal_supertrend_(st_flip); break;
            case StrategyKind::WILLIAMS_R:     fire = signal_williams_r_();        break;
            case StrategyKind::STOCH_RSI:      fire = signal_stoch_rsi_();         break;
            case StrategyKind::BREAKOUT_PULLBACK: fire = signal_breakout_pullback_(); break;
            case StrategyKind::MIMIC:         fire = (mimic_state_() == 1) ||
                                                      (intrabar_entry_px_ > 0.0);  break;  // intra-bar: caller pre-verified the live jump
        }
        if (!fire) return;

        // ── Phase-4 item 21: open a gate-attribution record for this RAW signal.
        // corr-id threads the whole chain; each gate below records its reason;
        // the counterfactual resolves forward from prices. Observational only.
        uint64_t corr = 0;
        cur_corr_id_ = 0;
        if (gate_sink_) {
            double sig_px = (intrabar_entry_px_ > 0.0) ? intrabar_entry_px_ : last_close_;
            int64_t sig_ts = bar_ts_ms_.empty() ? 0 : bar_ts_ms_.back();
            corr = gate_sink_->begin_signal(cfg_.tag, cfg_.symbol,
                                            strategy_name(cfg_.kind), sig_px, sig_ts);
            cur_corr_id_ = corr;
        }

        // ── Portfolio gate (Session 29b) ────────────────────────────────────
        // If main.cpp has disabled entries (max positions or drawdown breaker),
        // suppress immediately. Cheapest check — do first.
        if (!portfolio_entry_allowed_) {
            std::printf("[%s] PORTFOLIO_GATE: entries disabled — signal SUPPRESSED\n",
                cfg_.tag.c_str());
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, "PORTFOLIO_GATE", "entries disabled (max-pos/DD breaker)");
            return;
        }

        // ── Correlation-cluster exposure cap (Session 45) ───────────────────
        // Blocks a fresh entry when too many correlated positions are already
        // open (per-symbol and per-cluster caps enforced in main.cpp). Prevents
        // the May-30 failure: 6 same-symbol engines firing together, all
        // stopped out on one adverse beta move for an amplified loss.
        if (!cluster_gate_) {
            const char* gname = cluster_gate_name_   ? cluster_gate_name_   : "CLUSTER_GATE";
            const char* gwhy  = cluster_gate_reason_ ? cluster_gate_reason_ : "correlated exposure cap hit";
            std::printf("[%s] %s: %s — signal SUPPRESSED\n", cfg_.tag.c_str(), gname, gwhy);
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, gname, gwhy);
            return;
        }

        // ── S34: CONFIRMATION BAR (backward look — no time wait) ───────────
        // Require signal_confirm_bars consecutive bars with signal direction.
        // Looks BACKWARD through history rather than forward-waiting — if
        // the prior (N-1) bars already showed signal, enter NOW.
        // Filters DOA setups where 1-bar signal didn't follow through.
        if (cfg_.signal_confirm_bars > 1) {
            int needed = cfg_.signal_confirm_bars - 1;
            int prior_ok = 0;
            int prior_unknown = 0;
            for (int k = 1; k <= needed; k++) {
                int r = signal_at_back(k);
                if (r == 1) prior_ok++;
                else if (r == -1) prior_unknown++;
                else break;  // r == 0 -> broke confirmation
            }
            // If strategy doesn't support back-check (mean-revert kinds),
            // skip confirmation entirely — single-bar signal is fine for them.
            bool unsupported = (prior_unknown == needed);
            if (!unsupported && prior_ok < needed) {
                std::printf("[%s] CONFIRMATION_BAR: prior_ok=%d/%d — wait\n",
                    cfg_.tag.c_str(), prior_ok, needed);
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "CONFIRMATION_BAR",
                    "prior_ok=" + std::to_string(prior_ok) + "/" + std::to_string(needed));
                return;
            }
        }

        // ── Funding headwind filter (Session 30, Edge 1) ────────────────────
        // When funding is extremely positive (longs paying >10bp/8h), the crowd
        // is already long-heavy. Suppress new long entries — fade the crowd.
        if (funding_headwind_) {
            std::printf("[%s] FUNDING_HEADWIND: extreme positive funding — signal SUPPRESSED\n",
                cfg_.tag.c_str());
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, "FUNDING_HEADWIND", "extreme positive funding");
            return;
        }

        // ── Volatility regime gate (Session 30, Edge 3) ─────────────────────
        // LOW vol → suppress counter-trend (they need vol expansion to profit)
        // HIGH vol → suppress trend-following (whipsaws kill momentum entries)
        // MEDIUM → all allowed
        if (vol_regime_ == VolRegime::LOW && !is_trend_following()) {
            std::printf("[%s] VOL_REGIME: LOW vol — counter-trend SUPPRESSED (need vol for reversals)\n",
                cfg_.tag.c_str());
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, "VOL_REGIME", "LOW vol — counter-trend");
            return;
        }
        if (vol_regime_ == VolRegime::HIGH && is_trend_following()) {
            std::printf("[%s] VOL_REGIME: HIGH vol — trend-following SUPPRESSED (whipsaw risk)\n",
                cfg_.tag.c_str());
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, "VOL_REGIME", "HIGH vol — trend-following");
            return;
        }

        // ── BTC correlation regime filter (Session 29b) ─────────────────────
        // When correlation with BTC is extreme, altcoin alpha vanishes (herding).
        // Only applies to non-BTC engines. State fed by main.cpp.
        if (cfg_.corr_filter && corr_high_) {
            std::printf("[%s] CORR_FILTER: BTC correlation extreme — signal SUPPRESSED\n",
                cfg_.tag.c_str());
            std::fflush(stdout);
            if (gate_sink_) gate_sink_->suppressed(corr, "CORR_FILTER", "BTC correlation extreme");
            return;
        }

        // ── Time-of-day session filter (Session 29b) ────────────────────────
        // Suppress entries during low-activity sessions (e.g. Asian 00-08 UTC).
        // Only for sub-H6 engines where low liquidity causes false signals.
        if (cfg_.session_filter) {
            int64_t bar_open_ms = bar_ts_ms_.back();
            int bar_hour = utc_hour_from_ms_(bar_open_ms);
            bool in_suppressed = false;
            if (cfg_.session_suppress_start < cfg_.session_suppress_end) {
                in_suppressed = (bar_hour >= cfg_.session_suppress_start &&
                                 bar_hour < cfg_.session_suppress_end);
            } else {
                // Wrap-around (e.g. 22-06 = suppress late night through early morning)
                in_suppressed = (bar_hour >= cfg_.session_suppress_start ||
                                 bar_hour < cfg_.session_suppress_end);
            }
            if (in_suppressed) {
                std::printf("[%s] SESSION_FILTER: bar_hour=%d in suppressed zone [%d-%d) — signal SUPPRESSED\n",
                    cfg_.tag.c_str(), bar_hour,
                    cfg_.session_suppress_start, cfg_.session_suppress_end);
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "SESSION_FILTER",
                    "bar_hour=" + std::to_string(bar_hour) + " in suppressed session");
                return;
            }
        }

        // ── Volume regime filter (Session 29) ──────────────────────────────
        // If enabled and we have enough bar history, suppress ALL entries when
        // the just-closed bar's tick count is below vol_tick_ratio * average.
        // This catches weekend dead zones and exchange outage periods.
        if (cfg_.volume_gate) {
            int bars_seen = (int)tick_counts_.size();
            if (bars_seen >= cfg_.vol_tick_warmup) {
                // Use the PREVIOUS bar's tick count (last element in tick_counts_
                // is the bar we just closed — which is the one we're evaluating)
                int last_tick_count = tick_counts_.back();

                // Skip filler bars (tick_count == 0) — these are synthetic bars
                // created when multiple bar boundaries are crossed at once. They
                // do NOT indicate a dead zone, just a seeding/startup artifact.
                if (last_tick_count == 0) {
                    // Not a real bar — don't suppress, don't count against avg
                } else {
                    double avg = avg_tick_count_();
                    double threshold = cfg_.vol_tick_ratio * avg;
                    if (last_tick_count < (int)threshold) {
                        std::printf("[%s] VOLUME_GATE: low activity (ticks=%d < %.0f=%.0f%%*avg_%.0f) — signal SUPPRESSED\n",
                            cfg_.tag.c_str(), last_tick_count, threshold,
                            cfg_.vol_tick_ratio * 100.0, avg);
                        std::fflush(stdout);
                        if (gate_sink_) gate_sink_->suppressed(corr, "VOLUME_GATE",
                            "low activity ticks=" + std::to_string(last_tick_count));
                        return;
                    }
                }
            }
        }

        // ── Volatility regime filter (Session 28) ───────────────────────────
        // When vol_filter is enabled, check ATR(14)/ATR(50) ratio to suppress
        // counter-trend entries during chaotic or elevated-vol conditions.
        if (cfg_.vol_filter) {
            double vr = vol_ratio_();
            bool is_counter_trend = (cfg_.kind == StrategyKind::RSI_REVERT ||
                                     cfg_.kind == StrategyKind::BOLLINGER ||
                                     cfg_.kind == StrategyKind::KELTNER_REVERT);
            if (vr > cfg_.vol_chaos_threshold) {
                // Chaos regime — suppress ALL entries
                std::printf("[%s] VOL_FILTER: CHAOS regime (ratio=%.2f > %.2f) — signal SUPPRESSED\n",
                    cfg_.tag.c_str(), vr, cfg_.vol_chaos_threshold);
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "VOL_FILTER", "CHAOS regime");
                return;
            }
            if (vr > cfg_.vol_elevated_threshold && is_counter_trend) {
                // Elevated vol — suppress counter-trend only
                std::printf("[%s] VOL_FILTER: ELEVATED vol (ratio=%.2f > %.2f) — counter-trend SUPPRESSED\n",
                    cfg_.tag.c_str(), vr, cfg_.vol_elevated_threshold);
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "VOL_FILTER", "ELEVATED vol — counter-trend");
                return;
            }
        }

        // ── Multi-timeframe gate (Session 28, refined Session 29) ────────────
        // When mtf_gate is enabled and D1 trend has been bearish for 3+ consecutive
        // readings, suppress counter-trend entries. Single-bar bearish dips no
        // longer block mean-reversion (those are actually ideal entry conditions).
        if (cfg_.mtf_gate && !d1_bullish_ && d1_bearish_streak_ >= 5) {  // raised from 3 — shadow tuning: 3-day dips are normal, only suppress on sustained 5+ day downtrends
            bool is_counter_trend = (cfg_.kind == StrategyKind::RSI_REVERT ||
                                     cfg_.kind == StrategyKind::BOLLINGER ||
                                     cfg_.kind == StrategyKind::KELTNER_REVERT);
            if (is_counter_trend) {
                std::printf("[%s] MTF_GATE: D1 bearish (streak=%d) — counter-trend signal SUPPRESSED\n",
                    cfg_.tag.c_str(), d1_bearish_streak_);
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "MTF_GATE",
                    "D1 bearish streak=" + std::to_string(d1_bearish_streak_));
                return;
            }
        }

        // ── ADX regime filter (Session 29, refined Session 30) ────────────────
        // When adx_filter is enabled, suppress TREND-FOLLOWING entries when
        // ADX < threshold (market is ranging/choppy, no directional edge).
        // Session 30 refinement: funding tailwind lowers threshold by 5
        // (carry edge makes marginal setups viable).
        if (cfg_.adx_filter) {
            double adx_val = adx_(cfg_.adx_period);
            double effective_threshold = cfg_.adx_threshold;
            if (funding_tailwind_) {
                effective_threshold -= 5.0;  // carry edge relaxes the bar
            }
            if (adx_val < effective_threshold) {
                std::printf("[%s] ADX_FILTER: ADX=%.1f < %.1f%s — trend signal SUPPRESSED\n",
                    cfg_.tag.c_str(), adx_val, effective_threshold,
                    funding_tailwind_ ? " (tailwind-adjusted)" : "");
                std::fflush(stdout);
                if (gate_sink_) gate_sink_->suppressed(corr, "ADX_FILTER", "ADX below threshold");
                return;
            }
        }

        double a = atr_(cfg_.atr_period);
        if (a <= 0.0) return;
        // S44M #2: low-vol entry filter. Compare current ATR to rolling avg.
        // When ATR is suppressed (chop), skip entry — TSMOM bait into reversal.
        if (cfg_.low_vol_skip_ratio > 0.0 && cfg_.low_vol_avg_bars > 0
            && (int)closes_.size() > cfg_.low_vol_avg_bars + cfg_.atr_period) {
            double sum_atr = 0.0;
            int n_atr = std::min((int)closes_.size() - cfg_.atr_period, cfg_.low_vol_avg_bars);
            for (int i = 0; i < n_atr; ++i) {
                int idx = (int)closes_.size() - n_atr + i;
                if (idx >= cfg_.atr_period) {
                    sum_atr += std::abs(highs_[idx] - lows_[idx]);
                }
            }
            double avg_atr = (n_atr > 0) ? sum_atr / n_atr : a;
            if (avg_atr > 0.0 && a < cfg_.low_vol_skip_ratio * avg_atr) {
                if (gate_sink_) gate_sink_->suppressed(corr, "LOW_VOL_SKIP", "ATR below chop ratio");
                return;  // chop suppression
            }
        }

        // Entry will materialise on the NEXT bar's open — but in a live tick
        // stream, "next bar open" = current price right now (we're at the bar
        // boundary). For paper-exact mirroring of backtest, defer to first
        // tick of the next bar via a pending flag instead. We use simple
        // approximation: enter at last_close_ which is the price at this
        // moment of bar close. The error vs theoretical next-bar-open is
        // <1 tick on liquid pairs.
        // Intra-bar MIMIC entry opens at the LIVE price NOW; the normal path
        // opens at the just-closed bar's close (≈ next-bar-open on a live feed).
        entry_px_     = (intrabar_entry_px_ > 0.0) ? intrabar_entry_px_ : last_close_;
        atr_at_entry_ = a;
        sl_px_        = entry_px_ - cfg_.sl_atr_mult * a;
        // S44L H: swing-low SL — find min(low) over last N bars, use as
        // tighter bound. SL = max(atr_sl, swing_low) so we never go wider.
        if (cfg_.swing_low_bars > 0 && (int)lows_.size() >= cfg_.swing_low_bars) {
            double swing = lows_[lows_.size() - cfg_.swing_low_bars];
            for (size_t i = lows_.size() - cfg_.swing_low_bars; i < lows_.size(); ++i) {
                if (lows_[i] < swing) swing = lows_[i];
            }
            if (swing > sl_px_) sl_px_ = swing;  // tighter (closer to entry)
        }
        // ── HARD FLOOR (Session 32) — never lose more than hard_floor_bp ──
        if (cfg_.hard_floor_bp < 0.0) {
            double floor_px = entry_px_ * (1.0 + cfg_.hard_floor_bp / 1e4);
            if (sl_px_ < floor_px) {
                std::printf("[%s] HARD_FLOOR  atr_sl=%.6f  floor=%.6f(%.0fbp)  -> tighten\n",
                    cfg_.tag.c_str(), sl_px_, floor_px, cfg_.hard_floor_bp);
                std::fflush(stdout);
                sl_px_ = floor_px;
            }
        }
        entry_ts_ms_  = (intrabar_entry_ts_ > 0) ? intrabar_entry_ts_
                                                 : (cur_open_ts_ms_ + cfg_.tf_secs * 1000);
        time_exit_ts_ms_ = entry_ts_ms_ + (int64_t)cfg_.hold_bars * cfg_.tf_secs * 1000;
        in_position_  = true;
        bars_held_    = 0;

        // Phase-4 item 21: the signal passed every gate and a real entry was
        // taken — record it (no counterfactual; this trade actually happens).
        if (gate_sink_) gate_sink_->passed(corr);

        // Initialise trailing stop state
        trail_armed_   = false;
        trail_stop_px_ = 0.0;
        trail_arm_px_  = entry_px_ + cfg_.trail_arm_atr * a;
        mfe_px_        = entry_px_;
        mfe_bp_        = 0.0;

        // Initialise pyramid state (Session 31)
        pyramid_count_    = 0;
        pyramid_next_atr_ = cfg_.pyramid_arm_atr;  // first add level (used once trail arms)
        pyramid_adds_.clear();

        double arm_bp = (trail_arm_px_ / entry_px_ - 1.0) * 1e4;
        std::printf("[%s] ENTRY  px=%.6f  sl=%.6f  atr=%.6f  hold=%dbars  trail_arm=%.6f(+%.0fbp)  shadow=%d  funding=%s  conviction=%s  sizing=%.2f  pyramid=%s\n",
            cfg_.tag.c_str(), entry_px_, sl_px_, a, cfg_.hold_bars,
            trail_arm_px_, arm_bp,
            shadow_mode ? 1 : 0,
            funding_tailwind_ ? "TAILWIND" : "neutral",
            is_high_conviction() ? "HIGH" : "normal",
            sizing_mult_,
            cfg_.pyramid_enabled ? "ON" : "off");
        std::fflush(stdout);

        // RSIREV BE-floor (S-2026-07-23): DEFER the BUY. This is a BE-ENTRY — the leg
        // opens (routes real) ONLY when fav>=confirm (armed, in check_exits_). Firing the
        // BUY here would be an immediate-entry that can trade into a pre-BE loss
        // (feedback-no-immediate-entry-mimic-only / feedback-no-prebe-loss-ever). The
        // position is tracked internally (in_position_) but nothing is routed until arm;
        // an unarmed exit books 0 (see exit_position_). Guarded ⇒ every other leg routes
        // its BUY here exactly as before.
        const bool rsirev_defer_open = (cfg_.rsi_revert_intraday_floor && cfg_.rsi_level_revert);
        if (on_order_intent_ && !rsirev_defer_open) {
            OrderIntentRecord intent;
            intent.tag    = cfg_.tag;
            intent.symbol = cfg_.symbol;
            intent.is_buy = true;
            intent.ref_px = entry_px_;
            intent.ts_ms  = entry_ts_ms_;
            intent.risk_mult = risk_mult_;   // P1: carry DD-throttle x vol-overlay to qty calc
            intent.corr_id = cur_corr_id_;   // Phase-4 item 21: thread corr-id signal->order
            on_order_intent_(intent);
        }
    }

    void check_exits_(double price, int64_t ts_ms) {
        if (!in_position_) return;
        if (cfg_.ride_to_flip) return;   // MIMIC: NO trade-level price stops; exit only on flip (close_bar_)

        // ── RSIREV intraday BE-floor (S-2026-07-23) — CERTIFIED honest management ──
        // Guarded, OPT-IN. Reproduces backtest/rsirev_intraday_verify_full_bt.cpp's
        // intraday arm + BE/giveback stop, per tick, and BYPASSES the staged-ratchet/
        // early-kill/trail/giveback-cap/mfe-trail/time logic below (early return). The
        // RSI>=thr flip-out is handled at the daily close in close_bar_. Cert math:
        //   confirm = entry*(1 + max(60bp, 2*cost));  arm when a tick reaches confirm.
        //   peak    = running fractional MFE = (max_price - entry)/entry.
        //   armed stop = entry*(1 + max(0, peak*(1-g))), floored at BE(=entry); g=0.9.
        //   exit at the stop with honest worse-of fill (gap-through books the tick price).
        // BE-floor-on-open: leg books nothing until fav>=confirm (BE-ENTRY), so a stop
        // hit at/above BE nets >=0 before cost; a gap-through can still book a real tail
        // (honest — nNeg>0, NOT zero by construction — per feedback-no-prebe-loss-ever).
        if (cfg_.rsi_revert_intraday_floor && cfg_.rsi_level_revert) {
            // ENTRY-DAY skip: the cert does NOT manage intraday on the entry day —
            // management (arm + floor + peak) starts the FIRST FULL day after entry.
            // bars_held_==1 during the entry day, >=2 thereafter. Skipping the entry
            // day is load-bearing: without it the oversold-bounce noise churns into
            // repeated BE-stops (measured: WR 26% vs cert 63%). Return (don't fall
            // through to the staged-ratchet/trail block, which is not this engine).
            if (bars_held_ < 2) return;
            if (price > mfe_px_) {
                mfe_px_ = price;
                mfe_bp_ = (price / entry_px_ - 1.0) * 1e4;
            }
            double peak = (mfe_px_ - entry_px_) / entry_px_;      // running fractional MFE
            if (peak < 0.0) peak = 0.0;
            const double confirm_bp = std::max(60.0, cfg_.round_trip_bp * 2.0);
            const double confirm_px = entry_px_ * (1.0 + confirm_bp / 1e4);
            if (!trail_armed_ && price >= confirm_px) {           // BE-floor arms (reuse trail_armed_)
                trail_armed_ = true;
                std::printf("[%s] RSIREV_ARM  px=%.6f  confirm=%.6f(+%.1fbp)  entry=%.6f\n",
                    cfg_.tag.c_str(), price, confirm_px, confirm_bp, entry_px_);
                std::fflush(stdout);
                // BE-ENTRY: NOW the leg really opens — route the deferred BUY (the open
                // was withheld at signal so nothing routes into a pre-BE loss). ref_px is
                // the live confirm price (the real fill); P&L is still booked from the
                // original entry_px_ (the cert basis) at exit.
                if (on_order_intent_) {
                    OrderIntentRecord intent;
                    intent.tag = cfg_.tag; intent.symbol = cfg_.symbol;
                    intent.is_buy = true; intent.ref_px = price; intent.ts_ms = ts_ms;
                    intent.risk_mult = risk_mult_; intent.corr_id = cur_corr_id_;
                    on_order_intent_(intent);
                }
            }
            if (trail_armed_) {
                double stop = entry_px_ * (1.0 + std::max(0.0, peak * (1.0 - cfg_.rsirev_giveback_g)));
                if (stop < entry_px_) stop = entry_px_;           // floor at BE(=entry)
                if (price <= stop) {
                    double fill = stop;                            // touch books the stop level
                    if (cfg_.realistic_gap_fill && price < stop)   // gap-through books the worse tick price
                        fill = price * (1.0 - cfg_.gap_extra_slip_bp / 1e4);
                    exit_position_(fill, ts_ms, "RSIREV_FLOOR");
                    return;
                }
            }
            return;   // bypass the staged-ratchet/early-kill/trail/giveback/time block
        }

        // Update MFE tracking
        if (price > mfe_px_) {
            mfe_px_ = price;
            mfe_bp_ = (price / entry_px_ - 1.0) * 1e4;
        }

        // ── BE-lock price: entry + round-trip fees (Session 31) ─────────
        // This is the floor for the trail stop once armed. A winner can
        // NEVER become a loser after the trail arms.
        double be_px = entry_px_ * (1.0 + cfg_.round_trip_bp / 1e4);

        // ── STAGED BP RATCHET + PROGRESSIVE LOCK (Session 32b/c) ────────
        // Stage 2 (mfe in [ratchet_start_bp, be_arm_bp]): linear ramp -50bp -> 0bp.
        // Stage 3 (mfe >= be_arm_bp): lock = round_trip + (mfe-arm) * lock_pct
        //   where lock_pct grows progressively with MFE:
        //     50-100: ratchet_lock_pct (0.75)
        //     100-200: prog_lock_pct_2 (0.85)
        //     200-300: prog_lock_pct_3 (0.90)
        //     300+:    prog_lock_pct_4 (0.95)
        if (cfg_.ratchet_start_bp > 0.0 && mfe_bp_ >= cfg_.ratchet_start_bp) {
            double locked_bp;
            if (mfe_bp_ < cfg_.be_arm_bp) {
                // Stage 2: ramp -50bp -> 0bp linearly between start and arm
                double range = cfg_.be_arm_bp - cfg_.ratchet_start_bp;
                if (range > 0.0) {
                    locked_bp = -50.0 + (mfe_bp_ - cfg_.ratchet_start_bp) / range * 50.0;
                } else {
                    locked_bp = 0.0;
                }
            } else {
                // Stage 3: progressive lock_pct grows with MFE
                double lock_pct;
                if      (mfe_bp_ < 100.0) lock_pct = cfg_.ratchet_lock_pct;
                else if (mfe_bp_ < 200.0) lock_pct = cfg_.prog_lock_pct_2;
                else if (mfe_bp_ < 300.0) lock_pct = cfg_.prog_lock_pct_3;
                else                       lock_pct = cfg_.prog_lock_pct_4;
                locked_bp = cfg_.round_trip_bp + (mfe_bp_ - cfg_.be_arm_bp) * lock_pct;
            }
            double ratchet_sl = entry_px_ * (1.0 + locked_bp / 1e4);
            if (ratchet_sl > sl_px_) {
                double prev = sl_px_;
                sl_px_ = ratchet_sl;
                std::printf("[%s] STAGED_RATCHET  mfe=+%.1fbp  locked=%+.1fbp  sl: %.6f -> %.6f\n",
                    cfg_.tag.c_str(), mfe_bp_, locked_bp, prev, sl_px_);
                std::fflush(stdout);
            }
            if (trail_armed_ && ratchet_sl > trail_stop_px_) {
                trail_stop_px_ = ratchet_sl;
            }
        }

        // ── EARLY-KILL: dead-on-arrival dump exit (Session 32b) ─────────
        // If MFE never crossed early_kill_mfe AND price is currently deeper
        // than early_kill_bp, exit before the full hard floor kicks in.
        if (cfg_.early_kill_bp < 0.0 && mfe_bp_ < cfg_.early_kill_mfe) {
            int64_t held_ms = (entry_ts_ms_ > 0) ? (ts_ms - entry_ts_ms_) : 0;
            bool min_hold_ok = (cfg_.early_kill_min_hold_ms <= 0) ||
                               (held_ms >= cfg_.early_kill_min_hold_ms);
            double unreal_bp = (price / entry_px_ - 1.0) * 1e4;
            if (min_hold_ok && unreal_bp <= cfg_.early_kill_bp) {
                std::printf("[%s] EARLY_KILL  mfe=+%.1fbp(<%.0f)  unreal=%+.1fbp  held=%llds  exit @ %.6f\n",
                    cfg_.tag.c_str(), mfe_bp_, cfg_.early_kill_mfe, unreal_bp,
                    (long long)(held_ms / 1000), price);
                std::fflush(stdout);
                exit_position_(price, ts_ms, "EARLY_KILL");
                return;
            }
        }

        // ── GIVEBACK CAP (Session 32c) ──────────────────────────────────
        // Once MFE crosses giveback_arm_bp, force exit when current
        // unrealised drops by giveback_pct of the peak. Catches sharp
        // reversals that the ratchet's incremental updates miss.
        // Example: peak +200bp, giveback_pct=0.30 -> exit if cur <= +140bp.
        if (cfg_.giveback_arm_bp > 0.0 && mfe_bp_ >= cfg_.giveback_arm_bp) {
            double cur_bp = (price / entry_px_ - 1.0) * 1e4;
            double giveback_bp = mfe_bp_ - cur_bp;
            if (giveback_bp >= mfe_bp_ * cfg_.giveback_pct) {
                std::printf("[%s] GIVEBACK_CAP  peak_mfe=+%.1fbp  cur=%+.1fbp  giveback=%.1fbp(>=%.0f%%)  exit @ %.6f\n",
                    cfg_.tag.c_str(), mfe_bp_, cur_bp, giveback_bp,
                    cfg_.giveback_pct * 100.0, price);
                std::fflush(stdout);
                exit_position_(price, ts_ms, "GIVEBACK");
                return;
            }
        }

        // Trailing stop logic: arm when price reaches the arm level,
        // then ratchet the trail stop up as price makes new highs.
        if (!trail_armed_) {
            if (price >= trail_arm_px_) {
                trail_armed_ = true;
                trail_stop_px_ = mfe_px_ - cfg_.trail_dist_atr * atr_at_entry_;
                // ── BE-LOCK: ensure trail is at or above breakeven ──────
                if (trail_stop_px_ < be_px) {
                    trail_stop_px_ = be_px;
                }
                double trail_bp = (trail_stop_px_ / entry_px_ - 1.0) * 1e4;
                std::printf("[%s] TRAIL_ARM+BE_LOCK  px=%.6f  mfe=%.6f(+%.1fbp)  trail_stop=%.6f(+%.1fbp)  be=%.6f\n",
                    cfg_.tag.c_str(), price, mfe_px_, mfe_bp_,
                    trail_stop_px_, trail_bp, be_px);
                std::fflush(stdout);

                // ── Initialise pyramid trigger level (Session 31) ───────
                if (cfg_.pyramid_enabled) {
                    pyramid_next_atr_ = cfg_.pyramid_arm_atr;
                }
            }
        } else {
            // Ratchet: if price made a new high, update the trail stop.
            // Use tighter trail distance if profit exceeds tighten threshold.
            double cur_profit_atr = (mfe_px_ - entry_px_) / atr_at_entry_;
            double dist = cfg_.trail_dist_atr;
            if (cfg_.trail_tighten_atr > 0.0 && cur_profit_atr >= cfg_.trail_tighten_atr) {
                dist = cfg_.trail_tighten_dist_atr;
            }
            double new_trail = mfe_px_ - dist * atr_at_entry_;
            // ── BE-LOCK: trail can never go below breakeven (Session 31) ─
            if (new_trail < be_px) new_trail = be_px;
            if (new_trail > trail_stop_px_) {
                trail_stop_px_ = new_trail;
            }

            // ── Smart Pyramid: add size when profit deep enough (Session 31) ─
            // S38b: gate on portfolio_entry_allowed_ — if DD breaker or max-pos
            // tripped, suppress pyramid adds too. Prevents exposure growth
            // during a portfolio-level adverse event.
            if (cfg_.pyramid_enabled && pyramid_count_ < cfg_.pyramid_max_adds
                && portfolio_entry_allowed_) {
                double live_profit_atr = (price - entry_px_) / atr_at_entry_;
                if (live_profit_atr >= pyramid_next_atr_) {
                    PyramidAdd pa;
                    pa.entry_px  = price;
                    pa.size_mult = cfg_.pyramid_size_mult;
                    pyramid_adds_.push_back(pa);
                    pyramid_count_++;
                    // Next pyramid triggers step_atr higher
                    pyramid_next_atr_ = live_profit_atr + cfg_.pyramid_step_atr;

                    double add_bp = (price / entry_px_ - 1.0) * 1e4;
                    std::printf("[%s] PYRAMID_ADD #%d  px=%.6f  profit=+%.1f*ATR(+%.0fbp)  "
                                "size=%.0f%%  trail_at=%.6f  next_add=%.1f*ATR\n",
                        cfg_.tag.c_str(), pyramid_count_, price,
                        live_profit_atr, add_bp,
                        pa.size_mult * 100.0,
                        trail_stop_px_, pyramid_next_atr_);
                    std::fflush(stdout);

                    // Fire pyramid callback for main.cpp to execute the buy order
                    if (on_pyramid_) {
                        on_pyramid_(cfg_.tag, price, pa.size_mult, pyramid_count_);
                    }
                }
            }
        }

        // Exit check: use effective stop (max of hard SL and trail stop)
        double eff_stop = effective_stop_();
        if (price <= eff_stop) {
            const char* reason = (trail_armed_ && trail_stop_px_ >= sl_px_) ? "TRAIL" : "SL";
            // P0/S46: gap-honest fill. In LIVE the engine sees dense (~5s) ticks,
            // so the breaching tick `price` IS the real next-available fill — a
            // stop crossed by continuous trade fills within a few bp of the stop,
            // and a genuine gap fills at the jumped price. Default ON for live.
            // NOTE for backtest: the path-sim feeds only O/H/L/C, so `price` can be
            // a coarse-bar low that live would never ride down to — that is why the
            // HARNESS leaves this OFF for the baseline edge measure (fills at
            // eff_stop, correct for continuous liquid pairs) and only turns it ON
            // for the explicit --inject-gap stress test. gap_extra_slip_bp pads
            // book-depth slippage on thin names.
            double fill = eff_stop;
            if (cfg_.realistic_gap_fill && price < eff_stop) {
                fill = price * (1.0 - cfg_.gap_extra_slip_bp / 1e4);
            }
            exit_position_(fill, ts_ms, reason);
            return;
        }

        // S44L G: MFE-trail standalone. Once MFE crosses min_bp, exit if
        // current bp pulls back to retain% of MFE peak.
        if (cfg_.mfe_trail_retain > 0.0 && mfe_bp_ >= cfg_.mfe_trail_min_bp) {
            double current_bp = (price / entry_px_ - 1.0) * 1e4;
            if (current_bp < mfe_bp_ * cfg_.mfe_trail_retain) {
                exit_position_(price, ts_ms, "MFE_TRAIL");
                return;
            }
        }

        // Time exit: held long enough.
        if (ts_ms >= time_exit_ts_ms_) {
            exit_position_(price, ts_ms, "TIME");
        }
    }

    void exit_position_(double exit_px, int64_t ts_ms, const char* reason) {
        if (!in_position_) return;
        double gross_bp = (exit_px / entry_px_ - 1.0) * 1e4;
        double net_bp   = gross_bp - cfg_.round_trip_bp;

        // ── Pyramid P&L: each add exits at the same price (Session 31) ──
        double pyramid_bp = 0.0;
        for (const auto& pa : pyramid_adds_) {
            // Each pyramid add's P&L is weighted by its size_mult
            double pa_gross = (exit_px / pa.entry_px - 1.0) * 1e4;
            double pa_net   = pa_gross - cfg_.round_trip_bp;  // each add pays its own fees
            pyramid_bp += pa_net * pa.size_mult;  // weighted by size fraction
        }
        // Total trade result: base net + weighted pyramid net
        double total_net_bp = net_bp + pyramid_bp;

        // ── RSIREV BE-ENTRY flat-book (S-2026-07-23) ────────────────────────────
        // The leg was flagged unarmed at exit (never reached +confirm) ⇒ it never
        // really opened ⇒ book FLAT 0 (no P&L, no cost), matching the cert's
        // `(be_floor && !armed) ? 0`. No SELL is routed (no BUY was ever routed). The
        // trade is still COUNTED (trades_++) so ntr/WR match the cert. Guarded flag ⇒
        // no effect on any other engine.
        const bool rsirev_flat = rsirev_flat_book_;
        rsirev_flat_book_ = false;   // consume the one-shot flag
        if (rsirev_flat) { gross_bp = 0.0; net_bp = 0.0; pyramid_bp = 0.0; total_net_bp = 0.0; }

        trades_++;
        if (total_net_bp > 0) wins_++;
        total_bp_      += total_net_bp;
        last_trade_bp_  = total_net_bp;

        if (pyramid_count_ > 0) {
            std::printf("[%s] EXIT   reason=%s  px=%.6f  base=%+.1fbp  pyramid=%+.1fbp(%d adds)  "
                        "TOTAL=%+.1fbp  mfe=%+.1fbp  trades=%d wins=%d cumulative=%+.1fbp\n",
                cfg_.tag.c_str(), reason, exit_px, net_bp, pyramid_bp, pyramid_count_,
                total_net_bp, mfe_bp_, trades_, wins_, total_bp_);
        } else {
            std::printf("[%s] EXIT   reason=%s  px=%.6f  gross=%+8.2fbp  net=%+8.2fbp  "
                        "mfe=%+.1fbp  trades=%d wins=%d total=%+8.1fbp\n",
                cfg_.tag.c_str(), reason, exit_px, gross_bp, net_bp,
                mfe_bp_, trades_, wins_, total_bp_);
        }
        std::fflush(stdout);

        // Fire order intent (SELL) for paper broker mirror BEFORE trade record.
        // RSIREV BE-ENTRY flat exit routes NO sell (no buy was ever routed).
        if (on_order_intent_ && !rsirev_flat) {
            OrderIntentRecord intent;
            intent.tag    = cfg_.tag;
            intent.symbol = cfg_.symbol;
            intent.is_buy = false;
            intent.ref_px = exit_px;
            intent.ts_ms  = ts_ms;
            on_order_intent_(intent);
        }

        // Fire trade callback for persistence
        if (on_trade_) {
            TradeRecord rec;
            rec.tag         = cfg_.tag;
            rec.symbol      = cfg_.symbol;
            rec.strategy    = strategy_name(cfg_.kind);
            rec.reason      = reason;
            rec.entry_ts_ms = entry_ts_ms_;
            rec.exit_ts_ms  = ts_ms;
            rec.entry_px    = entry_px_;
            rec.exit_px     = exit_px;
            rec.sl_px       = sl_px_;
            rec.gross_bp    = gross_bp;
            rec.net_bp      = net_bp;
            rec.mfe_bp      = mfe_bp_;
            rec.trade_num   = trades_;
            rec.shadow      = shadow_mode;
            // Pyramid fields (Session 31)
            rec.pyramid_adds = pyramid_count_;
            rec.pyramid_bp   = pyramid_bp;
            rec.total_net_bp = total_net_bp;
            on_trade_(rec);
        }

        in_position_ = false;
        entry_px_ = 0.0;
        sl_px_    = 0.0;
        entry_ts_ms_     = 0;
        time_exit_ts_ms_ = 0;
        atr_at_entry_    = 0.0;
        bars_held_       = 0;

        // Reset trailing stop state
        trail_armed_    = false;
        trail_stop_px_  = 0.0;
        trail_arm_px_   = 0.0;
        mfe_px_         = 0.0;
        mfe_bp_         = 0.0;

        // Reset pyramid state (Session 31)
        pyramid_count_    = 0;
        pyramid_next_atr_ = 0.0;
        pyramid_adds_.clear();
    }
};

} // namespace chimera
