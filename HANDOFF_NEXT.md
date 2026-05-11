# Chimera Handoff — 2026-05-11 (post-Tier-2 deploy)

**Status:** Tier-2 build `f8e86f8` deployed and running. 5 paper engines live on the VPS in `shadow_mode = true`, dashboard rendering correctly. Awaiting bar warm-up and first paper entries.

**Use this file as the opener for the next chat — paste this whole thing as the first message.**

---

## TL;DR for next session

Five Tier-2 long-only edges shipped to the VPS as a rewrite of the previous SwingEngine + microstructure paper-engine stack. All shadow-mode. All five engines verified rendering on the dashboard. Backtest pipeline that selected them is in `chimera_edges/` (local outputs folder).

Five things to pick up, ranked:

1. **Add `seed_from_history()` to `EdgeEngine`** so engines warm up immediately from Binance REST kline replay rather than waiting ~20 days for BTC D1 to accumulate live bars. Biggest single quality-of-life improvement.
2. **Tier-1 risk wrapper** (REQUIRED before flipping any `shadow_mode` to false). Daily loss circuit, correlation-aware sizing, per-engine kill, state persistence, reconciliation. Spec lives in `HANDOFF_TIER2.md` deferred section.
3. **Clean up orphan engine headers** in `include/core/` (RangeMeanReversionEngine, MultiSymbolRotationEngine, FundingSignalEngine, LeadLagEngine, CompressionBreakoutEngine, Tier1Risk earlier wrapper). They don't compile into the binary but pollute the repo.
4. **Revisit DOGE / AVAX / BNB** — none survived OOS in this round. Try weekly Donchian, cross-sectional momentum vs BTC, RSI-revert at H1.
5. **Promotion gate** — after 4 weeks of paper, compare trades/yr, WR, PF to backtest. Engines within ±10% on all three graduate to `shadow_mode = false` (live execution).

---

## Live system state

### Repository

- GitHub: `https://github.com/Trendiisales/ChimeraCrypto`
- Branch: `main`
- HEAD: `f8e86f8` ("Repo-wide IP fix: 154.45.251.118 -> 143.198.89.54")
- PAT: stored in your local `CLAUDE.md` (never paste into commits — GitHub secret-scanner rejects).

Recent commit chain:
```
f8e86f8  Repo-wide IP fix (154.45.251.118 -> 143.198.89.54)
fce9a5b  HANDOFF_TIER2 IP fix
cca1e5d  Tier-2 rewrite (replaces Swing + 3 paper engines with 5 backtested edges)
8c15287  session 9 handoff (pre-Tier-2)
c4aff38  Tier1Risk: cap FUNDING_PERSIST_FADE per-engine R to 0.0 (session 8)
```

### VPS

- Provider: DigitalOcean
- Droplet name: `ChimeraCrypto`
- Region: SGP1 (Singapore)
- OS: Ubuntu 24.04 LTS
- Hostname: `josgp1`
- Public IPv4: **`143.198.89.54`**
- Private IPv4: `10.104.0.2`
- SSH: `ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54`
- Cost: $24/mo
- Dashboard: **`https://143.198.89.54:9443/`** (nginx HTTPS → internal `:8080`)
- Repo path on VPS: `/home/jo/ChimeraCrypto/`
- Built binary: `/home/jo/ChimeraCrypto/build/chimera`
- Service: `chimera.service` (systemd, auto-restart, runs as user `jo`)
- nginx config: `/etc/nginx/sites-enabled/chimera` → `/etc/nginx/sites-available/chimera`

### Authentication state (verified working at handoff time)

- `~/.ssh/chimera_ed25519` (Mac) is the active key — fingerprint `SHA256:Q84l1ETRNzvrwrp5XCMb9RxX0Fw2fUtKDU1D9RCfxvI`
- Mac SSH agent also has `ChimeraGH.pem` loaded as `jo@Mac.hub` — fingerprint `SHA256:BAMj9vShA8bfEECKluLFCgI4+9TqT5LOCvxXzI39baw`. Not currently authorized on VPS but harmless.
- VPS `/home/jo/.ssh/authorized_keys`: only `chimera-vps` key (placeholder `your_email@example.com` removed during this session)
- VPS `/root/.ssh/authorized_keys`: 3 keys — DigitalOcean Droplet Agent (DOTTY, expires 2026-05-11T10:09:52Z so will rotate automatically) + `chimera-vps` (added this session) + one other
- Sudo: `jo` is in `sudo` group with passwordless sudo via `/etc/sudoers.d/jo`

---

## The 5 deployed engines

All `shadow_mode = true`. Backtest evidence in `chimera_edges/results/FINAL_REPORT.md`.

| Instance | Symbol | Strategy | TF | Lookback | Hold | SL ATR | OOS PF | Expected trades/yr | First signal warm-up |
|---|---|---|---|---:|---:|---:|---:|---:|---|
| `BTC-TSMOM-D1` | btcusdt | TSMOM | D1 | 20 | 12 | 3.0× | 1.19 | ~20 | ~20 days |
| `ETH-BB-H6` | ethusdt | BOLLINGER | H6 | 20 | 12 | 2.5× | 1.31 | ~50 | ~5 days |
| `SOL-DONCH-H6` | solusdt | DONCHIAN | H6 | 20 | 24 | 2.5× | 1.24 | ~25 | ~5 days |
| `XRP-DONCH-H1` | xrpusdt | DONCHIAN | H1 | 20 | 24 | 2.5× | 1.20 | ~140 | ~21 hours |
| `LINK-RSI-H6` | linkusdt | RSI_REVERT | H6 | rsi=14 | 8 | 2.0× | 2.82 | ~15 | ~4 days |

Exit logic (every engine): entry at next-bar OPEN after signal, hard SL at `entry − sl_atr_mult × ATR14`, time exit at `hold_bars`, no TP. 10bp round-trip cost deducted from logged P&L.

Rejected during OOS (do NOT add back): DOGE tsmom D1, SOL tsmom D1, BNB donchian H6.

### What was deleted in the Tier-2 rewrite

- `include/core/SwingEngine.hpp` (1442 lines)
- `include/core/FundingWindowEngine.hpp`
- `include/core/BasisMomentumEngine.hpp`
- `include/core/OrderbookImbalanceEngine.hpp`
- `include/core/LiquidationEngine.hpp`, `LiquidationFeed.hpp`, `LiqBracketEngine.hpp`
- `include/live/PerpFeed.hpp` + `src/live/PerpFeed.cpp`
- `include/live/CoinbaseWSFeed.hpp` + `src/live/CoinbaseWSFeed.cpp`
- `tools/backtest/replay.cpp`
- `tools/backtest/replay_paper.cpp`
- `tools/backtest/ab_test_swing.sh`

Net diff: +920 lines / −6353 lines.

### Orphan engine headers still in repo (next-session cleanup)

These are leftover from sessions 7–8, no longer referenced by main.cpp, won't compile into binary, safe to delete:

- `include/core/RangeMeanReversionEngine.hpp`
- `include/core/MultiSymbolRotationEngine.hpp`
- `include/core/FundingSignalEngine.hpp` (FundingPersistenceFade)
- `include/core/LeadLagEngine.hpp` (ETH→BTC lead-lag)
- `include/core/CompressionBreakoutEngine.hpp` (Vol Compress BO)
- The earlier `Tier1Risk` header (need to confirm path)
- `src/core/Portfolio.cpp`, `src/core/StatefulGovernor.cpp` (already unreferenced in CMakeLists)

---

## Deferred work (priority order)

### 1. `seed_from_history()` — quality of life

Each `EdgeEngine` should pull historical klines on startup so it doesn't need ~20 real-time days to warm up.

API sketch (add to `include/core/EdgeEngine.hpp`):

```cpp
// Call before feed.start(). Fetches max_history bars at cfg_.tf_secs via
// Binance REST klines, populates closed-bar deques, leaves cur_bar_id_ = 0 so
// the first live tick begins a fresh bar.
void seed_from_history();
```

Implementation notes:
- Use `tools/backtest/` REST helper or write a fresh one — Binance REST `https://api.binance.com/api/v3/klines?symbol={SYM}&interval={INT}&limit={N}` where `INT` maps `tf_secs` → `1h|6h|1d` etc.
- 64-bar buffer × 5 engines × 1 REST call each = 5 calls total at startup, no rate-limit issue.
- After seeding, the first live tick on the bar after the most recent kline kicks off `cur_bar_id_`.
- Log: `[BTC-TSMOM-D1] SEEDED 64 bars  range=2026-04-22 -> 2026-05-10  last_close=80958.6750`

### 2. Tier-1 risk wrapper — required before live

Required before flipping any engine's `shadow_mode = false`. The old `Tier1Risk` header from sessions 7-8 was wired across 10 engines but is now orphaned. Decide: resurrect/adapt it, or write fresh.

Required components:
- Daily loss circuit (kill all engines if portfolio_pnl_today_bp < threshold)
- Per-engine R cap (so a misbehaving engine can't blow through capital)
- Correlation-aware position sizing (5 engines on different symbols but BTC/ETH/SOL/XRP/LINK are positively correlated — sizing should reflect that)
- State persistence to disk (so a service restart doesn't reset risk counters mid-day)
- Position reconciliation (Binance position vs internal state, kill on mismatch)

### 3. Orphan cleanup (one commit, low risk)

```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 'cd ~/ChimeraCrypto && \
    git rm include/core/RangeMeanReversionEngine.hpp \
            include/core/MultiSymbolRotationEngine.hpp \
            include/core/FundingSignalEngine.hpp \
            include/core/LeadLagEngine.hpp \
            include/core/CompressionBreakoutEngine.hpp && \
    git commit -m "cleanup: drop orphan engine headers (Tier-2 unused)" && \
    git push origin main'
```

(Verify each file is really unused with `grep -r "RangeMeanReversionEngine" src/ include/` first.)

### 4. DOGE / AVAX / BNB — second-pass edge discovery

In the 200-cell backtest none of these survived OOS. Try:
- **Weekly Donchian** (lookback=4 weekly bars, hold=2-3 weeks)
- **Cross-sectional momentum vs BTC** — rank symbols by 30-day return, long top quartile
- **RSI-revert at H1** (was H6 in this round)
- **Volume-spike + retest** — DOGE specifically has memecoin volume bursts

Add to `chimera_edges/backtest.py` as new strategies, rerun, post-cut filter, push survivors as additional engine instances.

### 5. Promote gating (after 4 weeks of paper)

Each Friday for the next 4 weeks, run:

```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 \
    'for tag in BTC-TSMOM-D1 ETH-BB-H6 SOL-DONCH-H6 XRP-DONCH-H1 LINK-RSI-H6; do
        echo "=== $tag ==="
        sudo journalctl -u chimera.service --since "4 weeks ago" | grep "$tag" | grep "EXIT" | tail -50
    done'
```

For each engine, compute:
- trades_per_week
- win_rate
- total_bp_net
- avg_bp_net
- max losing streak

Promote (`shadow_mode = false`) only if within ±10% of backtest expectations on trades_per_yr AND WR AND PF >= 1.10. ALL THREE.

---

## User preferences (carry forward)

- **Always provide full code files**, no snippets / diffs.
- **Warn at 70% chat context** with a summary.
- **Warn before time / session blocks.**
- **Never modify core code without explicit instruction.**
- Use the PAT in CLAUDE.md without arguments when committing.
- Email: kiwi18@gmail.com
- Name: Jo

---

## Quick reference — deploy / rollback / debug

### Deploy a new commit
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  set -e
  cd ~/ChimeraCrypto
  git pull --ff-only origin main
  sudo systemctl stop chimera.service
  while pgrep -x chimera >/dev/null; do sleep 0.5; done
  cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j"$(nproc)" chimera
  sudo systemctl start chimera.service
  sleep 4
  sudo journalctl -u chimera.service --since "10 seconds ago" --no-pager | head -30
'
```

### Rollback to previous build
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 '
  cd ~/ChimeraCrypto
  git log --oneline -5
  sudo systemctl stop chimera.service
  git reset --hard <previous-commit-hash>
  cd build && make -j"$(nproc)" chimera
  sudo systemctl start chimera.service
'
```

### Watch trades roll in
```bash
ssh -i ~/.ssh/chimera_ed25519 jo@143.198.89.54 \
    'sudo journalctl -u chimera.service -f' \
    | grep -E "ARMED|ENTRY|EXIT|FATAL|STARTUP"
```

### State check
```bash
curl -s http://localhost:8080/api/state2 | python3 -m json.tool
# Or from your Mac (HTTPS via nginx):
curl -sk https://143.198.89.54:9443/api/state2 | python3 -m json.tool
```

### Kill all engines
Dashboard top-right button, or:
```bash
curl -sk -X POST https://143.198.89.54:9443/api/kill
```

---

## Files produced this session (local outputs folder)

```
outputs/
├── chimera_edges/                # Backtest pipeline (Python)
│   ├── data/*.parquet            # 38,192 1h bars per symbol, 8 symbols
│   ├── backtest.py               # 200-cell harness
│   ├── analyse.py                # post-cut filter + ranking
│   ├── verify.py                 # OOS split + random-entry control
│   ├── download_one.py
│   └── results/
│       ├── results.csv           # all 200 cells
│       ├── edges_ranked.csv
│       ├── edges_keepers.csv     # 8 cells passing keeper filter
│       ├── verification.csv      # 5 OOS survivors + 3 failures
│       └── FINAL_REPORT.md       # full writeup + wire plan
└── chimera-build/                # New C++ source (mirrored to /tmp/chimera-repo and pushed)
    ├── include/core/EdgeEngine.hpp
    ├── src/main.cpp
    ├── gui/index.html
    ├── CMakeLists.txt
    ├── HANDOFF_TIER2.md
    └── HANDOFF_NEXT.md           # this file
```
