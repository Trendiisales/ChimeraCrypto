# Chimera Session 13 Handoff — Edge Hunting (Next Session)

## What happened this session

Completed the full edge-finding pipeline from Session 12's todo list:

1. **Built `scanner.cpp`** — multi-symbol, multi-strategy quick scanner (282 combos across BTC/ETH/SOL/XRP/LINK × TSMOM/DONCHIAN/BOLLINGER/RSI_REVERT × D1/H6)
2. **Built `optimizer_general.cpp`** — CLI-driven optimizer with 15,552-combo grid + neighbourhood stability scoring
3. **Found 3 new edges** — all TSMOM on D1. ETH was strongest (72.9% of param combos profitable), LINK most robust (100% neighbourhood stability)
4. **Updated `main.cpp`** — now has 4 active engines (was 1), 6 disabled as comments
5. **Updated `backtest_harness.cpp`** — matches main.cpp, loads D1 data for all 4 symbols, `--all` flag runs disabled engines too
6. **Verified compilation** — all 4 backtest binaries compile clean in sandbox

### Deployed engines (all TSMOM-D1, shadow mode, long-only)

| Engine | LB | HB | SL | TA | TD | Cost | OOS PF | OOS Sharpe | Nbr% |
|---|---|---|---|---|---|---|---|---|---|
| BTC-TSMOM-D1 | 10 | 12 | 3.0 | 1.0 | 0.4 | 17bp | 1.92 | 1.67 | 85% |
| ETH-TSMOM-D1 | 25 | 8 | 2.5 | 0.8 | 0.4 | 17bp | 3.15 | 3.17 | 91% |
| SOL-TSMOM-D1 | 10 | 20 | 2.0 | 0.5 | 0.3 | 20bp | 2.25 | 2.41 | 89% |
| LINK-TSMOM-D1 | 40 | 20 | 2.0 | 1.0 | 0.8 | 22bp | 2.18 | 1.92 | 100% |

### What was ruled out (no OOS edge after costs)

- DONCHIAN on H6/H1 (all symbols) — negative Sharpe
- BOLLINGER on H6 (all symbols) — negative Sharpe
- RSI_REVERT on H6 (all symbols) — too few trades or negative
- OVERNIGHT on H1 (BTC) — PF=0.31
- WEEKDAY on D1 (BTC) — PF=0.44

## Pending user action: DEPLOY

The user needs to run this from their **Mac terminal** (sandbox has no SSH keys):

```bash
cd ~/ChimeraCrypto && \
scp src/main.cpp jo@143.198.89.54:/home/jo/ChimeraCrypto/src/main.cpp && \
ssh jo@143.198.89.54 "cd /home/jo/ChimeraCrypto/build && cmake .. && make -j2 && sudo systemctl restart chimera"
```

Verify with:
```bash
curl -sk https://143.198.89.54:9443/api/state2 | python3 -m json.tool | head -40
```

Should show 4 engines in the `engines` array.

## VPS & access

- **VPS**: `jo@143.198.89.54` (Singapore, Ubuntu)
- **Dashboard**: https://143.198.89.54:9443/
- **API**: https://143.198.89.54:9443/api/state2
- **Trades**: https://143.198.89.54:9443/api/trades
- **Service**: `sudo systemctl restart chimera`
- **GitHub**: git@github.com:Trendiisales/Omega (SSH)

## What the next session should do: FIND NEW EDGES

The user said: "find me new edges." The 6 built-in EdgeEngine strategies on standard timeframes (D1, H6, H1) have been exhausted. Here's where to look:

### 1. H4 timeframe (never tested)

We tested D1 and H6 but never H4. TSMOM may work on H4 for the faster-moving alts (SOL, LINK). This requires:
- Synthesize H4 from existing H1 data (similar to how H6 synthesis works)
- Add H4 synthesis to scanner.cpp or create a dedicated H4 scanner
- Run optimizer_general if any H4 combos show promise

### 2. Multi-timeframe confirmation

Currently each engine is single-timeframe. A stronger signal could combine:
- D1 trend direction (TSMOM) as a filter
- H6 or H4 entry timing for better fill prices
- This would need EdgeEngine.hpp modification (user must approve)

### 3. Short-side capability

All engines are long-only. Adding short signals would:
- Capture downtrends (BTC/ETH have clear trending behaviour both ways)
- Roughly double the trade count
- This requires EdgeEngine.hpp changes (user must approve)

### 4. New strategy types

EdgeEngine supports 6 strategies but only TSMOM showed edge. Consider adding:
- **Mean reversion on D1** with wider bands (BB_K=2.5 or 3.0) — the scanner used BB_K=2.0 only
- **Volume-weighted momentum** — Binance klines include volume data we're not using
- **Volatility breakout** — enter on ATR expansion, not just price direction
- These would need new StrategyKind entries in EdgeEngine.hpp (user must approve)

### 5. More symbols

Only tested BTC/ETH/SOL/XRP/LINK. Could add:
- BNB (SYM_BNB=3, already in SymbolIndex)
- AVAX (SYM_AVAX=4)
- DOGE (SYM_DOGE=7)
- Need to download H1/D1 kline data for these from Binance first

### 6. Parameter re-optimization on fresh data

If the user has deployed and collected 2-4 weeks of shadow trades, compare live WR/PF against backtest. If divergence > 10%, re-optimize with expanded data window.

## Constraints (always apply)

- **All code is C++17**, header-only EdgeEngine
- **Always provide full files** — never diffs, snippets, or "add this here"
- **Don't modify EdgeEngine.hpp** unless the user explicitly says to
- **Shadow mode is intentional** — don't flag as a problem
- **Realistic costs**: 17bp BTC, 20bp mid alts (SOL/BNB), 22bp tail alts (LINK/AVAX/DOGE/XRP)
- **OOS validation required**: 80/20 split, neighbourhood stability ≥ 80%, PF > 1.15, Sharpe > 0.3
- **Deploy from Mac terminal only** — sandbox has no SSH keys to VPS

## Key files

| File | Purpose | Status |
|---|---|---|
| `src/main.cpp` | Live trading, 4 engines | UPDATED — needs deploy |
| `include/core/EdgeEngine.hpp` | Core engine (READ ONLY) | UNCHANGED |
| `backtest/scanner.cpp` | Multi-symbol strategy scanner | NEW |
| `backtest/optimizer_general.cpp` | CLI optimizer with stability scoring | NEW |
| `backtest/optimizer.cpp` | Original BTC-only optimizer | UNCHANGED |
| `backtest/backtest_harness.cpp` | Harness matching main.cpp | UPDATED |
| `backtest/data/` | 160 JSON kline files (BTC/ETH/SOL/XRP/LINK, H1+D1) | UNCHANGED |

## Build commands (all from `backtest/` directory)

```bash
g++ -std=c++17 -O2 -I../include backtest_harness.cpp -o backtest
g++ -std=c++17 -O2 -I../include scanner.cpp -o scanner
g++ -std=c++17 -O2 -I../include optimizer_general.cpp -o optimizer_general
g++ -std=c++17 -O2 -I../include optimizer.cpp -o optimizer
```

## Suggested skills for next session

None of the installed skills are directly relevant to crypto edge hunting. The work is: modify scanner/optimizer C++ code, run backtests, analyse results, update main.cpp if edge found.
