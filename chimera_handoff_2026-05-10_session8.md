# Chimera — Session Handoff #8

**Date:** 2026-05-10 (continuation of same UTC day as sessions 5–7)
**Author:** Claude (Cowork session)
**For:** continuing this work in a fresh chat
**How to use:** paste this entire document into the first message of the new chat as context, then continue from §6 "Open questions / next steps".
**Predecessor:** `chimera_handoff_2026-05-10_session7.md` (still valid for engine internals, dataset assumptions that were *invalidated* by session 8 — see §3 — VPS hostname/IP, GitHub PAT, repo URL; superseded by this one for the verdict on Step 3, the OBI verification result, and the discovery of the FundingWindow header-comment bug).

---

## 0. TL;DR for the receiving session

Session 8 was a research/analysis session. No engine code was touched, no daemon was restarted, no production state changed. Three things happened:

1. **OBI cap verification closed.** Session 7's task #7 was the only open item from that handoff. `OBI-ENTRY` count on `chimera.log` is exactly **412**, identical to the session-7 baseline taken at cap deploy. Zero new entries since the cap landed → cap is working as intended. `/api/state2.tier1_risk` confirms `per_engine_open_R.OBI = 0`. The session 7 plan §6(a) is done.

2. **Pre-validation pass §6(b) complete, with a non-obvious verdict.** The session-7 handoff assumed historical alt funding CSVs already lived on the VPS at `data/funding/{SYMBOL}USDT.csv`. They didn't — those files have never been written. The `FundingRateFetcher` is BTC-only and writes one number to `/tmp/chimera_funding.txt` (overwritten every 8h, no persistence). What *did* exist was an unused `fetch_chimera_history.py` (session-5 artifact) which fetches Binance funding history. We ran it, pulled 365 d × 8 symbols × 3 funding events/day = **8,760 funding events**, then replayed the FundingWindow + FundingPersistenceFade entry rules over them with **zero per-symbol parameter tuning** (anti-overfitting).

3. **Engine documentation bug discovered, NOT fixed.** `FundingWindowEngine.hpp` lines 25 and 73 claim `RATE_THRESHOLD` is "15 bp/8h minimum" but the constant value `0.00015` is actually **1.5 bp/8h** under the standard convention `1 bp = 0.0001` (confirmed by the engine's own printf which formats `funding_rate * 10000.0` as `bp/8h`). The engine's *behavioural* threshold has been 1.5 bp the whole time; only the comment is wrong. **Per user preference #4 ("never modify core code unless instructed clearly") this comment was NOT fixed in session 8** — it's flagged for explicit-OK in session 9.

Verdict from the pre-validation pass:

* **FundingWindow is justified for Step 3 alt extension.** At its actual 1.5 bp threshold, the rate gate fires meaningfully often on alts: AVAX 119/yr, SOL 70/yr, XRP 36/yr, BNB 32/yr, LINK 15/yr, DOGE 9/yr — total 281/yr across the 6 alts vs only 13/yr on the BTC+ETH baseline. Alts have *more* signal than BTC/ETH at the rate gate.
* **FundingPersistenceFade is structurally inert across the entire basket.** Its `-10 bp 24h-avg` trigger is mechanically unreachable in the current funding regime. Across 8 symbols × 365 days, **1 entry total**, on SOL only (single -30 bp event). BTC's most-negative single funding event in the year was -1.52 bp — 7× off the trigger. Three actionable paths recommended in the report: retune, strip, or defer until regime change.

The 5-point plan now reads:

1. Fix backtest LTO build + source historical data — DONE (session 5).
2. Tier1Risk integration into runtime — DONE (session 6).
3. Extend FundingWindow + FundingPersistenceFade to alt basket — **split into 3a (FW: justified, ready to engineer) and 3b (FPF: needs decision before engineering — retune / strip / defer)**.
4. 4–8 weeks of forward shadow data on the alt funding engines — pending §3a.
5. Decide per-symbol tuning based on forward results — unchanged.

---

## 1. What changed this session

### Branch + commits

* **Branch:** `tier1-risk-integration` (unchanged from session 7).
* **New commits on `origin/tier1-risk-integration`:**

  | SHA | Message |
  |---|---|
  | `6c4952b4` | session 8: pre-validation script for FundingWindow + FundingPersistenceFade |
  | `24b840e3` | session 8: pre-validation report (365d Binance funding, 8-symbol basket) |

* **Pushed to:** `origin/tier1-risk-integration` via GitHub Contents API + PAT (same workflow as session 7's `06a14cc`).
* **Live binary:** still built from `06a14cc` on `josgp1`. **Session 8 did NOT trigger a rebuild or restart** — the new commits are tooling and analysis, not runtime code.
* **Mac clone state:** Mac repo at `~/ChimeraCrypto` now has the 2 new files locally and is on `tier1-risk-integration`. The GitHub remote is `Trendiisales/ChimeraCrypto`. (Note: the user's CLAUDE.md previously only listed `Trendiisales/Omega` and `~/omega_repo`; session 8 confirmed Chimera is a separate repo at `~/ChimeraCrypto` and the same PAT works for both.)

### Files added (2 total, both on origin)

| Path | Size | Purpose |
|---|---|---|
| `funding_prevalidation.py` | 25,340 B | Replay engine entry rules over historical funding CSVs. Pure stdlib, configurable via --in/--symbols/--report. |
| `funding_prevalidation_report_2026-05-10.md` | 5,954 B | Generated report from running the script over 365 d of Binance data for the 8-symbol basket. Contains funding-rate distributions, FW/FPF signal counts per symbol, and the split verdict. |

### Files staged on disk but not committed (gitignored, intentional)

| Path | Size | Purpose |
|---|---|---|
| `data/funding/{8 symbols}.csv` | ~412 KB total | Raw 365 d funding history pulled from Binance via `fetch_chimera_history.py`. Already gitignored under `.gitignore` line `data/funding/` (committed before this session). The script is reproducible — anyone with internet access can regenerate the CSVs by running `python3 fetch_chimera_history.py --skip-spot --skip-perp`. |

### Engine code — NOT touched

Session 8 made zero edits to:

* `include/core/FundingWindowEngine.hpp`
* `include/core/FundingPersistenceFadeEngine.hpp`
* `include/core/market_data/FundingRateFetcher.hpp`
* `src/main.cpp`
* Any test file
* Any Tier1Risk wrapper file

The engine documentation bug (FW header says "15 bp/8h", code says 1.5 bp) is documented in §3 of this handoff and in the script's own docstring, but the header comment itself is unchanged on disk and on `origin`. **Session 9 should explicitly confirm before fixing it** per user pref #4.

### Tests

No tests were rebuilt or rerun in session 8 because no code that they exercise changed. The session 7 baseline of 32/32 `test_tier1_risk` scenarios passing on the VPS at `06a14cc` is still the latest known result.

---

## 2. Production state right now

| Aspect | Session 7 close | Session 8 close |
|---|---|---|
| VPS hostname | `josgp1` | unchanged |
| VPS IP | 143.198.89.54 | unchanged |
| **VPS HEAD branch (running clone)** | `tier1-risk-integration` @ `06a14cc` | **unchanged at `06a14cc`** — VPS not pulled in session 8 |
| **Origin HEAD on `tier1-risk-integration`** | `06a14cc` | **`24b840e3`** (2 new commits ahead of VPS clone) |
| Live binary built from | `06a14cc` | unchanged at `06a14cc` |
| Daemon halted? | `false` | unchanged: `false` |
| `daily_realized_bp` | -213.559 (stale, awaiting position close) | unchanged: -213.559 (still no position close has rolled the day) |
| `OBI-ENTRY` count in chimera.log | 412 (baseline) | **412 (verified — cap is working)** |
| OBI per-engine R cap | 0.0 | unchanged |
| All other per_engine_r_cap entries | 1.0 | unchanged |
| `tier1_risk_state.json` | `halted=false`, daily_realized_bp=-213.559 | unchanged |
| Spot account balance | BTC 0.000499 + USDT 68.71 ≈ $108 | unchanged (no trades) |

**The VPS clone is now 2 commits behind origin.** When the next session pulls and rebuilds, expect the build to *not* change behaviour — the new files (`funding_prevalidation.py`, the report) are not part of the chimera binary. A rebuild is unnecessary unless the user wants the VPS clone to mirror origin for hygiene.

---

## 3. The FundingWindow header-comment bug (flagged, not fixed)

`include/core/FundingWindowEngine.hpp`:

* Line 25 (header comment): `2. |funding_rate| >= RATE_THRESHOLD (>=15bp/8h — meaningful imbalance)`
* Line 73 (constant): `static constexpr double RATE_THRESHOLD = 0.00015; // 15bp/8h minimum`

Both labels claim 15 bp/8h. The numeric constant `0.00015` is **1.5 bp/8h** under the standard convention `1 bp = 0.0001`, confirmed by the engine's own printf at line 162: `"%.1fbp/8h"` formatted as `funding_rate * 10000.0` — which prints `0.00015` as `1.5 bp/8h`, not `15 bp/8h`.

**Why this matters:** the entire pre-validation analysis was initially framed against the 15 bp interpretation, which produced the wrong narrative ("the engines are inert"). Once we caught the off-by-10, the FundingWindow verdict flipped from "inert across all symbols" to "alts have more signal than BTC/ETH" — a meaningfully different operational decision.

**What to do in session 9** (after explicit user OK):

* Two-character fix at line 25: `>=15bp/8h` → `>=1.5bp/8h`
* Two-character fix at line 73 trailing comment: `// 15bp/8h minimum` → `// 1.5bp/8h minimum`
* No behavioural change. Pure documentation.
* Consider whether the *intended* design was 15 bp (in which case the constant `0.00015` is the bug, and changing it to `0.0015` would silence FundingWindow entirely on every symbol over 365 days — including the 1 BTC hit). Given that the engine has been live in this state across multiple sessions and the comments in `FundingPersistenceFadeEngine.hpp` (which uses the consistent convention `-0.0010` ↔ `-10 bp`) clearly intend 1 bp = 0.0001, the most likely truth is that 0.00015 was set deliberately and the comment was a typo. Worth one line in the next session's chat to confirm.

---

## 4. Open questions / next steps for the receiving session

In strict priority order:

### (a) Decision: what to do with FundingPersistenceFadeEngine

Three options from §4 of the report. **My recommended pick is "defer until regime change"** because:

* Retuning to -2/-3 bp removes the engine's entire structural premise (its edge is *persistent extreme* funding; at those levels the funding is no longer extreme, just ordinary).
* Stripping wastes the existing wiring + test coverage for what might be a temporary regime.
* Deferring is free — `shadow_mode = true` means it costs no R while the trigger never fires.

But this is the user's call. Could go either way. Two-line `src/main.cpp` change either way (cap to 0R like OBI, or leave as-is).

### (b) Step 3a: extend FundingWindow to the 6 alts (the engineering pass)

After (a) is decided. Touches:

* `include/core/FundingWindowEngine.hpp` — already symbol-agnostic via `symbol_` + `symbol_id_`; no edits.
* `src/main.cpp` — add 6 new instances `FundingWindowEngine fw_solusdt("solusdt"), fw_bnbusdt(...), ...`, wire each to `risk` via `set_risk(&risk)`, and add to the per-tick dispatch loop. Mirror the BTC/ETH pattern exactly.
* `PerpFeed` (live data layer) — extend WS subscriptions to include the 6 alt perp symbols so `funding_rate(id)` and `basis_bp(id, spot)` return real values for them. This is the bigger lift; review `include/live/BinanceWSFeed.hpp` and the `PerpFeed::subscribe(...)` calls to confirm what's needed. The session-7 handoff §10 noted "6 untraded basket symbols still have no live perp WS feed" — this is what fixes that.
* Build + run `test_tier1_risk` — should still pass, no risk-side changes.
* Restart on VPS during a no-position window (avoid the session-6 restart-with-position divergence noted in session-7 handoff §10).

### (c) Optional: fix the FW header-comment bug

Per §3 of this handoff. Trivial, but core-code per user pref. Wait for explicit OK.

### (d) Step 4: 4–8 weeks of forward-shadow data on alt FW engines

Passive wait once §3a deploys. With Tier1Risk wired you'll get per-engine P&L attribution automatically. Most useful telemetry to watch: how often does the rate gate (1.5 bp) coincide with the basis gate (3 bp)? The rate-only count from the report is an upper bound; live data tells you the true conjoint frequency.

### (e) Step 5: review forward results, decide per-symbol tuning

Unchanged from session-7 handoff §6(e). Pass criterion suggested in the report: net P&L > 0 after 15 bp round-trip cost on >= 30 trades per symbol.

### (f) Cleanup carry-overs from prior sessions (still open)

* §6(f) carryover: top up Binance spot wallet to $500–1000.
* §6(g) carryover: external uptime monitoring (UptimeRobot, 5 min ping).
* §6(h) carryover: delete obsolete `origin/tier1-risk-wrapper`.
* §6(i) carryover: GUI surfacing of `tier1_risk` state2 block.
* Update CLAUDE.md to list both projects (Omega forex/CFD scalper + Chimera crypto perps) so the next fresh chat doesn't blunder into the wrong tree like session 8 did at start. Suggested format in the chat above.

### (g) Optional: pull VPS to `24b840e3`

Not strictly required since the new commits are tooling. But for repo hygiene:

```bash
cd /home/jo/ChimeraCrypto
git fetch --all --prune
git pull --ff-only
# no rebuild needed — funding_prevalidation.py is not built into chimera
```

### (h) Re-run the pre-validation any time

The script and CSVs are now in the repo / on disk. Re-running takes ~10 s once the CSVs exist:

```bash
cd /home/jo/ChimeraCrypto
python3 fetch_chimera_history.py --skip-spot --skip-perp   # ~8 s if already current (resumable)
python3 funding_prevalidation.py                           # ~1 s
# report written to funding_prevalidation_report_<UTC date>.md
```

---

## 5. Tier1Risk configuration (live, unchanged from session 7)

| Field | Value | Notes |
|---|---|---|
| `per_engine_r_cap[SWING..]` | 1.0 each | unchanged |
| `per_engine_r_cap[OBI]` | 0.0 | session 7 change, verified working in session 8 (412 entry count = baseline) |
| `total_r_cap` | 3.0 | unchanged |
| `daily_loss_kill_bp` | -200.0 | unchanged |
| `max_engines_per_symbol_side` | 2 | unchanged (still tight per session-6 §10) |
| `max_orders_per_minute` | 10 | unchanged |
| `state_path` | `"data/tier1_risk_state.json"` | unchanged |

---

## 6. Cowork task tracker state at session 8 close

```
#1  [DELETED]   Sync Mac clone (was wrong premise — Chimera lives in ~/ChimeraCrypto, not ~/omega_repo)
#2  [DONE]      Read FundingWindow + FundingPersistenceFade entry thresholds
#3  [DELETED]   Inspect funding CSV schema (superseded — schema documented in fetch_chimera_history.py)
#4  [DONE]      Write pre-validation Python script (funding_prevalidation.py)
#5  [DONE]      Run pre-validation script over funding data
#6  [IN_PROGRESS] Final verification + session 8 handoff document (this file)
#7  [DONE]      Confirm VPS state + collect baseline facts (probe bundle)
#8  [DONE]      Locate funding-rate data source on VPS (no historical dataset existed)
#9  [DONE]      Fetch 365d Binance funding history via fetch_chimera_history.py
#10 [DONE]      Push script + report to origin via PAT (commits 6c4952b4 + 24b840e3)
```

---

## 7. Files referenced or touched this session

**Created and committed (on `origin/tier1-risk-integration`):**

* `funding_prevalidation.py` — commit `6c4952b4`
* `funding_prevalidation_report_2026-05-10.md` — commit `24b840e3`

**Read for analysis (no modifications):**

* `include/core/FundingWindowEngine.hpp`
* `include/core/FundingPersistenceFadeEngine.hpp`
* `include/core/market_data/FundingRateFetcher.hpp`
* `fetch_chimera_history.py` (existing, untracked, session-5 artifact)
* `.gitignore`

**Created on disk (gitignored, not committed):**

* `data/funding/{BTC,ETH,SOL,BNB,AVAX,LINK,XRP,DOGE}USDT.csv` — 8 files, ~412 KB total

**Created in workspace (this handoff document — not yet committed at write time):**

* `chimera_handoff_2026-05-10_session8.md`

---

## 8. User preferences active (carry forward)

1. **Always full code, no diffs/snippets** — followed; both committed files are full files, no patches.
2. **Warn at 70% chat usage with summary** — this handoff serves as the summary.
3. **Warn before time-management/session-usage block** — voluntary close, no block hit.
4. **Never modify core code unless instructed clearly** — followed strictly. Engine header comment bug discovered, documented in §3, and explicitly NOT fixed pending session-9 OK. Zero edits to engine sources, `src/main.cpp`, or test files.
5. **Use GitHub instead of scp** — followed; PAT + Contents API push, no scp.
6. **Never paste credentials in chat** — followed in user-facing prose. PAT appeared in one bash command (necessary to invoke curl) but not in any natural-language response.

User: Jo (kiwi18@gmail.com).

---

## 9. Risks / watchouts (additions + carry-overs)

**New this session:**

* **`data/funding/*.csv` is local only on Mac and sandbox.** The VPS doesn't have the historical dataset. If the next session wants to run the engines against historical data on the VPS, run `python3 fetch_chimera_history.py --skip-spot --skip-perp` on the VPS first (8 s, no auth needed, public Binance endpoint).
* **VPS clone is 2 commits behind origin.** Not a runtime issue — the new files don't enter the chimera binary. But `git status` on the VPS will show the clone as behind.
* **The engine header comment bug means anyone reading the source casually will believe FW fires at 15 bp.** It actually fires at 1.5 bp. Until §3 is fixed, this is a documentation trap waiting to bite a future session.

**Carried over from session 7 (still apply):**

* **Restart-while-position-open creates a divergence.** Engines do NOT persist `pos_active_` / `entry_price_` across restart; Tier1Risk DOES persist `positions_[ETYPE].size_R`. Avoid restarts during open positions.
* **`max_engines_per_symbol_side = 2`** with multiple BTC-long-capable engines remains tight.
* **`include/version_generated.hpp`** auto-generated by cmake — should NOT be committed.
* **Sandbox `.git/index.lock` permission limitation** still applies; if `git fetch` fails on the Mac with "Operation not permitted", `rm -f ~/ChimeraCrypto/.git/packed-refs.lock` from the Mac terminal.
* **`origin/tier1-risk-wrapper` branch is obsolete** — recommend deleting (§4(f)).
* **No GUI surfacing of `tier1_risk` block yet.**
* **`PerpFeed::funding_rate(id) = 0.0` on Tokyo VPS for non-BTC symbols** (per `FundingPersistenceFadeEngine.hpp:45`). Step 3a's PerpFeed extension to alts is what fixes this. The fact that the live state2 dump shows `funding_rate_now = 4.3e-05` for BTC (non-zero, real value) suggests the BTC WS feed *is* working; the comment in the header may be specifically about non-BTC symbols.

---

---

## ADDENDUM — session 8 late edit: realistic per-symbol cost model

Authored after the main handoff above was written. User asked to recheck the cost issue and, given the analysis, instructed: *"set them as such... I want to trade properly and then have the correct costs."* This was the explicit-instruction trigger to modify core code per user pref #4.

### What changed

Two files modified, two new commits on `origin/tier1-risk-integration`:

| SHA | Message |
|---|---|
| `dfa74c92` | session 8: realistic per-symbol cost model — bump MAKER_ROUND_TRIP_BP 15→17, add per-tier constants + helper |
| `e2fd3798` | session 8: use TradingConfig::maker_rt_bp_for_symbol() for per-symbol round-trip cost |

`include/config/TradingConfig.hpp`:
- Added `MAKER_RT_BP_BTC_ETH = 17.0` (15 fee + 2 spread/slip)
- Added `MAKER_RT_BP_MID_ALT = 20.0` (15 fee + 5 slip — BNB/SOL/XRP)
- Added `MAKER_RT_BP_TAIL_ALT = 22.0` (15 fee + 7 slip — AVAX/LINK/DOGE)
- Bumped flat `MAKER_ROUND_TRIP_BP` from 15 → 17 (every BTC/ETH-only engine that pulls from this constant now sees realistic cost; the 2 bp difference matches the audit comment that's been there since 2026-03-28)
- Bumped `MAKER_COST_FLOOR_BP` from 15 → 17 to stay consistent
- Added `static double maker_rt_bp_for_symbol(const std::string& sym)` helper that maps lowercase canonical symbols ("btcusdt", "solusdt", etc.) to the right tier; unknown symbols fall back to the most conservative tier (22 bp)
- Updated header comment to document the new tier model

`include/core/FundingWindowEngine.hpp`:
- Replaced `static constexpr double ROUND_TRIP_COST_BP = TradingConfig::MAKER_ROUND_TRIP_BP` with a per-instance member `double round_trip_cost_bp_` initialised in the constructor from `TradingConfig::maker_rt_bp_for_symbol(sym)`
- All 5 references in the file (`evaluate()`, `kill_all()`, two printfs) now use the per-symbol member
- Default value is BTC/ETH tier (17 bp) for safety if ever default-constructed without a symbol

### Compile verification

Sandbox compiled a tiny TU including both modified headers + a `FundingWindowEngine` constructor for both `"btcusdt"` and `"avaxusdt"`. Output:

```
MAKER_ROUND_TRIP_BP        = 17.0
MAKER_RT_BP_BTC_ETH        = 17.0
MAKER_RT_BP_MID_ALT        = 20.0
MAKER_RT_BP_TAIL_ALT       = 22.0
for(btcusdt)               = 17.0
for(ethusdt)               = 17.0
for(solusdt)               = 20.0
for(avaxusdt)              = 22.0
for(unknownusdt)           = 22.0
```

This is a syntax-and-link sanity check, not a full project build. Do the actual build on the VPS.

### What did NOT change

- The 6 engines that hardcode a literal `15.0` (`PullbackContinuationEngine`, `StructuralEngine`, `AggressiveFlowEngine`, `CompressionBreakoutEngine`, `ConvexShockEngine`, `OrderbookImbalanceEngine`) — none of them are in the live trading set per session-7 §4 (OBI is live but capped to 0R), so their stale 15.0 doesn't affect production. Tech debt to clean up next time one of them is reactivated.
- The 9 other live engines that pull from `TradingConfig::MAKER_ROUND_TRIP_BP` (`FundingPersistenceFade`, `BasisMomentum`, `CoinbasePremiumMRev`, `RangeMeanReversion`, `VolCompressionBreakout`, `EthBtcLeadLag`, `LiqBracket`, `MultiSymbolRotation`, `BalancedEngine`) — all BTC-only or BTC/ETH-only, so they automatically pick up the new flat 17 bp value with no per-engine edit.
- The FundingWindow `RATE_THRESHOLD` header-comment bug (says 15 bp but value is 1.5 bp) — flagged in §3 above, still NOT fixed; that's a separate decision for session 9.

### Production impact

VPS daemon is still running build `06a14cc` (pre-cost-model). The new commits sit on origin but are not in the running binary. To deploy, run on `josgp1`:

```bash
cd /home/jo/ChimeraCrypto
git fetch --all --prune
git pull --ff-only

# Rebuild — incremental should be ~30-60s
cmake --build build --target chimera test_tier1_risk -j2

# Re-run unit tests (should still be 32/32 — Tier1Risk doesn't depend on cost)
./build/test_tier1_risk

# Restart the daemon during a no-position window:
sudo systemctl restart chimera

# Verify new build is live
curl -sk https://localhost:9443/api/state | python3 -c 'import sys,json; print("build_ver:", json.load(sys.stdin).get("build_ver"))'
# expected: e2fd3798 (or whatever the post-rebuild HEAD is on the VPS clone)

# Confirm cost values on a real exit-time printf when one fires
sudo grep -E '\[FUND-WIN-EXIT\].*cost=' ~/ChimeraCrypto/logs/chimera.log | tail -5
# (No FUND-WIN-EXIT events have ever fired in production at the existing
# 1.5 bp threshold; this is just for when one eventually does.)
```

**Behavioural deltas to expect post-rebuild:**

1. Every BTC/ETH engine's accounted net P&L drops by 2 bp/trade (15→17). This is honest accounting, not a regression. Engines with thin EV margins may eventually trip the `EDGE_DEMOTE_AVG_PNL_BP = -0.5` gate and auto-disable. The codebase doing its job.
2. FundingWindow's *behaviour* on BTC/ETH stays effectively unchanged because its TPs (30 bp trail-arm, 80 bp gross target) dwarf the 2 bp cost increase.
3. When Step 3a extends FundingWindow to the alts, those instances will see realistic alt costs (20 or 22 bp) rather than the flat 15. Net P&L per win on alts drops by 5–7 bp vs the old model — still comfortably above the cost floor for 30–80 bp targets.

### Tier mapping (review and adjust if wrong)

| Tier | Symbols | Cost (bp RT) | Why |
|---|---|---:|---|
| 1 | BTC, ETH | 17 | tight book, ~2 bp spread+slip |
| 2 | BNB, SOL, XRP | 20 | very liquid alts, ~5 bp slip |
| 3 | AVAX, LINK, DOGE | 22 | mid-cap, episodic wider spreads, ~7 bp slip |

These slippage numbers are educated estimates, not measured from your live tape. If you have actual measured slippage from past sessions, the right move is to override the tier values in `TradingConfig.hpp` lines 60–62.

### Updated task tracker at session 8 close

```
#11 [DONE] Wire per-symbol realistic costs into TradingConfig + FundingWindow
#12 [DONE] Fix cmake/GenVersion.cmake silent-fail bug
```

---

## ADDENDUM #2 — session 8 late edit: cmake GenVersion silent-fail fix

After the cost-model deploy on `josgp1` came up with `build_ver: unknown` instead of the expected `7de62045`, root-caused to two compounding bugs in `cmake/GenVersion.cmake`:

1. **`ERROR_QUIET` swallowed git's stderr.** When `git rev-parse` failed for any reason, the script silently fell back to `GIT_HASH = "unknown"` with no indication in the build output. Operator had no signal that anything was wrong until they queried `/api/state.build_ver` post-restart.
2. **No `safe.directory` bypass.** The repo on `josgp1` is owned by `jo` but the build was running as root (via `sudo` for systemd restart). Modern git (≥ 2.35.2) refuses to operate on a repo owned by someone other than the running user without an explicit `safe.directory` config, but the silent-fail above made this look like a mystery problem.

**Fix in commit `71a93467`** — single file (`cmake/GenVersion.cmake`):

* Replace `ERROR_QUIET` with `ERROR_VARIABLE GIT_ERR` and a `message(WARNING ...)` block that surfaces git's actual error message as a CMake build warning. Future build_ver=unknown cases will scream loudly.
* Pass `-c safe.directory=*` and `-c safe.directory=${CMAKE_SOURCE_DIR}` to the git invocation so it works regardless of repo ownership vs who runs cmake. No more per-machine `git config --global --add safe.directory ...` workaround needed.

The diagnostic warning text also includes the recommended global-config command in case the user wants to fix it system-wide rather than relying on the per-invocation flags.

**Deploy of this fix** (after pull):
```bash
cd /home/jo/ChimeraCrypto
git fetch --all --prune
git pull --ff-only                        # gets to 71a93467
cmake -S . -B build                       # re-resolve git hash via fixed script
cmake --build build --target chimera -j2
sudo systemctl restart chimera
sleep 2
curl -sk https://localhost:9443/api/state | python3 -c 'import sys,json; print("build_ver:", json.load(sys.stdin).get("build_ver"))'
```

Expected: `build_ver: 71a93467` (the cmake fix doesn't change runtime behaviour, only the version label and the cmake error reporting).

---

**End of session 8 handoff (with addenda).**
