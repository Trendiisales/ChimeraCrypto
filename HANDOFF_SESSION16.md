# Session 16 Handoff — Chimera Edge System

**Date**: 2026-05-16
**Status**: 29 TSMOM engines deployed, all shadow mode, zero trades
**Next session goal**: Find the next edges

---

## What Was Done This Session

1. **Fixed config() accessor bug** — `EdgeEngine` has no public `config()` accessor (`cfg_` is private). The seeding loop in main.cpp was using `slot.engine->config()` which wouldn't compile. Fixed by adding `symbol_str`, `tf_secs`, `tag` fields to `EngineSlot` struct and populating from the config variables at push_back time.

2. **Redesigned GUI v2** — Complete rewrite of `gui/index.html`. Old design had oversized cards that couldn't handle 29 engines. New design:
   - Timeframe tabs (ALL / D1 / H12 / H6 / H4 / H1) with engine count badges
   - Compact table rows with backtest metadata (OOS PF, Sharpe, Nbr%, session)
   - Open positions panel (appears only when in trade) with live unrealized P&L, trailing stop state, MFE
   - Equity curve + P&L-by-engine horizontal bar chart
   - Trade history with per-engine filter buttons
   - Same API endpoints: `/api/state2`, `/api/trades`, `/api/kill`

3. **Built and deployed** — `make -j2` passed clean with LTO on VPS. Service restarted.

---

## Current Engine Fleet (29 total)

| TF  | Count | Symbols | Session | Notes |
|-----|-------|---------|---------|-------|
| D1  | 5     | BTC, ETH, SOL, LINK, BNB | S13-14 | Original engines |
| H12 | 3     | BTC, DOGE, AVAX | S14 | |
| H6  | 8     | All 8 symbols | S15 | **Strongest TF ever** — every symbol passes |
| H4  | 7     | XRP, BNB, LINK, SOL, BTC, ETH, AVAX | S14 | |
| H1  | 3     | XRP, SOL, LINK | S15 | Only 3/8 pass (cost drag kills the rest) |

Full parameter details for all 29 engines: see memory file `project_chimera_state.md`

---

## Deploy Protocol — READ THIS

**CRITICAL RULES:**
- **SSH ONLY for git** — `git@github.com:Trendiisales/ChimeraCrypto.git`. NEVER use PAT/HTTPS. The PAT is dead.
- **VPS has NO GitHub SSH key** — cannot `git pull` on VPS. Will fail with "Permission denied (publickey)".
- **Deploy via SCP from Mac**, never git pull on VPS:

```bash
# From Mac terminal:
scp ~/ChimeraCrypto/src/main.cpp jo@143.198.89.54:~/ChimeraCrypto/src/main.cpp
scp ~/ChimeraCrypto/gui/index.html jo@143.198.89.54:~/ChimeraCrypto/gui/index.html
# (add any other changed files)

# On VPS:
cd ~/ChimeraCrypto/build && make -j2
sudo systemctl restart chimera
```

- **Git commit/push from Mac only**: `cd ~/ChimeraCrypto && git add -A && git commit -m "msg" && git push`
- **This sandbox CANNOT SSH to VPS** — all deploy commands must be run by the user in their Mac terminal
- **DO NOT MODIFY EdgeEngine.hpp** without explicit user permission

---

## VPS Details

- **IP**: 143.198.89.54 (Singapore, DigitalOcean)
- **SSH user**: jo
- **Service**: `chimera.service` (systemd)
- **Dashboard**: https://143.198.89.54:9443/
- **API**: https://143.198.89.54:9443/api/state2
- **Trades**: https://143.198.89.54:9443/api/trades
- **Build**: cmake + make in `/home/jo/ChimeraCrypto/build/`

---

## What's Been Ruled Out

- **Strategies**: DONCHIAN, BOLLINGER, RSI_REVERT, OVERNIGHT, WEEKDAY — all tested across all symbols, no OOS edge after costs
- **DOGE-TSMOM-H4**: Nbr=49% — isolated peak, not robust
- **H1 TSMOM for BTC/ETH/BNB/DOGE/AVAX**: cost drag (17-22bp) eats the edge at high frequency

---

## Where to Find Next Edges

### Available strategies in EdgeEngine (StrategyKind enum):
TSMOM, DONCHIAN, BOLLINGER, RSI_REVERT, OVERNIGHT, WEEKDAY — all are wired up and ready to optimize.

### Untested avenues (priority order):

1. **Short-side TSMOM** — EdgeEngine is currently long-only. Would require core code modification (user permission needed). Huge potential — crypto has violent drawdowns that momentum shorts could capture.

2. **New symbols** — Only 8 symbols tested (BTC, ETH, SOL, LINK, BNB, AVAX, XRP, DOGE). Candidates: AAVE, DOT, MATIC, NEAR, APT, ARB, OP, SUI. Would need adding to SymbolIndex.hpp and BinanceWSFeed.

3. **H2 / H3 timeframes** — H6 was a goldmine nobody expected. H2 and H3 are between H1 (too noisy/costly) and H4 (already tested). Could be sweet spots. optimizer_general.cpp's `synthesize_tf()` can build any timeframe from H1 data.

4. **Volume-weighted momentum** — Use volume to weight momentum signal strength. Would need new strategy implementation in EdgeEngine.

5. **Regime filters (ATR/ADX gating)** — Only enter when volatility or trend strength is above threshold. Needs external gate mechanism in EdgeEngine (core mod, requires permission).

6. **Multi-TF confirmation** — Only enter H4 trade if D1 also in uptrend. Tested briefly in S15, couldn't implement without core mod since entries are decided internally by EdgeEngine.

7. **Wider Bollinger bands (K=2.5/3.0)** — Scanner showed some combos profitable but not fully optimized. BOLLINGER was ruled out at K=2.0 but wider bands might work on higher TFs.

### Key tool for finding edges:

```bash
# On VPS — optimizer_general.cpp
cd ~/ChimeraCrypto/backtest
./optimizer_general <symbol> <strategy> <tf> <cost_bp>

# Example: optimize AAVE TSMOM on H6 with 22bp costs
./optimizer_general aaveusdt TSMOM H6 22

# Runs 15,552 parameter combos, reports:
# - Top 10 by Profit Factor
# - Neighbourhood stability scores
# - OOS 80/20 split validation
```

### Deploy criteria for new engines:
- PF > 1.15
- Sharpe > 0.3
- Neighbourhood stability >= 80%

---

## Key Files

| File | Purpose | Modify? |
|------|---------|---------|
| `src/main.cpp` | All 29 engine configs, HTTP server, seeding, trade journal | YES — add new engines here |
| `gui/index.html` | Dashboard v2 with TF tabs | YES — update if adding features |
| `include/core/EdgeEngine.hpp` | Core engine logic | **NO** — needs explicit permission |
| `include/core/SymbolIndex.hpp` | Symbol enum + name mapping | YES — add new symbols here |
| `backtest/optimizer_general.cpp` | 15,552-combo grid search | YES — add new TFs/strategies |
| `backtest/backtest_harness.cpp` | Matches main.cpp engine set | NEEDS UPDATE to 29 engines |

---

## Pending Items

- `backtest_harness.cpp` still configured for 18 engines (needs update to 29)
- Zero trades generated — all engines are shadow mode (this is intentional, don't flag it)
- No live capital deployed yet

---

## User Preferences

- Always provide **full files**, never diffs/snippets/patches
- Shadow mode is intentional — treat paper trades as live, don't suggest switching
- SSH only for git — never PATs, never HTTPS
- Warn at 70% context usage with summary
- Don't modify core code (EdgeEngine.hpp) unless clearly instructed
