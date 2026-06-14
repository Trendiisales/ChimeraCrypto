# Data Integrity Protocol — MANDATORY before any Chimera backtest/search

## Why this exists — the 2026-06-14 data disaster

A multi-day "nothing works, kill everything" conclusion was built on **garbage data**.
The minute CSVs in `data/klines_spot/` and `data/bull_min/` had been produced by
different fetch runs with **different date ranges**:

- `data/klines_spot/BTCUSDT_1m.csv` covered **2025-05 → 2026-05 only** (12 months)
- `data/bull_min/` covered **2024**
- alt files were freshly re-fetched, **mixed vintages**

The search harness assumed multi-year coverage and bucketed trades by calendar
year. Every "cross-regime" / "bull vs bear" number was meaningless — windows that
had **zero bars** were scored as real results. The apparent edge (and its apparent
absence) were both artifacts. Root cause: **nothing checked span, consistency, or
provenance before backtesting.**

## The three guards (all mandatory)

### 1. Front gate — `tools/validate_dataset.py`
Run on the dataset directory BEFORE any backtest. Exit 0 = safe; non-zero = STOP.

```bash
python3 tools/validate_dataset.py data/multiyr --require-cycles 2021,2022,2023,2024
```

Per file: monotonic timestamps, no dupes, all prices > 0, OHLC sanity
(`h>=max(o,c)`, `l<=min(o,c)`, `h>=l`), modal interval detection, gap audit.
Dataset-level:
- **CONSISTENCY** — every file shares one bar interval (no mixed TF). FAIL if not.
- **VINTAGE** — every file ends on the same date (±tol). A file ending early =
  stale/delisted (e.g. MATIC→POL ended 2024-09). **FAIL** — this is the disaster
  signature. Late *start* dates are fine (coins list on different days) — WARN only,
  because `--require-cycles` guarantees the windows that matter are covered.
- Writes `MANIFEST.json` (per-file sha + span + bars + interval) so a later run can
  prove the files are the same ones that were validated.

### 2. Harness load-guard — `backtest/data_guard.hpp`
Every backtest/search MUST load through `guard::load_guarded(path, sym, expect_interval_ms)`.
It **aborts the process** on out-of-order/duplicate timestamps, non-positive or
OHLC-invalid bars, or an interval that disagrees with what the harness expects
(wrong-TF file). Then call `guard::assert_windows(...)` once: it aborts if any
window the harness will score has fewer than `min_bars` — so a window with no data
can **never** be silently counted again. Belt-and-braces with the Python gate.

### 3. Provenance — fetch on ONE consistent range
The fetcher (`tools/fetch_multiyr.py` pattern) pulls every symbol over the **same**
`[START, NOW]` range at the **same** interval, in one run. Never mix files from
different fetch runs into one dataset directory. If you add a symbol later, refetch
the whole directory or run the validator and confirm `end spread = 0`.

## Standard pre-backtest checklist

1. `python3 tools/validate_dataset.py <datadir> --require-cycles <years>` → must exit 0.
2. Harness loads via `guard::load_guarded(...)` with the correct `expect_interval_ms`.
3. Harness calls `guard::assert_windows(...)` for every window it will score.
4. `MANIFEST.json` present and `consistent: true`.
5. Only then run the backtest. Quote the dataset span + sha in any result you report.

## Known-good dataset (as of 2026-06-14)
`data/multiyr/` — 18 symbols, 15m, 2020-01-01 → 2026-06-14, all cover 2021-2024,
end-spread 0d, validated. Rejected: `data/_rejected/MATICUSDT_15m.csv` (stale, ends
2024-09, delisted as MATIC→POL).
