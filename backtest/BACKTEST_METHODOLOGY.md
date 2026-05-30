# ChimeraCrypto Backtest Methodology (MANDATORY)

The harness can lie in two ways that both **inflate** results. Every validation,
re-gate, sweep, discovery, or cull decision MUST use BOTH flags below, or the
numbers are not trustworthy.

## 1. `--fine-fill` — real intrabar path (fidelity)

Without it, the harness synthesizes each higher-TF bar (H2–D1) from H1 and
**guesses** the intrabar order (O→H→L on down bars). That arms the trailing
profit-lock on a phantom high and produces impossible results — e.g. median
PF 3+ with zero losing engines during a verified −75% alt crash.

`--fine-fill` feeds the **real constituent H1 bars in sequence** (low-first
within each H1 = conservative); the engine aggregates them into the tf bar and
`check_exits_` runs every H1 tick = true intrabar path. Impact: roster median
PF **4.00 → 1.29** (the synthetic path was inflating ~3×).

(1-minute data exists for ~59 symbols but only ~7 days — too short for history,
so H1 is the working granularity. Residual within-H1 ordering is guessed but
minor.)

## 2. `--regime-gate` — match the live bear-halt (don't over-cull)

The LIVE bot has a per-symbol + BTC bear-halt (P2): it does **not** enter longs
while the symbol/market trend is down. The harness, without `--regime-gate`,
counts those bear-market trades the live bot would never take — so it
**under-states** viable engines and **over-culls** them.

`--regime-gate` suppresses entries while the symbol's own ~10-D1 trend is down
(via `set_cluster_gate`), replicating the live halt. Impact: viable roster
**85 → 147** engines, median PF **1.33 → 1.44**. ~39 wrongly-culled engines were
recovered (e.g. AVAX-TSMOM-D1 PF 2.29).

Note: the harness gate uses the symbol's own trend only; live also requires BTC
bullish, so live blocks slightly more (a touch fewer trades than the gated
backtest shows). Directionally correct for viability.

## Standard validation command

```
./backtest --roster <csv> --preset prod_safety --hard-floor-bp -170 \
           --fine-fill --regime-gate --last-days 365
```

Plus robustness: require profitable on **365d AND 730d**, in **≥3 of 4
walk-forward 90d windows**, with adequate sample (n≥40) and bounded maxDD.

## Hard truth

Even with both flags, the edge is thin (median PF ~1.4) and the roster is
selection-biased (engines discovered on this data). Absolute PF is still
optimistic. **Shadow-mode live results are the only real proof** — do not flip
off shadow until shadow shows a real edge across a down stretch.
