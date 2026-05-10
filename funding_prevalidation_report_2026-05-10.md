# Chimera funding pre-validation report

**Generated:** 2026-05-10T09:48:54.422913+00:00
**Branch / commit:** tier1-risk-integration @ 06a14cc (session 8)
**Engines replayed:** FundingWindowEngine, FundingPersistenceFadeEngine
**Anti-overfitting:** zero per-symbol parameter tuning. All thresholds are verbatim from the BTC/ETH-tuned engine headers.

## 1. Funding-rate distribution per symbol (bp / 8h)

| Symbol | Events | First | Last | Min | P1 | Median | Mean | P99 | Max | % < 0 | % <= -3bp | % <= -10bp |
|---|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| BTCUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -1.52 |   -0.88 |   +0.37 |   +0.33 |   +1.00 |   +1.00 |  23.1% |  0.00% |  0.00% |
| ETHUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -3.65 |   -1.52 |   +0.36 |   +0.30 |   +1.00 |   +1.00 |  25.1% |  0.09% |  0.00% |
| SOLUSDT | 1095 | 2025-05-10 | 2026-05-10 |  -30.28 |   -5.31 |   +0.18 |   -0.04 |   +1.00 |   +2.59 |  41.0% |  1.64% |  0.46% |
| BNBUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -3.52 |   -2.02 |   +0.00 |   +0.08 |   +1.54 |   +3.29 |   5.9% |  0.27% |  0.00% |
| AVAXUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -6.63 |   -4.06 |   +0.35 |   +0.01 |   +1.00 |   +1.00 |  38.8% |  2.37% |  0.00% |
| LINKUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -3.01 |   -1.76 |   +0.65 |   +0.44 |   +1.00 |   +1.00 |  22.5% |  0.09% |  0.00% |
| XRPUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -4.42 |   -2.09 |   +0.32 |   +0.21 |   +1.00 |   +4.36 |  35.3% |  0.18% |  0.00% |
| DOGEUSDT | 1095 | 2025-05-10 | 2026-05-10 |   -2.60 |   -1.41 |   +0.43 |   +0.32 |   +1.00 |   +4.53 |  29.9% |  0.00% |  0.00% |

## 2. FundingWindowEngine signal frequency (rate gate, 365 d)

Entry gate (rate component only): `|funding_rate| >= 1.5 bp/8h` (constant `RATE_THRESHOLD = 0.00015`; engine header comment claims 15 bp/8h but is off by 10x — see script docstring). This is an **upper bound** on real signal frequency — the engine additionally requires `|basis| >= 3 bp` and `secs_to_funding <= 180`, both of which can only reduce the count.

| Symbol | Engine? | Rate hits | Pos | Neg | After 4h cooldown | Approx hits / yr |
|---|---|---:|---:|---:|---:|---:|
| BTCUSDT | live | 1 | 0 | 1 | 1 | 1 |
| ETHUSDT | live | 12 | 0 | 12 | 12 | 12 |
| SOLUSDT | Step 3 candidate | 70 | 1 | 69 | 70 | 70 |
| BNBUSDT | Step 3 candidate | 32 | 11 | 21 | 32 | 32 |
| AVAXUSDT | Step 3 candidate | 119 | 0 | 119 | 119 | 119 |
| LINKUSDT | Step 3 candidate | 15 | 0 | 15 | 15 | 15 |
| XRPUSDT | Step 3 candidate | 36 | 4 | 32 | 36 | 36 |
| DOGEUSDT | Step 3 candidate | 9 | 1 | 8 | 9 | 9 |

## 3. FundingPersistenceFadeEngine signal frequency (full gate, 365 d)

Entry gate: `avg_24h <= -10 bp/8h` AND `max_8h <= -3 bp/8h` AND buffer span >= 23 h. Exit: `avg_24h >= 0` (funding revert). Cooldown: 3 days between entries. Discrete 8h-event approximation of the engine's per-minute live buffer (see script docstring for fidelity caveat).

| Symbol | Engine? | Entries | In-trigger events | % time in trigger | Longest streak | Min avg-24h observed |
|---|---|---:|---:|---:|---:|---:|
| BTCUSDT | live | 0 | 0 |  0.00% | 0 |  -1.41 bp |
| ETHUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -1.49 bp |
| SOLUSDT | Step 3 candidate | 1 | 5 |  0.46% | 5 | -16.88 bp |
| BNBUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -1.78 bp |
| AVAXUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -4.86 bp |
| LINKUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -1.01 bp |
| XRPUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -3.06 bp |
| DOGEUSDT | Step 3 candidate | 0 | 0 |  0.00% | 0 |  -1.84 bp |

## 4. Verdict

- **FundingWindow rate-gate hits across all 8 symbols / 365 d:** 294. Across the 6 alts only: 281.
- **FundingPersistenceFade entries across all 8 symbols / 365 d:** 1. Across the alts only: 1.

### FundingWindowEngine

Step 3 is **justified** for FundingWindow. The 1.5 bp/8h rate gate fires meaningfully often on the alts in the current regime — meaningfully more than on BTC/ETH where the engine is currently live. Recommended path:

  1. Extend FundingWindow to the 6-alt basket using inherited thresholds (no per-symbol tuning — anti-overfitting). Touches engine instantiation in `src/main.cpp` and PerpFeed WS subscriptions for the alt symbols.
  2. Deploy in `shadow_mode = true` with `Tier1Risk` `per_engine_r_cap[FUNDING_WINDOW] = 1.0` (already the default).
  3. Collect 4-8 weeks of forward-shadow telemetry per Step 4. Pass criterion at Step 5 review: net P&L > 0 after 15 bp round-trip cost on >= 30 trades per symbol.
  4. Caveat: the rate-only count is an upper bound. Real signal frequency depends on how often the basis dislocation (>=3 bp) coincides with the rate spike. Forward shadow data will reveal this directly without the need for historical perp/spot kline reconstruction.

### FundingPersistenceFadeEngine

FundingPersistenceFade is **structurally inert** in the current funding regime across the entire basket — `entries = 1` across 8 symbols × 365 days. The -10 bp 24h-avg trigger is mechanically unreachable: BTC's most-negative single funding event in the year was -1.52 bp (10x off), and no other symbol except SOL even comes close. The engine has not fired in production not because of a regime issue but because the threshold was set for a different funding world. Three actionable paths:

  1. **Retune** `FUNDING_TRIGGER` from -10 bp to -2 to -3 bp and `FUNDING_RECENT_MAX` from -3 bp to -1 bp, then forward-shadow. Caveat: small-threshold tuning increases false-trigger frequency; the engine's edge is supposed to be *persistent* extreme funding — at -2 bp it isn't extreme.
  2. **Strip the engine and reclaim the budget** in `Tier1Risk::per_engine_r_cap[FUNDING_PERSIST]`. Mirrors the OBI cap = 0R decision from session 7.
  3. **Defer until regime change.** Keep BTC live in `shadow_mode` against the original thresholds — costs nothing while quiet, auto-fires if 2021-style funding returns.

