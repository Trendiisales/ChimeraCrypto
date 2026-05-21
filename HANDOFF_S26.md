---
session: 26
date: 2026-05-17
focus_next: Find more trading edges — run optimizer sweeps on untested strategy/TF/symbol combinations
---

# Session 26 Handoff

## What was done

1. **Fixed GUI uptime display** — removed broken `$('#lastUpdated').textContent` reference crashing the refresh path
2. **Added position snapshot system** — 60s periodic writes to `data/open_positions.json` for crash recovery
3. **Improved systemd service** — `Restart=on-failure`, `RestartSec=5`, `TimeoutStopSec=10`
4. **Found 12 new edges** via optimizer sweep on RSI_REVERT H4 and BOLLINGER H4/H2 (previously untested combos)
5. **Validated all 12** with walk-forward (validate_engines.cpp, 60/20/20 split) — all passed standard 80/20
6. **Added 10 truly new engines** to main.cpp (2 were duplicates of existing), bringing total from 245 → 255
7. **Deployed to VPS** — build succeeded, service running with 255 engines
8. **Updated spreadsheet** — added missing S24 (15) + S26 (10) engines, rebuilt ranked sheet

## Current system state

- **VPS**: 143.198.89.54, Ubuntu, chimera.service active (255 engines, shadow mode)
- **Code**: `src/main.cpp` (6985 lines), all engines instantiated and registered
- **Strategies**: TSMOM (~100), RSI_REVERT (~60), BOLLINGER (~55), DONCHIAN (~40)
- **Symbols**: BTC, ETH, SOL, XRP, LINK, BNB, DOGE, AVAX, NEAR, SUI, APT, ARB
- **Timeframes**: H1, H2, H3, H4, H6, H8, H12, H16, D1, D2, D3

## What's been swept (DON'T re-do)

| Strategy | Timeframes fully swept | Notes |
|----------|----------------------|-------|
| TSMOM | H1-D3, all 12 symbols | Complete coverage |
| RSI_REVERT | H1-H16, all symbols | H4 added S26 |
| BOLLINGER | H1-H16, all symbols | H4/H2 added S26 |
| DONCHIAN | H2-D3, partial symbols | Some gaps remain |

## Where to find more edges (next session)

1. **DONCHIAN H2/H3/H4 on newer symbols** — SUI, APT, ARB only have H4-H6+ coverage for DONCH. Lower TFs untested.
2. **RSI_REVERT D1/D2** — only BTC/DOGE/LINK have daily RSI engines. Rest of the 12 symbols untested at daily.
3. **BOLLINGER D1/D2** — zero daily BOLL engines exist. Could be a whole new batch.
4. **Cross-strategy param re-sweep** — some S19 engines used conservative params. Re-sweeping with wider grids on the extended data (8yr) might find better optima.
5. **New strategy types** — would require EdgeEngine.hpp modification (user must explicitly approve). Ideas: MACD crossover, volume-weighted breakouts, multi-timeframe confirmation.

## Key files

- `src/main.cpp` — all 255 engine configs + main loop
- `include/core/EdgeEngine.hpp` — DO NOT MODIFY without explicit permission
- `backtest/optimizer_general.cpp` — grid sweep tool (usage: `./optimizer <symbol> <strategy> <tf_secs>`)
- `backtest/validate_engines.cpp` — walk-forward validator
- `backtest/data/` — H1 kline JSON files (NOT in git)
- `chimera_engine_roster.xlsx` — engine tracking spreadsheet
- `scripts/deploy_s26.sh` — deployment script template

## Deploy workflow

```bash
# From Mac only (VPS has no git):
cd ~/ChimeraCrypto
scp src/main.cpp jo@143.198.89.54:/home/jo/ChimeraCrypto/src/main.cpp
scp include/core/EdgeEngine.hpp jo@143.198.89.54:/home/jo/ChimeraCrypto/include/core/EdgeEngine.hpp
ssh jo@143.198.89.54 "cd /home/jo/ChimeraCrypto && sudo systemctl stop chimera && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc) && sudo systemctl start chimera"
```

## Optimizer usage

```bash
# On Mac (compile + run):
cd ~/ChimeraCrypto/backtest
g++ -O2 -std=c++17 optimizer_general.cpp -o optimizer
./optimizer solusdt DONCHIAN 7200    # SOL DONCH H2
./optimizer aptusdt RSI_REVERT 86400  # APT RSI D1
./optimizer btcusdt BOLLINGER 86400   # BTC BOLL D1
```

Output: ranked candidates with PF, Sharpe, Neighbourhood%, OOS trades. Deploy criteria: PF ≥ 1.3, Nbr ≥ 60%, Trades ≥ 20.

## Constraints (from memory)

- **Spot-only**: no shorting, no perps, no futures — permanent rule
- **Shadow mode is intentional**: don't flag as problem, treat paper trades as live
- **Never modify EdgeEngine.hpp** without explicit user instruction
- **Full code always**: never provide diffs/snippets, always full files
- **Deploy via SCP only**: VPS has no git SSH key
- **Git**: never use PAT/HTTPS, SSH only; backtest/data/ excluded from git

## Suggested skills for next session

None required — this is pure C++ optimizer work + spreadsheet updates. Standard file tools + bash are sufficient.
