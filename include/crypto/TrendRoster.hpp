// ============================================================================
// TrendRoster.hpp — S-2026-07-21 (branch crypto-port-trend-book)
//
// The VERIFIED 19-leg DirectionalTrendRoster, ported into ChimeraCrypto as a
// SHADOW-FIRST, long-only spot directional book (NOT a mimic — real signal, no
// BE-clamp). Source of truth: /Users/jo/Crypto research
//   backtest/CRYPTO_RECOMMENDED_BOOK_AND_SIZING_2026-07-21.md
//   backtest/CRYPTO_BOOK_RETEST_VERIFY_2026-07-21.md  (blended OOS Sharpe 1.71)
// vault [[DirectionalTrendRosterOOS]].
//
// This header defines the roster as data + a factory that returns fully-formed
// chimera::EdgeEngine::Config objects. main.cpp constructs one EdgeEngine per leg
// and drives it via the g_slots tick loop (the SAME pattern as the existing
// NEAR/THETA/SUSHI/ADA/DOT REGIME_SWITCH directional parents). All legs are:
//   • SHADOW (shadow_mode default true; no live flag set here)
//   • long-only spot, ride_to_flip (exit only on signal flip — the research
//     run_bt semantics; NO trade-level stops — matches the validated harness)
//   • NO 200DMA (regime_gate_ma is never set — the crypto hard rule)
//   • per-coin corrected round-trip cost (the 56bp stress cost is DEAD)
//
// KELTNER exit — FIXED S-2026-07-21 (crypto-keltner-pool-fix). The legacy
// KELTNER_BREAK rode a long to the LOWER band (a different engine: SOL was +1775%
// vs validated +315%). make_config now sets keltner_exit_reenter_band=true on the
// Keltner legs so they EXIT on band re-entry (close no longer > upper) = the exact
// validated research Kelt(20,2.0) long-only path. Penny-match re-verified in
// backtest/keltner_pool_reverify_bt.cpp. The already-wired main.cpp g_slots keep
// the legacy exit (flag default false) — untouched.
// ============================================================================
#pragma once

#include "core/EdgeEngine.hpp"
#include <vector>
#include <string>

namespace chimera { namespace trend_roster {

struct LegSpec {
    const char*  coin;      // display, e.g. "BTC"
    const char*  symbol;    // exchange symbol, e.g. "btcusdt"  (NDX = index, feed pending)
    const char*  role;      // "workhorse" / "trend" / "regime" / "satellite" / "add"
    StrategyKind kind;
    double       cost_bp;   // corrected per-coin round-trip cost
    bool         is_index;  // true = NDX (needs a non-Binance index feed)
};

// The 19 legs (order matches the recommended book table 1..19).
inline const std::vector<LegSpec>& legs() {
    static const std::vector<LegSpec> L = {
        // #  coin  symbol       role         kind                       cost  index
        {  "BTC", "btcusdt",  "workhorse", StrategyKind::EMAX,          14.0, false }, // 1
        {  "ETH", "ethusdt",  "workhorse", StrategyKind::EMAX,          28.0, false }, // 2
        {  "SOL", "solusdt",  "workhorse", StrategyKind::EMAX,          11.0, false }, // 3
        {  "BTC", "btcusdt",  "trend",     StrategyKind::KELTNER_BREAK, 14.0, false }, // 4
        {  "ETH", "ethusdt",  "trend",     StrategyKind::KELTNER_BREAK, 28.0, false }, // 5
        {  "SOL", "solusdt",  "trend",     StrategyKind::KELTNER_BREAK, 11.0, false }, // 6
        {  "BTC", "btcusdt",  "regime",    StrategyKind::REGIME_SWITCH, 14.0, false }, // 7
        {  "ETH", "ethusdt",  "regime",    StrategyKind::REGIME_SWITCH, 28.0, false }, // 8
        {  "SOL", "solusdt",  "regime",    StrategyKind::REGIME_SWITCH, 11.0, false }, // 9
        {  "ADA", "adausdt",  "satellite", StrategyKind::KELTNER_BREAK, 18.0, false }, // 10
        {  "BTC", "btcusdt",  "satellite", StrategyKind::ROC,           14.0, false }, // 11
        {  "SOL", "solusdt",  "satellite", StrategyKind::ROC,           11.0, false }, // 12
        {  "BTC", "btcusdt",  "satellite", StrategyKind::IBS,           14.0, false }, // 13
        {  "SOL", "solusdt",  "satellite", StrategyKind::IBS,           11.0, false }, // 14
        {  "NDX", "ndx",      "diversify", StrategyKind::TSMOM,          4.0, true  }, // 15 (TSMom50)
        {  "NDX", "ndx",      "diversify", StrategyKind::RSI_REVERT,     4.0, true  }, // 16 (RSIrev, approx)
        {  "XRP", "xrpusdt",  "add",       StrategyKind::KELTNER_BREAK, 30.0, false }, // 17
        {  "XLM", "xlmusdt",  "add",       StrategyKind::KELTNER_BREAK, 40.0, false }, // 18
        {  "GRT", "grtusdt",  "add",       StrategyKind::KELTNER_BREAK, 60.0, false }, // 19
        // S-2026-07-23 SOL+XRP RSIrev intraday BE-floor legs (certified
        // backtest/rsirev_intraday_verify_full_bt.cpp: SOL FULL/OOS +191/+48% Sh1.42/1.77
        // WR63% DD20%; XRP +177/+104% Sh1.32/1.34 WR43.5% DD11%; measured cost SOL 11 /
        // XRP 30 bp). kind RSI_REVERT + is_index=false is the UNIQUE key make_config uses
        // to build the intraday-floor config (ride_to_flip=false, rsi_revert_intraday_floor
        // =true, RSI14<25 entry / >=25 exit, g0.9) — distinct from the is_index NDX RSIrev
        // leg (#16), which stays ride_to_flip.
        {  "SOL", "solusdt",  "meanrev",   StrategyKind::RSI_REVERT,    11.0, false }, // 20 (SOL-RSIREV)
        {  "XRP", "xrpusdt",  "meanrev",   StrategyKind::RSI_REVERT,    30.0, false }, // 21 (XRP-RSIREV)
    };
    return L;
}

// Build the EdgeEngine::Config for one leg — SHADOW-first, ride_to_flip, NO 200DMA,
// daily bars, research-canonical params (EMA20/50, Kelt20/2.0, Regime20/0.40/0.25,
// Roc N=20/thr=0, IBS 0.15/0.85, TSMom L=50).
inline EdgeEngine::Config make_config(const LegSpec& s) {
    EdgeEngine::Config c;
    c.symbol         = s.symbol;
    c.tag            = std::string(s.coin) + "-" + strategy_name(s.kind) + "-TRENDROSTER";
    c.kind           = s.kind;
    c.tf_secs        = 86400;         // daily
    c.ride_to_flip   = true;          // exit only on signal flip — NO trade-level stops (validated harness)
    c.round_trip_bp  = s.cost_bp;     // corrected per-coin cost
    c.realistic_gap_fill = true;
    c.max_history    = 260;           // covers EMAX 4*slow seed (4*50)
    // research-canonical params
    c.lookback       = (s.kind == StrategyKind::TSMOM) ? 50 : 20;
    c.ema_fast       = 20;
    c.ema_slow       = 50;
    c.keltner_ema_len= 20;
    c.keltner_atr_mult = 2.0;
    // S-2026-07-22 (crypto-trigger-sensitivity-sweep): per-coin Keltner retune on the
    // 3 high-beta ALT legs where the uniform (N20,M2.0) band was TOO WIDE — it entered
    // the up-run late and gave back the early leg. OOS-validated (fit 2021-22, tested
    // 2023-26): SOL +37→+123, XRP +218→+305, XLM +332→+326, all WF folds → 3/3, lower
    // DD. Majors (BTC/ETH EMAx+Kelt, ADA) left at (N20,M2.0) — tightening them only adds
    // fakeouts (ETH IS-best 20/30 was a clear overfit). N drives BOTH the midline EMA and
    // the ATR window (keltner_exit_reenter_band). Ref backtest/CRYPTO_TRIGGER_SENSITIVITY_SWEEP_2026-07-22.md.
    if (s.kind == StrategyKind::KELTNER_BREAK) {
        if      (std::string(s.coin) == "SOL") { c.keltner_ema_len = 20; c.keltner_atr_mult = 1.5; }
        else if (std::string(s.coin) == "XRP") { c.keltner_ema_len = 30; c.keltner_atr_mult = 1.5; }
        else if (std::string(s.coin) == "XLM") { c.keltner_ema_len = 10; c.keltner_atr_mult = 1.5; }
    }
    c.roc_thr        = 0.0;
    c.ibs_lo         = 0.15;
    c.ibs_hi         = 0.85;
    // S-2026-07-21 (crypto-keltner-pool-fix): the Keltner legs use the VALIDATED
    // research exit (flat on band re-entry), NOT the legacy ride-to-lower divergence.
    c.keltner_exit_reenter_band = (s.kind == StrategyKind::KELTNER_BREAK);
    // S-2026-07-21 (crypto-final-closeout): the NDX RSIrev leg uses the VALIDATED
    // research level-revert (long while RSI<oversold, ride_to_flip exit on recovery),
    // NOT the legacy cross-up (which fires ~0 trades). Default false keeps every
    // live Session 19/21 RSI_REVERT g_slot byte-identical.
    c.rsi_level_revert = (s.kind == StrategyKind::RSI_REVERT);
    // VOL-TARGET sizing (ported): trend/Kelt/Regime/Roc legs take the pool vt=0.020;
    // IBS + NDX index legs stay size=1.0 (vt=0), matching the research 19-leg pool.
    c.vt_target      = (s.kind == StrategyKind::IBS || s.is_index) ? 0.0 : 0.020;
    // ── S-2026-07-23 SOL+XRP RSIrev intraday BE-floor legs (#20/#21) ─────────────
    // UNIQUE key: RSI_REVERT + non-index (the NDX RSIrev #16 is is_index=true). These
    // legs opt IN to the certified honest-intraday BE-floor management instead of
    // ride_to_flip: arm at entry*(1+max(60bp,2*cost)), floor stop at BE, g0.9 giveback,
    // honest worse-of fill; RSI14<25 entry / >=25 flip-out. Cert: rsirev_intraday_verify_
    // full_bt.cpp (SOL +191/+48% Sh1.42/1.77 WR63% DD20%; XRP +177/+104% Sh1.32/1.34
    // WR43.5% DD11%; measured cost SOL 11 / XRP 30 bp).
    // ADVERSE-PROTECTION: BE-floor-on-open (arm≥confirm=max(60bp,2×cost), stop floored at
    // BE) + profit-lock g0.9 giveback; honest worse-of fill books a real tail on gaps
    // (nNeg>0, NOT zero by construction) — feedback-no-prebe-loss-ever / feedback-profit-
    // lock-mandatory. Backtested verdict = certified net-positive FULL+OOS both legs.
    if (s.kind == StrategyKind::RSI_REVERT && !s.is_index) {
        c.rsi_revert_intraday_floor = true;
        c.ride_to_flip   = false;                 // OVERRIDE base(true): floor lives in check_exits_
        c.rsi_threshold  = 25.0;                  // RSI14<25 entry, >=25 flip-out
        c.atr_period     = 14;                     // RSI window = 14
        c.rsirev_giveback_g = 0.9;                 // profit-lock g0.9 (give back 10% of peak)
        c.vt_target      = 0.0;                     // unsized standalone book (matches cert)
        c.tag            = std::string(s.coin) + "-RSIREV";
        // ── RUNNER lever (S-2026-07-23b, "L2 run 50% %trail15") ──────────────────
        // At the RSI>=25 flip, close 50% and let 50% ride a 15%-below-peak %-trail
        // (floored at BE). Certified FREE +50% Sharpe (1.92->2.87) + WR (51->61.5%) at
        // ~flat net & identical poolDD — backtest/rsirev_levers_bt.cpp "L2 run 50%
        // %trail15", re-proven live-path faithful (rsirev_live_path_parity_bt.cpp).
        c.rsirev_runner_frac  = 0.50;              // keep 50% as runner at the flip
        c.rsirev_runner_trail = 0.15;              // 15%-below-peak %-trail, floored BE
    }
    // NOTE: regime_gate_ma is intentionally NEVER set — NO 200DMA in crypto (hard rule).
    return c;
}

} } // namespace chimera::trend_roster
