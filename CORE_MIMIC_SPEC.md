# CORE/MIMIC Campaign — Operator Design Spec (VERBATIM)

> Pasted by operator 2026-07-15 (session 15k → resumed 15l). This is the
> authoritative design for safe live crypto. It SUPERSEDES the "mimic opens at
> BE-touch" model (15j `mimic_floor`, now reverted). Do NOT re-derive — build
> from these exact words. Governing rule at the very bottom.

---

Best design: a protected CORE campaign with a funded MIMIC

The mimic must not open merely because the original trade touches break-even. That creates the exact failure you are trying to avoid: the mimic enters late, immediately owes another 20 bps round-trip cost, and loses money on the first ordinary pullback.

The correct sequence is:

QUALIFIED UPWARD MOVE
        ↓
CORE ENTRY
        ↓
CORE COVERS ALL REAL COSTS
        ↓
CORE LOCKS A POSITIVE CAMPAIGN FLOOR
        ↓
FRESH CONTINUATION / HIGHER-LOW / REBREAK
        ↓
MIMIC OPENS, SIZED FROM LOCKED CORE PROFIT
        ↓
SHARED REVERSAL CONTROLLER EXITS BOTH

The CORE and MIMIC remain independent for entry price, quantity, cost basis, peak, order IDs and accounting. However, once both are open, one campaign-level controller protects the combined position.

## 1. The unavoidable truth about "no negative trades"

A newly opened spot-long position is immediately economically negative because selling it again incurs the remaining fee, spread and slippage.

Therefore, these three requirements cannot all coexist:

Open a new mimic BUY.
Exit it immediately when price reverses.
Never record a negative mimic trade.

If price reverses one tick after the mimic fills, the mimic must either:

exit at a loss, or
remain open and violate the reversal-exit rule.

There is no order type or strategy that removes that contradiction.

What we can enforce is:

Once the CORE has entered protected mode, opening and subsequently liquidating the combined CORE + MIMIC campaign must remain positive under a predefined stress-slippage assumption.

The mimic may individually lose on an immediate reversal, but that loss is paid entirely from profit already locked by the CORE. The campaign remains positive.

If every individual trade must always finish positive, the system must never open the mimic—or any trade—because negative outcomes cannot be mathematically excluded.

## 2. Stop using "20 bps" as the break-even price

Your 20 bps is only the base round-trip cost. The engine must calculate an authoritative safe cost dynamically:

    safe_cost_bps =
        actual_buy_fee
      + estimated_sell_fee
      + actual_entry_slippage
      + p99_expected_exit_slippage
      + spread/market-impact allowance
      + latency allowance
      + rounding/dust allowance

For example only:

    Base round trip cost      20 bps
    Stress slippage reserve   10 bps
    Latency/safety reserve     5 bps
    --------------------------------
    Protected cost            35 bps

The actual number must come from your fill telemetry. BTC and ETH may support something near this during liquid periods; thinner coins may need a much larger allowance.

Every decision must use the price at which the full quantity could actually be sold into current bid depth—not last price, midpoint or best bid alone.

Define:

    CoreNetLiquidationPnL =
        depth-adjusted sell proceeds
      - actual core purchase cost
      - estimated sell commission
      - stress exit slippage

The CORE has not covered break-even until this value is positive.

## 3. The correct CORE trigger

At 20 bps base cost, this cannot be a tiny scalping strategy. Naive short-horizon strategies are easily destroyed by costs; recent research evaluating crypto momentum under realistic assumptions found that many apparently profitable approaches became insignificant or unprofitable once transaction costs and real price paths were included. Another 2026 study found naive hourly strategies failed with costs of only 10 bps.

The best entry structure is:

Breakout → controlled pullback → reclaim

Do not buy the initial vertical breakout.

### CORE regime gate

Only permit a trade when all of the following are true:

1-hour and 4-hour trend regimes are upward.
Spot price is above the relevant anchored VWAP.
Market structure has higher highs and higher lows.
BTC market regime is positive, even when trading ETH.
Spread and projected depth slippage are below limits.
The expected remaining move is at least approximately three times the safe cost.

For a 35 bps safe cost:

    Minimum expected remaining move ≈ 105 bps

That is not a target guarantee. It is the minimum economic room required to justify paying the costs.

### CORE setup

A strong starting setup to backtest is:

    15-minute compression/range
        ↓
    Range breakout with real aggressive spot buying
        ↓
    Pullback holds above the old range high or anchored VWAP
        ↓
    A higher low forms
        ↓
    Price reclaims the pullback high
        ↓
    CORE BUY

The confirmation should use:

spot aggressive buy volume;
short and medium-window order-flow imbalance;
stable or improving bid depth;
no material spread expansion;
increasing real volume;
perp order flow as confirmation only;
no extreme perp premium or obviously exhausted liquidation spike.

Because you cannot trade perps, their data should be used as a leading or confirming signal for spot, never as the traded instrument.

### Starting parameter grid to test

These are research starting ranges, not hard-coded final values:

    Input                                Candidate research range
    Maximum spread                       2–4 bps
    Maximum projected entry slippage     2–4 bps
    15-minute compression width          35–120 bps
    Aggressive buy share, short window   58–65%
    Aggressive buy share, medium window  54–60%
    OFI z-score                          1.25–2.0
    Expected remaining move              3.0–4.0 × safe cost
    Pullback depth                       no more than 35–50% of breakout impulse

Start with BTCUSDT and ETHUSDT. SOL or smaller coins should remain disabled unless their measured cost, depth and continuation statistics independently pass the same gate.

## 4. CORE state before the mimic

The CORE should progress through three distinct cost states.

### State A — CORE_OPEN_RISK

The position has not covered its economic cost.

A real protective stop must still exist because capital can be lost before break-even. There is no way to remove this risk without refusing the trade.

The initial stop should be structural, below the pullback higher low, and position sizing must make that loss acceptable.

### State B — CORE_COST_COVERED

The current depth-adjusted liquidation value has covered:

    base cost
    + all actual costs
    + stress exit cost

Do not move directly to mimic entry.

Require the condition to persist—for example across multiple market-data updates or a minimum dwell period—so a single trade print cannot trigger it.

### State C — CORE_PROTECTED

Move the campaign floor to a price that leaves a positive result after stress exit costs.

For example:

    Minimum locked net profit = +5 to +10 bps

The protection order must be acknowledged before mimic eligibility becomes true.

## 5. The mimic must be funded by locked CORE profit

This is the central mathematical rule.

Let:

    Qc = core quantity
    Qm = mimic quantity
    Ec = actual core entry VWAP
    Em = actual mimic entry VWAP
    F  = stress-adjusted common liquidation floor
    R  = required retained campaign profit

Calculate:

    CoreProfitAtFloor =
        core exit proceeds at F
      - core purchase cost
      - all remaining core exit costs

Then calculate the mimic's loss if reversal happens immediately:

    MimicLossPerUnitAtFloor =
        mimic purchase cost per unit
      - mimic exit proceeds per unit at F

The maximum mimic quantity is:

    QmMax =
        (CoreProfitAtFloor - RequiredReserve)
        / MimicLossPerUnitAtFloor

Then apply:

    Qm = min(
        QmMax,
        configured_mimic_cap,
        portfolio_cap,
        available_liquidity_cap
    )

I would initially impose:

    Qm <= 0.50 × Qc

Most campaigns will support less than that.

### Why an equal-size mimic usually fails

Assume:

    Safe round-trip cost       35 bps
    CORE current move         +100 bps
    Stress floor               +70 bps
    MIMIC entry                +90 bps

At the floor:

    CORE gross gain            +70 bps
    CORE net gain              +35 bps

    MIMIC price loss           -20 bps
    MIMIC cost                 -35 bps
    MIMIC net loss             -55 bps

With equal size:

    Campaign result = +35 - 55 = -20 bps

That violates the protection requirement.

If we require the campaign to retain at least +10 bps:

    Maximum mimic/core ratio =
    (35 - 10) / 55
    = 0.4545

So the mimic could be approximately 0.45× CORE size, not 1.0×.

This sizing must be recalculated for every potential mimic entry.

## 6. The correct MIMIC trigger

Covering CORE break-even creates eligibility, not an immediate order.

The actual MIMIC entry requires a second independent continuation setup:

    CORE is protected
            +
    CORE profit can fund MIMIC stress loss
            +
    Price forms another higher low
            +
    Price rebreaks the latest continuation high
            +
    Spot order flow re-accelerates
            +
    Expected remaining move still covers MIMIC costs

A good pattern is:

    CORE enters on first pullback/reclaim
    Price advances and covers protected cost
    Price makes a small second consolidation
    Consolidation remains above CORE break-even
    Positive spot aggression returns
    Price breaks the consolidation high
    MIMIC enters

Do not open the mimic:

on the first touch of break-even;
during a vertical price spike;
while spread is expanding;
after order flow has already peaked;
when the only confirmation is perp liquidation activity;
if the required campaign floor would be inside ordinary market noise;
when the maximum safe mimic size is economically insignificant.

The mimic receives:

separate leg ID
separate entry price
separate fill records
separate fees
separate peak
separate PnL
separate eligibility flags

But it cannot independently sell quantity that the central inventory allocator does not own.

## 7. Exit logic for both positions

There should be two separate exit layers.

### Layer 1 — hard campaign profit floor

This is non-negotiable.

At every update calculate the stress liquidation value of the entire campaign:

    CampaignStressPnL =
        CoreStressPnL
      + MimicStressPnL

If it reaches the retained-profit floor:

    cancel outstanding BUY orders
    block new mimic entries
    sell all campaign inventory
    reconcile actual fills
    enter cooldown

This trigger does not wait for indicators.

### Layer 2 — normal reversal detection

The normal reversal controller should combine price structure and order flow.

A practical condition is:

    price reversal
    AND
    at least one order-flow confirmation

Where price reversal means one of:

depth-adjusted executable bid breaks the latest higher low;
peak-to-current giveback exceeds the adaptive trail;
price loses the breakout level or campaign anchored VWAP.

Order-flow confirmation can be:

spot OFI becomes materially negative;
aggressive sell volume overtakes aggressive buy volume;
bid depth collapses;
spread expands while price fails to make a new high;
short-window CVD turns negative.

### Adaptive trail

Use:

    trail_bps = max(
        volatility_noise_floor,
        spread_and_slippage_floor,
        structural_distance
    )

A candidate implementation would use something similar to:

    volatility_noise_floor = 0.35 to 0.50 × 5-minute ATR in bps

    execution_noise_floor =
        2 × current spread
      + 2 × p95 exit slippage
      + latency buffer

A 10 bps trail is generally nonsensical when your economic cost is approximately 20–35 bps. It will convert normal noise into repeated fee-paying exits.

## 8. How orders should actually be executed

For the entry, prefer a maker or pegged limit around the pullback/reclaim where practical. Binance's current Spot API supports pegged orders derived from the best bid or offer, including pegged limit and stop-limit orders. However, contingent pegged orders are revalidated when triggered and can be rejected, so they must not be the only protection mechanism.

For protection:

Maintain a server-side catastrophic stop.
Maintain a local depth-aware reversal and campaign-floor controller.
Exit with an aggressive marketable limit or IOC.
If the quantity remains unfilled beyond a very short emergency interval, complete liquidation with a market order.

Binance Spot trailing stops track favourable price movement and trigger after a specified reversal in basis points. They can also be used in the contingent leg of an OCO, where triggering the trailing leg cancels the other leg.

However, Binance's trailing calculation is based on market trade prices, while your protection requirement is based on the price available for your full sell quantity. Therefore:

    Exchange trailing stop = backup protection.
    Local depth-adjusted campaign controller = authoritative protection.

Binance's OPO/OPOCO facilities can attach pending SELL protection to a fully filled working BUY and use the amount received from that BUY, but they cannot implement the later independent MIMIC BUY. They are useful for protecting each filled long leg, not for creating the mimic logic itself.

## 9. Exit both with one inventory authority

Even though CORE and MIMIC are independent strategy legs, do not allow both objects to independently sell without coordination.

Use:

    Campaign inventory:
        core_remaining_qty
      + mimic_remaining_qty
      = exchange_owned_qty

On a shared reversal:

    requested_exit_qty =
        core_remaining_qty
      + mimic_remaining_qty

The central gateway sends one authoritative liquidation instruction and allocates the resulting fills back to CORE and MIMIC, preferably pro rata or according to an explicitly defined accounting rule.

This prevents:

duplicate sell orders;
overselling;
one leg remaining open accidentally;
mismatched partial fills;
one engine cancelling another engine's protection;
internal inventory differing from exchange inventory.

On Spot, the exchange normally sees fungible coin inventory rather than two truly separate positions. Independence is therefore internal unless you use separate exchange subaccounts.

## 10. Required state machine

    IDLE
      |
      | qualifying setup
      v
    CORE_PENDING
      |
      | confirmed fill
      v
    CORE_OPEN_RISK
      |
      | safe cost covered
      v
    CORE_COST_COVERED
      |
      | positive floor installed and acknowledged
      v
    CORE_PROTECTED
      |
      | new continuation + funded mimic sizing
      v
    MIMIC_PENDING
      |
      | confirmed mimic fill
      v
    BOTH_OPEN_PROTECTED
      |
      | reversal or campaign floor
      v
    EXITING
      |
      | exchange confirms flat
      v
    COOLDOWN

Mandatory guards:

    MIMIC_PENDING requires:
        core_protection_acknowledged
        && core_qty > 0
        && no_existing_mimic
        && fresh_continuation_signal
        && campaign_floor_after_mimic >= reserve
        && exchange_feed_healthy
        && user_data_stream_healthy
        && inventory_reconciled

## 11. Failure cases the engine must explicitly handle

The logic is incomplete unless it handles all of these:

partial CORE fills;
partial MIMIC fills;
fees deducted in base, quote or BNB;
minimum quantity and minimum notional;
dust remaining after liquidation;
stale or out-of-order market data;
user-data stream disconnection;
unknown order state after API timeout;
stop rejection;
stop-limit trigger without a fill;
cancel/replace race;
restart while positions are open;
mismatch between exchange balance and internal inventory;
two simultaneous exit triggers;
MIMIC order filling after a reversal was already detected;
spread or depth deterioration between trigger and fill.

Binance explicitly warns that an API timeout may leave execution status unknown rather than proving the order failed; order status must be recovered from the user-data stream or queried before another order is sent.

The engine must be exchange-truth driven:

    request sent ≠ order accepted
    order accepted ≠ filled
    filled ≠ fully filled
    cancel requested ≠ cancelled
    stop triggered ≠ quantity sold

## 12. Your present Chimera blockers matter

This should not be attached to the current default live path until the critical audit issues are repaired.

The July 14 audit found that default active Chimera engines could lack a live order-intent route, and that default close callbacks bypassed the canonical accounting, cooldown and protection path. That would make a protected CORE/MIMIC campaign unsafe because the displayed position, locked PnL and actual broker inventory could diverge.

Before LIVE, the system needs:

mandatory unique engine registration;
complete order and protection callbacks;
central close/accounting routing;
partial-fill reconciliation;
one authoritative cost model;
deterministic replay;
forced win/loss tests;
catastrophe flatten tests;
restart reconciliation.

Those are also explicitly required by the audit's acceptance criteria.

## Recommended final specification

### CORE
Size: 1.00 risk unit
Entry: first pullback/reclaim after confirmed upward breakout
Minimum expected move: >= 3 × safe total cost
Initial stop: structural, risk-sized
BE calculation: full depth-adjusted net liquidation value
Protected floor: +5 to +10 bps net after stress costs

### MIMIC
Maximum size: initially 0.50 × CORE
Normal size: dynamically calculated from CORE locked profit
Entry: second higher-low and rebreak, not first BE touch
Expected remaining move: >= 2–3 × mimic safe cost
Maximum opens: one mimic per CORE campaign
No opening unless campaign floor remains positive after immediate reversal

### Combined exit

    Hard exit:
        campaign stress PnL reaches retained-profit floor

    Normal exit:
        structural/giveback reversal
        plus negative order-flow confirmation

    Emergency:
        feed/account/order-state failure
        → cancel buys
        → reconcile
        → flatten through central gateway

## The rule that makes it viable

    Never ask:
    "Has the CORE reached break-even?"

    Ask:
    "If the MIMIC fills now and the market immediately reverses to our
    stress liquidation floor, will CORE + MIMIC still finish positive?"

Only when the answer is yes should the MIMIC order be permitted.
