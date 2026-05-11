# Chimera Tier-2 Rewrite — 2026-05-11

**Status:** Code complete, awaiting deploy on VPS. All 5 new engines run in `shadow_mode = true` by default — promote to live only after 4 weeks of paper logs match backtest WR/PF within +/- 10%.

**Hard constraint reminder:** SPOT ONLY. No perps, no margin, no shorting, no options. Every trade is a spot LONG.

---

## What changed in this commit

### Added
- `include/core/EdgeEngine.hpp` — single configurable engine class for all 5 keeper edges. Internal bar synthesis, ATR(14) stops, time exits, RSI/Bollinger/Donchian/TSMOM signals.
- `HANDOFF_TIER2.md` — this file.

### Rewrote
- `src/main.cpp` — instantiates 5 `EdgeEngine` instances, routes ticks to whichever engine matches the symbol id. New `/api/state2` returns `{"build":...,"engines":[...]}`. `/api/kill` flattens every engine.
- `gui/index.html` — replaced the SwingEngine + QUAD-paper-engine dashboard with a 5-engine grid. Polls `/api/state2` once per second.
- `CMakeLists.txt` — dropped `chimera_backtest` and `chimera_backtest_paper` binaries (Python pipeline in `chimera_edges/` supersedes them). Dropped `PerpFeed.cpp` and `CoinbaseWSFeed.cpp` sources (no consumer left).

### Deleted
- `include/core/SwingEngine.hpp`
- `include/core/FundingWindowEngine.hpp`
- `include/core/BasisMomentumEngine.hpp`
- `include/core/OrderbookImbalanceEngine.hpp`
- `include/core/LiquidationEngine.hpp`
- `include/core/LiquidationFeed.hpp`
- `include/core/LiqBracketEngine.hpp`
- `include/live/PerpFeed.hpp` + `src/live/PerpFeed.cpp`
- `include/live/CoinbaseWSFeed.hpp` + `src/live/CoinbaseWSFeed.cpp`
- `tools/backtest/replay.cpp`
- `tools/backtest/replay_paper.cpp`
- `tools/backtest/ab_test_swing.sh`

---

## The 5 keeper edges (sourced from the Omega-style backtest pipeline)

Backtest period: 2022-01-01 -> 2026-05-11. 10bp round-trip cost. Out-of-sample window: post-2025-01-01.

| Instance         | Symbol  | Strategy   | TF | Lookback | Hold | SL ATR | OOS PF | Expected trades/yr |
|------------------|---------|------------|----|---------:|-----:|-------:|-------:|-------------------:|
| `BTC-TSMOM-D1`   | btcusdt | TSMOM      | D1 |       20 |   12 |   3.0x |  1.19  |               ~20  |
| `ETH-BB-H6`      | ethusdt | BOLLINGER  | H6 |       20 |   12 |   2.5x |  1.31  |               ~50  |
| `SOL-DONCH-H6`   | solusdt | DONCHIAN   | H6 |       20 |   24 |   2.5x |  1.24  |               ~25  |
| `XRP-DONCH-H1`   | xrpusdt | DONCHIAN   | H1 |       20 |   24 |   2.5x |  1.20  |              ~140  |
| `LINK-RSI-H6`    | linkusdt| RSI_REVERT | H6 |   rsi=14 |    8 |   2.0x |  2.82  |               ~15  |

Expected aggregate firing rate: ~250 trades/year combined. Far above the previous setup which fired ~1 trade per 27 hours.

### Why these specific cells

- **BTC tsmom D1:** 57% IS WR, 55.6% OOS WR — highest win-rate of any keeper. Classic time-series momentum on daily closes; 20-day return > 0 triggers entry next day at open.
- **ETH bollinger H6:** Cleanest OOS profile; IS and OOS roughly proportional. ETH has more 5–8h pullbacks within larger trends than BTC.
- **SOL donchian H6:** Close above 20-bar prior 6h high. SOL trends harder than majors — breakouts pay.
- **XRP donchian H1:** High firing rate, small per-trade. XRP has long quiet ranges punctuated by sudden 4–8h breakouts (news/ETF flow).
- **LINK rsi_revert H6:** Mean-reversion on RSI cross-up from <= 30 — beaten-down trend-follower symbol, multi-month down-cycles with predictable bottoms.

Rejected during OOS verification (do NOT wire): `DOGE tsmom D1`, `SOL tsmom D1`, `BNB donchian H6`. All daily trend-following on alts — that regime broke in 2025+. SOL kept via donchian H6 instead; DOGE and BNB and AVAX have no surviving edge in this strategy set (revisit with weekly Donchian or cross-sectional momentum later).

---

## Deploy + verify

```bash
# On Mac
cd ~/ChimeraCrypto/chimera-git
git pull --ff-only origin main

# On VPS
ssh -i ~/.ssh/chimera_ed25519 jo@154.45.251.118
sudo systemctl stop chimera.service
while pgrep -x chimera >/dev/null; do sleep 0.5; done
cd /home/jo/ChimeraCrypto && git pull --ff-only origin main
cd build && cmake .. && make -j"$(nproc)" chimera
sudo systemctl start chimera.service

# Verify the new wiring
journalctl -u chimera.service -f --since "30 seconds ago" | grep -E "STARTUP|ARMED|ENTRY|EXIT|FATAL"
# Expect:
#   [STARTUP] Chimera — Tier-2 Edge Engines | build=...
#   [STARTUP] Instance lock acquired PID=...
#   [BTC-TSMOM-D1]  ARMED  ...
#   [ETH-BB-H6]     ARMED  ...
#   [SOL-DONCH-H6]  ARMED  ...
#   [XRP-DONCH-H1]  ARMED  ...
#   [LINK-RSI-H6]   ARMED  ...
#   [STARTUP] Spot feed live. 5 paper engines running...

# Confirm /api/state2 returns 5 engines
curl -s http://localhost:8080/api/state2 | python3 -m json.tool | head -40
```

**First entries to expect:** XRP H1 engine should fire within 24-72 hours (140 trades/year = roughly one every 2.5 days). ETH H6 within a week. The D1 engine (BTC) and the slower H6 engines may take 1-3 weeks before the first entry — that's by design.

---

## What to watch in the first 4 weeks

Goal: confirm paper firing rate, WR, and PF match the backtest within +/- 10% before promoting any engine to live.

```bash
# Tally per-engine results from the journal
for tag in BTC-TSMOM-D1 ETH-BB-H6 SOL-DONCH-H6 XRP-DONCH-H1 LINK-RSI-H6; do
    echo "=== $tag ==="
    sudo journalctl -u chimera.service --since "4 weeks ago" \
        | grep "$tag" | grep "EXIT" | tail -50
done
```

Expected after 4 weeks (rough; depends on regime):

| Engine          | Expected trades | Expected WR | Promote if... |
|-----------------|----------------:|------------:|---------------|
| BTC-TSMOM-D1    |             1-2 |       50%+  | too few trades to decide yet; hold |
| ETH-BB-H6       |             3-5 |       45%+  | WR >= 50% and total bp positive |
| SOL-DONCH-H6    |             1-3 |       40%+  | wait 8 weeks — slow but high avg |
| XRP-DONCH-H1    |           10-12 |       38%+  | most actionable signal first |
| LINK-RSI-H6     |             1-2 |       55%+  | hold; rare but high-PF |

---

## Rollback

```bash
ssh -i ~/.ssh/chimera_ed25519 jo@154.45.251.118
cd /home/jo/ChimeraCrypto
git log --oneline -5                          # find the pre-Tier-2 commit
sudo systemctl stop chimera.service
git reset --hard <previous-commit-hash>
cd build && make -j"$(nproc)" chimera
sudo systemctl start chimera.service
```

The pre-Tier-2 commit (HANDOFF_MOVE2 set) is preserved in git history. Rollback is one `git reset --hard` + rebuild.

---

## Deferred to follow-up sessions

- **Tier 1 risk wrapper** — daily loss circuit, correlation-aware sizing, per-engine kill, state persistence, reconciliation. REQUIRED before any engine flips from `shadow_mode = true` to live execution.
- **Bar seeding from history** — current implementation cold-starts bar buffers from live ticks. For the D1 engine that means 20+ days before the first signal can fire. Add `seed_from_history()` (Binance REST kline replay) to warm up the buffer on startup. Lower priority — D1 was meant to be slow.
- **AVAX, BNB, DOGE re-test** — none survived OOS in this strategy set. Try weekly Donchian, cross-sectional momentum vs BTC, or RSI-revert at H1.
- **Higher-resolution backtest** — current results are based on 1h Binance kline closes. If any engine shows promise, validate with 1m or aggTrade replay.
