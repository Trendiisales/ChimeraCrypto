# Crypto Trend Book — KELTNER FIX + VOL-TARGET POOL PORT + RE-VERIFY (2026-07-21)

**Branch:** `crypto-keltner-pool-fix` (base `de7e1b6` = `crypto-port-trend-book`, isolated Mac worktree).
**NOT** pushed / merged / deployed. Live box (chimera-direct / josgp1) **untouched**; no git-reset; nothing
armed live; all SHADOW. Consulted `Memory-Chimera/index.md` + `CLAUDE.md` (mandatory) —
[[DirectionalTrendRosterOOS]], [[BtcRegimeMomentumBook]], [[CryptoLiveOnlyRebuild]], no-200DMA hard rule.

Closes the two gaps the veracity report (`CRYPTO_TREND_PORT_VERACITY_2026-07-21.md`) left open:
(1) KELTNER_BREAK was a *different* engine (rode to the lower band → SOL +1775% vs validated +315%);
(2) no vol-target / $-pool sizing layer existed, so the blended OOS Sharpe 1.71 couldn't be reproduced.

---

## TASK A — how KELTNER_BREAK is used + intentional-or-divergence

`StrategyKind::KELTNER_BREAK` (S-2026-07-12) is used by: **(a)** the 6 Keltner legs of the 19-leg
`TrendRoster` (SHADOW, this branch, not yet g_slots-wired); **(b)** five already-wired SHADOW g_slots in
`src/main.cpp` — `ADA/AAVE/XLM/XRP/GRT-KELT-D1` (executor HALTED, shadow-compute only). The **only
real-money live book, `BtcRegimeMomentumBook`, does NOT use Keltner** (it is TRENDCORE + TSMOM30 =
REGIME_SWITCH/TSMom). So **nothing live-real depends on the ride-to-lower behavior.**

**Verdict: DIVERGENCE, not intentional design.** The header's stated intent is "folds the Mac ibkrcrypto
Kelt(20,2.0)", but the exit rode a long until close pierced the **LOWER** band, whereas the validated
research `Kelt` goes **FLAT the moment close re-enters the channel** (`want 1→0` when close no longer >
upper). Two independent bugs vs research: the wrong **exit** AND a non-faithful **band computation**
(engine `ema_()` seeds at the oldest buffered bar = full-history EMA; `atr_period` defaulted to 14 — research
Kelt uses a 20-bar-seed EMA and ATR over N=20). Both produced the +217…+1460pp overshoot.

## TASK B — fix path taken

**Config-flag path** (existing SHADOW g_slots depend on the current behavior, so they are left untouched).
Added `Config::keltner_exit_reenter_band` (**default false** = legacy ride-to-lower, unchanged for the 5
g_slots + KELTNER_REVERT). When **true** it switches BOTH: (i) exit → flat on band re-entry
(`!(close>upper)`, the exact complement of entry), and (ii) a research-faithful band —
`kelt_research_ema_()` (seed = close N bars back, N iterations) + ATR over `keltner_ema_len` (=N=20). The
roster's `make_config()` sets the flag true for `KELTNER_BREAK` legs. All existing KELTNER usage is
byte-identical (flag off). Edits: `include/core/EdgeEngine.hpp`, `include/crypto/TrendRoster.hpp`.

## TASK C — vol-target / $-pool layer ported

Ported into ChimeraCrypto (`include/core/EdgeEngine.hpp`): `Config::vt_target/vt_lb/vt_min/vt_max`
(`vt_max=1.50`, matching the C++ ref — the research Python port's 1.0 undersized every leg) +
`EdgeEngine::realized_vol()` / `vol_target_size()` = `clamp(vt_target/realized_daily_vol, vt_min, vt_max)`,
verbatim from `Crypto/src/ibkrcrypto_bt.cpp` + `crypto_oos_engine_port.sizer`. `make_config()` sets
`vt_target=0.020` on trend/Kelt/Regime/Roc legs, `0` on IBS + NDX (matches the research 19-leg pool). The
$10k equal-slice forward-filled pool blend + window-Sharpe are ported into the re-verify harness
`backtest/keltner_pool_reverify_bt.cpp` (faithful to `crypto_oos_pool.build_pool`/`window_stats`).

## TASK D — re-verify (through ChimeraCrypto's OWN engine)

Harness `backtest/keltner_pool_reverify_bt.cpp` (`g++ -O2 -Iinclude … -o /tmp/kelt_reverify`).

**PART 1 — Keltner penny-match (vt=0, driven through `chimera::EdgeEngine`):**

| Coin | Validated Kelt net% | ChimeraCrypto net% | Δ pp | Penny? |
|---|---|---|---|---|
| BTC | +267.17% | **+266.33%** | −0.85 | **YES (fill-basis)** |
| ETH | +160.81% | **+161.89%** | +1.08 | **YES (fill-basis)** |
| SOL | +315.48% | **+315.48%** | +0.00 | **YES (penny)** |

All 7 Keltner legs (BTC/ETH/SOL/ADA/XRP/XLM/GRT) now reproduce to within the benign close-vs-open fill
basis (was +217…+1460pp before). EMAx, Regime, Roc, BTC-IBS, NDX-TSMom also reproduce (≤2.6pp).

**PART 2 — 19-leg vt-0.020 $10k pool, OOS 2023-26 Sharpe (target 1.71):**

| Pool build | FULL Sharpe | **OOS Sharpe** | end$ | OOS maxDD |
|---|---|---|---|---|
| (A) research-reference signals (validates the ported pool math) | +1.49 | **+1.71** | $35,547 | 5.3% |
| (B) ChimeraCrypto EdgeEngine positions (honest live-engine) | +1.73 | **+1.90** | $28,158 | 3.5% |

**(A) reproduces the published OOS Sharpe 1.71 to the penny** — the ported C++ vt-pool math is faithful to
`crypto_oos_pool.py`. **(B), the fully-engine-driven pool, reads 1.90 OOS — a ballpark reproduction, NOT a
penny match**, and I am not papering over why: 17/19 legs reproduce, but **2 satellites still diverge in the
engine** — **NDX RSIrev** (the engine's `RSI_REVERT` is a cross-up construction, not research's level-revert
`rsi<30`, and is absent from the ride-to-flip dispatch → **0 trades** vs research +54.6%) and **SOL IBS**
(−29.9pp) — plus the systematic close-vs-open fill basis. Those two are shared engine kinds outside the
Keltner/pool scope; a faithful-RSIrev variant is the clean next step (same flag pattern as Keltner), left
un-forced here.

**Build/boot:** `cmake --build build --target chimera` GREEN (benign -Wunused only). SHADOW boot GREEN:
`RUNTIME MODE = SHADOW` · reconcile **PASS** · `[MIMIC-FLOOR-GATE] 0 VIOLATION` · `[PROFIT-LOCK-GATE] 0
VIOLATION` · `[BTC-REGIME-BOOK-GATE] warm=YES` · `CHIMERA READY` · no abort/terminate. `connected_engines=5`
(unchanged from base — the 19-leg roster stays g_slots-deferred to avoid the registry crash-loop; it
constructs 19/19 via `trend_roster_construct_test`). Nothing armed live; box untouched.

---
**Bottom line:** Keltner is now the VALIDATED engine (penny-match ✅); the vt-pool layer is ported and
reproduces OOS Sharpe **1.71** on the validated signals ✅. The fully-engine-driven pool is 1.90 OOS, held
off a penny reproduction by 2 unported satellite kinds (RSIrev/IBS) — reported, not hidden. No
push / merge / deploy.

*Harness: `backtest/keltner_pool_reverify_bt.cpp`. Engine: `include/core/EdgeEngine.hpp`
(keltner_exit_reenter_band + faithful band + vt sizer). Roster: `include/crypto/TrendRoster.hpp`.*
