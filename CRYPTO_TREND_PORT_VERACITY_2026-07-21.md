# Crypto Trend Book — PORT + VERACITY through ChimeraCrypto's OWN engine (2026-07-21)

**Branch:** `crypto-port-trend-book` (base `1456a97`, isolated Mac worktree). **NOT** pushed / merged /
deployed. Live box (chimera-direct / josgp1) **untouched**; no git-reset; nothing armed live; all SHADOW.
Consulted `Memory-Chimera/index.md` + `CLAUDE.md` (mandatory) — [[DirectionalTrendRosterOOS]],
[[CryptoLiveOnlyRebuild]], [[BtcRegimeMomentumBook]], no-200DMA hard rule.

Target book: the verified 19-leg roster from `/Users/jo/Crypto` research
(`CRYPTO_RECOMMENDED_BOOK_AND_SIZING` / `CRYPTO_BOOK_RETEST_VERIFY_2026-07-21.md`) — blended **OOS
Sharpe 1.71**, CAGR 12% OOS, full-sample maxDD ~12%, vt-0.020, RECENT ≈ breakeven, no 200DMA, long-only spot.

---

## TASK 1 — PORT (shadow-first)

### Correction to the premise (honest): the engines were NOT all present
The assessment said "ChimeraCrypto already has EMAx/Keltner/Regime/Roc/IBS/TSMom." **False.** The live
`chimera::EdgeEngine` `StrategyKind` enum had **TSMOM, KELTNER_BREAK, REGIME_SWITCH** but **NO standalone
EMAx, Roc, or IBS**. The **workhorse EMAx (~55% of book P&L) did not exist in the live binary.** IBS existed
only as a sub-branch inside REGIME_SWITCH's chop leg, not as a tradeable kind.

### What was wired (this branch)
1. **Ported 3 missing engine kinds** into `include/core/EdgeEngine.hpp`, research-faithful (verbatim signal
   logic copied from `Crypto/src/ibkrcrypto_bt.cpp`), long-only, `ride_to_flip` (exit on signal-flip, no stops):
   - `EMAX` — EMA(F)>EMA(S) with the research 4×p seed window; max_history auto-raised to 4×ema_slow+5.
   - `ROC` — N-bar %change > thr.
   - `IBS` — standalone (c−l)/(h−l) < lo.
   - Plus `strategy_name`/`is_trend_kind`/flip-out dispatch entries, and TSMOM ride-to-flip.
2. **`include/crypto/TrendRoster.hpp`** — the 19-leg roster as data + a `make_config()` factory: SHADOW,
   `ride_to_flip`, **NO 200DMA** (`regime_gate_ma` never set), daily bars, research-canonical params, corrected
   per-coin cost (BTC14 ETH28 SOL11 ADA18 XRP30 XLM40 GRT60 NDX4).
3. **`backtest/trend_roster_construct_test.cpp`** — **19/19 legs CONSTRUCT** in the real engine, shadow-first,
   ride_to_flip, no-200DMA (build-verified). 2 NDX index legs flagged: need a non-Binance index feed.

The 19 legs: BTC/ETH/SOL × {EMAX, KELTNER_BREAK, REGIME_SWITCH} + ADA KELTNER_BREAK + BTC/SOL ROC +
BTC/SOL IBS + NDX {TSMOM50, RSI_REVERT} + XRP/XLM/GRT KELTNER_BREAK.

### Build + boot gates (all GREEN)
- **Full build:** `cmake --build build --target chimera -j` → **GREEN** (0 errors, only benign `-Wunused`;
  binary produced). My EdgeEngine additions compile into the live binary.
- **SHADOW boot** of the built binary: `RUNTIME MODE = SHADOW` · `[MIMIC-FLOOR-GATE] 0 VIOLATION` ·
  `[PROFIT-LOCK-GATE] 0 VIOLATION` · `[REGISTRY] reconcile PASS` · `[BTC-REGIME-BOOK-GATE] warm=YES` ·
  0 fatal/terminate/abort · `CHIMERA READY`. My changes are **boot-safe**.

### What REMAINS (not done — deliberately, to respect the crash-loop / no-deploy safety rules)
- **main.cpp registration of the 19 legs** (construct + push to `g_slots`, mirroring the NEAR/THETA/SUSHI/
  ADA/DOT REGIME_SWITCH parents). Deferred: the registry `validate()` aborts on a declared-vs-wired
  mismatch (the `9a9b464` crash-loop) — this needs a full boot/deploy verification cycle, out of scope for a
  branch-only, box-untouched session. Until wired, connected_engines stays 5 (existing parents), not 24.
- **Coin price/CSV feed + seed wiring** for the roster legs (ADA/XRP/XLM/GRT have SYM_ ids; NDX needs an
  index feed).
- **A faithful-Keltner exit** (see veracity §Kelt) and **a vol-target $-pool layer** (see veracity §pool).

---

## TASK 2 — VERACITY (the trust-critical part)

Re-ran the backtest **through ChimeraCrypto's ACTUAL engine code** (`chimera::EdgeEngine`, driven bar-by-bar,
4 ticks/bar to rebuild OHLC, `ride_to_flip`, gates neutral, per-coin `round_trip_bp`) and compared to the
**verified research reference** run in-process (the EMAx/Kelt/Regime signal structs + `run_bt` copied
**verbatim** from `Crypto/src/ibkrcrypto_bt.cpp`, vt=0, carry=0, per-coin cost). Both net% are **additive
per-trade sums** (identical basis). Harness: `backtest/trend_roster_veracity_bt.cpp`. Data: the same
`/Users/jo/Crypto/backtest/data/{BTC,ETH,SOL}USDT_1d.csv`, FULL 2017–2026.

**The reference reproduces the published penny table EXACTLY** (BTC EMAx +574.76%, ETH +734.72%, SOL
+836.75% — matches `CRYPTO_BOOK_RETEST_VERIFY` to the penny → data path + reference validated).

| Coin | Engine | RESEARCH net% (verified) | ChimeraCrypto ENGINE net% | Δ (pp) | Penny? | Faithful? |
|---|---|---|---|---|---|---|
| BTC | EMAx(20,50)  | **+574.76%** | **+574.73%** | −0.03  | no (bit) | **YES — reproduces** |
| ETH | EMAx(20,50)  | **+734.72%** | **+734.81%** | +0.09  | no (bit) | **YES — reproduces** |
| SOL | EMAx(20,50)  | **+836.75%** | **+838.44%** | +1.69  | no (bit) | **YES — reproduces** |
| BTC | Regime       | +302.97% | +301.59% | −1.37 | no (bit) | **YES — reproduces** |
| ETH | Regime       | +125.91% | +124.53% | −1.38 | no (bit) | **YES — reproduces** |
| SOL | Regime       | +583.32% | +580.91% | −2.41 | no (bit) | **YES — reproduces** |
| BTC | Kelt(20,2.0) | +267.17% | +484.25% | **+217.08** | **NO** | **NO — different engine** |
| ETH | Kelt(20,2.0) | +160.81% | +1113.40% | **+952.59** | **NO** | **NO — different engine** |
| SOL | Kelt(20,2.0) | +315.48% | +1775.43% | **+1459.96** | **NO** | **NO — different engine** |

### Verdict — penny-match: **NO (strict); but nuanced and mostly honest-positive**
- **EMAx (the workhorse) and Regime ARE faithfully reproduced by ChimeraCrypto's engine.** Net% matches to
  **−0.03 / +0.09 pp on BTC/ETH EMAx** and within **1.4–2.4 pp** on Regime. **Not bit-penny** because the live
  engine fills at the **signal bar's close** while the validated harness fills at the **next bar's open** — a
  benign difference on daily crypto bars (open ≈ prior close), which accumulates a few pp over hundreds of
  Regime trades. Signal logic is identical (verified by code + by result). **The workhorse engine in the live
  binary IS the validated one** (once the EMAx port above lands).
- **KELTNER_BREAK does NOT reproduce the validated Keltner** — off by +217 to +1460 pp. **Root cause (code-
  confirmed, not introduced by me):** ChimeraCrypto's `KELTNER_BREAK` (S-2026-07-12) rides until price falls
  through the **LOWER** band, whereas the validated research `Kelt` exits when price falls back **INSIDE the
  upper band** (`want→0`). Different exit → completely different (here, far larger) trades. Its header comment
  "folds the Mac ibkrcrypto Kelt(20,2.0)" is **inaccurate** for the exit. **The 6 Keltner legs (4,5,6,10,17,18,
  19 — a third of the book) are NOT the validated engine** until a faithful-Keltner exit is added.
- **The blended vt-0.020 pool figures (OOS Sharpe 1.71 / CAGR 12% / maxDD ~12%) are NOT reproducible in
  ChimeraCrypto at all.** `chimera::EdgeEngine` has **no vol-target sizing and no $-pool blend** — those are
  portfolio-construction properties that live only in the research pool harness (`crypto_oos_pool.py`).
  Reproducing the headline Sharpe requires porting a vol-target pool layer, which does not exist in the binary.

### Honest bottom line
The verified numbers were produced ENTIRELY by the `/Users/jo/Crypto` C++↔Python research pair; ChimeraCrypto
was never in that validation loop. This session proves: **(a)** the workhorse EMAx (now ported) and the
existing Regime engine reproduce the validated per-leg net% to within a fraction of a pp (fill-basis aside);
**(b)** the existing KELTNER_BREAK is a *different* engine from the validated Keltner and must be fixed before
the Keltner legs can be trusted; **(c)** the headline blended Sharpe cannot be reproduced without a vol-target
pool layer the binary lacks. Nothing here was papered over. No push / merge / deploy; box untouched.

*Harnesses: `backtest/trend_roster_veracity_bt.cpp`, `backtest/trend_roster_construct_test.cpp`. Ported engine:
`include/core/EdgeEngine.hpp` (EMAX/ROC/IBS). Roster: `include/crypto/TrendRoster.hpp`.*
