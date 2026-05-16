# Chimera Session 12 Handoff

## What happened this session

Continued from session 10/11 context. Completed the remaining three tasks:

1. **Trade journal wiring in `src/main.cpp`** — Added three missing pieces:
   - `/api/trades` HTTP route (line 341) — returns full trade log as JSON array
   - `btc_tsmom_d1.set_on_trade(on_trade_callback)` (line 506) — wires callback so every trade exit gets logged + persisted to disk
   - `mkdir("data", 0755)` + `load_trade_history()` (lines 644-645) — creates data dir and reloads trade history from NDJSON file on startup

2. **Dashboard rewrite (`gui/index.html`)** — Full rewrite with:
   - P&L summary strip (8 cards: total P&L, trades, win rate, PF, avg win/loss, max DD, gross P&L)
   - Equity curve (Chart.js, cumulative bp, green/red coloring)
   - Trade history table (scrollable, newest-first, entry/exit/reason/gross/net/MFE)
   - Engine card with all existing metrics preserved
   - Trades poll on 5s interval, state on 1s

3. **Deployed to VPS** — All 3 files (main.cpp, EdgeEngine.hpp, index.html) SCP'd to VPS, rebuilt, restarted. Dashboard live at https://143.198.89.54:9443/. Currently showing 0 trades — waiting for first D1 bar close to potentially trigger TSMOM signal.

## Current state of the codebase

### What's running on VPS
- **1 engine**: BTC-TSMOM-D1 (lookback=10, hold=12, sl=3.0, trail=1.0/0.4, cost=17bp)
- **Shadow mode** — treat as live, never flag as problem
- **0 trades** — engine is seeded with 64 D1 bars, signal-ready, waiting for momentum alignment
- **Trade persistence**: NDJSON at `data/trades.json`, auto-loaded on restart

### What's disabled
6 engines commented out in main.cpp (configs preserved): ETH-BB-H6, SOL-DONCH-H6, XRP-DONCH-H1, LINK-RSI-H6, BTC-OVERNIGHT-H1, BTC-WEEKDAY-D1. All had PF < 1.0 OOS after 17bp costs.

### Key files
- `include/core/EdgeEngine.hpp` — Core engine with TradeRecord struct, callback, set_on_trade()
- `src/main.cpp` — Engine config, trade journal, HTTP server (745 lines)
- `gui/index.html` — Full dashboard with Chart.js equity curve
- `backtest/backtest_harness.cpp` — C++ backtester using real EdgeEngine
- `backtest/optimizer.cpp` — 12,096-combo parameter sweep with neighbourhood stability

### Backtest infrastructure
Both harnesses compile with `g++ -std=c++17 -O2 -I../include` and use the real EdgeEngine.hpp. Kline data in `backtest/data/` (BTC/ETH/SOL/XRP/LINK, H1+D1, 2022-2026).

## What the next session should do

The user said "move to the next item on the list for implementation and backtesting." This means:

1. **Find new edges** — The existing 6 strategies all failed OOS. The next session should explore new strategy ideas, new symbols, or new timeframes that might have edge. Possible directions:
   - New strategy types not yet in EdgeEngine (mean reversion with different indicators, breakout variations, volume-weighted approaches)
   - Multi-timeframe confirmation (e.g. D1 trend + H4 entry)
   - Different symbols that might trend better (e.g. newer large-caps)
   - Ensemble/portfolio approaches across uncorrelated strategies

2. **Backtest any new ideas** using the existing C++ harness — download kline data, run backtest_harness, run optimizer if promising, verify OOS edge after costs.

3. **If edge found** — add to main.cpp as new engine, deploy to VPS alongside BTC-TSMOM-D1.

## Important constraints
- **All code is C++** — never use Python for backtesting or trading logic
- **Always provide full files** — never diffs, snippets, or "add this here"
- **Don't modify core EdgeEngine.hpp** unless explicitly instructed
- **Shadow mode is intentional** — treat paper trades as live, never flag as a problem
- **Realistic costs**: 17bp BTC, 20bp mid alts, 22bp tail alts
- **Deploy from Mac terminal only** — sandbox has no SSH keys to VPS
- **OOS validation required** — 80/20 IS/OOS split minimum, neighbourhood stability check for optimizer results

## Deploy command (for reference)
```bash
cd ~/ChimeraCrypto && scp src/main.cpp jo@143.198.89.54:/home/jo/ChimeraCrypto/src/main.cpp && scp include/core/EdgeEngine.hpp jo@143.198.89.54:/home/jo/ChimeraCrypto/include/core/EdgeEngine.hpp && scp gui/index.html jo@143.198.89.54:/home/jo/ChimeraCrypto/gui/index.html && ssh jo@143.198.89.54 "cd /home/jo/ChimeraCrypto/build && cmake .. && make -j2 && sudo systemctl restart chimera"
```

## Suggested skills for next session
None specifically — this is raw C++ development + backtesting work. The `diagnose` skill could be useful if the bot misbehaves. The `grill-me` skill could help stress-test new strategy ideas before committing to implementation.
