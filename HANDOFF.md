# Handoff — ChimeraCrypto Session 19 (2026-05-16)

**Status:** 49 engines deployed on VPS (shadow mode). 37 TSMOM + 12 counter-trend (RSI_REVERT + BOLLINGER). All spot-long-only. Counter-trend engines fire in bear/ranging markets when TSMOM is flat. Bar persistence active for all 49.

Read this top-to-bottom before touching any code.

---

## 1. Mission

Jo wants a crypto trading bot that actually trades. The system runs on a Singapore VPS, executes on Binance spot (long-only, no perps, no shorting). Session 19 solved the bear-market gap: 12 counter-trend engines (RSI oversold bounce + Bollinger dip-buy) now complement the 37 TSMOM momentum engines. The next session should find MORE edges — additional strategies, symbols, or timeframes.

---

## 2. Current system state

### Infrastructure
- **VPS:** jo@143.198.89.54 (Singapore, Ubuntu, systemd unit `chimera`)
- **Deploy:** SCP from Mac as `jo@` — VPS has NO GitHub SSH key, cannot git pull
- **GUI:** nginx reverse proxy 9443→8080, `gui/index.html` v3 with TF tabs, signal monitor, bell sounds
- **API:** https://143.198.89.54:9443/api/state2 (confirmed `engine_count: 49`)
- **Build:** CMake, C++17, `cd build && cmake .. && make -j2`
- **Logs:** `/home/jo/ChimeraCrypto/logs/chimera.log`

### 49 active engines (all shadow, all spot-long-only)

#### TSMOM Momentum (37 engines) — Sessions 13-17
| TF | Count | Symbols | Session |
|----|-------|---------|---------|
| D1 (86400s) | 5 | BTC, ETH, SOL, LINK, BNB | 13-14 |
| H12 (43200s) | 3 | BTC, DOGE, AVAX | 14 |
| H6 (21600s) | 8 | All 8 symbols | 15 |
| H4 (14400s) | 7 | All except DOGE | 14 |
| H3 (10800s) | 6 | BTC, ETH, SOL, XRP, LINK, BNB | 17 |
| H2 (7200s) | 5 | BTC, ETH, SOL, XRP, LINK | 17 |
| H1 (3600s) | 3 | XRP, SOL, LINK | 15 |

#### Counter-Trend (12 engines) — Session 19
Fire in bear/ranging markets when TSMOM sits flat. Negatively correlated with momentum.

**Tier 1 (7):**
| Tag | Symbol | Strategy | TF | OOS PF | Nbr% | Key Params |
|-----|--------|----------|----|--------|------|------------|
| ETH-RSI30-H3 | ETHUSDT | RSI_REVERT | H3 | 2.41 | 92% | lb=20, hold=24, sl=4.0, rsi_th=30 |
| ETH-RSI30-H4 | ETHUSDT | RSI_REVERT | H4 | 2.13 | 88% | lb=35, hold=16, sl=4.0, rsi_th=30 |
| DOGE-RSI30-H3 | DOGEUSDT | RSI_REVERT | H3 | 1.98 | 85% | lb=40, hold=20, sl=3.5, rsi_th=30 |
| AVAX-RSI25-H2 | AVAXUSDT | RSI_REVERT | H2 | 2.27 | 90% | lb=25, hold=8, sl=1.5, rsi_th=25 |
| DOGE-RSI25-H2 | DOGEUSDT | RSI_REVERT | H2 | 1.85 | 83% | lb=25, hold=24, sl=1.5, rsi_th=25 |
| BTC-RSI35-H3 | BTCUSDT | RSI_REVERT | H3 | 1.92 | 87% | lb=25, hold=24, sl=3.5, rsi_th=35 |
| BNB-BOLL25-H3 | BNBUSDT | BOLLINGER | H3 | 2.08 | 86% | lb=15, hold=8, sl=4.0, bb_k=2.5 |

**Tier 2 (5):**
| Tag | Symbol | Strategy | TF | OOS PF | Nbr% | Key Params |
|-----|--------|----------|----|--------|------|------------|
| ETH-BOLL25-H3 | ETHUSDT | BOLLINGER | H3 | 1.74 | 81% | lb=35, hold=8, sl=4.0, bb_k=2.5 |
| BTC-RSI25-H2 | BTCUSDT | RSI_REVERT | H2 | 1.68 | 79% | lb=25, hold=8, sl=1.5, rsi_th=25 |
| LINK-BOLL30-H1 | LINKUSDT | BOLLINGER | H1 | 1.59 | 77% | lb=30, hold=8, sl=2.5, bb_k=3.0 |
| XRP-RSI30-H6 | XRPUSDT | RSI_REVERT | H6 | 1.82 | 84% | lb=20, hold=16, sl=4.0, rsi_th=30 |
| XRP-RSI30-H2 | XRPUSDT | RSI_REVERT | H2 | 1.71 | 80% | lb=20, hold=12, sl=1.5, rsi_th=30 |

### Current VPS engine status (as of deploy)
- 7 counter-trend engines signal_ready (H4/H2/H1/H6 — REST-seeded, 64 bars)
- 5 counter-trend engines cold-starting (all H3 — building bars from live ticks, 2-3 days to signal_ready)
- All 37 TSMOM engines signal_ready, all flat (bearish market, correct behaviour)

### Zero trades explanation
TSMOM: `close[now] > close[now - lookback]` → go long. Market is bearish → stays flat. Correct.
RSI_REVERT: buys when RSI crosses UP from oversold threshold. Will fire when market dips hard enough.
BOLLINGER: buys when price pierces lower band then closes above. Will fire on extreme wicks.

---

## 3. Strategies tested and RULED OUT (no OOS edge after costs)

| Strategy | Config tested | OOS PF | Why it failed |
|---|---|---|---|
| DONCHIAN | All 8 symbols, H6/H1/D1 | 0.82-0.83 | False breakouts in crypto |
| BOLLINGER (K=2.0) | All 8 symbols, H6 | 0.72 | Mean reversion doesn't hold at K=2.0 |
| RSI_REVERT | All 8 symbols, H6 only | 1.17 (4 trades) | No statistical significance at H6 |
| OVERNIGHT | BTC H1, 21:00 UTC entry | 0.31 | Overnight premium nonexistent |
| WEEKDAY | BTC D1, Monday entry | 0.44 | Monday effect is dead |
| Short-side TSMOM | N/A | N/A | RULED OUT — spot-only constraint, no shorting possible |

**Note:** RSI_REVERT and BOLLINGER were re-tested in Session 19 with wider parameter sweeps (RSI thresholds 25/30/35, Bollinger K=2.5/3.0) across ALL timeframes (not just H6). Found strong edges on H2/H3/H4/H1 that the original H6-only tests missed.

---

## 4. NEXT SESSION: Find more edges

### High-priority untested avenues

**1. Volume-weighted momentum**
- Current TSMOM ignores volume entirely
- 1m kline CSVs have volume, quote_volume, taker_buy_base, taker_buy_quote columns
- Signal: TSMOM + rising volume = enter. TSMOM + falling volume = skip.
- Could improve existing engines without a new strategy kind
- Needs: volume data plumbed into EdgeEngine (currently only OHLC)

**2. Multi-TF confirmation filter**
- H4 enters only if D1 is also bullish
- Can be implemented in main.cpp tick routing without core engine changes
- Risk: over-filtering kills edge through reduced trade count

**3. Regime/volatility gating**
- Skip entries when ATR/price exceeds threshold (choppy) or is below threshold (dead)
- Needs EdgeEngine core mod for external gate function

**4. New symbols**
- Only 8 tested. AAVE, DOT, MATIC, UNI, ATOM, FTM etc. untested
- Needs new kline downloads

**5. DONCHIAN with tighter params**
- Failed at default params but never tested with different lookback/hold combos
- Might work on faster timeframes (H1/H2) with tight stops

**6. RSI_REVERT / BOLLINGER on remaining symbol-TF combos**
- Session 19 found edges on specific combos. Full sweep may reveal more.
- edge_hunter.cpp already built for this — run on VPS for faster execution

---

## 5. Key files

| File | Purpose | Notes |
|---|---|---|
| `include/core/EdgeEngine.hpp` | Core engine class (806 lines) | **DO NOT MODIFY without explicit permission.** Header-only. All strategy logic, bar synthesis, trailing stops. Supports TSMOM, DONCHIAN, BOLLINGER, RSI_REVERT, OVERNIGHT, WEEKDAY. |
| `src/main.cpp` | 49 engine configs, HTTP server, REST seeding, warm-start, trade/bar journal, WS feed | ~1810 lines |
| `gui/index.html` | Dashboard v3 | TF tabs, signal monitor, momentum column, sparklines, bell sounds |
| `backtest/optimizer_general.cpp` | Original grid optimizer (617 lines) | 15,552-combo sweep. Hardcoded bb_k=2.0, rsi_threshold=30. Neighbourhood stability scoring. |
| `backtest/optimizer_v2.cpp` | Extended optimizer | NEW Session 19. CLI args for bb_k and rsi_threshold. |
| `backtest/edge_hunter.cpp` | Bear-market strategy scanner | NEW Session 19. 446K backtests across BOLL×RSI×8sym×7TF. |
| `backtest/scanner_v2.cpp` | 1,104-combo quick sweep | Lighter than full optimizer |
| `data/klines_spot/*.csv` | 8 symbols × 1m spot data | 365 days (2025-05-10 to 2026-05-10), 525,601 bars each |
| `data/klines_perp/*.csv` | 8 symbols × 1m perp data | Same range — exists for analysis but NOT for trading (spot only) |
| `data/bars/{TAG}.ndjson` | Per-engine bar history | Warm-start persistence, created by on_bar_callback |
| `data/trades.json` | Completed trade journal | NDJSON, loaded on startup |

### Cost assumptions (round-trip bp)
BTC: 17, ETH: 17, SOL: 20, XRP: 20, LINK: 22, BNB: 20, DOGE: 22, AVAX: 22

---

## 6. User preferences (critical)

- **SPOT ONLY** — no perpetual futures, no shorting, no margin. Permanently.
- **Always give full code files** — never snippets, never diffs, never "add this here"
- **Never modify EdgeEngine.hpp without explicit permission** — ask first every time
- **Shadow mode is intentional** — don't flag zero trades as a problem, treat paper trades as live
- **SSH only for git ops** — NEVER use GitHub PAT (it's dead)
- **VPS deploy = SCP from Mac as jo@** — VPS cannot git pull, user is `jo` not `root`
- **Warn at 70% context usage** — give summary before hitting limits
- **Be direct** — don't ask 4 clarifying questions. Make the call, explain the reasoning.

---

## 7. Deploy protocol

```bash
# From Mac terminal — SCP files
scp <files> jo@143.198.89.54:/home/jo/ChimeraCrypto/<same-path>

# SSH into VPS and rebuild
ssh jo@143.198.89.54
cd /home/jo/ChimeraCrypto/build && cmake .. && make -j2
sudo systemctl restart chimera
journalctl -u chimera --no-pager -n 50

# OR check app log directly
tail -50 /home/jo/ChimeraCrypto/logs/chimera.log
```

Git commit from Mac only:
```bash
cd ~/ChimeraCrypto && git add -A && git commit -m "..." && git push origin main
```

---

## 8. Pending minor items
- **Startup printf fix** — main.cpp updated locally (37→49) but not yet SCP'd to VPS. Cosmetic only — API already reports 49.
- **Git commit** — `src/main.cpp`, `backtest/edge_hunter.cpp`, `backtest/optimizer_v2.cpp` not yet committed/pushed.
- **Verify bar persistence** — Check `ls /home/jo/ChimeraCrypto/data/bars/` after 09:00 SGT to confirm .ndjson files appear.

---

## 9. First message for next session

> "Read HANDOFF.md in the ChimeraCrypto folder — it has the full system state. I now have 49 engines (37 TSMOM + 12 counter-trend) all in shadow mode. I need more edges. Try volume-weighted momentum, multi-TF confirmation, new symbols, or whatever you think will produce uncorrelated tradeable signals. Run backtests, validate with the optimizer, and deploy winners in shadow mode. Remember: spot-only, no shorting, no perps."
