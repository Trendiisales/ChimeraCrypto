# Session 23 Handoff — Chimera Crypto Trading System

## System State After Session 23

**VPS**: 143.198.89.54 (Singapore) — RUNNING, all systems go
**Engines**: 230 total, ALL 230 signal-ready (was 135/230 before fix)
**Binary**: Rebuilt and restarted, confirmed live with 5 immediate D3 entries
**Open Positions at handoff**: ETH-TSMOM-D3, LINK-TSMOM-D3, BNB-TSMOM-D3, DOGE-TSMOM-D3, SUI-TSMOM-D3
**Git**: Not pushed yet — push from Mac with specific file adds (same protocol as Session 22)

---

## What Session 23 Accomplished

### 1. Diagnosed Zero-Trade Problem (CRITICAL FIX)
**Root cause**: The warm-start seeding logic had a bug where partial saved bar files
(e.g. 11 bars accumulated from short restarts) would BLOCK the REST API fetch that
provides 64 bars. Result: 95 of 230 engines stuck below their lookback threshold,
unable to evaluate signals.

**Fix**: Rewrote seeding strategy:
- Engines with native Binance intervals (H1,H2,H4,H6,H8,H12,D1,D3): ALWAYS use REST (64 bars guaranteed)
- Engines without native intervals (H3,H16,D2): NEW H1 aggregation (fetch H1 klines, aggregate to target TF)
- Saved bars only used as last-resort fallback for exotic TFs when aggregation fails

**Result**: `REST=161  H1-agg=69  saved=0  cold=0` — ALL 230 engines seeded, ZERO cold starts.

### 2. H1 Kline Aggregation for Exotic Timeframes (NEW)
Added `seed_engine_from_h1_aggregation()` function that:
- Fetches up to 1000 H1 bars from Binance REST API
- Aggregates them into H3 (3h), H16 (16h), or D2 (48h) bars
- Aligns bar boundaries to epoch (same as live tick bar synthesis)
- Includes the final partially-complete bar (critical for D2 where 1000/48 ≈ 21 bars)

Coverage:
- H3 engines: 192 H1 bars → 64 complete H3 bars
- H16 engines: 1000 H1 bars → 62 complete H16 bars
- D2 engines: 1000 H1 bars → 21 D2 bars (just enough for lookback=20)

### 3. Added `/api/positions` Endpoint (NEW)
`GET /api/positions` returns only engines currently holding a position, with:
- tag, symbol, timeframe (human-readable "6H", "1D", etc.)
- entry_px, spot_px, unrealised P&L in basis points
- sl_px, trail_stop_px, trail_armed status
- mfe_bp (max favourable excursion), bars_held
- oos_pf, oos_sharpe, session (backtest metadata)

### 4. Improved Startup Logging
- Progress counter during seeding (every 25 engines)
- Summary line: `REST=N  H1-agg=N  saved=N  cold=N`
- Fixed stale print messages (was "49 engines", now shows actual count)
- Logs go to `/home/jo/ChimeraCrypto/logs/chimera.log` (not journalctl)

### 5. Confirmed System Is Trading
Within seconds of restarting with the fix, 5 D3 TSMOM engines entered positions.
Signal monitor showing bullish/bearish evaluations across all timeframes. Ticks flowing
at 10,000+ per few seconds across all 12 symbols.

---

## Files Modified
- `src/main.cpp` — seeding fix, H1 aggregation, /api/positions, startup logging

---

## Current Engine Coverage Matrix

```
Strategy x Timeframe (symbols covered out of 12):

          H1   H2   H3   H4   H6   H8  H12  H16   D1   D2   D3
TSMOM      3    7    8    9   12   12    4   12    6   11   11
RSI        2    8    7    5   10   10    7    8    3    0    0
BOLL       1    3    5    4   12   12    5    5    0    0    0
DONCH      0    0    0    1    4    5    0    8    0    5    5

Total: 230/528 possible combos = 43% coverage
```

---

## Strategy for Finding More Engines/Regimes

### PRIORITY 1: Fill Coverage Gaps (No Code Changes Required)
These use existing strategies in EdgeEngine.hpp — just need optimizer runs + main.cpp entries.

#### A. DONCHIAN Expansion (currently 28 engines, most underexploited strategy)
DONCHIAN has ZERO engines on H1, H2, H3, H12, D1 and only 1 on H4.
The H8/H16/D2/D3 results were strong (PF 3-10+), suggesting breakout works well
on multiple timeframes that haven't been tested.

**Run these optimizer sweeps:**
```bash
cd backtest
for SYM in btcusdt ethusdt solusdt bnbusdt xrpusdt linkusdt dogeusdt avaxusdt suiusdt aptusdt nearusdt arbusdt; do
  for TF in H2 H3 H4 H6 H12 D1; do
    ./optimizer_general $SYM DONCHIAN $TF 20 >> donchian_sweep.txt 2>&1
  done
done
```
Expected yield: 20-40 new engines. DONCHIAN on H4/H6/H12 are completely untested.

#### B. RSI_REVERT on Daily+ Timeframes (currently 0 engines on D1/D2/D3)
Counter-trend mean-reversion on daily timeframes is a known edge in crypto.
Oversold RSI bounces on D1/D2 capture multi-day reversals.

```bash
for SYM in btcusdt ethusdt solusdt bnbusdt xrpusdt linkusdt dogeusdt avaxusdt suiusdt aptusdt nearusdt arbusdt; do
  for TF in D1 D2 D3; do
    ./optimizer_general $SYM RSI_REVERT $TF 20 >> rsi_daily_sweep.txt 2>&1
  done
done
```
Expected yield: 10-20 new engines. The daily RSI bounce is one of the most robust crypto signals.

#### C. BOLLINGER on Daily+ Timeframes (currently 0 engines on D1/D2/D3)
Bollinger band touches on daily bars capture volatility mean-reversion.

```bash
for SYM in btcusdt ethusdt solusdt bnbusdt xrpusdt linkusdt dogeusdt avaxusdt suiusdt aptusdt nearusdt arbusdt; do
  for TF in D1 D2 D3; do
    ./optimizer_general $SYM BOLLINGER $TF 20 >> boll_daily_sweep.txt 2>&1
  done
done
```
Expected yield: 10-15 new engines.

#### D. Fill TSMOM Gaps (H12 only has 4/12 symbols)
```bash
for SYM in ethusdt solusdt bnbusdt linkusdt xrpusdt dogeusdt suiusdt aptusdt; do
  ./optimizer_general $SYM TSMOM H12 20 >> tsmom_h12_sweep.txt 2>&1
done
```

### PRIORITY 2: New Strategy Types (Requires EdgeEngine.hpp Modification)
These need new StrategyKind entries and signal functions. **User must explicitly approve.**

#### A. EMA Crossover (Trend-Following)
**Signal**: Fast EMA (e.g. 8) crosses above Slow EMA (e.g. 21)
**Why**: Different signal characteristics than TSMOM — captures trend starts rather than trend continuations. Less correlated with existing TSMOM signals.

Implementation:
- Add `EMA_CROSS` to StrategyKind enum
- Add ema_fast_/ema_slow_ helper functions (exponential moving average)
- Signal fires when fast EMA crosses above slow EMA on bar close
- Config params: `ema_fast_len` (default 8), `ema_slow_len` (default 21)

#### B. MACD Histogram Reversal (Momentum Shift)
**Signal**: MACD histogram turns positive (crosses zero from below)
**Why**: Detects momentum shifts earlier than TSMOM. Catches the transition from bearish to bullish momentum.

Implementation:
- Add `MACD_REV` to StrategyKind
- MACD line = EMA(12) - EMA(26), Signal line = EMA(9) of MACD
- Histogram = MACD - Signal. Fire when histogram crosses zero from below.
- Config params: `macd_fast` (12), `macd_slow` (26), `macd_signal` (9)

#### C. Keltner Channel Breakout (Volatility Breakout)
**Signal**: Close breaks above upper Keltner channel (SMA + ATR_mult * ATR)
**Why**: Similar to Bollinger but uses ATR instead of standard deviation. More stable in trending markets, less prone to false signals during low-vol consolidation.

Implementation:
- Add `KELTNER` to StrategyKind
- Upper channel = SMA(lookback) + keltner_mult * ATR(atr_period)
- Signal fires when close > upper channel
- Config params: `keltner_mult` (default 2.0)

#### D. Bollinger Squeeze → Breakout (Volatility Regime)
**Signal**: Bollinger bandwidth drops below threshold (squeeze), then price breaks above upper band
**Why**: Low-volatility consolidation followed by expansion is one of the highest-probability setups in crypto. Catches explosive moves.

Implementation:
- Add `BOLL_SQUEEZE` to StrategyKind
- Track Bollinger bandwidth over lookback bars
- Squeeze detected when bandwidth < threshold (e.g. 20th percentile)
- Signal fires when squeeze is active AND close > upper Bollinger band
- Config params: `squeeze_percentile` (default 20), same bb_k for bands

#### E. Volume-Weighted Breakout (Enhanced DONCHIAN)
**Signal**: DONCHIAN breakout confirmed by above-average volume
**Why**: Reduces false breakouts. Breakouts with volume are far more likely to sustain.
**Dependency**: Requires volume data in bar synthesis (currently only OHLC tracked).

### PRIORITY 3: Regime Filters (Overlay on Existing Strategies)
These modify when existing engines are ALLOWED to trade, not what signals they generate.

#### A. ATR Regime Filter
**Concept**: Only allow entries when ATR/close (normalized volatility) is within a target range.
- High-vol regime: ATR/close > 3% → trend-following strategies only
- Low-vol regime: ATR/close < 1.5% → mean-reversion strategies only
- Medium: allow both

**Implementation**: Add optional `min_vol_pct`/`max_vol_pct` to Config. Check in evaluate_signal_() before firing.

#### B. BTC Trend Filter
**Concept**: Only allow long entries on altcoins when BTC is in an uptrend.
- BTC TSMOM(20) > 0 → all engines can trade
- BTC TSMOM(20) < 0 → suppress altcoin entries (BTC-only engines still trade)

**Implementation**: Pass BTC momentum as a global signal; check in evaluate_signal_(). Requires cross-engine communication (currently engines are independent).

#### C. Drawdown Throttle
**Concept**: Reduce position count when portfolio drawdown exceeds threshold.
- DD < 5%: all engines active
- DD 5-10%: only top-50% engines by PF allowed to enter
- DD > 10%: halt new entries, let existing positions play out

---

## Recommended Next Session Priorities

1. **Run DONCHIAN sweep** on H2/H3/H4/H6/H12/D1 (highest expected yield, zero code changes)
2. **Run RSI_REVERT + BOLLINGER sweep** on D1/D2/D3 (daily mean-reversion, zero code changes)
3. **Fill TSMOM H12 gaps** (quick win, zero code changes)
4. **Monte Carlo validate** all new engines with extended data (same as Session 22)
5. **If user approves code changes**: implement EMA_CROSS and MACD_REV (most impactful new strategies)
6. **After 1-2 weeks of shadow data**: evaluate which engines to promote to live

## Expected Yield from Gap-Filling
- DONCHIAN expansion: ~20-40 engines
- RSI daily: ~10-20 engines
- BOLL daily: ~10-15 engines
- TSMOM H12 fill: ~6-8 engines
- **Total potential: ~50-80 new engines → 280-310 total**

With new strategies (EMA_CROSS, MACD_REV, KELTNER, BOLL_SQUEEZE):
- Each strategy across 12 symbols × 5 best timeframes ≈ 30-50 engines per strategy
- **Total potential: 400-500 engines**

---

## Deploy Protocol Reminder
- **Mac only** for git push (SSH key is on Mac)
- **SCP from Mac** to VPS (VPS has no GitHub SSH key)
- **VPS build**: `cd /home/jo/ChimeraCrypto/build && cmake .. && make -j$(nproc)`
- **Restart**: `sudo systemctl restart chimera`
- **Monitor**: `tail -f /home/jo/ChimeraCrypto/logs/chimera.log`
- **Positions**: `curl -s http://localhost:8080/api/positions | python3 -m json.tool`
- **Shadow mode**: All engines paper-trade. Promote to live after shadow matches backtest ±10%

## Git Push (Still Pending from Session 22 + Session 23)
```bash
cd ~/ChimeraCrypto
rm -f .git/index.lock
git add src/main.cpp HANDOFF_SESSION23.md
git commit -m "Session 23: fix seeding bug, H1 aggregation, /api/positions"
git push
```
