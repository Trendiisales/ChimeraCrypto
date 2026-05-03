# Chimera Engine Roadmap

This document classifies every engine `.hpp` in the repo as a **keeper** (worth
wiring up later as a parallel strategy) or a **discard** (delete via
`tools/cleanup_dead_engines.sh`). It also sketches the order in which keepers
should be brought online and what supporting infrastructure each one needs.

The current binary instantiates exactly **one** trading engine: `SwingEngine`.
Everything else listed as a "keeper" below is currently dormant — its file
exists but `main.cpp` does not include or instantiate it.

## Wired today

| File                                      | Status                                                              |
| ----------------------------------------- | ------------------------------------------------------------------- |
| `include/core/SwingEngine.hpp`            | Live. H4/D1 spot swing engine (S1 / S2 / S3 / S4 strategies).       |
| `include/execution/ExchangeLatencyEngine.hpp` | Used as a global by `BinanceWSFeed.cpp`. Utility, not trading. |
| `tools/backtest/replay.cpp`               | Backtest harness for SwingEngine.                                   |

## Keepers (dormant, slated for future wiring)

Listed in the order I'd bring them online. Each one fills a distinct niche so
they diversify rather than overlap.

### Tier 2 — adds the missing strategy diversification

| File                                       | Niche                                | Notes |
| ------------------------------------------ | ------------------------------------ | ----- |
| `include/core/OrderbookImbalanceEngine.hpp` | Microstructure mean-reversion       | Fades extreme book imbalance in GRIND regimes. 12 bp cost floor, regime-gated. Most cleanly written of the dormant set. |
| `include/core/SessionMomentumEngine.hpp`   | Intraday momentum                    | London/NY/Asia open windows. 20-min entry window, 22 bp TP / 6 bp SL. |
| `include/core/BasisMomentumEngine.hpp`     | Perp → spot lead-lag                 | Buys spot when basis spikes positive. Long-only on spot. |
| `include/core/FundingWindowEngine.hpp`     | Funding rate snap-back               | Trades the 3-min window before 0/8/16 UTC funding payments. BTC/ETH only. |
| `include/core/LiquidationEngine.hpp`       | Liquidation cascade                  | Buys spot when short liquidations hit perp. Needs `LiquidationFeed` wired in. |

### Tier 3 — additional uncorrelated edges

| File                                       | Niche                                | Notes |
| ------------------------------------------ | ------------------------------------ | ----- |
| `include/core/AggressiveFlowEngine.hpp`    | Order flow continuation              | Follows sustained buy/sell aggressor flow. Long-only on spot. |
| `include/core/DivergenceEngine.hpp`        | Cross-symbol microstructure mean rev | Buys the laggard when one of BTC/ETH/SOL moves and the others don't. |
| `include/core/LeadLagEngine.hpp`           | Multi-symbol lead-lag                | BTC/ETH/SOL → alt followers. Realistic 50–200 ms latency model. |
| `include/core/CompressionBreakoutEngine.hpp` | Vol compression → expansion        | Tick-level compression breakout. Distinct from SwingEngine S4 (which is on H4 bars). |
| `include/core/VolumeShockEngine.hpp`       | Volume-confirmed momentum            | Volume spike + price displacement = continuation. |
| `include/core/SpreadCompressionEngine.hpp` | Spread-tightening directional signal | MM commits → price moves. Maker-fill design. |
| `include/core/StatArbEngine.hpp`           | BTC/ETH cointegration                | 4 h Z-score mean reversion on log price ratio. |
| `include/core/LiqBracketEngine.hpp`        | Triple-confirmed bracket             | Compression + liquidation + perp lead. Conservative event trigger. |

### Infrastructure / risk layer (not a strategy, but worth keeping)

| File                                          | Role                                                                |
| --------------------------------------------- | ------------------------------------------------------------------- |
| `include/core/CapitalScalingEngine.hpp`       | Asymmetric position-size scaling based on rolling expectancy.       |
| `include/recon/ReconciliationEngine.hpp`      | Compares engine-tracked positions vs. exchange. Foundation for risk layer. |

## Discards

These files are removed by `tools/cleanup_dead_engines.sh`. None are wired up,
none are referenced by SwingEngine or the keepers, and each falls into one of
the discard categories below.

**Obsolete:** `TrendEngine.hpp` (explicitly replaced by SwingEngine).

**Old multi-engine wrappers** that depended on a dozen since-orphaned headers:
`BalancedEngine.hpp`, `EnhancedBalancedEngine.hpp`, `TripleEngineBalancedEngine.hpp`,
`QuadEngineBalancedEngine.hpp` (this one printed the dashboard's "QUAD ENGINE"
banner), `HybridRegimeController.hpp`, `UltraController.hpp`, `UltraEngine.hpp`.

**Generic stubs without standalone strategy:** `AlignedEngines.hpp`,
`PositionEngine.hpp`, `MicroSignalEngine.hpp`, `StructuralEngine.hpp`,
`ConvexShockEngine.hpp`, `PullbackContinuationEngine.hpp`.

**Weak / duplicate signals:** `NGASLeadLagEngine.hpp` (natural-gas → crypto
macro is speculative at best), `FundingSignalEngine.hpp` (functionality is
covered better by `FundingWindowEngine.hpp`).

**Top-level + the entire `include/engine/` folder:** `InstitutionalEngine.hpp`
(top-level) plus all of `include/engine/{AdaptiveFadeController,
CompressionEngine, DominanceControlPlane, InstitutionalEngine,
LiquidityVacuumEngine, MultiSymbolAlignmentEngine, VolatilityExpansionEngine}.hpp`.
This is the "institutional" attempt that pulls in 20+ missing dependencies
(`Portfolio`, `PositionLedger`, `OrderTracker`, `PnLTracker`, etc.).

**Orphan .cpp files** that aren't even in `CMakeLists.txt`:
`src/core/Portfolio.cpp`, `src/core/StatefulGovernor.cpp`.

## Recommended sequence (revised)

1. **Backtest the keeper-tuned SwingEngine** with `chimera_backtest --bars 1500`.
   Confirm it has positive expectancy on 6+ months of history before wiring
   anything new.
2. **Run the cleanup script** in dry-run, review, then `--force` to delete the
   discards. This shrinks the cognitive load of the repo.
3. **Build the risk-manager wrapper** (Tier 1 from the earlier audit) — daily
   loss circuit breaker, correlation-aware sizing, per-symbol kill switch.
4. **Wire one Tier-2 engine** (recommend `OrderbookImbalanceEngine` first — it
   has zero external dependencies beyond `MarketTick`, runs in different
   regimes than Swing, and is the cleanest of the dormant set).
5. **Backtest the two engines together**, prove diversification, repeat for the
   next Tier-2 engine.
6. Only after 2–3 engines are validated together: consider going off shadow
   mode for live trading.

## Honest limits of this audit

I did not deep-read every keeper line by line. Each keeper has been classified
as "worth retaining" based on header comments, the strategy logic in the first
30–60 lines, and whether it fills a niche the other keepers don't. When you
actually wire a keeper into `main.cpp`, expect to spend a session per engine
fixing missing dependencies — they all reference helper modules that may have
been renamed or removed since the engine was originally written.
