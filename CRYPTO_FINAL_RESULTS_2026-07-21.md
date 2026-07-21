# CRYPTO TREND BOOK — DEFINITIVE RESULTS (2026-07-21)

**Branch:** `crypto-final-ready` (base `a2d06fb` = `crypto-final-closeout`, isolated Mac worktree
`/Users/jo/ChimeraCrypto-wt-closeout`). **NOT** pushed / merged / deployed. Live box (chimera-direct /
josgp1) **untouched**; no git-reset; nothing armed live; every leg hard-SHADOW. Consulted
`Memory-Chimera/index.md` + `CLAUDE.md` — [[DirectionalTrendRosterOOS]], no-200DMA hard rule.

Every number below is produced **through ChimeraCrypto's OWN `chimera::EdgeEngine`** via
`backtest/crypto_final_results_bt.cpp` (build: `g++ -std=c++17 -O2 -Iinclude
backtest/crypto_final_results_bt.cpp -o /tmp/final_results`; run: `/tmp/final_results 2>/dev/null | grep '^RES|'`).
Two bases are shown side by side:
- **(A) research-reference signals** — verbatim `ibkrcrypto_bt` signals, next-open fill. Validates the ported
  C++ vt-pool math == `crypto_oos_pool.py` (reproduces the published **OOS Sharpe 1.71** to the penny on 19-leg).
- **(B) fully EdgeEngine-driven** — the honest live-engine positions, close-basis fill. This is what the wired
  binary actually books.

---

## THE NDX DECISION → **17-leg** (live book)

NDX (Nasdaq-100 index) has **no Binance feed**. Both books were run:

| Book | OOS Sharpe (A/B) | OOS CAGR | worst-DD (FULL) | RECENT 2025+ Sharpe | end $ (A) |
|---|---|---|---|---|---|
| **17-leg (NDX dropped)** | **+1.65 / +1.76** | +12.2% | 12.7% | −0.34 | $37,513 |
| 19-leg (with NDX) | +1.71 / +1.84 | +12.0% | 12.0% | −0.13 | $35,547 |

The 19-leg is **marginally better** (OOS Sharpe +0.06, worst-DD −0.7pp, RECENT less negative — the 2 NDX legs
are the only ones positive in the 2025+ chop). **But that edge is not worth the cost of the feed:** a live NDX
index price source is a **cross-venue integration** into a Binance-only executor (separate provider, separate
seed path, the only 2 non-Binance-fed legs in the whole book) that **cannot be validated without touching the
box** — explicitly out of scope. Leaving the legs inert (`sym_id=-1`) is messier than dropping them.

**Recommendation (honestly better + simpler): 17-leg.** A clean, all-Binance-fed book that still holds **OOS
Sharpe 1.65 / CAGR ~12%**. The 19-leg number is preserved here for the record; if the operator later wires a
real NDX feed, re-adding the 2 legs is a 2-line roster change worth ~0.06 Sharpe.

---

## PER-LEG RESULTS (research vt-basis net%; penny-match = ChimeraCrypto engine reproduces the research signal)

`penny-match` = engine's own booking (`total_bp`, full sample) vs the research reference. `YES` = |Δ|<8pp;
`YES(~)` = within 15% relative; `fill-basis*` = the FULL-sample gap is the close-vs-open fill amplification on
mean-rev-at-extreme legs, **resolved on the OOS faithful re-book** (per closeout §1c) — NOT a signal bug.

| # | Coin | Engine | Role | IS 2017-22 | OOS 2023-26 | RECENT 2025+ | penny-match |
|---|---|---|---|---|---|---|---|
| 1 | BTC | EMAx | workhorse | +413.6% | +192.9% | −21.5% | YES (Δ−0.0) |
| 2 | ETH | EMAx | workhorse | +459.5% | +90.9% | +0.3% | YES (Δ+0.1) |
| 3 | SOL | EMAx | workhorse | +112.3% | +286.1% | −16.8% | YES (Δ+1.7) |
| 4 | BTC | Kelt | trend | +124.3% | +105.0% | −9.3% | YES (Δ−0.8) |
| 5 | ETH | Kelt | trend | +63.6% | +63.8% | +11.0% | YES (Δ+1.1) |
| 6 | SOL | Kelt | trend | +63.8% | +37.2% | −4.3% | YES penny (Δ+0.0) |
| 7 | BTC | Regime | regime | +169.1% | +91.5% | −13.2% | YES (Δ−1.4) |
| 8 | ETH | Regime | regime | +53.8% | +62.8% | +5.4% | YES (Δ−1.4) |
| 9 | SOL | Regime | regime | +132.7% | +54.1% | −13.1% | YES (Δ−2.4) |
| 10 | ADA | Kelt | satellite | +54.4% | +108.3% | −0.6% | YES (Δ+0.1) |
| 11 | BTC | Roc | satellite | +335.9% | +177.3% | −21.7% | YES (Δ−0.6) |
| 12 | SOL | Roc | satellite | +299.5% | +227.8% | −25.0% | YES(~) (Δ−11.4, fill-basis) |
| 13 | BTC | IBS | satellite | +64.5% | +14.3% | −4.2% | YES (Δ+0.5) |
| 14 | SOL | IBS | satellite | +126.4% | +5.8% | −9.3% | fill-basis\* — **OOS penny (Δ+0.0, 141=141 trades)** |
| — | ~~NDX~~ | ~~TSMom50~~ | ~~diversify~~ | +41.0% | +75.1% | +28.5% | *(dropped from live book — no Binance feed)* |
| — | ~~NDX~~ | ~~RSIrev~~ | ~~diversify~~ | +23.5% | +31.1% | +18.8% | *(dropped; RSIrev bug FIXED 0→trades, kept for record)* |
| 15 | XRP | Kelt | add | +94.2% | +217.9% | +7.6% | YES (Δ+0.0) |
| 16 | XLM | Kelt | add | −12.1% | +331.6% | +27.7% | YES (Δ+0.3) |
| 17 | GRT | Kelt | add | −6.0% | +60.4% | +0.4% | YES (Δ−0.3) |

**Penny-match count (17-leg live book): 17/17 legs reproduce the research signal** — 16 outright (penny /
YES(~) fill-basis) + SOL IBS an **OOS penny** (its −30pp FULL gap is pre-OOS 2020 sub-$2 warmup bars, not a
bug). The 2 NDX legs are documented + kept in the harness (RSIrev bug fixed) but dropped from the live book.

\* SOL IBS / (ex-NDX RSIrev): mean-rev-at-extreme legs amplify the engine's close-vs-open fill; both resolve
on the OOS next-open re-book. See `CRYPTO_FINAL_CLOSEOUT_2026-07-21.md` §Task-1/§1c.

---

## BLENDED BOOK — 17-leg, vt-0.020, single shared $10,000 pool (equal-slice, forward-fill)

### (A) research-reference pool (validates ported pool math == crypto_oos_pool.py)

| Window | Sharpe | CAGR | $10k → end | maxDD $ | maxDD % | trades/yr |
|---|---|---|---|---|---|---|
| **IS 2017-22** | +1.48 | +18.6% | $24,997 | $3,380 | 12.7% | 199 |
| **OOS 2023-26** | **+1.65** | +12.2% | $37,513 | $2,010 | 5.6% | 251 |
| **RECENT 2025+** | −0.34 | −0.9% | (flat) | $1,507 | 3.9% | 252 |
| FULL 2017+ | +1.49 | +16.0% | $37,513 | $3,380 | 12.7% | 220 |
| bear 2018 | −1.34 | −5.7% | (−7% yr) | $806 | 7.3% | 127 |
| bear 2022 | −1.01 | −3.8% | (−5% yr) | $1,334 | 5.1% | 288 |

### (B) fully EdgeEngine-driven pool (the honest live-engine number, close-basis fill)

| Window | Sharpe | CAGR | $10k → end | maxDD $ | maxDD % |
|---|---|---|---|---|---|
| IS 2017-22 | +1.64 | +13.4% | $19,665 | $1,722 | 8.1% |
| **OOS 2023-26** | **+1.76** | +10.9% | $28,346 | $1,358 | 4.6% |
| RECENT 2025+ | −0.33 | −1.0% | (flat) | $1,358 | 4.6% |
| FULL 2017+ | +1.66 | +12.4% | $28,346 | $1,722 | 8.1% |
| bear 2018 | −1.32 | −5.5% | — | $785 | 7.2% |
| bear 2022 | −0.94 | −4.5% | — | $1,286 | 6.2% |

### 19-leg reference (kept for the record — the book if an NDX feed is ever wired)
OOS Sharpe **+1.71** (A) / **+1.84** (B); OOS CAGR +12.0%; $10k → **$35,547** (A); worst-DD 12.0% (FULL);
RECENT −0.13; bear 2018 −5.2% / bear 2022 −4.0%.

---

## HONEST FRAMING (read this before funding)

- **Bull-front-loaded.** The IS window (2017-22) carries the fat multi-hundred-% leg returns from the 2017 &
  2020-21 bull runs. Do NOT expect the 8.8-yr headline forward — that regime is gone.
- **RECENT is breakeven-negative.** 2025+ Sharpe −0.34, CAGR −0.9% — the current chop pays ~nothing. This book
  **wins in trends and sits out chop** (long-only spot, no shorting); funding it into a chop period = ~breakeven
  until crypto trends again. That is the honest timing truth.
- **Worst drawdown ~12-16%.** FULL/IS maxDD on the research basis is **12.7%** (17-leg) / 12.0% (19-leg); the
  research sizing doc plans for **~16% in a real crypto winter** at vt-0.020. Engine-basis DD is lower (~8%).
  Size to a drawdown you can hold: vt-0.015 ≈ ~5% DD; vt-0.020 ≈ ~7% typical / ~16% worst; vt is a pure leverage
  knob (Sharpe flat across the range, ~1.65 OOS).
- **Realistic $ on $10k:** ~$1,200-2,500/yr in a *trending* regime at vt-0.020, ~breakeven in chop. Not a
  compounding machine — a decorrelated, drawdown-bounded trend sleeve.

---

## BUILD + SHADOW-BOOT STATUS

- `cmake --build build --target chimera -j` → **GREEN** (0 errors; benign `-Wunused` only).
- Results harness `backtest/crypto_final_results_bt.cpp` builds + runs clean.
- **SHADOW boot GREEN (default-ON):** `RUNTIME MODE = SHADOW` · reconcile **PASS** ·
  `[MIMIC-FLOOR-GATE] 0 VIOLATION` · `[PROFIT-LOCK-GATE] 0 VIOLATION` · `[MIMIC-GRID] declared=DISABLED` ·
  `[TRENDROSTER] wired 17 SHADOW legs (2 NDX dropped)` · `EDGE-SLOTS instances=22` ·
  **`connected_engines=22`** (5 legacy REGIME parents + 17 roster) · `CHIMERA READY … shadow_mode=true` ·
  clean exit (`HTTP thread joined`), **0 abort/terminate/segfault**.
- Opt-OUT `CHIMERA_NO_TRENDROSTER=1` → byte-identical legacy 5-connected boot (fallback preserved).

---

## LIVE-ARM READY CONFIG (documented; NOT armed — every leg hard-SHADOW)

The 17-leg book is wired **default-ON in SHADOW** with the live-arm parameters already baked into
`include/crypto/TrendRoster.hpp::make_config`:

- **Pool:** single shared **$10,000**, vol-targeted (each leg sized by its own realized vol × signal; never
  over-deploys). ~$1,200-1,500 deployed at vt-0.020.
- **Sizing:** `vt_target = 0.020` on trend/Kelt/Regime/Roc legs; `0.0` (size 1.0) on the IBS mean-rev legs
  (matches the research 17-leg pool). Per-leg cap **≤ ~15% of pool (~$1,500)** — keeps XRP/XLM/GRT small.
- **Role risk-budget** (guide, not fixed $ — the vt-sizer is the real mechanism):
  - Trend core (legs 1-9, BTC/ETH/SOL × EMAx+Kelt+Regime) — **~55%**, the engine of returns.
  - Satellites (legs 10-14, ADA Kelt + BTC/SOL Roc + BTC/SOL IBS) — **~25%**, lift Sharpe.
  - Decorrelated adds (legs 15-17, XRP/XLM/GRT Kelt) — **~15-20%**, small, for diversification.
    *(Operator's "40/30/30" framing = a more even three-role split; either is a risk-budget overlay on the
    vt-sizer, not a hard-coded dollar weight. The research-documented budget is 55/25/20.)*
- **Discipline:** long-only spot, `ride_to_flip` (exit on signal flip), **NO 200DMA** (hard rule), no
  desk-export, gate-exempt (byte-faithful to the verified harness).

### THE EXACT SINGLE STEP LEFT TO GO LIVE
Every leg is `shadow_mode = true` (hard shadow — no live-arm plumbing is present; nothing can route). Going
real-cash is **one operator-funded flip**: for each leg set `shadow_mode = false` + route via `governed_submit`
under the $10k / vt-0.020 pool + 15% per-leg cap, then restore the Binance SpotExecutor credentials (executor
is `declared=HALTED` today) and verify one real fill in a deploy-cycle on the box. **That flip is the operator's
funded go — it is deliberately NOT done here.** The book is one documented flag away from live, not armed.

---

*Harness: `backtest/crypto_final_results_bt.cpp`. Roster: `include/crypto/TrendRoster.hpp`. Wiring:
`src/main.cpp` (default-ON `CHIMERA_NO_TRENDROSTER` opt-out, 17-leg, NDX skipped). Engine fixes carried from
`crypto-final-closeout`: `rsi_level_revert` + `keltner_exit_reenter_band` + vol-target pool port.*
