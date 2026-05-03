# Move 2 Implementation — Wire Notes

**Status:** Code changes complete for FundingWindow + BasisMomentum + OBI, all paper-only in shadow mode. SwingEngine v9 is unchanged (per HANDOFF.md guardrail). Awaiting deploy.

**Hard constraint reminder:** SPOT ONLY. No perps, no margin, no shorting, no options. Perp data (PerpFeed) is used as a *signal source* only — every trade is a spot LONG.

---

## What changed in this session

Five files modified, no new files in `src/` or `include/`:

1. **`include/core/FundingWindowEngine.hpp`** — additive wrappers (`shadow_mode`, `kill_all`, `state_json`, `on_tick` adapter). `evaluate()` byte-for-byte unchanged.
2. **`include/core/BasisMomentumEngine.hpp`** — same additive wrapper pattern as FundingWindow. `evaluate()` byte-for-byte unchanged.
3. **`include/core/OrderbookImbalanceEngine.hpp`** — same additive wrapper pattern. `evaluate()` byte-for-byte unchanged.
4. **`src/main.cpp`** — instantiates SwingEngine (live shadow) + 3 paper engines (FundingWindow, BasisMomentum, OBI), each on BTC + ETH. Routes ticks through all of them. Wires `PerpFeed`. New `/api/state2` returns a structured JSON dict with one array per paper engine. `/api/kill` flattens every engine.
5. **`gui/index.html`** — banner subtitle changed `QUAD ENGINE` → `CRYPTO Engine`.

`CMakeLists.txt` needs no change — every modified header is header-only and `PerpFeed.cpp` was already in the build.

### Known limitations baked into this batch (intentional, called out)

- **OBI's `regime` is hardcoded to 1 (GRIND)** at the call site in `main.cpp`. The other gates (book imbalance ≥ 0.55, spread ≤ 2.5bp, perp basis ≤ 8bp) are doing the actual filtering. Once a real per-symbol regime classifier exists outside SwingEngine, swap the hardcode for the real value.
- **OBI's and BasisMomentum's `vol_ratio` inputs are hardcoded** (1.5 and 1.0 respectively) — both above their gate thresholds. Same fix path as regime.
- **OBI's `state_json()` reports book_imbalance and spread as 0** because `main.cpp` doesn't cache the latest tick outside the SwingEngine. Accurate per-tick values are in the `[OBI-ENTRY]` log lines. Fix would be to cache tick.book_imbalance / tick.spread_bps per symbol.
- **`available_R` is hardcoded to 1.0** for all paper engines — placeholder until the Tier 1 risk wrapper exists.

---

## Deferred to follow-up sessions

- **SessionMomentumEngine.** Needs a small `PositionTracker` helper because — unlike the other Tier 2 engines — it only emits entry signals via `check_signal()` and doesn't manage its own positions. Wiring it would require more `main.cpp` surgery than we want in the same batch as 3 other engines.
- **chimera_backtest extension for the new engines.** Currently `tools/backtest/replay.cpp` only drives SwingEngine. To validate the 3 new engines on history we need: (a) inject a virtual time source into `FundingWindowEngine::seconds_to_funding()` so it can replay historical funding boundaries; (b) fetch historical funding rates via `fapi/v1/fundingRate?symbol=...&limit=1000` paginated; (c) fetch historical perp markPrice / aggTrade for BasisMomentum's basis + flow inputs; (d) generate / replay depth5 snapshots for OBI's `book_imbalance` + `spread_bps`. Each is a multi-hour task. **Interim verification path: shadow-mode paper logs for 1-2 weeks** — same approach used to validate v9 SwingEngine before its harness existed.
- **Real per-symbol regime classifier + vol_ratio.** Replace the OBI / BasisMomentum hardcodes once we decide the right place to compute them (probably a `RegimeClassifier` helper in `main.cpp` that reads spot price history).
- **Dashboard wiring of `/api/state2`.** The endpoint exists and returns structured JSON, but `gui/app.js` doesn't fetch it yet. For now: `curl http://154.45.251.118:8080/api/state2 | jq`.
- **Tier 1 risk wrapper** (daily loss circuit, correlation-aware sizing, per-engine kill, state persistence, reconciliation). Required before any of these engines move from `shadow_mode = true` to live execution.

---

## Static-check status

I could not run `cmake && make` from the Mac/sandbox environment (the build sandbox was unavailable, and `CMakeLists.txt` is Linux-pathed regardless). The code passes a static cross-check:

- Every `#include` resolves to a file in the repo.
- Every external symbol I call exists in the header it comes from.
- The wrappers added to all three paper engines are additive — original `evaluate()` byte-for-byte unchanged in each.
- `MarketTick` exposes `book_imbalance`, `spread_bps`, `depth_imbalance`, etc. (verified in `live/BinanceWSFeed.hpp`).

**Most-likely-to-trip on the first VPS build, in order:**

1. **`-Wunused-but-set-variable` warnings** on existing engine fields (`entry_rate_`, `entry_basis_`, `entry_secs_` in FundingWindow, similar in BasisMomentum). These were unused in the originals too. Cosmetic.
2. **`PerpFeed` startup race.** `perp_feed.ready(id)` returns false until first markPrice update arrives. The wiring guards on `ready(id)` so paper engines just wait silently — expect zero `[FUND-WIN-*]` / `[BASIS-*]` / `[OBI-*]` lines for ~30 seconds after `[STARTUP]`.
3. **`/api/state2` substring match ordering.** I check `/api/state2` *before* `/api/state` in `http_server_thread`. If you ever swap them, the new endpoint silently breaks.

---

## Deploy + verify on the VPS

This is one deploy. All three paper engines come up together with the GUI banner fix.

```bash
# On Mac
cd ~/ChimeraCrypto/chimera-git
git status
# Expect modified:
#   include/core/FundingWindowEngine.hpp
#   include/core/BasisMomentumEngine.hpp
#   include/core/OrderbookImbalanceEngine.hpp
#   src/main.cpp
#   gui/index.html
#   HANDOFF_MOVE2.md
git add -A
git commit -m "Move 2: wire FundingWindow + BasisMomentum + OBI in shadow mode (BTC+ETH)"
git push origin main

# On VPS
ssh -i ~/.ssh/chimera_ed25519 jo@154.45.251.118
sudo systemctl stop chimera.service
while pgrep -x chimera >/dev/null; do sleep 0.5; done
cd /home/jo/ChimeraCrypto && git pull --ff-only origin main
cd build && make -j"$(nproc)" chimera chimera_backtest
sudo systemctl start chimera.service

# Verify the new wiring (look for these 4 lines under STARTUP)
journalctl -u chimera.service -f --since "30 seconds ago" | grep -E "STARTUP|FUND-WIN|BASIS|OBI|FATAL"
# Expect:
#   [STARTUP] Chimera — Swing + FundingWindow + BasisMomentum + OBI | build=...
#   [STARTUP] Spot feed live. SwingEngine running on 8 symbols (ETH-only trades).
#   [STARTUP] Perp feed live. Paper engines on BTC + ETH:
#   [STARTUP]   - FundingWindow (pre-funding basis snap-back)
#   [STARTUP]   - BasisMomentum (perp→spot lead-lag)
#   [STARTUP]   - OrderbookImbalance (short-term mean-reversion)
#   [STARTUP] All paper engines run in shadow_mode (printf log only, no executor).

# Confirm /api/state2 is reachable and structured
curl -s http://localhost:8080/api/state2 | head -c 800
# Expect a JSON object like:
#   {"funding_window":[{...btcusdt},{...ethusdt}],
#    "basis_momentum":[{...btcusdt},{...ethusdt}],
#    "obi":[{...btcusdt},{...ethusdt}]}
```

If `make` fails on the VPS, paste the first 30 lines of the error here — most-likely cause is a missing `#include` cascade in one of the three engine headers when their inline functions are first instantiated together.

### What to watch over the next 1-2 weeks (the verification phase)

The engines are all paper-only — every "trade" is a printf log line, no real positions, no risk to capital. Goal of this phase is to discover whether each one is actually capturing edge.

```bash
# Watch live for any paper engine activity
sudo journalctl -u chimera.service -f | grep -E "FUND-WIN|BASIS|OBI"
```

Realistic frequencies:

- **FundingWindow:** entries gated to the 3-min window before 00/08/16 UTC. Most funding events pass quietly because the basis/rate thresholds are tight. Expect **a few entries per week** at most.
- **BasisMomentum:** entries fire on basis spikes ≥ 8bp + 12bp delta + flow ≥ 0.30. Should fire **multiple times per day** in active markets, possibly less in dead chop.
- **OBI:** entries fire on extreme book imbalance ≥ 0.55 + tight spread ≤ 2.5bp + perp basis ≤ 8bp. Hard to predict frequency — could be **hourly or could be silent**. The 18-25ms latency caveat means the first entries we see may be losers if the imbalance signal is too stale.

After 1-2 weeks of shadow:

```bash
# Tally per-engine results from the journal
sudo journalctl -u chimera.service --since "2 weeks ago" | grep "FUND-WIN-EXIT" | tail -50
sudo journalctl -u chimera.service --since "2 weeks ago" | grep "BASIS-EXIT"     | tail -50
sudo journalctl -u chimera.service --since "2 weeks ago" | grep "OBI-EXIT"       | tail -50
```

Paste those tail blocks here and we'll calculate per-engine win rate, average net bp, total bp, and decide which (if any) graduate to the next step (proper backtest harness or live execution gating).

### Roll back if needed

```bash
ssh -i ~/.ssh/chimera_ed25519 jo@154.45.251.118
cd /home/jo/ChimeraCrypto
git log --oneline -5                          # find the previous v9 commit hash
sudo systemctl stop chimera.service
git reset --hard <previous-commit-hash>
cd build && make -j"$(nproc)" chimera
sudo systemctl start chimera.service
```

Rollback is clean — SwingEngine is untouched.

---

## What the next session should pick up

Priority order:

1. **Triage 1-2 weeks of paper logs** for FundingWindow / BasisMomentum / OBI. Decide which (if any) show signal. This is the "we still don't have a BTC edge — find one" task; one of these three is the candidate, or none of them are and we move on to other Tier 2 engines.
2. **Wire SessionMomentumEngine** with a thin `PositionTracker` helper since it doesn't manage positions internally.
3. **Build `chimera_backtest_paper`** — a separate binary that replays each paper engine on history. Validates whatever shadow logs suggested.
4. **Replace the regime / vol_ratio hardcodes** in main.cpp with a real per-symbol classifier.
5. **Tier 1 risk wrapper.**

Cleanup that can happen any time:

- Run `tools/cleanup_dead_engines.sh` (still dry-run-safe; pass `--force` to delete).
- Wire `gui/app.js` to fetch `/api/state2` and render the paper engines into the dashboard's empty BRACKET / BASIS / FUND slots.
