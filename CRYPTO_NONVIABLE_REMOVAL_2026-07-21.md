# Crypto Non-Viable Mimic-Grid Removal — 2026-07-21

**Branch:** `crypto-remove-nonviable-mimic-grid` (NOT pushed / merged / deployed — box untouched)
**Base:** HEAD `2986cb1` (the live-binary lineage; `/Users/jo/ChimeraCrypto`, isolated Mac worktree)
**Operator order:** remove the OLD, UNVERIFIED strategy engines + flag them non-viable, so only the
honest/verified directional book remains. HARD SAFETY: no `git reset`, no deploy, no box (josgp1) touch.

## Why (the finding)
The live binary's real-order strategy path was the **mimic / companion / clip grid**
(`MimicLadderCompanion` cells in `_grid` → `g_mimic_mirror` → `governed_submit` → Binance MARKET),
NOT the verified directional trend book. Session 2026-07-21 (`Crypto/backtest/CRYPTO_LIVE_REBUILD_ASSESS`)
proved this whole class is the **BE-clamp illusion class**: it looked profitable ONLY under
anchored-`le` / BE-clamp shadow accounting; under HONEST per-leg fills it is **net-negative** — the
identical failure to the Omega ladders, and to the box-side honest re-cert that FAILED **369/380** live
cells (vault S-20z / S-20u / S-20, `MimicShadowEntryBasisError`). The `MIMIC-FLOOR-GATE` only ever proved
the DESIGN was floored-on-open (a config property), never that a clip cannot book negative on a real fill.

## What was REMOVED (tombstoned, `#if 0` guard in `src/main.cpp` ~L5164–L5687)
The entire mimic-book population span is guarded out, leaving `_grid` **EMPTY** — nothing feeds
`g_mimic_mirror`, so **zero mimic Binance orders** can fire. 21 live mimic cells across 4 families:

| Family | Builder | Cells | Symbols |
|---|---|---|---|
| REGIME-BEMIMIC | `make_be_mimic` | 3 | THETA, SUSHI, ADA |
| SWEET MIMIC-FLOOR | `make_mimic_floor_cell` | 5 | BNB, UNI, NEAR, TRX, DOGE |
| PJ jump_floor mimic | (dedicated) | 2 | GRT-PJ5W1 signal-only + GRT-PJ5W1-MIM |
| BE-CASCADE (`_bc_cells`/`_bc_fast`/`_bc_slow`/`_bc_eth_lowthr`/`_bc_alt_lowthr`) | `make_becascade_cell` | 11 | DOGE×7, RUNE×2, AVAX, INJ (the S-20z "honest survivors" — retired here: net-negative on honest fills) |

The now-unused factory lambdas (`make_be_mimic`, `make_mimic_floor_cell`, `make_becascade_cell`) remain
defined above the guard (harmless `-Wunused` warnings; no `-Werror` in `CMakeLists.txt`).

## What was KEPT (verified / honest directional book — do NOT confuse with the mimic grid)
- **`BtcRegimeMomentumBook`** (`g_btc_regime_book`) — LIVE directional long-only BTC spot book
  (TRENDCORE Donchian/EMA200 + TSMOM30, 50/50; certified, real fills, no BE-clamp; armed only in LIVE).
  This is the ONE verified live directional real-order book in the binary.
- **`g_slots` directional roster** — the REGIME_SWITCH parents (`near/theta/sushi/ada/dot_regime_d1`,
  declared ABOVE the guard, used at the `g_slots.push_back(...)` directional path) + the D1/H-tf
  TSMom/EMAx/Keltner/Roc/IBS EdgeEngines. These compute directional signals; their executor path is
  `HALTED`/shadow (`EXECUTOR declared=HALTED`), unchanged by this edit.

## Build + boot verification (local Mac, SHADOW)
- **Build:** `cmake --build build --target chimera -j` → **GREEN** (`Built target chimera`; only benign
  `-Wunused` warnings, same class as the prior `#2b` removal of init_grids/init_macro_base).
- **Boot (SHADOW):** `RUNTIME MODE = SHADOW`; `reconcile PASS` (Phase-2 + registry); **no crash / no
  STARTUP ABORT**; process ran stable then stopped cleanly.
  - `[MIMIC-FLOOR-GATE] scanned 0 companions: 0 floored, 0 pending-exception, 0 VIOLATION`
  - `[PROFIT-LOCK-GATE] 0 locked, 0 documented exception(s), 0 VIOLATION`
  - `[REGISTRY] MIMIC-GRID declared=DISABLED wired=1 connected=0 instances=0` (self-degraded, validate PASS)
  - `[CLIP-MIMHOOK] GRT-PJ5W1-MIM ... MISSING (parent=null mimic=null)` — the per-tick hook self-guards on null
  - `[BTC-REGIME-BOOK-GATE] ... daily=599 bars warm=YES` (directional book intact, ready)
  - `connected_engines=5` (the directional EDGE-SLOTS roster)

## HONEST framing (as ordered)
Removing the mimic grid leaves the binary with **essentially no live real-order strategy** beyond
`BtcRegimeMomentumBook` (BTC directional, LIVE-only). The full 16/18-leg directional long-only trend
roster (`DirectionalTrendRosterOOS`, Memory-Chimera) is **NOT yet ported** into this binary as a
live-order path — that port is the expected NEXT step. This is the intended outcome: strip the illusion
class first, then wire the verified trend book.

## AMBIGUOUS engines left for operator decision
**None.** Every cell in `_grid` was an unambiguous mimic/companion/clip/BECASC cell. The directional
`g_slots` roster and `BtcRegimeMomentumBook` are unambiguously the honest directional class and were kept.
The `g_slots` engines named `*-MIMIC-H1` are directional EdgeEngine strategies (legacy naming), NOT
`MimicLadderCompanion` grid cells — out of scope, kept.
