# Engine Promotion Gate — TRUE Held-Out Walk-Forward

Effective 2026-05-29 (S43). Applies to every engine before it can ship live.

## Background

S38–S42 cohorts (~500 engines added over 24h) used a "4-window" gate that was
actually **4 nested in-sample lookbacks** (134/180/365/730d) all ending at
"now." This is not walk-forward — the optimizer saw every bar it was later
scored on. The gate produced wave after wave of survivorship-biased engines
that ate live drawdowns once regime turned (TSMOM whipsaw on alts).

Cull executed 2026-05-29: 278 untested engines removed, 137 repromoted as S43
after passing the gate below. 124 SKIPPED because their symbol launched after
the holdout window — they cannot be WF-validated until history accumulates.

## Required protocol

Every engine config (every new `chimera::EdgeEngine::Config` block) must, before
its `wire_engine(...)` call lands in `main.cpp`, pass:

```
./backtest/backtest --roster <csv> --preset prod_tiered_pyramid_xlow \
    --last-days 365 --end-days-ago 1095     # IS  window: [-1460, -1095]

./backtest/backtest --roster <csv> --preset prod_tiered_pyramid_xlow \
    --last-days 365 --end-days-ago 730      # OOS window: [-1095,  -730]
```

The OOS window's end (`-730d`) must precede any sweep / discovery run's data
cutoff that produced this engine. If a future discover sweep extends to a more
recent cutoff, push both windows earlier accordingly.

### Pass criteria
- IS PF ≥ 1.3 AND OOS PF ≥ 1.3
- IS trades ≥ 20 AND OOS trades ≥ 20
- IS net bp > 0 AND OOS net bp > 0
- IS and OOS records NOT identical (guards against data underfill on
  recently-launched symbols)
- PF == 99.9 (no-loss sentinel) rejected unless n ≥ 50

### Conditional / fail
- 1.1 ≤ PF < 1.3 in either window → CONDITIONAL (stays culled)
- Anything else → FAIL (stays culled)
- Symbol launched after `-1460d` → SKIP_NO_DATA (cannot validate, stays culled)

## Tooling

- `backtest/backtest_harness.cpp` accepts `--end-days-ago N` to trim recent N
  days from kline data before splitting seed/OOS.
- Cull workflow scripts: `/tmp/build_cull_roster.py`, `/tmp/merge_holdout.py`,
  `/tmp/build_s43.py` (one-shot for the May 2026 cull; reuse pattern for
  future cohorts).

## No exceptions

If a cohort cannot pass this gate, it does not go live. Symbol-too-new is not
a license to deploy untested — it is a deferral until enough data exists for
honest walk-forward.
