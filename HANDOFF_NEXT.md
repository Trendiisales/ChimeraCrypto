# Chimera Handoff — 2026-05-11 (post-GUI fix)

**Status:** Tier-2 build `57501a9` is on GitHub. Five paper engines (BTC/ETH/SOL/XRP/LINK) shipped as Tier-2 rewrite. Dashboard now shows live spot prices for all 8 feed symbols immediately on load (no warm-up needed for the display). Engines themselves still cold-start their bar history from live ticks — fixing that is priority #1 for the next session.

**Use this file as the opener for the next chat — paste this whole thing as the first message.**

---

## TL;DR for next session

The Tier-2 GUI was missing real-time symbol pricing because the rewrite replaced the old engine stack with brand-new EdgeEngine instances whose internal bar-aggregator starts empty (`bars_in_buffer = 0`, `last_close = 0.0`). The previous GUI session fixed the display layer by routing live WebSocket tick prices into a new `spot_prices` field on `/api/state2` and rendering an 8-symbol live ticker strip + per-card "Live Price" line.

The display is fixed. The underlying engine cold-start is not. Priority next session is `seed_from_history()` — pull Binance REST klines on engine construction so the bar deques are pre-populated with real historical OHLC. Without this, BTC-TSMOM-D1 needs ~20 calendar days to take its first trade.

Five things to pick up, ranked:

1. **`seed_from_history()` on `EdgeEngine`** — see "Open work" below for the full spec. 3-file change. Critical: this is what the user is unhappy about.
2. **Verify the GUI fix actually deployed** — pull commit `57501a9` to the VPS, rebuild, confirm dashboard shows live BTC/ETH/SOL/BNB/AVAX/LINK/XRP/DOGE prices in the top strip and per-card "Live Price" rows.
3. **Tier-1 risk wrapper** — required before flipping any `shadow_mode` to false. Daily loss circuit, correlation-aware sizing, per-engine kill, state persistence, reconciliation. Spec lives in `HANDOFF_TIER2.md` deferred section.
4. **Clean up orphan engine headers** in `include/core/` (RangeMeanReversionEngine, MultiSymbolRotationEngine, FundingSignalEngine, LeadLagEngine, CompressionBreakoutEngine, Tier1Risk earlier wrapper). They don't compile into the binary but pollute the repo.
5. **Promotion gate** — after 4 weeks of paper trades, compare trades/yr, WR, PF to backtest. Engines within ±10% on all three graduate to `shadow_mode = false` (live execution).

---

## Live system state

### Repository

- GitHub: `https://github.com/Trendiisales/ChimeraCrypto`
- Branch: `main`
- HEAD: `57501a9` ("GUI: live spot-price strip + per-card live price (no warmup needed)")
- PAT: stored in user's local `CLAUDE.md` (NEVER paste into commits — GitHub secret-scanner rejects, and it's a security risk to expose tokens in chat).

Recent commit chain:
```
57501a9  GUI: live spot-price strip + per-card live price (no warmup needed)
19e5d44  Add HANDOFF_NEXT.md (previous opener)
f8e86f8  Repo-wide IP fix (154.45.251.118 -> 143.198.89.54)
fce9a5b  HANDOFF_TIER2 IP fix
cca1e5d  Tier-2 rewrite (replaces SwingEngine + 3 paper engines with 5 backtested edges)
c4aff38  Tier1Risk: cap FUNDING_PERSIST_FADE per-engine R to 0.0 (session 8)
```

### VPS

- Provider: DigitalOcean
- Droplet name: `ChimeraCrypto`
- Region: SGP1 (Singapore)
- OS: Ubuntu 24.04 LTS
- Hostname: `josgp1`
- Public IPv4: **`143.198.89.54`**
- SSH: `ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54`
- Dashboard: **`https://143.198.89.54:9443/`** (nginx HTTPS → internal `:8080`)
- Repo path on VPS: `/home/jo/ChimeraCrypto/`
- Built binary: `/home/jo/ChimeraCrypto/build/chimera`
- Service: `chimera.service` (systemd, auto-restart, runs as user `jo`)
- nginx config: `/etc/nginx/sites-enabled/chimera`

### Authentication

- Mac key: `~/.ssh/chimera_ed25519` — fingerprint `SHA256:Q84l1ETRNzvrwrp5XCMb9RxX0Fw2fUtKDU1D9RCfxvI`
- Public key on VPS in both `/home/jo/.ssh/authorized_keys` and `/root/.ssh/authorized_keys` under comment `chimera-vps`
- Placeholder `your_email@example.com` key was removed last session — only the real key remains
- jo has passwordless sudo via `/etc/sudoers.d/jo`

### What is currently running on the VPS

Five `EdgeEngine` instances, all `shadow_mode = true`:

| Tag | Symbol | Strategy | Timeframe | OOS PF | Expected trades/yr |
|---|---|---|---|---|---|
| BTC-TSMOM-D1 | btcusdt | tsmom | D1 | 1.19 | ~20 |
| ETH-BB-H6 | ethusdt | bollinger | H6 | 1.31 | ~50 |
| SOL-DONCH-H6 | solusdt | donchian | H6 | 1.24 | ~25 |
| XRP-DONCH-H1 | xrpusdt | donchian | H1 | 1.20 | ~140 |
| LINK-RSI-H6 | linkusdt | rsi_revert | H6 | 2.82 | ~15 |

WebSocket spot feed subscribes to all 8 symbols in `include/core/SymbolIndex.hpp` (BTC, ETH, SOL, BNB, AVAX, LINK, XRP, DOGE). BNB/AVAX/DOGE flow into `g_last_spot_px_bits[]` for display but have no engine.

### API schema (post commit 57501a9)

`GET /api/state2` returns:

```json
{
  "build": "57501a9",
  "spot_prices": {
    "btcusdt":  61234.500000,
    "ethusdt":   2934.120000,
    "solusdt":    143.870000,
    "bnbusdt":    589.400000,
    "avaxusdt":    24.180000,
    "linkusdt":    14.560000,
    "xrpusdt":      2.480000,
    "dogeusdt":     0.156400
  },
  "engines": [ {...}, {...}, {...}, {...}, {...} ]
}
```

Each engine object: `{ tag, symbol, strategy, tf_secs, shadow, halted, in_position, entry_px, sl_px, last_close, trades, wins, total_bp, last_bp, bars_in_buffer }`.

`POST /api/kill` — flatten every open paper position + halt all engines. Returns `{"ok":true}`.

---

## Open work

### #1 — `seed_from_history()` for `EdgeEngine` (CRITICAL, user-requested)

**Problem.** The Tier-2 rewrite (commit `cca1e5d`) replaced `SwingEngine` + `FundingWindow` + `BasisMomentum` + `OBI` with brand-new `EdgeEngine` instances. The new class accumulates bars from live ticks. Even though the *service* has been running for weeks, these *specific engines* are new code with no history. Result: BTC-TSMOM-D1 needs ~20 calendar days to take its first trade because it accumulates one D1 bar per day from live ticks.

**Fix.** On engine construction, fetch the last N OHLC klines from Binance REST and pre-populate `opens_`/`highs_`/`lows_`/`closes_`/`bar_ts_ms_` deques. Engines warm-cold-start in seconds instead of days.

**Files to change (3):**

1. `include/live/BinanceREST.hpp` — add a public method:
   ```cpp
   struct Kline { int64_t open_ts_ms; double o, h, l, c; };
   std::vector<Kline> fetch_klines(const std::string& symbol,
                                   const std::string& interval,
                                   int limit = 64) const;
   ```
   Calls `GET /api/v3/klines?symbol=BTCUSDT&interval=1d&limit=64` (public endpoint, no auth needed). Parse the JSON array — Binance returns `[[open_time, open, high, low, close, ...], ...]`. Use the existing `curl` machinery in the file.

2. `include/core/EdgeEngine.hpp` — add a public method:
   ```cpp
   void seed_bars(const std::vector<BinanceREST::Kline>& bars) {
       for (auto& b : bars) {
           opens_.push_back(b.o);
           highs_.push_back(b.h);
           lows_.push_back(b.l);
           closes_.push_back(b.c);
           bar_ts_ms_.push_back(b.open_ts_ms);
       }
       while ((int)closes_.size() > cfg_.max_history) {
           opens_.pop_front(); highs_.pop_front(); lows_.pop_front();
           closes_.pop_front(); bar_ts_ms_.pop_front();
       }
       if (!closes_.empty()) last_close_ = closes_.back();
   }
   ```
   Map `cfg_.tf_secs` → Binance interval string: `3600 → "1h"`, `21600 → "6h"`, `86400 → "1d"`.

3. `src/main.cpp::main()` — between engine construction and `feed.start()`, call `seed_bars()` for each engine. Use a single `BinanceREST` instance (already constructed for the executor) or a new dedicated read-only one. Log how many bars each engine got. If a fetch fails, log and continue — engine will fall back to live-tick warm-up.

**Verification after deploy:**
- `bars_in_buffer` on every engine card should be ≥ `lookback` (20) within seconds of service start
- `last_close` on every engine card should be non-zero
- First entries can fire on the next bar close (within an hour for XRP H1, within a day for BTC D1)

### #2 — Verify GUI fix actually rendered on VPS

User just pushed `57501a9` from chat — needs to verify it built and deployed:

```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  cd ~/ChimeraCrypto && git pull --ff-only origin main && git log --oneline -3
  sudo systemctl stop chimera.service
  while pgrep -x chimera >/dev/null; do sleep 0.5; done
  cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j"$(nproc)" chimera 2>&1 | tail -20
  sudo systemctl start chimera.service
  sleep 4
  curl -s http://localhost:8080/api/state2 | python3 -m json.tool | head -30
'
```

Then open `https://143.198.89.54:9443/` — expect to see the 8-symbol ticker strip at top, plus a "Live Price" row inside each of the 5 engine cards.

### #3 — Tier-1 risk wrapper

Required before flipping any `shadow_mode = false`. Full spec lives in `HANDOFF_TIER2.md` deferred section. Components:
- Daily loss circuit (kill all if intraday drawdown > X bp)
- Correlation-aware position sizing (cap aggregate exposure across correlated symbols)
- Per-engine kill switch (independent of /api/kill)
- State persistence (positions, daily P&L, kill flags survive restarts)
- Reconciliation (compare engine-state position vs SpotExecutor cash + holdings, halt if drift)

### #4 — Repo cleanup

Orphan headers in `include/core/` that don't compile into the binary:
- `RangeMeanReversionEngine.hpp`
- `MultiSymbolRotationEngine.hpp`
- `FundingSignalEngine.hpp`
- `LeadLagEngine.hpp`
- `CompressionBreakoutEngine.hpp`
- earlier `Tier1Risk` wrapper

Either delete them or move to `archive/`.

### #5 — Promotion gate

After 4 weeks of paper trades, compare each engine's:
- trades/yr
- win rate
- profit factor

against backtest expectations. Engines within ±10% on all three graduate to `shadow_mode = false`. Anything outside that range stays paper or gets killed.

---

## User preferences (apply throughout)

- **Full code with context.** No snippets, no diffs, no partial files. Provide the complete file every time.
- **Warn at 70% chat context.** Give a summary before approaching the limit.
- **Warn before time/session usage block.**
- **Never modify core code unless instructed clearly.** "Core" = engine classes, signal logic, risk wrapper. GUI/HTML/build files are fair game when asked.
- **Mac dev path:** `jo@Jos-MacBook-Pro ChimeraCrypto %` (i.e. `~/ChimeraCrypto` on the Mac)
- **Email:** `kiwi18@gmail.com`

---

## Quick deploy / rollback reference

**Deploy current main to VPS:**
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  set -e
  cd ~/ChimeraCrypto
  git pull --ff-only origin main
  sudo systemctl stop chimera.service
  while pgrep -x chimera >/dev/null; do sleep 0.5; done
  cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j"$(nproc)" chimera 2>&1 | tail -20
  sudo systemctl start chimera.service
  sleep 4
  sudo journalctl -u chimera.service --since "10 seconds ago" --no-pager | head -30
'
```

**Roll back to a specific commit:**
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  cd ~/ChimeraCrypto && git fetch origin
  git checkout <commit>
  sudo systemctl restart chimera.service
'
```

**Force-flatten everything (paper):**
```bash
curl -X POST http://143.198.89.54:8080/api/kill
```

**Tail the log:**
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 \
    'sudo journalctl -u chimera.service -f' \
    | grep -E "ARMED|ENTRY|EXIT|FATAL|STARTUP|TICK"
```

**Backup-tarball before risky deploy:**
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  cd ~/ChimeraCrypto
  tar czf ~/chimera-backup-$(date +%Y%m%d-%H%M).tgz --exclude=build --exclude=.git .
  ls -lah ~/chimera-backup-*.tgz | tail -5
'
```

---

## What changed this session (chronological)

1. Diagnosed why the new Tier-2 dashboard showed `—` everywhere despite the service running for weeks → confirmed the Tier-2 rewrite replaced the old engine stack with brand-new `EdgeEngine` instances that have no bar history.
2. Modified `src/main.cpp::build_state_json()` to add a `spot_prices` object pulling from `g_last_spot_px_bits[]` (the WebSocket tick cache). Independent of engine bar accumulation — shows real prices on first paint.
3. Rewrote `gui/index.html`:
   - Added an 8-symbol live ticker strip at top (BTC/ETH/SOL/BNB/AVAX/LINK/XRP/DOGE). Engine-traded symbols highlighted in accent colour; spot-only symbols dimmer.
   - Each engine card got a prominent "Live Price" row pulled from `spot_prices[engine.symbol]`. Falls back to `last_close` once that's non-zero.
   - Up/down price colouring on 1s poll diff.
   - "Last Close" preserved as a separate small metric (still useful once engine warms up).
4. Committed and pushed as `57501a9`.
5. **Did NOT touch any engine/trading logic.** GUI display fix only.

---

## Files of note (paths on Mac / GitHub)

- `src/main.cpp` — wraps engines, runs HTTP server, owns the WS feed callback
- `gui/index.html` — single-file dashboard
- `include/core/EdgeEngine.hpp` — the Tier-2 engine class (touching this needs explicit user instruction)
- `include/core/SymbolIndex.hpp` — central symbol registry (8 symbols)
- `include/live/BinanceWSFeed.hpp` — WebSocket feed (libwebsockets)
- `include/live/BinanceREST.hpp` — REST executor + auth (curl) — `fetch_klines()` to be added here for seed_from_history
- `include/live/SpotExecutor.hpp` — spot-only executor (long buys + sells, no shorts)
- `CMakeLists.txt` — build config
- `HANDOFF_TIER2.md` — deeper spec for risk wrapper + deferred items
- `HANDOFF_NEXT.md` — this file (regenerate at end of each session)
