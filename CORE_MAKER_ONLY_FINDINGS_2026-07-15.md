# CORE maker-only re-backtest — findings (2026-07-15p, resolves the activation FORK)

**Question (operator):** "surely we are maker only trading long and spot" — does the ETH+XRP CORE
edge (validated under TAKER economics, real depth cost 28bp) hold if the desk trades **maker-only**?

**Harness:** `Crypto/backtest/core_trigger_maker_bt.cpp` — a fill-realism variant of the validated
`core_trigger_p2_bt.cpp`, variable-for-variable identical trigger/gates/exit/depth-cost. The ONLY
change is entry fill: a resting post-only bid (0 entry slip) that fills **only if price revisits it**
within `CT_MAKER_WAIT` bars, else the setup is a **MISS** (no trade). Exit stays TAKER (MARKET) —
matches the live `execute()` path even in a maker world (maker exits won't fill in a fast reversal).

Config = the validated passing cell: short_thr 0.64 / trail 240 / Q$100k / real perp-depth cost.

## Results

| mode | placement | ETH | XRP | verdict |
|---|---|---|---|---|
| 0 TAKER (self-check) | fill @ signal close | n13 net **+2120** PF3.71 WF+1150/+969 2×+1755 | n11 net **+2005** PF5.51 WF+719/+1287 2×+1691 | reproduces validated numbers EXACTLY → harness faithful |
| 1 MAKER-chase | resting bid @ rebreak close | n13 miss0 net **+2120** PF3.71 2×+1756 | n11 miss0 net **+2009** PF5.53 2×+1698 | edge holds, ~identical to taker |
| 2 MAKER-retest | resting bid @ breakout level (rhi) | **n0 miss14** (no trades) | **n1 miss10** net−25 | edge FAILS — passive placement captures nothing |

- **Mode 1 fills trivially (miss=0 even at wait=1):** the rebreak bar closes at `b.c[i]`; on 15m the very
  next bar's low almost always dips back to that close, so a bid resting **at** the close fills next-bar
  essentially always. This is the OPTIMISTIC bound — it is basically a taker fill at the same price.
- **Mode 2 (truly passive, bid at the old range-high retest) captures ~nothing** (0–1 fills, 10–14 misses):
  by the time the rebreak + gates fire, price is well above rhi and rarely retraces there within the window.
  This is the honest "passive-pullback" placement the handoff flagged — and it confirms the breakout
  re-acceleration entry does **not** suit a patient resting bid.
- **Cost saving from maker is negligible at this size:** ETH/XRP books are deep, so the entry depth-slip
  saved by resting vs crossing is ~0 (XRP +2005 → +2009; ETH unchanged). The only real maker upside is the
  **fee** (fee_rt 20→15 → +65/+55bp), and that is an **account-tier (BNB/VIP) lever**, independent of the
  strategy — it applies to the taker path too via BNB fee discount.
- Null (randz 300, maker-chase): ETH 100%ile, XRP 97%ile — edge remains structural.

## Conclusion → recommend FORK option (1): activate as validated (taker-RT, 28bp)

Maker-only offers **no material benefit** for this strategy at this size and adds **winner-miss risk**:
- The only maker placement that fills (at the touch/rebreak close) is economically ~identical to the taker
  entry — same price, ~0 entry-slip saving on deep coins, no PnL improvement.
- The only placement that would save more (deep passive) **misses ~every trade** (mode 2 → 0 book).
- The fee saving the operator is chasing is a tier lever (turn on BNB discount / climb VIP), not a reason
  to redesign the entry to maker-only.

So: **ship the validated taker path** (marketable-limit entry + MARKET exit, 28bp). If the operator wants
the fee saving, capture it via account tier / BNB discount on the SAME taker entry — do not switch to a
passive resting bid, which trades a negligible saving for a real risk of missing the runaway winners that
carry this thin (7–18/yr) book.

Repro: `g++ -O2 -std=c++17 -o /tmp/ct_maker backtest/core_trigger_maker_bt.cpp` then
`CT_MAKER_MODE={0,1,2} [CT_MAKER_WAIT=n] [CP_FEE_RT=15] /tmp/ct_maker`.
