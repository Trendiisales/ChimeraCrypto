# BtcRegimeMomentumBook — LIVE ENGINE PARITY (independently reproduced, S-2026-07-20af)

**Trust gate.** The parity artifact in the Crypto repo
(`backtest/btc_trendcore_2026-07-20/parity_cpp_vs_python.txt`) claimed the live
book matches the python cert — but no harness on disk produced it. This session
WROTE the harness (`btc_regime_book_parity_bt.cpp`, drives the actual live
`BtcRegimeMomentumBook::on_h1_bar()` engine, `-DBTC_BOOK_STANDALONE`) and RAN it
against the exact cert CSVs. Result: **EXACT reproduction, per sleeve, both
datasets.** The live engine IS the certified engine.

## Reproduced numbers (harness output vs python cert target)

| cell | live engine (harness) | python cert | verdict |
|---|---|---|---|
| TRENDCORE orig 2021+ | n=21 +6813bp PF1.69 | n=21 +6813bp PF1.69 | EXACT |
| TSMOM30 orig 2021+   | n=91 +15475bp PF1.71 | n=91 +15475bp PF1.71 | EXACT |
| TRENDCORE ext 2017+  | n=37 +18824bp PF2.14 | n=37 +18824bp PF2.14 | EXACT |
| TSMOM30 ext 2017+    | n=134 +70931bp PF2.84 | n=134 +70931bp PF2.84 | EXACT |

## Pooled — two conventions, reconciled by arithmetic (no discrepancy)

- **POOLED add** (harness live behavior): orig +22287bp / ext +89755bp — both
  sleeves at FULL clip. This is how the LIVE book behaves: each sleeve submits
  its own `order_usd/px`, so both-long = 2 independent clips = 2× exposure.
- **POOLED 50/50** (cert artifact convention): orig +11144bp / ext +44878bp —
  half capital each. `(6813+15475)/2 = 11144` and `(18824+70931)/2 = 44878`,
  both exact. PF is identical (1.70 / 2.63) because scale cancels.

**LIVE-EXPOSURE HONESTY:** the certified headline "+11144bp PF1.70" is the 50/50
(half-capital) framing. In production the two sleeves are additive independent
clips — peak gross exposure = 2× the pilot `order_usd` when both are long. Both
sleeves are certified net-positive standalone after cost (both WF halves, both
regimes), so the additive book is sound; size the pilot with 2× peak in mind.

## Honest edge framing (from the cert, preserved)
Edge = REGIME + TRAIL + MOMENTUM (+ BTC bull beta), NOT breakout entry timing.
The TRENDCORE breakout trigger sits at the 59th percentile of the random-entry
regime null on full history — indistinguishable from a random entry inside the
same regime under identical management. TRENDCORE alone fails concentration/folds
on extended history; the two sleeves certify only as the ensemble. TSMOM30 is the
workhorse of the pooled net. This is NOT a magic-entry book.

## Build / status
- ChimeraCrypto incremental build GREEN with the book wired (main.cpp: include,
  static instance, live-config gate, seed_daily warmup, load, boot_summary,
  on_tick driver on btcusdt). LSP `-Iinclude` false-positives only.
- `live_enabled` gated on `RuntimeMode::LIVE`; parity/shadow route nothing.
- **NOT DEPLOYED.** Committed working-tree, HOLD for operator deploy order.
