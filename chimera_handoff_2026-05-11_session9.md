# Chimera — Session Handoff #9
**Date:** 2026-05-11 (continuation of work begun late on 2026-05-10 UTC)
**Author:** Claude (Cowork session)
**For:** continuing Chimera work in a fresh chat
**How to use:** paste this entire document into the first message of the new chat as context, then continue from §6 "Open questions / next steps".
**Predecessor:** `chimera_handoff_2026-05-10_session8.md` (still valid for engine internals, the funding pre-validation analysis, the cost-model rollout, and the FundingWindow header-comment bug; superseded by this one for: live origin/VPS state, the new FPF cap commit, the SwingEngine trail-arm design observation).

---

## 0. TL;DR for the receiving session

Session 9 was a short execution session focused on closing the §4(a) FPF decision from session 8. Three things happened:

1. **FPF decision implemented and deployed.** Option A from the session-8 menu — defer (`per_engine_r_cap[FUNDING_PERSIST_FADE] = 0.0`) — was committed as `c4aff38` and is now live on `josgp1` after a clean restart at 2026-05-10 10:53 UTC. All 9 commits from session 8 (cost-model bumps, FW per-symbol wiring, GenVersion fix, pre-validation tooling, 3 handoff markdowns) deployed alongside the FPF cap. The 5-point plan now reads: Step 3 split into 3a (FW alt extension, ready to engineer) and 3b (**closed** — FPF deferred via 0R cap).

2. **Deploy verification clean.** `build_ver = c4aff38`, both caps print at startup, all 8 other engines at 1.0R default, 32/32 `test_tier1_risk` scenarios pass, daemon active with `halted=false`, all 10 engines flat at restart. The empty `per_engine_r_cap` JSON in `/api/state2` is a known observability gap (§4f of session 8) — caps are baked into the binary and printed at boot, just not exposed via the API.

3. **SwingEngine trail-arm design issue surfaced and deferred.** A live paper ETH SwingEngine position peaked at +95.8 bp and gave back to -98.1 bp net. Root cause: SwingEngine v9's `S1_TRAIL_ARM_ATR = 2.0` requires a `2×ATR4h` favourable move to arm; at ETH's current `ATR4h ≈ 91 bp` that's a `+182 bp` arm threshold, so a +95.8 bp peak is below it. The trail literally cannot engage. The GUI displays "arming at +30bp" which is **misleading** — that string is sourced from a position-state field (`trail_arm_bp`) that does not reflect the active S1 strategy's actual threshold. This is a real strategy-vs-cost-model coherence concern — we just spent session 8 tightening cost accounting, while SwingEngine's design philosophy permits 100+ bp give-back from peak — but **deferred at user request** in favour of pivoting to Omega. No code change was made to SwingEngine or the GUI. The position is paper (shadow_mode = true); no real-money loss.

---

## 1. What changed this session

### Branch + commits

* **Branch:** `tier1-risk-integration` (unchanged from session 8).
* **New commit on `origin/tier1-risk-integration`:**

| SHA | Message |
|---|---|
| `c4aff38` | Tier1Risk: cap FUNDING_PERSIST_FADE per-engine R to 0.0 (session 8) |

  The commit message is dated "session 8" because the FPF decision was the open item from session 8's TL;DR — keeping the label consistent with the analytical framing. The execution happened in session 9.

* **Pushed to:** `origin/tier1-risk-integration` via `git push` from Jo's Mac terminal (not PAT this session — the user's normal git credentials worked, no scp).
* **Live binary on `josgp1`:** **built from `c4aff38`** (the new HEAD). Restart timestamp: 2026-05-10 10:53:19 UTC.

### Files touched (single commit, single file)

| Path | Change |
|---|---|
| `src/main.cpp` | +17 / -1. Inserted a session-8-style explanatory comment block (-10 bp 24h-avg trigger inert, 365d × 8 symbols = 1 candidate entry, anti-overfitting policy), added `risk_cfg.per_engine_r_cap[(int)chimera::risk::EngineType::FUNDING_PERSIST_FADE] = 0.0;` immediately after the existing OBI cap line, added a parallel `[STARTUP] Tier1Risk FUNDING_PERSIST_FADE cap = 0.0R (session 8 — deferred, regime-inert)` printf. The `-1` is the OBI printf reformatted for column alignment with the new FPF printf — pure cosmetic. |

### Engine source code — NOT touched (other than the line above)

Per user preference #4 ("never modify core code unless instructed clearly"), this session made zero edits to:

* `include/core/SwingEngine.hpp` (despite the trail-arm question — explicitly deferred)
* `include/core/FundingWindowEngine.hpp` (the 15 bp/8h header comment bug from session-8 §3 — still pending an explicit OK)
* `include/core/FundingPersistenceFadeEngine.hpp`
* `include/config/TradingConfig.hpp` (the cost-model bumps were already committed in session 8 via dfa74c9 + e2fd379)
* `cmake/GenVersion.cmake` (already fixed in session 8 via 71a9346)
* Any GUI file (the "arming at +30bp" misleading label is documented but unchanged)
* Any test file (32/32 still passing on `c4aff38`)

### Tests

`test_tier1_risk`: 32 passed / 0 failed on `c4aff38` on the VPS.

### Pre-existing work-in-progress on the Mac repo

Session 9 found the Mac clone of `~/ChimeraCrypto` had four uncommitted source-file edits in the working tree at session start (cost-model + FW per-symbol + cmake fix + auto-generated version_generated.hpp) plus several untracked files. These were stashed under `stash@{0}` before applying the FPF cap, and **dropped** after the rebase against origin succeeded (because the same edits were already committed to origin as `dfa74c9` + `e2fd379` + `71a9346` and would have generated trivial-but-noisy conflicts on stash-pop). No content was lost — the on-disk state matches origin exactly post-rebase.

The Mac fetch at session start had been silently failing on `origin/tier1-risk-integration` ref updates due to sandbox permission warnings on `.git/objects/*` temp files; this confused the diagnosis briefly. The handoff was right that the 8 session-8 commits were on origin; the Mac clone just hadn't been able to pull them. Once the index lock was cleared and fetch retried, everything resolved cleanly.

---

## 2. Production state right now

| Aspect | Session 8 close | Session 9 close |
|---|---|---|
| VPS hostname | `josgp1` | unchanged |
| VPS IP | 143.198.89.54 | unchanged |
| **VPS HEAD `tier1-risk-integration`** | `06a14cc` (built) | **`c4aff38` (built, live)** |
| **Origin HEAD `tier1-risk-integration`** | `5499574` (3 ahead of VPS) | **`c4aff38`** (in sync with VPS) |
| Live binary built from | `06a14cc` | **`c4aff38`** |
| Daemon halted? | `false` | `false` (unchanged) |
| `daily_realized_bp` | -213.559 (stale) | -213.559 (still stale — no position close has rolled the day) |
| `OBI-ENTRY` count in chimera.log | 412 | 412 (no further entries — cap working) |
| **`FUNDING_PERSIST_FADE` cap** | not set (default 1.0R) | **0.0R (session 9 deploy)** |
| OBI per-engine R cap | 0.0 | unchanged 0.0 |
| All other per_engine_r_cap | 1.0 | unchanged 1.0 |
| Spot account balance | BTC 0.000499 + USDT 68.71 ≈ $108 | unchanged (no live trades since session-6 close) |
| Open positions at session close | 0 | 1 (ETH paper SwingEngine, see §3) |

**The Mac clone is in sync** with origin and the VPS — same `c4aff38`. No clean-up needed.

---

## 3. The SwingEngine trail-arm design observation (flagged, not fixed)

Live ETH paper position observed during the session:

* Entry log line: `[SWING-SHADOW] ETH LONG S1 entry=2347.5550 sl=2309.0579 tp=0.0000 atr4h=21.3873 rsi=59.1 qty=0.21298`
* GUI live-positions row at observation: `ETH LIQ -83.1bp, peak +95.8bp, -98.1bp net, arming at +30bp...`

### Engine actually does

From `include/core/SwingEngine.hpp:378`: `static constexpr double S1_TRAIL_ARM_ATR = 2.0;` — arm condition is `move >= 2.0 × h4.atr14` (line 1180).

For this ETH position, `atr4h = 21.3873` absolute price units. Entry price 2347.5550 puts ATR in bp at `(21.3873 / 2347.5550) × 10000 ≈ 91.1 bp`. The arm threshold for this position is therefore `2 × 91.1 = ~182 bp`. The peak of +95.8 bp was **about half-way** to that threshold. Trail never armed → no protection → position is drifting toward the 163.9 bp ATR stop at 2309.0579.

The full 4-stage trail progression (all measured in `move / atr14` from entry):

| Stage | Trigger | Trail distance from MFE |
|---|---|---|
| 1 (arm) | ≥ 2.0× ATR | 1.5× ATR (also capped at 40% of MFE) |
| 2 | ≥ 3.0× ATR | 1.0× ATR |
| 3 (LOCKED) | ≥ 8.0× ATR | 0.5× ATR |

For ETH @ 91 bp ATR, those translate to: arm at +182 bp, promote at +273 bp, LOCKED at +728 bp. The engine's design clearly targets *very large* winners; anything that peaks below 2× ATR gets full-stopped on retrace.

### GUI shows

From `gui/app.js:415`:
```js
else if(mfe>=p.trail_arm_bp*0.6){trailTxt="arming at +"+p.trail_arm_bp+"bp...";trailClass="armed";}
else{trailTxt="SL -"+p.sl_bp+"bp  |  trail arms at +"+p.trail_arm_bp+"bp";}
```

The string `"arming at +30bp..."` is rendered using `p.trail_arm_bp = 30`. The engine is publishing `trail_arm_bp = 30` to its position-state JSON, **not the actual 2×ATR ≈ 182 bp** that S1 uses. This is the mismatch the user spotted operationally: the GUI implies the trail is about to engage, but the engine's real arm threshold is ~6× further away.

Two possibilities for where `trail_arm_bp = 30` comes from (neither verified — investigation was deferred):
1. A stale or hard-coded default in SwingEngine's state-emit code (around line 603–615 in `SwingEngine.hpp`), possibly meant for the bracket (S2) strategy, not S1.
2. A separate near-term protective stop / break-even threshold that's smaller than the trail arm. SwingEngine code references `BE_HIT` as an exit reason (line 1323 area), implying there *is* break-even logic that may report a smaller threshold.

### The strategic coherence question

User flagged this as **"counter to what we are doing"** and the framing is fair. Session 8 tightened cost accounting (15 → 17 bp baseline, 20/22 bp tiers on alts) — every basis point matters more. Meanwhile SwingEngine v9's trail design tolerates a +95 bp peak round-tripping to a -98 bp loss without any protective action. The 2× ATR arm was set under the old 15 bp cost regime; it has not been re-evaluated against the new cost reality.

**Open strategy question for a future session (NOT for execution without explicit instruction per pref #4):**

> Should `S1_TRAIL_ARM_ATR` be lowered (e.g., 0.75 or 1.0) so the trail engages on smaller wins, accepting more noise-exits as the trade-off?

Inline comments in `SwingEngine.hpp` lines 67–132 document multiple prior iterations where tighter trails got whipsawed; v9 settled on 2.0× ATR specifically to avoid that. A retune would need forward-shadow data showing that the current regime's winners would survive a tighter trail without being noise-exited prematurely. The session-9 user decision was to **defer this conversation** until there is meaningful forward data (likely after Step 3a deploys and we have alt FW exits to compare against).

### What was explicitly NOT done in session 9 about this

* No edit to `SwingEngine.hpp` constants.
* No edit to `gui/app.js` to fix the "+30bp" label.
* No structured "MFE peak vs final exit" reporter added.

All three deferred at user's explicit request to pivot to Omega.

---

## 4. Open questions / next steps for the receiving session (Chimera)

The user pivoted to Omega at session-9 close. None of these are urgent; they are the carryover pile for the next time Chimera comes back to the front burner.

### (a) Decide: tighten SwingEngine trail-arm? (NEW, from §3 above)

After enough forward data accumulates. Options:
* **Status quo (2.0× ATR):** accept the give-back behaviour, count on big winners to compensate.
* **Tighten arm (e.g., 0.75–1.0× ATR):** lock in profit faster, accept more noise-exits. Would also tighten the MFE_TRAIL_FRAC (currently 0.40) for consistency.
* **Add break-even arm:** preserve the 2.0× ATR full trail arm but introduce an earlier "move SL to entry" step at, say, 1.0× ATR. Cheapest design change, biggest psychological/operational win — never let a meaningful winner become a full loss.
* **Fix GUI label first:** the "+30 bp" misleading display is a separate small fix worth doing whatever else is decided.

Worth one chat in a future session, with data in hand.

### (b) FW header-comment bug (carry-over from session-8 §3)

`include/core/FundingWindowEngine.hpp` lines 25 + 73 say "15 bp/8h" but the constant `0.00015` is 1.5 bp/8h. Two-character doc fix. Still awaiting explicit OK per pref #4. Trivial.

### (c) Step 3a: FundingWindow extension to 6 alts (carry-over from session-8 §4b)

The bigger engineering pass. Touches:
* `src/main.cpp` — add `FundingWindowEngine fw_solusdt("solusdt"), fw_bnbusdt(...), ...` instances, wire each `set_risk(&risk)`, add to per-tick dispatch. Mirror BTC/ETH pattern.
* `include/live/BinanceWSFeed.hpp` (or `PerpFeed`) — extend WS subscriptions to include the 6 alt perp symbols so `funding_rate(id)` and `basis_bp(id, spot)` return real values for them. This is the bulk of the work; need to confirm exactly which subscription calls need to fan out.
* No changes to FundingWindowEngine itself — already symbol-agnostic.
* Rebuild + 32/32 tests + restart during no-position window.

Pre-validation projected ~281 entries/year across the 6 alts vs current 13/year on BTC+ETH only.

### (d) Step 4: 4–8 weeks forward-shadow data on alt FW (passive wait after §c)

With Tier1Risk wired you get per-engine P&L attribution automatically. Watch: how often does the 1.5 bp rate gate coincide with the 3 bp basis gate? Live data tells the true conjoint frequency.

### (e) Step 5: per-symbol cost-tuning decisions (unchanged from session-8 §4e)

Pass criterion suggested by session-8 report: net P&L > 0 after the now-tier-aware round-trip cost (17/20/22 bp) on ≥ 30 trades per symbol.

### (f) Cleanup carryovers (still open from session 8)

* Top up Binance spot wallet to $500–1000 (was $108 at session-8 close).
* External uptime monitoring (UptimeRobot, 5 min ping).
* Delete obsolete `origin/tier1-risk-wrapper`.
* GUI surfacing of `tier1_risk.per_engine_r_cap` block — currently empty in `/api/state2` JSON despite caps being printed at startup (session-9 confirmed this gap). Would unblock cap-state monitoring without grepping log files.
* Update `CLAUDE.md` to list both Omega and Chimera repos so a fresh chat doesn't blunder into the wrong tree (session-8 caveat — still applies).

### (g) Optional housekeeping

* The chimera.log file ownership flipped to `root:root` briefly during session-9 deploy (root started the systemd service). Was chown'd back to `jo:jo` before close. Future restarts as root will flip it again — possibly worth a systemd `User=jo` directive, but only if it actually causes operational pain.
* The `daily_realized_bp = -213.559` figure has been stale across 3 restarts now (session 6, 7, 9). It will roll on the next position close per the engine's logic. If positions stay flat for a long time, this value will misleadingly suggest a recent daily loss when in fact it's from May 10 session 6.

---

## 5. Tier1Risk configuration (live on c4aff38)

| Field | Value | Notes |
|---|---|---|
| `per_engine_r_cap[SWING]` | 1.0 | default |
| `per_engine_r_cap[FUNDING_WINDOW]` | 1.0 | default |
| `per_engine_r_cap[BASIS_MOMENTUM]` | 1.0 | default |
| `per_engine_r_cap[OBI]` | **0.0** | session 7 — disabled pending review |
| `per_engine_r_cap[ETH_BTC_LEADLAG]` | 1.0 | default |
| `per_engine_r_cap[COINBASE_PREMIUM_MREV]` | 1.0 | default |
| `per_engine_r_cap[FUNDING_PERSIST_FADE]` | **0.0** | **session 9 — deferred, regime-inert** |
| `per_engine_r_cap[VOL_COMPRESSION_BREAKOUT]` | 1.0 | default |
| `per_engine_r_cap[RANGE_MEAN_REVERSION]` | 1.0 | default |
| `per_engine_r_cap[MULTI_SYMBOL_ROTATION]` | 1.0 | default |
| `total_r_cap` | 3.0 | default |
| `daily_loss_kill_bp` | -200.0 | unchanged |
| `max_engines_per_symbol_side` | 2 | unchanged |
| `max_orders_per_minute` | 10 | unchanged |
| `state_path` | `data/tier1_risk_state.json` | unchanged |

---

## 6. Cowork task tracker state at session 9 close

```
#1 [DONE]      Verify origin state vs handoff claims (found mismatch — sandbox fetch had failed silently)
#2 [DONE]      Locate per_engine_r_cap config in src/main.cpp
#3 [DONE]      Apply FPF cap to src/main.cpp + write full file (commit c4aff38 — formerly fe5954b, rebased)
#4 [DONE]      Verify enum value for FPF EngineType (FUNDING_PERSIST_FADE = 6)
#5 [DONE]      Provide commit + VPS deploy commands
#6 [DONE]      Final verification step (build_ver, startup banner, tests, daemon active)
#7 [DONE]      Diagnose ETH SwingEngine trail-arm failure (root cause: 2×ATR ≈ 182 bp vs +95.8 bp peak)
#8 [DEFERRED]  Resolve GUI vs engine trail-arm mismatch (pivot to Omega)
```

---

## 7. Files referenced or touched this session

**Created and committed (on `origin/tier1-risk-integration`):**

* `src/main.cpp` — modified, commit `c4aff38`

**Read for analysis (no modifications):**

* `include/risk/Tier1Risk.hpp` — confirmed `FUNDING_PERSIST_FADE = 6` in `EngineType` enum
* `include/core/SwingEngine.hpp` — trail-arm constants, bar-close gating comments, position-state emit fields
* `tools/test/test_tier1_risk.cpp` — confirmed tests `fill(1.0)` explicitly and are independent of the FPF cap default
* `gui/app.js` — found the "arming at +30bp" rendering at line 415

**Created in workspace (this handoff document):**

* `chimera_handoff_2026-05-11_session9.md` — this file

---

## 8. User preferences active (carry forward unchanged from session 8)

1. **Always full code, no diffs/snippets** — followed; the single committed file (`src/main.cpp`) is full file in the repo, snapshot also placed in sandbox outputs.
2. **Warn at 70% chat usage with summary** — this handoff serves as the summary at session close.
3. **Warn before time-management/session-usage block** — voluntary close, no block.
4. **Never modify core code unless instructed clearly** — followed strictly. SwingEngine trail-arm change explicitly deferred despite analytical concern. FW header-comment bug still pending explicit OK. GUI "+30bp" misleading label not touched.
5. **Use GitHub instead of scp** — followed; standard `git push` from Mac terminal worked, no PAT needed this session.
6. **Never paste credentials in chat** — followed.

User: Jo (kiwi18@gmail.com).

---

## 9. Risks / watchouts (additions + carry-overs)

**New this session:**

* **SwingEngine trail-arm coherence vs cost model.** Documented in §3. Not a code bug; a strategy-design question the new tighter cost accounting (deployed via session 8 cost-model commits) makes more urgent. Decision deferred to future Chimera session with forward data.
* **GUI "arming at +30bp" is misleading** when the actual S1 arm threshold is +182 bp on ETH at current ATR. If anyone is making operational decisions watching the GUI rather than the logs, they'll be surprised when expected trail-stops don't fire. Worth a small GUI fix or a SwingEngine state-emit fix in a future session.
* **Cost-model bump (dfa74c9 + e2fd379) now live.** Every BTC/ETH engine's accounted net P&L is 2 bp tighter than before. Watch chimera.log for `[EDGE-DEMOTE]` lines in the next few days — engines that auto-disable are the cost model doing its job, not a regression. Re-evaluate per-engine viability when demotions accumulate.
* **`daily_realized_bp = -213.559` persists across all restarts since session 6.** Will roll on next position close. If positions stay flat for many days, this value continues to misleadingly suggest a recent daily loss.

**Carried over from session 8 (still apply):**

* Restart-while-position-open creates an engine/Tier1Risk persistence divergence. Always restart during no-position window. Session 9 did this correctly.
* `max_engines_per_symbol_side = 2` with multiple BTC-long-capable engines remains tight.
* `include/version_generated.hpp` is auto-generated by cmake — should NOT be committed.
* The Mac sandbox `.git/index.lock` permission limitation is known: if a sandbox-side `git` op fails with "Operation not permitted" on `.git/objects/*`, run `rm -f ~/ChimeraCrypto/.git/index.lock` from the Mac terminal (or use the `allow_cowork_file_delete` tool from a Cowork session).
* `origin/tier1-risk-wrapper` branch is obsolete — recommend deleting.
* No GUI surfacing of `tier1_risk.per_engine_r_cap` block — caps are printed at startup banner only, not exposed in `/api/state2` JSON.
* `PerpFeed::funding_rate(id) = 0.0` on Tokyo VPS for non-BTC symbols. Step 3a's PerpFeed extension to alts fixes this.

---

## 10. Quick-start for the next Chimera chat

If you're a future Claude picking this up: paste this whole document, then ask the user one of:

1. *"Want to fix the FW header comment bug first (trivial), or jump to Step 3a (alt FW extension)?"*
2. *"Want to take another look at the SwingEngine trail-arm coherence question, or wait for more forward data?"*
3. *"Or are we still on the Omega pivot and Chimera stays parked?"*

State of the world they should know:

* Production is healthy on `c4aff38`. Don't redeploy unless you have a code reason.
* Two engines are 0R-capped (OBI session 7, FPF session 9). All others are at default 1.0R.
* Cost model is tighter than it was 2 sessions ago. Watch for auto-demotions.
* One paper position was open at session-9 close (ETH SwingEngine, drawing down toward stop). It is not real money. Check `/api/state2` for current state.

---

**End of session 9 handoff.**
