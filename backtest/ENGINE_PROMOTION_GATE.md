# Engine Promotion Gate — TRUE Held-Out Walk-Forward

Effective 2026-05-29 (S43 / S44 / S44b). Applies to every engine before it can
ship live.

## Background

S38–S42 cohorts (~500 engines added over 24h) used a "4-window" gate that was
actually **4 nested in-sample lookbacks** (134/180/365/730d) all ending at
"now." Not walk-forward — the optimizer saw every bar it was later scored on.
Wave after wave of survivorship-biased engines ate live drawdowns once regime
turned.

Cull executed 2026-05-29: 278 untested S41/S42 engines removed. 137 repromoted
as S43 after passing the strict held-out gate. Re-discovery sweep produced 142
S43b held-out engines for the new-token cohort. S44/S44b tightened risk overlay
application and added TF-aware sample-size floors.

## Required protocol

Every engine config must pass before its `wire_engine(...)` call lands in
`main.cpp`:

```
./backtest/backtest --roster <csv> --preset prod_tiered_pyramid_elite \
    --last-days 365 --end-days-ago 1095     # IS  window: [-1460, -1095]

./backtest/backtest --roster <csv> --preset prod_tiered_pyramid_elite \
    --last-days 365 --end-days-ago 730      # OOS window: [-1095,  -730]
```

The OOS window's end must precede any sweep / discovery run's data cutoff that
produced this engine. If a future discover sweep extends to a more recent
cutoff, push both windows earlier accordingly.

For symbols launched after the strict holdout window (cannot true-WF-validate),
use the alternative protocol:

```
./backtest/backtest --discover --preset prod_tiered_pyramid_elite \
    --last-days 365 --end-days-ago 180    # discover blind to last 180d

# Top per (sym,kind,tf) by IS PF -> roster CSV -> validate on last 180d
./backtest/backtest --roster <picked>.csv --preset prod_tiered_pyramid_elite \
    --last-days 180                       # OOS = true forward holdout
```

### Pass criteria (TF-aware n floor — S44b)

OOS PF, OOS Sharpe, IS PF gates per tier:

| Tier      | OOS PF  | OOS Sharpe | bp/DD  | IS PF  | n floor (varies by TF) |
|-----------|---------|------------|--------|--------|------------------------|
| ELITE     | ≥ 2.0   | ≥ 4.0      | ≥ 8    | ≥ 1.5  | max(100, 2× base)      |
| STRONG    | ≥ 1.7   | ≥ 3.0      | ≥ 5    | ≥ 1.3  | max(50, 1.5× base)     |
| STANDARD  | ≥ 1.3   | ≥ 2.0      | ≥ 2    | ≥ 1.3  | base                   |
| MARGINAL  | ≥ 1.0   | —          | —      | —      | (do not deploy)        |

**Base n by timeframe** (180d window):

| TF | Bars in 180d | base_n |
|----|--------------|--------|
| H1 | 4320 | 50 |
| H2 | 2160 | 40 |
| H3 | 1440 | 35 |
| H4 | 1080 | 30 |
| H6 | 720  | 25 |
| H8 | 540  | 20 |
| H12 | 360 | 15 |
| H16 | 270 | 12 |
| D1 | 180  | 10 |
| D2 | 90   | 8  |
| D3 | 60   | 6  |

Rationale: slow-TF engines (H12/D1) cannot structurally hit n>=30 in a 180d
OOS window after warmup + hold-period limits. Forcing a uniform floor kills
legitimate slow edges (ETH-ICHI-H12 was wrongly culled by uniform n>=30 with
OOS_n=27, restored under TF-aware H12 floor of 15).

### Mandatory sanity guards

- Reject PF == 99.9 (no-loss sentinel) unless n ≥ 50.
- Reject if IS records EXACTLY match OOS records (data underfill on
  recently-launched symbols).
- Reject if OOS_PF < 0.5 × IS_PF (edge collapse).

## Risk overlay requirement (S44b)

Every engine wired via `wire_engine()` MUST receive the full overlay stack:

```c++
engine.enable_pyramid_elite();           // arm 0.5 ATR, step 0.3, mult 0.75, max 4
engine.apply_safety_preset();            // staged BE-lock; destructive layers off
engine.enable_volume_gate(true);
if (engine.is_trend_following()) {
    engine.enable_adx_filter(true);
    engine.set_adx_threshold(25.0);
} else {
    engine.enable_vol_filter(true);
    if (engine.cfg().tf_secs < 86400) engine.enable_mtf_gate(true);
}
if (engine.cfg().symbol != "btcusdt") engine.enable_corr_filter(true);
```

This is applied INSIDE the `wire_engine` lambda in `main.cpp` (line ~1471), so
every wired engine — `g_slots` or `#include`'d cohort — gets it uniformly.
Prior to S44b, only `g_slots` engines received these overlays; `#include`'d
S38-S43b cohorts traded raw.

## Decay monitoring

`backtest/decay_monitor.sh` runs weekly via cron (Sundays 03:00 UTC). Snapshot
each engine's rolling 60d OOS PF, diff against prior week, alert on:

- `DECAY` — current PF < 1.1 with n ≥ 20 (real signal of edge loss)
- `DROP`  — week-over-week PF dropped > 40% AND current PF < 1.5

Alerts written to `data/decay/cron.log`. Snapshots persist in `data/decay/`.

## Tooling

- `backtest/backtest_harness.cpp` — `--end-days-ago N` trims last N days
  before the seed/OOS split. `--preset prod_tiered_pyramid_elite` for the
  current pyramid params.
- `/tmp/cull_v3.py` — comments `wire_engine()` + `g_slots.push_back()` for
  every tag in a cull list. Handles both standard `_cfg` naming AND legacy
  naming where the var name differs from the cfg name (via two-step
  var→cfg→tag chain).
- `/tmp/tf_aware_tier.py` — re-classifies every live engine with TF-aware
  floors. Run after every decay-monitor week to spot regime drift.
- `/tmp/build_cull_roster.py`, `/tmp/merge_holdout.py`, `/tmp/build_s43.py`
  — original S43 cull workflow.
- `/tmp/extract_untested.py` — extracts strategy-specific fields
  (`ichi_*_period`, `keltner_*`) into roster CSV. Harness reads these via
  S44 patch to the roster CSV reader.

## No exceptions

- A cohort that cannot pass this gate does not go live.
- Symbol-too-new for true holdout is not a license to deploy untested — it
  is a deferral until enough data exists for honest walk-forward.
- The `enable_pyramid_elite` + safety overlay set is non-negotiable.
- `git checkout` between successive edits has bitten this protocol twice
  in one session; always verify file state against intended diff before
  committing.
