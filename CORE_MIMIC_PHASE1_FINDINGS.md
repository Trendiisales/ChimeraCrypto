# CORE/MIMIC Phase 1 — CORE Edge Validation (KILL-EARLY GATE) — FINDINGS

**Date:** 2026-07-15l · **Harness:** `/Users/jo/Crypto/backtest/core_trigger_bt.cpp`
**Verdict: CONDITIONAL PASS — proceed. CORE archetype has a real, null-verified structural
edge on ETH + XRP. Dead on BTC and 5 other majors. Build the campaign on ETH+XRP, not BTC+ETH.**

## What was tested
Spec §3 CORE trigger, faithfully but crudely (Phase-1 simplifications documented in harness header):
- 15m bars aggregated from 1m spot klines, window 2025-05-10 .. 2026-05-10 (~1yr), ALL 8 coins
  with local 1m data (BTC ETH SOL BNB XRP DOGE AVAX LINK — "never half the symbols").
- Trigger: compression (6×15m bars, width ≤80bp) → breakout (close>range_hi WITH agg-buy share
  short≥thr AND medium≥0.58, from kline taker_buy_base) → pullback HOLDS above max(range_hi,
  anchored-VWAP), depth ≤0.5×impulse → RECLAIM (close > pre-pullback peak) → regime gate
  [trend up SMA16 & SMA64, price>anchored-VWAP, higher-low>breakout, BTC-regime positive,
  ATR-projected room ≥3×cost] → CORE BUY.
- Exit (CORE trend-ride): structural stop below the higher-low; adaptive giveback trail
  max(trailmin, atr×mult); lost-anchored-VWAP. net_bp = move − safe_cost.
- Cost: FLAT conservative safe_cost = 35bp (operator example 20+10+5). 2×-cost gate = 70bp.
- Gate: base(net>0 & PF≥1.3 & WF-H1>0 & WF-H2>0) AND 2×-cost(net>0 & PF≥1.3). Long-only.

## Result — passing universe = ETH + XRP (2 of 8)
Plateau = trend-ride trail ≥160bp × short_thr 0.58–0.64 (multiple adjacent PASS, not one cell).

| coin | cell A (st0.64/tm160) | cell B (st0.58/tm200) | randz vs null | verdict |
|---|---|---|---|---|
| **ETH** | n11 win55% net+844 PF2.62 WF+551/+293 2x-PASS | n15 win53% net+1102 PF2.36 WF+1049/+53 2x-PASS | **99th %ile** both | **PASS** |
| **XRP** | n7 win43% net+455 PF1.92 WF+360/+95 2x-PASS | n18 win28% net+1262 PF1.92 WF+338/+924 2x-PASS | **96–97th %ile** both | **PASS** |
| BTC | n17 net−821 PF0.50 | n29 net−989 PF0.59 | 33–50th %ile | fail (within-null) |
| SOL | net−623 PF0.37 | net−629 PF0.40 | 46th %ile | fail (within-null) |
| BNB | net−1028 PF0.49 | net−1587 PF0.51 | — | fail |
| DOGE | net−147 PF0.71 | net−176 PF0.79 | — | fail |
| AVAX | net−874 PF0.04 | net−484 PF0.51 | — | fail |
| LINK | net−84 PF0.76 | net−30 PF0.95 | — | fail |

## Why this is a real result, not a strawman or an overfit
1. **Not a strawman kill.** First config (tight 40bp trail) lost everywhere — but that trail is
   nonsensical vs a 35bp cost for a move meant to run 105bp+ (spec §7's own warning). Widening to
   a trend-ride trail (≥160bp) is what the mechanism requires; only then does the edge appear.
2. **Not an overfit cell.** ETH+XRP pass a contiguous PLATEAU (trail 160–240 × short_thr 0.58–0.64),
   on BOTH walk-forward halves, at 2×-cost — and pass on BOTH plateau cells (cross-config stable).
3. **Edge is STRUCTURAL (null-verified).** RANDZ null-test (500 seeds): enter at RANDOM regime-up
   bars, same count, IDENTICAL exit. ETH real net sits at the 99th %ile, XRP at 96–97th, of that
   null; null MEANS are NEGATIVE. So random "buy uptrend + wide trail" LOSES — the compression→
   breakout→pullback→reclaim STRUCTURE creates the edge, not the trend/trail. Failers (SOL) sit at
   ~46th %ile = indistinguishable from random.

## Honest weaknesses / caveats
- **Thin frequency.** ETH 11–15 trades/yr, XRP 7–18. Campaign frequency ≤ CORE frequency → a
  low-frequency book. Two passers help; more will come from (a) finer entry on the downloading
  tick tape and (b) the same-gate test re-run on more liquid alts.
- **Crude Phase-1 form.** 15m bars, flat 35bp cost, kline taker-buy proxy (not tick OFI), SMA
  trend proxy, ATR-room proxy. All refine in Phase 2 with the perp bookDepth + spot aggTrades
  now downloading. The edge should be RE-CONFIRMED (not assumed) under the finer model.
- **BTC-gate index alignment** across coins assumes identical 15m bar indexing (same span/bucketing);
  fine here, but verify if a coin has missing minutes.

## Decision & next
- **Proceed to Phase 2 with CORE instruments = ETH + XRP.** Exclude BTC + the 5 failers with the
  EXPLICIT stated reason (fail the gate + within-null) — same logic the spec applies to SOL/smaller
  ("disabled unless they independently pass the same gate"). Do NOT force BTC into the book.
- Phase 2: build the real dynamic depth-adjusted safe_cost (perp bookDepth) + tick OFI (spot
  aggTrades), then RE-VALIDATE this CORE edge under the finer cost/flow model before committing the
  campaign machinery. If it survives, build CORE 3-state cost machine (Phase 3) on ETH+XRP.
- Standing order "extend winning engines": once tick data lands, re-run the same-gate universe scan
  — the passing set may grow beyond ETH+XRP at finer resolution.
