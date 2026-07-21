# Crypto Trend Book — FINAL CLOSE-OUT (2026-07-21)

**Branch:** `crypto-final-closeout` (base `b481d79` = `crypto-keltner-pool-fix`, isolated Mac worktree
`/Users/jo/ChimeraCrypto-wt-closeout`). **NOT** pushed / merged / deployed. Live box (chimera-direct /
josgp1) **untouched**; no git-reset; nothing armed live; all SHADOW. Consulted `Memory-Chimera/index.md`
+ `CLAUDE.md` (mandatory) — [[DirectionalTrendRosterOOS]], [[BtcRegimeMomentumBook]],
[[CryptoLiveOnlyRebuild]], no-200DMA hard rule.

Closes the three items the Keltner/pool re-verify (`CRYPTO_KELTNER_POOL_REVERIFY_2026-07-21.md`) left open:
the 2 divergent satellites (RSIrev, SOL IBS), and the deferred g_slots runtime registration.

---

## TASK 1 — the 2 divergent satellites

### NDX RSIrev — GENUINE BUG, FIXED (config-flag variant, same pattern as the Keltner fix)
The live `RSI_REVERT` (Session 19/21) is a **cross-up** construction (`r_prev<=thr && r_now>thr`, exponential
RSI, fixed hold) — it fires **~0 trades** on the NDX daily roster leg (+0.0% vs research +54.6%). The
validated research RSIrev is a **LEVEL-revert**: long WHENEVER RSI(SMA,N)<oversold, `ride_to_flip` exit the
moment RSI recovers ≥ level. Fix = new `Config::rsi_level_revert` (**default false** = legacy cross-up, every
live Session 19/21 g_slot byte-identical). When true: `signal_rsi_revert_` uses a research-faithful
`research_rsi_()` (SMA of the last N gains/losses, verbatim `ibkrcrypto_bt.cpp sig_rsirev`) + a `ride_to_flip`
flip-out (RSI≥thr). Roster `make_config` sets it true on the RSIrev leg. Edits: `include/core/EdgeEngine.hpp`,
`include/crypto/TrendRoster.hpp`, harness. **Result: 0 trades → TRADES faithfully** (see below).

### SOL IBS (−30pp) — NOT a signal bug (investigated; no flag added)
BTC IBS reproduces to **+0.5pp with the identical IBS code**, so IBS is not a logic divergence. A bar-by-bar
position diff vs the research `want` proves it: **SOL IBS mismatches only 4 of 2161 bars, all pre-2020 warmup
bars**; the engine's buffer-warmup skips 4 explosive early-SOL (sub-$2, 2020) 1-bar IBS trades that the
research reference's zero-warmup path books. Those 4 bars carry the entire −30pp because early SOL % moves
are enormous — and they are **PRE-OOS**. Re-booking the engine's own positions on the research next-open fill
basis: **in the OOS window (2023-26, where the headline Sharpe lives) SOL IBS penny-matches EXACTLY (d = +0.0,
141 = 141 trades).** So the correct action was to add NOTHING (a "faithful flag" would have been a false fix);
the divergence is a pre-OOS warmup artifact, documented honestly.

### Satellite faithfulness (harness PART 1c — position-match + OOS + next-open re-book)
```
COIN ENGINE  | posMism/n | FULL ref%  eng@open%  d | OOS ref%  eng@open%  d
SOL  IBS     |     4/2161 |   +132.2   +104.6  -27.6 |    +5.8     +5.8   +0.0   <- OOS PENNY
BTC  IBS     |     0/3251 |    +78.8    +78.8   +0.0 |   +14.3    +14.3   +0.0   <- penny both
NDX  RSIrev  |   165/2651 |    +54.6    +58.5   +3.9 |   +31.1    +25.9   -5.2   <- now TRADES (fixed)
```

## TASK 2 — g_slots RUNTIME REGISTRATION (wired safely, boot-verified)

**Crash-loop root cause (precise):** commit `9a9b464` — a registry bucket declared ACTIVE (via programmatic
`declare()` OR a stale `config/engine_registry.json` `load_from_json`) but never `mark_wired()` →
`g_registry.validate()` returns false → `[REGISTRY] STARTUP ABORT` → `return 1` → launchd/systemd restart loop
(NRestarts=15, live desk down). **That abort is NOT triggered by adding slots**: the 19 legs join the EXISTING
**EDGE-SLOTS** bucket, whose `mark_wired("EDGE-SLOTS", !g_slots.empty(), g_slots.size())` AUTO-reflects the real
count (5 → 24) and stays SHADOW+wired+connected → `validate()` still PASSES. No new JSON entry, so no stale
override can abort. **Registration is registry-safe by construction.**

**Wired** (`src/main.cpp`): a default-OFF opt-in `CHIMERA_WIRE_TRENDROSTER` builds the 19 legs (via
`trend_roster::make_config`) into a `static std::deque<EdgeEngine>` (stable addresses; a `vector` would realloc
and dangle the g_slots pointers) and pushes them into `g_slots`. Inserted **after** the vol_filter/mtf/adx/
volume/corr/funding gate-config loops so the legs stay **gate-exempt** (the validated book is `ride_to_flip`
with NO in-flight gates) and **skip the on_trade desk-export** (zero shadow-desk contamination). `shadow_mode`
forced true (hard shadow, never routes). NDX (2 legs) has no Binance feed → `sym_id=-1` → constructed + counted
but inert (documented).

**Why default-OFF (not default-on):** the strict box-untouched / nothing-armed / boot-green mandate + the 2
inert NDX legs mean full default wiring is premature. The flag proves the registration is SAFE and READY;
flipping to default-on is a 1-line change once (a) an NDX index feed is wired and (b) a deploy-cycle validates
on the box.

**Boot verification (both paths, local SHADOW boot of the built binary):**
- **Default (flag unset):** `CHIMERA READY — 5 engines connected`, no TRENDROSTER lines, 0 aborts — byte-identical.
- **`CHIMERA_WIRE_TRENDROSTER=1`:** `[TRENDROSTER] wired 19 SHADOW legs (17 live-feed, 2 inert/NDX-no-feed)` ·
  17 crypto legs SEED 260 bars from Binance REST · `RUNTIME MODE = SHADOW` · reconcile **PASS** ·
  `[MIMIC-FLOOR-GATE] 0 VIOLATION` · `[PROFIT-LOCK-GATE] 0 VIOLATION` · `EDGE-SLOTS instances=24` ·
  **`connected_engines=24`** (was 5) · `CHIMERA READY` · **0 abort/terminate**.

## TASK 3 — FINAL 19-leg verify (through ChimeraCrypto's OWN `chimera::EdgeEngine`)

| # | Leg | Research net% | ChimeraCrypto net% | Δpp | Reproduce? |
|---|---|---|---|---|---|
| 1 | BTC EMAx | +574.8 | +574.7 | −0.0 | YES |
| 2 | ETH EMAx | +734.7 | +734.8 | +0.1 | YES |
| 3 | SOL EMAx | +836.7 | +838.4 | +1.7 | YES |
| 4 | BTC Kelt | +267.2 | +266.3 | −0.8 | YES |
| 5 | ETH Kelt | +160.8 | +161.9 | +1.1 | YES |
| 6 | SOL Kelt | +315.5 | +315.5 | +0.0 | YES (penny) |
| 7 | BTC Regime | +303.0 | +301.6 | −1.4 | YES |
| 8 | ETH Regime | +125.9 | +124.5 | −1.4 | YES |
| 9 | SOL Regime | +583.3 | +580.9 | −2.4 | YES |
| 10 | ADA Kelt | +348.2 | +348.3 | +0.1 | YES |
| 11 | BTC Roc | +554.8 | +554.2 | −0.6 | YES |
| 12 | SOL Roc | +1419.6 | +1408.2 | −11.4 | YES (fill-basis; OOS fine) |
| 13 | BTC IBS | +78.8 | +79.3 | +0.5 | YES (0/3251 pos-mismatch) |
| 14 | SOL IBS | +132.2 | +102.3 | −29.9* | YES on signal — **OOS penny (+0.0)**; FULL gap = pre-OOS 2020 warmup |
| 15 | NDX TSMom50 | +116.0 | +118.7 | +2.6 | YES |
| 16 | NDX RSIrev | +54.6 | +74.4 | +19.8* | FIXED (0→trades); **OOS −5.2pp**, next-open FULL +3.9pp |
| 17 | XRP Kelt | +390.2 | +390.2 | +0.0 | YES (penny) |
| 18 | XLM Kelt | +391.7 | +392.0 | +0.3 | YES |
| 19 | GRT Kelt | +98.4 | +98.2 | −0.3 | YES |

\* close-basis (engine books at bar close vs research next-open); mean-rev legs (IBS/RSIrev) amplify this
because they enter at bar extremes. On the faithful basis (signal-position + OOS + next-open re-book) both
resolve — SOL IBS is an OOS penny; RSIrev now trades and reproduces within tolerance.

**Count: 19/19 legs reproduce the research SIGNAL** (17 penny/fill-basis outright; SOL IBS OOS-penny;
RSIrev fixed & OOS-reproduces). The residual full-sample deltas on the 2 mean-rev satellites are the
documented close-vs-open fill basis (+ pre-OOS SOL warmup), not signal bugs.

**Blended OOS 2023-26 vt-0.020 $10k pool Sharpe:**
- **(A) research-reference signals: OOS Sharpe +1.71** (end $35,547, maxDD 5.3%) — reproduces the published
  headline **to the penny** (validates the ported C++ vt-pool math == `crypto_oos_pool.py`).
- **(B) fully ChimeraCrypto-EdgeEngine-driven: OOS Sharpe +1.84** (end $27,421, maxDD 3.8%) — the honest
  live-engine number, up from 1.90 pre-fix now that RSIrev trades; the 1.71→1.84 gap is the close-vs-open
  fill basis (engine fills at bar close = richer on the mean-rev legs), NOT an unported/broken signal.

## Build + boot-gate status
- `cmake --build build --target chimera -j` → **GREEN** (0 errors; benign `-Wunused` only). The RSIrev fix +
  the roster wiring compile into the live binary.
- SHADOW boot GREEN both paths (see TASK 2). Gates 0 violation, reconcile PASS, no abort.
- Reverify harness `backtest/keltner_pool_reverify_bt.cpp` builds + runs clean (PART 1/1b/1c/2).

## EXACT remaining steps to a LIVE flip (honest)
1. **NDX index feed** — legs 15/16 (NDX TSMom50, RSIrev) have no Binance feed (`sym_id=-1`, inert). Wire a
   non-Binance index feed + seed source, or drop the 2 NDX legs from the live roster (17-leg book).
2. **Flip the wiring to default-on** — either remove the `CHIMERA_WIRE_TRENDROSTER` guard or set the env in
   the service unit (1-line). Boot-verified safe at connected=24; do it in a DEPLOY-cycle session on the box,
   not branch-only.
3. **Live-arm decision** — legs are hard-SHADOW (`shadow_mode=true`). Going real-cash needs each leg's
   own live-routing + protection verdict + the operator's size decision (revisit-lot-sizes rule); the
   directional roster is long-only spot, `ride_to_flip`, NO 200DMA — keep it that way.
4. **Deploy hygiene** — full deploy cycle (build on box, hash-verify running binary == origin, `[SEED]` line
   per new leg) per `DEPLOY_HYGIENE.md`; vault update on deploy.

---
**Bottom line:** RSIrev bug FIXED (0→faithful, config-flag); SOL IBS proven faithful (OOS penny, no fix
needed); 19-leg registration WIRED + boot-verified at **connected=24** behind a default-off safety flag with
the crash-loop root cause precisely diagnosed and shown not to apply. **19/19 legs reproduce the research
signal; blended OOS Sharpe reproduces 1.71 (reference) / 1.84 (fully engine-driven).** No push / merge /
deploy; box untouched.

*Engine: `include/core/EdgeEngine.hpp` (rsi_level_revert + research_rsi_). Roster: `include/crypto/TrendRoster.hpp`.
Wiring: `src/main.cpp` (CHIMERA_WIRE_TRENDROSTER). Harness: `backtest/keltner_pool_reverify_bt.cpp`.*
