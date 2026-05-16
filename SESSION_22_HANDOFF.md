# Session 22 Handoff — Chimera Crypto Trading System

## System State After Session 22

**VPS**: 143.198.89.54 (Singapore) — RUNNING, rebuilt and restarted
**Engines**: 230 total (was 192), all shadow mode, spot-long-only
**Binary**: Rebuilt and restarted via `sudo systemctl restart chimera`
**Git**: Lock file issue — data/ too large for git. Use specific file adds:
```bash
rm -f ~/ChimeraCrypto/.git/index.lock
git add src/main.cpp chimera_engine_roster.xlsx .gitignore backtest/extend_h1_data.py backtest/backfill_h1.py backtest/monte_carlo_pf.py backtest/optimizer_general.cpp
git commit -m "Session 22"
git push
```

---

## What Session 22 Accomplished

### 1. Extended Historical Data (MAJOR)
Downloaded maximum available H1 kline history from Binance for all 12 symbols:

| Symbol | Before | After | Coverage |
|--------|--------|-------|----------|
| BTC | 30K bars (1.1yr) | 96.5K bars (8.7yr) | 2017-08 → 2026-05 |
| ETH | 30K (1.1yr) | 96.5K (8.7yr) | 2017-08 → 2026-05 |
| NEAR | 38K (1.1yr) | 105.5K (5.6yr) | 2020-10 → 2026-05 |
| XRP | 30K (1.1yr) | 90.2K (8.0yr) | 2018-05 → 2026-05 |
| LINK | 30K (1.1yr) | 84.2K (7.3yr) | 2019-01 → 2026-05 |
| SOL | 30K (1.1yr) | 70.4K (5.8yr) | 2020-08 → 2026-05 |
| APT | 31K (1.1yr) | 52.7K (3.6yr) | 2022-10 → 2026-05 |
| ARB | 27K (1.1yr) | 45.2K (3.1yr) | 2023-03 → 2026-05 |
| SUI | 26K (1.1yr) | 43.2K (3.0yr) | 2023-05 → 2026-05 |
| BNB | 38K (4.4yr) | 38K (4.4yr) | unchanged |
| AVAX | 38K (4.4yr) | 38K (4.4yr) | unchanged |
| DOGE | 38K (4.4yr) | 38K (4.4yr) | unchanged |

Scripts saved in `backtest/`:
- `extend_h1_data.py` — forward + backfill downloader (run periodically to keep data current)
- `backfill_h1.py` — backfill-only (pre-listing-date data)
- `monte_carlo_pf.py` — bootstrap PF confidence intervals

### 2. Monte Carlo Validation
Ran 10,000 bootstrap simulations on 42 high-PF engines:
- **16 STRONG** (5th percentile PF >= 1.15)
- **6 OK** (5th percentile PF >= 1.0)
- **20 MARGINAL** (P(PF>1.15) >= 60%)
- **0 WEAK** — every engine has positive expected PF

All 20 MARGINAL engines were re-run with extended data and **all promoted** — trade counts jumped from 8-14 to 20-328.

### 3. Overfit Detection
**SOL-RSI-H6 removed** — extended data exposed overfit:
- Before: PF 38.77, Sharpe 0.82, Nbr 100%, 12 trades
- After: PF 1.24, Sharpe 0.56, Nbr 31%, 46 trades
- This is exactly what extended data testing is for.

### 4. New Edges Discovered (39 brand new + 18 re-optimized)

**RSI_REVERT H8** (9 engines): Counter-trend at 8-hour timeframe
- Best: XRP (PF 14.85, Nbr 98%), ETH (PF 1.70, Nbr 100%, 49 trades)

**BOLLINGER H8** (12 engines): ALL 12 symbols pass
- Best: LINK (PF 6.83, Nbr 86%), BNB (PF 5.10), SOL (PF 4.44, Nbr 81%)

**RSI_REVERT H16** (8 engines): Counter-trend at 16-hour
- Best: ETH (PF 158.17, Sharpe 9.07, Nbr 100%, 24 trades)

**BOLLINGER H16** (5 engines): Sparse but strong
- Best: LINK (PF 6.77, Nbr 100%)

**DONCHIAN H8** (5 engines): Breakout at 8-hour — previously failed, now works
- Best: XRP (PF 3.05, Nbr 100%, 45 trades), NEAR (PF 2.43, Nbr 73%, 55 trades)

**DONCHIAN H16** (8 engines): Breakout at 16-hour
- Best: BNB (PF 9.25, Nbr 65%), XRP (PF 4.88, Nbr 100%)

**DONCHIAN D2** (5 engines): 2-day breakout — brand new territory
- Best: BNB (PF 99.90, Nbr 93%), XRP (PF 10.03, Nbr 100%), BTC (PF 5.56, Nbr 84%)

**DONCHIAN D3** (5 engines): 3-day breakout
- Best: BTC (PF 128.98, Nbr 98%), ETH (PF 9.21, Nbr 87%), XRP (PF 5.39, Nbr 100%)

### 5. Re-validation of ALL Existing Engines
Every TSMOM H8/H16 engine re-run with extended data — all confirmed:
- ETH-TSMOM-H8: 14 trades → **282 trades**, PF 1.60, Sharpe 2.26
- XRP-TSMOM-H8: 12 → **328**, PF 1.97, Sharpe 3.26
- BNB-TSMOM-H8: 20 → **138**, PF 2.86, Sharpe 3.67
- BTC-TSMOM-D2: 8 → **49**, PF 8.34, Sharpe 4.08

---

## Engine Roster Breakdown (230 total)

| Strategy | Count |
|----------|-------|
| TSMOM | 95 |
| RSI_REVERT | 51 |
| BOLLINGER | 44 |
| DONCHIAN | 28 |

| Timeframe | Count |
|-----------|-------|
| H8 | 39 |
| H6 | 37 |
| H16 | 33 |
| H4 | 18 |
| D2 | 16 |
| D3 | 16 |
| H12 | 16 |
| H3 | 15 |
| H2 | 14 |
| D1 | 9 |
| H1 | 5 |

---

## Key Files Modified

- `src/main.cpp` — 230 engines (5670 lines), Session 22 block starts around line 4491
- `chimera_engine_roster.xlsx` — 228 engines in spreadsheet (3 sheets: Roster, Ranked, How It Works)
- `backtest/optimizer_general.cpp` — supports all TFs including D3/D2/H16/H8
- `backtest/data/` — extended H1 klines (600+ JSON files, DO NOT put in git)
- `.gitignore` — added `backtest/data/` to prevent git OOM kills

---

## Next Session Priorities

1. **Git push still pending** — lock file issue needs resolution (use specific file adds, not `git add -A`)
2. **Shadow monitoring** — 230 engines now live. After 4 weeks, evaluate which match backtest ±10%
3. **Walk-forward analysis** — the next validation level (split data into rolling windows)
4. **Possible new strategies** — would require EdgeEngine.hpp modification (user must explicitly approve):
   - EMA crossover
   - MACD divergence  
   - Volume-weighted momentum
   - Volatility regime filter (ATR-based)
5. **Re-run extend_h1_data.py periodically** to keep data current (monthly or before re-optimization)
6. **Consider promoting strongest engines to LIVE** after shadow validation:
   - XRP-TSMOM-H8 (328 trades, PF 1.97, Sharpe 3.26)
   - ETH-TSMOM-H8 (282 trades, PF 1.60, Sharpe 2.26)
   - NEAR-TSMOM-H8 (207 trades, PF 1.86, Sharpe 3.17)
   - BNB-TSMOM-H8 (138 trades, PF 2.86, Sharpe 3.67)

---

## Deploy Protocol Reminder

- **Mac only** for git push (SSH key is on Mac)
- **SCP from Mac** to VPS (VPS has no GitHub SSH key)
- **VPS build**: `cd /home/jo/ChimeraCrypto/build && cmake .. && make -j$(nproc)`
- **Restart**: `sudo systemctl restart chimera`
- **Shadow mode**: All engines paper-trade. Promote to live after 4 weeks matching backtest ±10%
