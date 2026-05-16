# Chimera Session 15 Handoff — More Edges

## Current State (after Session 14)

18 TSMOM engines deployed to VPS (143.198.89.54), all shadow mode, binary built and restarted.

### Deployed Engines

**D1 (5):** BTC (PF=1.92), ETH (PF=3.15), SOL (PF=2.25), LINK (PF=2.18), BNB (PF=3.16)
**H4 (7):** XRP (PF=2.43), BNB (PF=1.91), LINK (PF=1.91), SOL (PF=1.89), BTC (PF=1.82), ETH (PF=1.76), AVAX (PF=1.47)
**H12 (3):** BTC (PF=3.63), DOGE (PF=2.78), AVAX (PF=2.61)

All pass deploy criteria: PF > 1.15, Sharpe > 0.3, Neighbourhood stability >= 80%.

### What Was Ruled Out (don't re-test)

- **Strategies**: DONCHIAN, BOLLINGER, RSI_REVERT, OVERNIGHT, WEEKDAY — no edge on any symbol/TF
- **Combos**: DOGE-TSMOM-H4 (Nbr=49%, isolated peak)
- **Wider Bollinger K=2.5/3.0**: Scanned but no edge after costs

### Key Session 14 Discovery

H4 and H12 timeframes had never been tested. Scanner v2 found 61% of H12 combos and 51% of H4 combos profitable (vs only 6.9% on D1). This was the single biggest edge discovery in the project's history.

## Untested Avenues for Session 15

1. **Short-side signals** — EdgeEngine currently long-only. Adding short would require modifying EdgeEngine.hpp (core code — needs explicit user permission). Could double trade count.

2. **Volume-weighted momentum** — TSMOM uses pure price momentum (close vs close N bars ago). Volume confirmation could filter false signals. Would need new StrategyKind or TSMOM enhancement.

3. **Multi-TF confirmation** — e.g. only take H4 entries when D1 trend agrees. Could improve win rate at cost of trade count. Implementable as a filter layer outside EdgeEngine.

4. **H1 TSMOM full optimization** — Scanner showed some H1 combos profitable but the optimizer was never run on them. High trade count = more statistical power but also more cost drag (17-22bp per round trip).

5. **Regime filters** — Gate entries on volatility regime (high ATR = trending = better for TSMOM) or trend strength (ADX). Could be added as a pre-filter in main.cpp without touching EdgeEngine.

6. **Cross-pair correlation** — When BTC trends, alts often follow. Could size positions based on BTC regime or add/remove engines dynamically.

7. **Dynamic position sizing** — Currently flat 1x per engine. Kelly criterion or volatility-targeting could improve risk-adjusted returns.

## Tools Available

- `backtest/scanner_v2.cpp` — 1104-combo multi-TF/multi-symbol sweep
- `backtest/optimizer_general.cpp` — 15,552-combo grid search with neighbourhood stability (supports D1/H12/H6/H4/H1)
- `backtest/backtest_harness.cpp` — exact-match backtest of all 18 engines
- `backtest/data/` — H1 + D1 klines for all 8 symbols (BTC/ETH/SOL/LINK/BNB/XRP/AVAX/DOGE)

## Deploy Protocol

**Git push**: FROM MAC ONLY, SSH only (`git@github.com:Trendiisales/ChimeraCrypto.git`). NEVER use PATs or HTTPS.

**VPS deploy**:
```
# From Mac:
scp <files> jo@143.198.89.54:/home/jo/ChimeraCrypto/<path>
# Then SSH in and:
cd /home/jo/ChimeraCrypto/build && cmake .. && make -j2 && sudo systemctl restart chimera
```

**NOTE**: Mac SSH to jo@VPS had "Permission denied (publickey)" in Session 14. May need SSH key fix before SCP works. User got around it another way.

**VPS root has NO GitHub SSH key** — never push from VPS.

## Code Architecture (read-only reference)

- `include/core/EdgeEngine.hpp` — DO NOT MODIFY without explicit permission. Header-only, 724 lines. Supports TSMOM/DONCHIAN/BOLLINGER/RSI_REVERT/OVERNIGHT/WEEKDAY.
- `include/core/SymbolIndex.hpp` — 8 symbols: BTC=0, ETH=1, SOL=2, BNB=3, AVAX=4, LINK=5, XRP=6, DOGE=7
- `src/main.cpp` — 18 engines, HTTP GUI, WebSocket feed, trade journal
- Config defaults: atr_period=14, bb_k=2.0, rsi_threshold=30.0, max_history=64

## Pending Items

- [ ] Git commit + push from Mac (Session 14 changes not yet committed)
- [ ] Verify 18 engines running: `curl https://143.198.89.54:9443/api/state2 | jq '.engines | length'`
- [ ] Monitor shadow trades over coming days to validate live matches backtest
