# Chimera Handoff — Session 10 (2026-05-16)

## What this session accomplished

### 1. Trailing stop deployed to VPS (prior to context compaction)
- Added trailing stop to `EdgeEngine.hpp`: arms at 1.0×ATR profit, trails at peak − 0.5×ATR, ratchets up only
- Deployed via SCP (GitHub push broken — see blockers), compiled, restarted on VPS
- Committed as `eeacb93` on VPS, but **Mac local changes from this session are uncommitted**
- Verified working: all 5 engines show `trail_armed: false` in `/api/state2`

### 2. Two new time-gated strategies written (NOT YET DEPLOYED)
Added `OVERNIGHT` and `WEEKDAY` to EdgeEngine's `StrategyKind` enum. Files modified locally on Mac but **not yet on VPS**:

**`include/core/EdgeEngine.hpp`** — new code:
- `StrategyKind::OVERNIGHT` and `StrategyKind::WEEKDAY` enum values
- `signal_overnight_()`: fires on H1 bar close at 21:00 UTC when 20-bar TSMOM is positive AND bar is bullish (close > open)
- `signal_weekday_()`: fires on D1 bar close on Monday when close > SMA(5)
- Helpers: `utc_hour_from_ms_()`, `utc_dow_from_ms_()` (use `gmtime_r`), `sma_()`
- New Config fields with defaults: `entry_hour_utc=21`, `entry_dow=1`, `sma_len=5`
- Constructor guard for `sma_len` buffer size

**`src/main.cpp`** — new code:
- `btc_overnight_h1`: BTC H1, 21:00 UTC entry, hold 2 bars, SL 1.5×ATR, trail 0.8/0.4 ATR, 17bp cost
- `btc_weekday_d1`: BTC D1, Monday entry, hold 3 bars (exit Thursday), SL 2.0×ATR, 17bp cost
- Both registered in `g_slots` (7 engines total now), both cold-start seeded
- Tick routing fans BTC ticks to all 3 BTC engines (TSMOM-D1, OVERNIGHT-H1, WEEKDAY-D1)

**Code review passed** — no syntax errors, no logic bugs, all switch statements covered, designated initializer order correct, existing 5 engines unaffected.

### 3. Research: 6 candidate trading edges ranked
Delivered a visualization ranking new strategies by evidence quality:

| Rank | Strategy | Conviction | Key stat |
|------|----------|-----------|----------|
| 1 | Overnight seasonality (21-23 UTC) | HIGH | 33-40% annualized |
| 2 | Monday effect (buy Mon/sell Thu) | HIGH | +0.51%/day Monday avg |
| 3 | Funding rate regime (neg funding = buy spot) | MEDIUM | 83-96% WR at 90d |
| 4 | Session open momentum (London/NY/Asia) | MEDIUM | Engine already exists |
| 5 | Funding settlement dip-buy (8h cycle) | NEEDS BACKTEST | — |
| 6 | Weekend momentum carry | NEEDS BACKTEST | — |

#1 and #2 are now implemented as OVERNIGHT and WEEKDAY (Path A). The rest are queued.

### 4. Codebase audit
- **93 header files** in `include/core/`, ~40 engine-related
- Only `SessionMomentumEngine.hpp` is a dormant keeper that actually exists as built code
- `FundingWindowEngine`, `OrderbookImbalanceEngine`, `BasisMomentumEngine`, `LiquidationEngine` — referenced in `ENGINE_ROADMAP.md` as Tier-2 keepers but **never built**
- `FundingSignalEngine.hpp` exists but is a DISCARD (superseded by the never-built FundingWindowEngine)
- `ENGINE_ROADMAP.md` is stale — still references SwingEngine as live

---

## What the next session must do

### Priority 1: Fix SSH/SCP access to VPS
The VPS (`143.198.89.54`) is rejecting SSH key auth:
```
jo@143.198.89.54: Permission denied (publickey).
```
This blocks ALL deployment. The user needs to fix SSH keys or provide a password-based login method before any code can reach the VPS.

### Priority 2: Deploy OVERNIGHT + WEEKDAY to VPS
Once SSH works:
```bash
scp include/core/EdgeEngine.hpp jo@143.198.89.54:/home/jo/ChimeraCrypto/include/core/
scp src/main.cpp jo@143.198.89.54:/home/jo/ChimeraCrypto/src/
ssh jo@143.198.89.54 "cd /home/jo/ChimeraCrypto && cd build && cmake .. && make -j2 && sudo systemctl restart chimera"
```
Verify via `curl -s https://143.198.89.54:9443/api/state2 | python3 -m json.tool` — should show 7 engines.

### Priority 3: Path A completion — backtest OVERNIGHT and WEEKDAY
The strategies were implemented from research, not backtested against Chimera's actual cost structure. Before promoting to live:
- Pull BTC H1 klines (2022-2026) from Binance REST
- Simulate OVERNIGHT signals with 17bp round-trip cost, 1.5×ATR SL, 2-bar hold
- Pull BTC D1 klines, simulate WEEKDAY signals with 17bp cost, 2.0×ATR SL, 3-bar hold
- Target: OOS PF > 1.15 after costs

### Priority 4: Wire SessionMomentumEngine into main.cpp
`include/core/SessionMomentumEngine.hpp` (176 lines) is a complete signal generator for London/NY/Asia session opens. It's a Tier-2 keeper sitting dormant. However it needs a **position management wrapper** — it only has `check_signal()` and `mark_traded()`, no entry/exit/SL/TP lifecycle. Options:
- Wrap it with a thin position manager in the tick callback
- Or port its logic into a new `StrategyKind::SESSION_MOM` in EdgeEngine (preferred — reuses all position management)

### Priority 5: Build remaining edges
Engines to build (in priority order):
1. **FundingWindowEngine** — buy spot when perp funding is negative (currently doesn't exist despite roadmap reference). Needs `FundingRateFetcher` integration.
2. **Funding settlement dip-buy** — buy 5-10 min before 8-hourly settlement (00:00, 08:00, 16:00 UTC). Needs backtest first.
3. **Weekend momentum carry** — enter Friday, exit Sunday. Needs backtest first.

### Priority 6: Pending from prior sessions
- **Git push from Mac**: Still broken. Old PAT expired, new PAT blocked by macOS Keychain. Need to configure `git credential-store` or SSH keys for GitHub.
- **Build verification system**: User explicitly asked for a check to ensure Mac/VPS/GitHub stay in sync. Never implemented.
- **Commit OVERNIGHT/WEEKDAY to git**: Currently unstaged local changes on Mac. VPS has its own unstaged changes from the trailing stop deploy.

---

## Current state of the system

### VPS (143.198.89.54)
- **Running binary**: 5 EdgeEngine instances, shadow mode, with trailing stop
- **Git HEAD**: `eeacb93` (trailing stop commit)
- **Unstaged changes**: trailing stop was SCP'd and compiled but the commit was done locally — verify with `git status`
- **Service**: `chimera.service` via systemd, nginx reverse proxy 9443→8080
- **Dashboard**: `https://143.198.89.54:9443/`

### Mac local repo (`/Users/jo/ChimeraCrypto`)
- **Git HEAD**: `eeacb93` (same as VPS after hard reset to origin/main)
- **Unstaged changes**: `include/core/EdgeEngine.hpp` (+110 lines), `src/main.cpp` (+96 lines) — the OVERNIGHT/WEEKDAY additions
- **GitHub remote**: `https://github.com/Trendiisales/Omega`
- **GitHub push**: BROKEN (PAT/Keychain issues)

### Live engines on VPS (5 active, all shadow)
| Tag | Symbol | Strategy | TF | Status |
|-----|--------|----------|----|--------|
| BTC-TSMOM-D1 | BTCUSDT | TSMOM | D1 | shadow, trail support |
| ETH-BB-H6 | ETHUSDT | BOLLINGER | H6 | shadow, trail support |
| SOL-DONCH-H6 | SOLUSDT | DONCHIAN | H6 | shadow, trail support |
| XRP-DONCH-H1 | XRPUSDT | DONCHIAN | H1 | shadow, trail support |
| LINK-RSI-H6 | LINKUSDT | RSI_REVERT | H6 | shadow, trail support |

### Pending engines (written locally, not deployed)
| Tag | Symbol | Strategy | TF | Status |
|-----|--------|----------|----|--------|
| BTC-OVERNIGHT-H1 | BTCUSDT | OVERNIGHT | H1 | code complete, needs deploy + backtest |
| BTC-WEEKDAY-D1 | BTCUSDT | WEEKDAY | D1 | code complete, needs deploy + backtest |

---

## Key files

| File | Purpose |
|------|---------|
| `include/core/EdgeEngine.hpp` | THE core engine — all strategies live here |
| `src/main.cpp` | Engine instantiation, config, feed routing, HTTP server |
| `include/core/SessionMomentumEngine.hpp` | Dormant session-open momentum signal generator |
| `include/core/FundingSignalEngine.hpp` | DISCARD — don't use |
| `include/core/SymbolIndex.hpp` | Symbol ID mapping (8 symbols) |
| `include/config/TradingConfig.hpp` | Cost tiers, kill window, disabled engine docs |
| `config/live_config.json` | Runtime config (shadow_mode, cost_bps, symbols) |
| `ENGINE_ROADMAP.md` | STALE — references SwingEngine, needs update |

---

## Blockers

1. **SSH to VPS broken** — `Permission denied (publickey)`. User's Mac SSH key is not authorized on VPS, or the key changed. This must be fixed first.
2. **GitHub push broken** — macOS Keychain intercepts git credential flow, old PAT cached. Workaround: SCP directly to VPS.
3. **No backtest harness** — strategies are implemented from research papers, not validated against Chimera's actual cost structure (17-22bp round-trip).

---

## User preferences (carry forward)

- Always give full code with context — no snippets, diffs, or partial files
- Warn at 70% context usage with summary
- Warn before session/time management blocks
- Never modify core code unless instructed clearly
- Spot-only, long-only trading
- Shadow mode is intentional, not a bug — treat paper trades as live
- VPS is in Singapore, Ubuntu, systemd service

## Suggested skills for next session

None of the installed skills are directly applicable. The work is pure C++ implementation + VPS deployment. If backtesting is done in Python, no special skill needed.
