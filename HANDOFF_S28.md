# Chimera Session 28 Handoff

## What Was Done This Session

### 1. New Strategy Engines Added (8 total)
Optimizer grid extended to 6D (added `signal_mult` dimension for strategy-specific parameters).

**KELTNER_REVERT** (6 engines):
- DOGE-KELTNER-H6, DOGE-KELTNER-H8, LINK-KELTNER-H12, BTC-KELTNER-H12, SUI-KELTNER-H12, APT-KELTNER-H8
- Mean-reversion using Keltner Channel (EMA + ATR bands), buy when price touches lower band

**DUAL_THRUST** (2 engines):
- SOL-DT-H12, XRP-DT-H8
- Range breakout using N-bar high/low range with k-factor multiplier
- Note: Most symbols showed FRAGILE neighbourhood scores; only SOL-H12 and XRP-H8 passed strict criteria

### 2. Position Resume System (Critical)
- On shutdown: persists all open positions to `data/open_positions.json` (instead of force-closing)
- On startup: reads snapshot, calls `resume_position()` on matching engines with full state
- State preserved: entry_px, sl_px, atr_at_entry, trail state, MFE, bars_held, time_exit_ts

### 3. Volatility Filter (vol_filter)
- Activated on **125 counter-trend engines** (RSI_REVERT, BOLLINGER, KELTNER_REVERT)
- ATR(14)/ATR(50) ratio classifies regime:
  - Normal (<1.2): all entries allowed
  - Elevated (1.2-1.6): counter-trend suppressed
  - Chaos (>1.6): ALL entries suppressed

### 4. Multi-Timeframe Gate (mtf_gate)
- Activated on **122 sub-D1 counter-trend engines**
- D1 TSMOM trend state per symbol propagated every 60s
- When D1 is bearish → counter-trend longs are suppressed (don't buy dips in downtrends)

## Current State
- **265 engines** running, all shadow mode
- **1 open position**: LINK-TSMOM-D3 (entry=9.70, SL=8.196, +15.5bp MFE)
- **62 historical trades** in journal
- Build: a18f76f
- VPS: 143.198.89.54, service=chimera.service

## What Was Tried But Rejected
- **MACD_CROSS**: Dead — no configs passed walk-forward validation on any symbol
- **DUAL_THRUST on most symbols**: FRAGILE neighbourhood scores (low stability), only SOL-H12 and XRP-H8 survived

## Files Modified This Session
- `include/core/EdgeEngine.hpp` — ResumeState, resume_position(), vol_filter, mtf_gate, d1_bullish, is_trend_following()
- `src/main.cpp` — 8 new engines, position resume on startup, shutdown persist, filter activation loop, D1 propagation
- `backtest/optimizer_general.cpp` — 6D grid for KELTNER/DUAL_THRUST
- `backtest/walk_forward_validate.cpp` — new file, 60/20/20 validation harness

## Next Session: New Engines/Regimes Research

### Strategies NOT yet explored:
1. **VWAP Reversion** — Intraday VWAP deviation mean-revert (needs tick-level VWAP calc)
2. **Ichimoku Cloud** — Cloud breakout + Tenkan/Kijun cross (trend-following variant)
3. **Heikin-Ashi Smoothed** — HA candle trend detection with ATR confirmation
4. **Williams %R** — Similar to RSI but different normalization, may find different entries
5. **ADX Filter + Trend** — Use ADX as regime gate (only trade when ADX>25 = strong trend)
6. **Stochastic RSI** — Faster oscillator for mean-reversion timing
7. **SuperTrend** — ATR-based trailing trend indicator, popular in crypto

### Regime Filters NOT yet explored:
1. **Correlation regime** — Suppress when BTC correlation is extreme (altcoin herding)
2. **Volume regime** — Suppress entries in abnormally low volume (weekend dead zones)
3. **Spread/liquidity filter** — Suppress when bid-ask spread exceeds threshold
4. **Momentum regime** — ADX-based, only allow trend entries when ADX confirms direction
5. **Time-of-day filter** — Asian/London/NY session activity patterns

### Prioritized by likely ROI:
1. ADX regime filter (simple, proven, applies to all TSMOM engines)
2. Volume regime filter (already have depth5 data streaming)
3. Ichimoku Cloud (complementary to existing trend strategies)
4. SuperTrend (very popular in crypto, different signal timing than TSMOM)
5. Correlation regime (unique edge, reduces drawdown in panic selloffs)

## Deploy Command (for reference)
```bash
cd ~/ChimeraCrypto
scp include/core/EdgeEngine.hpp jo@143.198.89.54:/home/jo/ChimeraCrypto/include/core/EdgeEngine.hpp
scp src/main.cpp jo@143.198.89.54:/home/jo/ChimeraCrypto/src/main.cpp
ssh jo@143.198.89.54 "cd /home/jo/ChimeraCrypto/build && cmake .. && make -j2 && sudo systemctl restart chimera"
```

## Verify
```bash
curl -sk https://143.198.89.54:9443/api/state2 | python3 -m json.tool | head -50
ssh jo@143.198.89.54 "sudo journalctl -u chimera --since '1 min ago' --no-pager | tail -20"
```
