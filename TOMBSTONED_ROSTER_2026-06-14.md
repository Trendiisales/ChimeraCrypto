# FULL ROSTER TOMBSTONE — 2026-06-14

All 191 live engines culled. The book runs 0 engines. Start over.

## Why
The standalone harness AND the live ranking were OPTIMISTIC: on a long-only book
they handed longs the favorable intra-bar price on DOWN bars (High-before-Low),
manufacturing wins. Fixed to honest SL-first fills (commit 95f455c). Then an
independent full re-validation of all 283 compiled configs on real Binance H1
(2021-2026), using the honest fills:

| test                                  | viable |
|---------------------------------------|--------|
| honest fills, full history            | 0 / 283 |
| OLD optimistic fills, full history    | 0 / 252 |
| honest fills, CLEAN BULL (2023-24)    | 0 / 247 |

Not one engine is net-positive in any regime under honest fills. The old
engine_rankings.csv (e.g. ARB-BOLL20k25-H1 STRONG PF 4.40 -> honest PF 0.02 /
-119k bp over 822 trades) was fill-optimism + cherry-picked walk-forward OOS
windows. Root cause: high-frequency BOLL/TSMOM cells (700-2400 trades) cannot
overcome the ~20bp round-trip cost; there is no per-trade edge.

## Rule going forward
DO NOT re-wire any engine off the old rankings. Every new engine MUST pass the
honest validator before wiring:
  backtest/revalidate_all.cpp   (SL-first conservative fills; reuses run_backtest)
Per-engine honest numbers: backtest/honest_revalidation_2026-06-14.csv
Re-enable = remove tag from config/culled_engines.txt, but ONLY with honest proof.
