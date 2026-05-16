# Session 20 Handoff — Chimera Edge System

**Date**: 2026-05-16
**Status**: 61 engines wired in main.cpp, all shadow mode. VPS build succeeded. Spreadsheet updated.
**Pending**: Git commit/push from Mac, VPS service restart.
**Next session goal**: Hunt for additional regimes and untapped edges — look where other traders don't.

---

## What Was Done This Session

1. **Added 3 engines from existing symbols**: DOGE-TSMOM-H2 (PF=1.21), BNB-TSMOM-H2 (PF=1.19), DOGE-TSMOM-H3 (PF=1.25)
2. **Expanded to 4 new symbols**: Downloaded H1 klines from Binance for SUI (26,614 bars), APT (31,328 bars), NEAR (38,313 bars), ARB (27,594 bars)
3. **Ran optimizer on all new symbols across all timeframes (D1, H12, H6, H4, H3, H2, H1)**
4. **NEAR = strongest symbol ever tested** — passes 5 of 6 TFs:
   - D1: PF=2.79, Sharpe=2.61, Nbr=100%, 46 trades
   - H12: PF=1.92, Sharpe=3.03, Nbr=95%, 126 trades
   - H6: PF=1.85, Sharpe=3.62, Nbr=100%, 257 trades
   - H4: PF=2.17, Sharpe=3.59, Nbr=100%, 209 trades
   - H3: PF=1.75, Sharpe=3.65, Nbr=87%, 351 trades
5. **SUI passes H6** (PF=1.80, Nbr=100%) and **H4** (PF=1.44, Nbr=88%)
6. **APT passes H6** (PF=1.82, Nbr=92%)
7. **ARB passes H6** (PF=1.48, Nbr=80%)
8. **Wired all 12 new engines into main.cpp** (total 49 -> 61)
9. **Updated SymbolIndex.hpp**: MAX_SYMBOLS 8 -> 12, added SUI/APT/NEAR/ARB
10. **Updated chimera_engine_roster.xlsx** with all 3 sheets (Roster, Ranked, How It Works)

---

## Current Engine Fleet (61 total)

| TF  | Count | Symbols | Session |
|-----|-------|---------|---------|
| D1  | 6     | BTC, ETH, SOL, LINK, BNB, NEAR | S13-14, S20 |
| H12 | 4     | BTC, DOGE, AVAX, NEAR | S14, S20 |
| H6  | 12    | All 8 original + NEAR, SUI, APT, ARB | S15, S20 |
| H4  | 9     | XRP, BNB, LINK, SOL, BTC, ETH, AVAX, NEAR, SUI | S14, S20 |
| H3  | 7     | BTC, ETH, SOL, XRP, LINK, BNB, DOGE | S17, S20 |
| H2  | 7     | BTC, ETH, SOL, XRP, LINK, DOGE, BNB | S17, S20 |
| H1  | 3     | XRP, SOL, LINK | S15 |
| Counter-trend | 12 | ETH, DOGE, AVAX, BTC, BNB, XRP, LINK | S19 |
| **NEAR** | 5 | D1, H12, H6, H4, H3 (best symbol in fleet) | S20 |

---

## Deploy Protocol — READ THIS

**CRITICAL RULES:**
- **SSH ONLY for git** — `git@github.com:Trendiisales/ChimeraCrypto.git`. NEVER use PAT/HTTPS.
- **VPS has NO GitHub SSH key** — cannot `git pull` on VPS. Will fail.
- **Deploy via SCP from Mac**, never git pull on VPS:

```bash
# From Mac terminal:
scp ~/ChimeraCrypto/src/main.cpp jo@143.198.89.54:~/ChimeraCrypto/src/main.cpp
scp ~/ChimeraCrypto/include/core/SymbolIndex.hpp jo@143.198.89.54:~/ChimeraCrypto/include/core/SymbolIndex.hpp
# (add any other changed files)

# On VPS (ssh jo@143.198.89.54):
cd ~/ChimeraCrypto/build && make -j2
sudo systemctl restart chimera
```

- **Git commit/push from Mac only**: `cd ~/ChimeraCrypto && git add -A && git commit -m "msg" && git push`
- **This sandbox CANNOT SSH to VPS** — deploy commands must be run by the user in Mac terminal
- **DO NOT MODIFY EdgeEngine.hpp** without explicit user permission

---

## VPS Details

- **IP**: 143.198.89.54 (Singapore, DigitalOcean)
- **SSH user**: jo (NOT root — `/home/jo/ChimeraCrypto/`, NOT `/root/`)
- **Service**: `chimera.service` (systemd)
- **Dashboard**: https://143.198.89.54:9443/
- **API**: `/api/state2`, `/api/trades`, `/api/kill`
- **Build**: cmake + make in `/home/jo/ChimeraCrypto/build/`

---

## What's Been Tested & Ruled Out

### Strategies fully tested on original 8 symbols (BTC, ETH, SOL, BNB, AVAX, LINK, XRP, DOGE):
- **TSMOM**: Fully exhausted across D1, H12, H6, H4, H3, H2, H1 — all passing combos deployed
- **RSI_REVERT**: 12 counter-trend engines deployed (Session 19). Remaining combos below threshold.
- **BOLLINGER**: 12 counter-trend engines deployed (Session 19). Remaining combos below threshold.
- **DONCHIAN**: Tested across all symbols/TFs, no OOS edge after costs
- **OVERNIGHT**: Tested on BTC only — structurally dead for 24/7 crypto markets (PF=0.31)
- **WEEKDAY**: Tested on BTC only — no edge (PF=0.44)

### New symbols tested on TSMOM:
- **NEAR**: 5/6 TFs pass (all except H2) — NEAR-H2 was not tested or failed
- **SUI**: H6 and H4 pass, others fail
- **APT**: Only H6 passes
- **ARB**: Only H6 passes
- **AVAX-H3**: Top result is FRAGILE (Nbr=26%), but one config at LB=5,HB=12 shows Nbr=40% "GOOD"

### Not yet tested on new symbols (NEAR, SUI, APT, ARB):
- DONCHIAN, BOLLINGER, RSI_REVERT across all TFs
- OVERNIGHT, WEEKDAY

---

## Next Session: Regime Hunting

The user wants to find **additional regimes** — edges where other traders don't trade, or hidden gems. Here are the opportunities to explore:

### 1. Counter-Trend on New Symbols (HIGH PRIORITY)
RSI_REVERT and BOLLINGER were only tested on the original 8 symbols. NEAR/SUI/APT/ARB have NOT been tested with counter-trend strategies. Given NEAR's exceptional TSMOM performance, its mean-reversion characteristics could be equally strong.

**Action**: Run `optimizer_general` with RSI_REVERT and BOLLINGER kinds for nearusdt, suiusdt, aptusdt, arbusdt across H1-H6 timeframes.

### 2. Exotic Timeframes (MEDIUM PRIORITY)
No engines exist for these timeframes:
- **H8** (8-hour): Between H6 and H12, potentially a sweet spot
- **H16**: Between H12 and D1
- **D2** / **D3**: Multi-day momentum, fewer trades but potentially very high PF
- **W1** (weekly): Weekly momentum — very few trades but could be extremely robust
These require the optimizer to synthesize from H1 bars (same as H3 does today).

### 3. Volume-Weighted / Regime-Filtered Entries (MEDIUM-HIGH PRIORITY)
Current TSMOM is pure price momentum. Possible enhancements:
- **Volume breakout filter**: Only enter TSMOM when volume exceeds N-bar average (confirms conviction)
- **Volatility regime filter**: Only trade TSMOM when ATR is within a range (avoid chop / avoid crash)
- **Correlation regime**: Only trade alts when BTC momentum aligns (lead-lag already in the codebase concept)
These would require adding new StrategyKind entries to EdgeEngine.hpp (user permission needed).

### 4. Cross-Symbol / Relative Strength (LOW-MEDIUM PRIORITY)
- **Pair rotation**: Instead of absolute TSMOM, rank symbols by relative strength and only trade the top N
- **BTC-lead filter**: Only enter alt positions when BTC TSMOM is also positive (reduces whipsaw in alt-driven drawdowns)
Would require new engine logic.

### 5. Time-of-Day / Session Effects (LOW PRIORITY but unexplored)
- OVERNIGHT was only tested on BTC at 21:00 UTC
- Asian session (00:00-08:00 UTC) vs European (08:00-16:00) vs US (16:00-00:00) effects
- Specific hour-of-day entry filters for H1 engines
Probably dead for crypto (24/7 market) but worth a quick scan on NEAR/XRP which show strongest edges.

### 6. Additional Symbols to Screen (OPTIONAL)
Beyond the 12 already in the system, candidates:
- **PEPE**, **WIF**, **BONK** — meme coins with extreme momentum properties
- **FIL**, **ATOM**, **DOT** — L1 infrastructure tokens
- **OP**, **MATIC/POL** — L2 tokens
- **TRX**, **TON** — high-volume, different community
Download H1 klines via `backtest/download_klines.py` (edit SYMBOLS list), add to SymbolIndex.hpp.

---

## Key Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | All 61 engine configs + g_slots registration |
| `include/core/EdgeEngine.hpp` | **DO NOT MODIFY** — header-only engine with all strategy logic |
| `include/core/SymbolIndex.hpp` | Symbol registry (12 symbols, MAX_SYMBOLS=12) |
| `backtest/optimizer_general.cpp` | Grid search optimizer — 15,552 combos per symbol/TF |
| `backtest/download_klines.py` | Binance H1 kline downloader |
| `backtest/data/` | 313 JSON part files across 12 symbols |
| `chimera_engine_roster.xlsx` | Master spreadsheet (3 sheets) |
| `gui/index.html` | Dashboard UI |

---

## Deploy Criteria (unchanged)

- PF > 1.15
- Sharpe > 0.3
- Neighbourhood stability >= 80% (preferred, 40% minimum for "OK" grade)
- Minimum 8 OOS trades
- All engines start in shadow mode, promote after 4 weeks matching backtest +/- 10%

---

## Optimizer Usage

```bash
# Compile (in sandbox or on Mac):
cd backtest && g++ -O2 -std=c++17 -o optimizer_general optimizer_general.cpp -I../include

# Run:
./optimizer_general <symbol> <timeframe> [cost_bp]
# e.g.: ./optimizer_general nearusdt H6 22

# Timeframes: D1, H12, H6, H4, H3, H2, H1
# Cost defaults: D1=17bp, H12=20bp, H6=22bp, H4=22bp, H3=22bp, H2=20bp, H1=22bp
```

Data must exist in `backtest/data/<symbol>_h1_part*.json`. Use `download_klines.py` to fetch.

---

## Suggested Skills for Next Session

- None specifically — this is raw C++ backtesting and grid search work. The optimizer runs in the sandbox.
- If creating new strategy kinds: user must grant permission to modify EdgeEngine.hpp first.
