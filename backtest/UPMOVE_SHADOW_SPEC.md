# UpMoveTrail shadow cell — spec

WF-confirmed up-move / hard-trail engine with rolling trailing-roster cull.
Status: **SHADOW ONLY** (logs target signal, places no orders). Forward track
record owed before any live size. Origin: operator session 2026-06-16.
Vault: Memory-Chimera `UpMoveTrailLossMitigation`.

## Engine (validated config — do not retune without a fresh WF)
| part | rule |
|---|---|
| entry trend | own-coin golden: EMA50 > EMA200 AND close > EMA50 |
| entry trigger | Donchian-24 breakout: close > prior 24-bar high |
| entry cost gate | ATR(24) >= 3 x round-trip cost (17bp) — only fire when move clears fees |
| exit trail | chandelier 5 x ATR (max_close − 5·ATR) — WIDE, let winners run |
| exit hard stop | 3 x ATR below entry — catches rare disaster, not noise |
| exit time stop | exit if still underwater after 48 bars (~2 days) — PATIENT, not twitchy |
| regime gate | BTC daily dual_50_200 (close>50d AND (close>200d OR 200d rising)) → whole book flat in bear |
| roster | justification-gated: USE a coin only if trailing-180d engine net >= +5 over >=8 trades; else it SITS OUT (not banned) until its record justifies it. No name-list, symmetric, re-eval each run. |

Timeframe 1h. Long-only spot. Cost 17bp round-trip.

## Why each lever (evidence)
- Wide trail + patient cut = min-loss frontier; tight stop/BE DESTROY edge
  (BE flips bull +1160→−1015; proven on 2 independent bears).
- "Exit on negative" is dead 3 ways (PnL stop, BE, structural). Wide trail IS
  "exit on reversal" done right.
- Roster: real-time trailing cull captures +625 OOS (vs +460 all-15, +700 oracle
  needs hindsight). NOT survivorship — decidable on trailing data only.
- Regime gate is the primary loss-negator (skips whole bear regime); exit logic
  + roster minimise what leaks when the gate is late.

## Confirmed numbers (basket, cost-incl, sum of per-trade net%)
| window | all-15 | drop LTC | rolling-roster |
|---|---|---|---|
| TRAIN 2021-23 | +2216 | +2219 | — |
| OOS 2024-25 | +460 | +604 | **+625** |
| 2022 bear | −166 | −165 | **−3** |
| 2026 bear (−28.6%) | −28 | −25 | **+7** |
- Curve-fit gate PASSED: Spearman(train,test)=0.865; plateau 18/18; recommended
  config ranks #1/72 on blind OOS; identical config ranking on 2 separate bears.
- Caveat: breadth ~10/15 even after roster; only 2 bears in all of crypto history.

## Files
- `upmove_shadow_cell.py` — runnable shadow logger (signal_state shared bt/live)
- `upmove_trail*.py`, `upmove_minloss_sweep.py`, `upmove_wf_confirm.py` — research harness
- `upmove_shadow_log.jsonl` — daily shadow targets (append-only)

## Deploy path (when operator says go-live shadow)
1. Port `signal_state` + roster + regime fns into `sleeves/chimera_sleeves.py` as
   a new sleeve (same code path as backtest — already structured that way).
2. Wire to the daily 00:30 UTC cron after `update_data`+`validate` (data-veracity gate).
3. Shadow-log target weights only; NO executor. Going live = separate explicit step.
4. Monthly: roster auto re-evaluates from trailing-180d engine net.
