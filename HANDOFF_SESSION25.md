# Session 25 Handoff — Chimera Crypto Trading System

**Date**: 2026-05-17
**Git hash**: Post a18f76f (Session 25 changes committed)
**VPS**: 143.198.89.54 — chimera.service active, 245 engines

---

## Accomplishments This Session

### 1. Staged Ratcheting Trail — A/B Tested & Deployed
- **What**: When unrealised profit exceeds `trail_tighten_atr × ATR`, the trailing stop tightens from `trail_dist_atr` to `trail_tighten_dist_atr`. Locks in large winners without cutting normal trades.
- **EdgeEngine.hpp**: Added `trail_tighten_atr` and `trail_tighten_dist_atr` config params + ratchet logic in `check_exits_()`.
- **A/B backtested 10 engines** — every single one improved:

| Engine | Baseline PF | Best Ratchet PF | Config | MaxDD Change |
|--------|------------|----------------|--------|--------------|
| XRP-TSMOM-H6 | 1.36 | 1.53 | 1.5/0.20 | -2085bp |
| BTC-TSMOM-D1 | 2.66 | 2.80 | 3.0/0.25 | -26bp |
| ETH-TSMOM-H8 | 1.03 | 1.23 | 1.5/0.10 | -480bp |
| XRP-DONCH-H2 | 1.75 | 2.80 | 1.5/0.15 | -712bp |
| ETH-DONCH-H4 | 1.38 | 1.55 | 1.5/0.30 | -23bp |
| DOGE-TSMOM-H6 | 1.05 | 1.19 | 1.5/0.10 | -1812bp |
| SOL-TSMOM-H6 | 0.91 | 1.04 | 2.0/0.10 | -462bp |
| BNB-TSMOM-H6 | 1.01 | 1.09 | 2.0/0.15 | -412bp |
| SUI-TSMOM-H6 | 1.00 | 1.08 | 1.5/0.10 | -235bp |
| BTC-DONCH-H6 | 1.05 | 1.13 | 1.5/0.10 | -329bp |

- **Applied to all 245 engines**: 204 sub-daily engines → 1.5/0.15, 41 daily+ engines → 3.0/0.25
- **Tool**: `backtest/ratchet_test.cpp` — standalone A/B comparison, fixed data loader bug

### 2. Graceful Shutdown
- **Problem**: Open positions were lost on `systemctl restart` — unrealised profits evaporated.
- **Fix**: Added `graceful_close()` to EdgeEngine (exits position with reason "SHUTDOWN" without halting).
- **main.cpp shutdown handler**: On SIGTERM, iterates all engines, closes open positions at last known spot price, persists to `data/trades.json`, then exits.
- **Completed trades (closed before restart) were already persisting correctly** — only open positions were lost.

### 3. GUI Upgrades
- **UTC clock + uptime timer** in header (updates every second)
- **Bell toggle** (🔔 icon) — click to enable/disable trade alert sounds (win chime, loss tone, entry ascending tones)
- **All 12 symbols** in ticker strip (added NEAR, SUI, APT, ARB)
- **Exotic TF tabs** added: H8, H16, D2, D3
- **`startup_ts`** added to /api/state2 for uptime calculation

### 4. Backtest Tool Fix
- `ratchet_test.cpp` had a JSON parsing bug: `load_all_parts()` wasn't skipping the outer `[` bracket, causing every kline file to load 0 bars.
- Fixed by adding proper `load_klines_from_json()` function matching `optimizer_general.cpp` pattern.

---

## Current System State

- **Engines**: 245 (all shadow mode, long-only spot)
- **Strategies**: 95 TSMOM + 51 RSI_REVERT + 44 BOLLINGER + 28 DONCHIAN + 27 misc
- **Symbols**: 12 (BTC, ETH, SOL, BNB, AVAX, LINK, XRP, DOGE, NEAR, SUI, APT, ARB)
- **Timeframes**: H1, H2, H3, H4, H6, H8, H12, H16, D1, D2, D3
- **Completed trades on disk**: 3 (ARB-RSI-H1, APT-RSI-H1, ARB-RSI-H2 — all winners)
- **Ratcheting**: Active on all 245 engines

---

## Files Changed This Session

| File | Changes |
|------|---------|
| `include/core/EdgeEngine.hpp` | `trail_tighten_atr`, `trail_tighten_dist_atr` config params, ratchet logic in `check_exits_()`, `graceful_close()` method, JSON output for new params |
| `src/main.cpp` | Ratchet params on all 245 engines, `g_startup_ts_ms` global, `startup_ts` in state2 JSON, graceful shutdown handler |
| `gui/index.html` | Clock + uptime, bell toggle, 12 symbols, exotic TF tabs (H8/H16/D2/D3) |
| `backtest/ratchet_test.cpp` | Fixed `load_all_parts()` JSON parser, added `load_klines_from_json()` |

---

## Priority 1: Next Session — Find More Edges

### Coverage Gaps Still Open
Run optimizer sweeps for these underexplored combinations:

**RSI_REVERT on shorter timeframes (H2/H4/H6)**:
- RSI counter-trend has only been tested on H1, H6, H8, H16, D1, D2, D3
- H2 and H4 are untested for RSI — could yield new engines
- Command: `./optimizer_general <symbol> RSI_REVERT H2 <cost_bp>`

**DONCHIAN on D1/D2/D3**:
- DONCHIAN has good coverage on H2-H12 but limited on daily TFs
- Some symbols may have breakout edges on daily bars
- Command: `./optimizer_general <symbol> DONCHIAN D1 <cost_bp>`

**BOLLINGER on H2/H4/H6/H8**:
- BOLLINGER has been tested on H6, H8, H12, H16, D1, D2, D3
- Shorter timeframes (H2/H4) untested — BB mean-reversion may work well intraday

### Cost Structure Reference
- BTC: 17bp, ETH: 17bp
- SOL/BNB/XRP/LINK: 20bp
- DOGE/AVAX/NEAR/SUI/APT/ARB: 22bp

### Deploy Criteria
- PF ≥ 1.3, Neighbourhood ≥ 60%, Trades ≥ 20
- Relaxed for daily+ TFs: Trades ≥ 8 acceptable

---

## Priority 2: Validate Borderline Engines

These engines from Session 24 had marginal stats and need extended shadow observation before adding to main.cpp:

Engines with low trade counts or borderline Nbr% should run shadow for 4+ weeks. Compare live shadow WR/PF against backtest within ±10% before promoting.

---

## Priority 3: Per-Engine Ratchet Optimisation

The current ratchet settings (1.5/0.15 and 3.0/0.25) are good defaults based on the A/B testing, but individual engines could benefit from per-engine optimised ratchet params. Use `ratchet_test.cpp` to find optimal settings for high-value engines:

```bash
cd backtest
./ratchet_test <symbol> <strategy> <tf> <cost_bp> <lb> <hb> <sl> <ta> <td>
```

Priority engines for individual optimisation:
- XRP-DONCH-H2 (showed PF 1.75→2.80 — massive improvement, worth fine-tuning)
- BTC-TSMOM-D1 (flagship engine, 3.0/0.25 may not be optimal)
- ETH-TSMOM-H8 (1.5/0.10 was best — already different from default 1.5/0.15)

---

## Deploy Protocol (Unchanged)

```bash
# From Mac terminal:
cd ~/ChimeraCrypto
git add <files> && git commit -m "..." && git push

# SCP to VPS:
scp ~/ChimeraCrypto/src/main.cpp root@143.198.89.54:/home/jo/ChimeraCrypto/src/main.cpp
scp ~/ChimeraCrypto/include/core/EdgeEngine.hpp root@143.198.89.54:/home/jo/ChimeraCrypto/include/core/EdgeEngine.hpp

# SSH to VPS and rebuild:
cd /home/jo/ChimeraCrypto/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j2
sudo systemctl restart chimera

# Verify:
sudo systemctl status chimera
curl -s http://localhost:8080/api/state2 | python3 -m json.tool | head -5
```

**IMPORTANT**: SCP files BEFORE running cmake/make. Previous session accidentally built before SCP landed.

---

## Known Issues

1. **GUI firewall**: Port 8080 may not be in ufw allow list. Fix: `sudo ufw allow 8080/tcp`
2. **Browser audio**: Trade bells require one click on the page to unlock Web Audio API. Click the 🔔 icon.
3. **VPS has no GitHub SSH key**: Cannot `git pull` on VPS. Must SCP from Mac.
4. **Backtest data too large for git**: Use specific `git add` commands, never `git add -A` from backtest/data/

---

## Session 25 Trade Results

Pre-restart trades (all winners):
- ARB-RSI-H1: +30.8bp (trail exit)
- APT-RSI-H1: +2.1bp (trail exit)
- ARB-RSI-H2: +10.8bp (trail exit)

Open positions lost to restart (before graceful shutdown was deployed):
- LINK-TSMOM-D3: +129.9bp unrealised (lost)
- BNB-TSMOM-D3: +62.0bp unrealised (lost)
- DOGE-TSMOM-D3: +111.2bp unrealised (lost)
- SUI-TSMOM-D3: +176.2bp unrealised (lost)

**Total captured**: +43.7bp | **Total lost to restart**: ~479bp
**Graceful shutdown now prevents this from happening again.**
