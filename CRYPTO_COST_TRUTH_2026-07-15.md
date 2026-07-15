# CRYPTO COST TRUTH — authoritative, measured, permanent (2026-07-15m)

**Why this doc exists:** crypto execution cost has been re-calibrated *every session*
(20bp → 35bp → re-litigated again) because it was a **hand-set flat literal with no
measured basis**. That is the recurring waste. This doc is the permanent record and the
END of guessing: cost is now **measured from the real order book**, per coin, per size.

## Root cause of the recurrence
- Old basis: `CryptoCostLedger` flat `known_rt_bp = 20.0` (+ ad-hoc bumps to 35bp in
  backtests). No anchor → drifts → every session re-derives it → operator re-checks.
- Symptom on the live desk: companions that capture favourable MFE but exit ~flat
  (e.g. ETH mimic peak +0.52% → closed −0.1bp / −$0.13) — the cost/trail drag made visible.

## The fix (permanent, code-anchored — do NOT hand-edit cost literals again)
1. `include/core/DepthLiquidationModel.hpp` — walks the **real perp bookDepth ladder**
   (Binance-Vision daily dumps, ±1..5% cumulative bid/ask depth+notional, ~30s) to price a
   **full-quantity** sell/buy → depth-adjusted slippage p50/p95/p99 per coin per notional Q.
2. `include/core/CryptoCostLedger.hpp::safe_cost_bps()` — the single authoritative number:
   `buy_fee + sell_fee + p99_entry_slip + p99_exit_slip + spread + latency + dust`.
3. Measurement tool: `backtest/depth_cost_model_bt.cpp`. Re-validation harness:
   `Crypto/backtest/core_trigger_p2_bt.cpp` (per-trade depth cost at entry/exit timestamps).

## MEASURED cost per coin (366 days, 2025-05..2026-05, incl. the Oct-2025 crash) — perp-book proxy
`safe_cost_bps = fee20 + p99_entry_slip + p99_exit_slip + reserve8`

| coin | $5k | $25k | $100k | $500k | $2M | note |
|---|---|---|---|---|---|---|
| **ETH** | 28.0 | 28.1 | 28.2 | 29.1 | 33.1 | deep; flat-35 was PESSIMISTIC at every size |
| **XRP** | 28.1 | 28.3 | 30.1 | 41.2 | 93.6 | thin — blows out with size; cap notional ~$100k |
| BTC | — | — | 28.1 | — | — | deepest book; cost ≈ ETH |
| SOL | — | — | 28.7 | — | — | deep; cost ≈ ETH |
| BNB | — | — | 30.1 | — | — | ≈ XRP-at-100k |
| DOGE | — | — | 30.1 | — | — | ≈ XRP-at-100k |
| AVAX | — | — | 35.9 | — | — | thin — cost > flat-35 already at $100k |
| LINK | — | — | 35.0 | — | — | thin — cost = flat-35 at $100k |

All 8 measured 2026-07-15o (AVAX/LINK depth backfilled). The 6 bottom coins are all
**within-null** on the CORE trigger regardless of cost — cheaper cost cannot rescue a
non-edge, and AVAX/LINK are already ≥ the flat-35 assumption at $100k. Confirms ETH+XRP
as the only tradeable universe.

**Design-Q answer:** a shared ETH+XRP campaign must cap notional **~$100k** to keep both
coins ≤ ~30bp. ETH scales much larger; XRP is the binding constraint.

## GATE — what each coin must pass to be tradeable
Long-only spot · omit 2022 (data starts 2025) · CORE trigger cell short_thr 0.64 / trail 240 / Q$100k:
1. `net > 0` after **measured** cost
2. `PF ≥ 1.3`
3. both walk-forward halves `> 0` (WF-H1 & WF-H2)
4. **2×-cost** re-run still `net > 0 & PF ≥ 1.3`
5. **structural**: real net beats the random-entry-in-uptrend null at **≥ 95th %ile** (randz)

## RESULT — rerun under measured cost (2026-07-15m). Passing universe = ETH + XRP (unchanged)

| coin | n | net_bp | PF | WF-H1/H2 | 2×-cost | randz %ile | VERDICT |
|---|---|---|---|---|---|---|---|
| **ETH** | 13 | **+2120** | 3.71 | +1150/+969 | PASS | **100%** | **✅ TRADEABLE** |
| **XRP** | 11 | **+2005** | 5.51 | +719/+1287 | PASS | **97%** | **✅ TRADEABLE** |
| BNB | 34 | +497 | 1.19 | +638/−141 | fail | 93% | ❌ (PF<1.3, WF-H2<0, 2×, null) |
| LINK | 12 | +329 | 1.37 | +70/+259 | fail(−31) | 91% | ❌ (2×-cost, null) |
| BTC | 21 | −1072 | 0.40 | −396/−676 | fail | 6% | ❌ within-null |
| SOL | 13 | −796 | 0.30 | −408/−387 | fail | 22% | ❌ within-null |
| DOGE | 15 | −717 | 0.33 | −186/−530 | fail | 34% | ❌ within-null |
| AVAX | 9 | −588 | 0.17 | −302/−287 | fail | 21% | ❌ within-null |

**The measured cost CONFIRMED ETH+XRP — it did not add or remove any coin.** BNB/LINK are the
near-misses (91-93%ile, PF just over 1.0) but fail the 2×-cost and structural bars. The six
failers are structurally within-null (buy-any-uptrend + wide-trail loses the same) — cheaper
cost cannot rescue a non-edge, and their real depth cost can only be ≥ ETH's.

## Standing rule (added to memory `feedback-crypto-cost-authoritative-depth-model`)
Before any crypto cost claim: **run/read the depth model** for that coin+size. Never quote
20/35 from memory. This is the anchor that stops the recalibration loop.
