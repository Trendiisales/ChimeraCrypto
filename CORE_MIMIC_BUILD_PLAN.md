# CORE/MIMIC — Phased Build Plan

> Grounded in 3-agent recon of `/Users/jo/ChimeraCrypto` (2026-07-15l). Spec of
> record: `CORE_MIMIC_SPEC.md` (operator verbatim). Everything SHADOW ($0) today.
> Sequenced by dependency + de-risk-early. §12 (live-path repair) gates the LIVE
> cutover, NOT the build/backtest — so we validate the edge cheaply first.

## Ground truth (recon summary)

**Existing scaffold to REUSE (do not greenfield):**
- `include/core/CryptoCampaignManager.hpp` — CORE-parent + MIMIC-lot campaign book,
  fee-BE floor + net-lock + HWM-trail + funding-eq sizing, `mimic_enabled=false`.
  Closest to §10 state machine. Virtual-lot, shadow-close today. The H1-upjump
  predecessor this spec supersedes. Trigger differs (WINDOW/upjump vs new
  compression→breakout→pullback→reclaim CORE).
- `include/core/UpJumpLadderCompanion.hpp` — current companion; registration pattern
  (`_grid` stable-address vector, `_all_clips`, MIMIC-FLOOR-VIOLATION boot gate).

**Canonical live path (parent EdgeEngine) — the target to route into:**
`EdgeEngine.exit_position_ → on_order_intent_ → governed_submit → ExecutionGateway::submit
→ SpotExecutor::execute (private, gateway-only) → ExchangeLedger g_ledger (authoritative)
+ engine_enter_cooldown`. This IS canonical. Companions bypass it.

**§12 blockers (both OPEN by design, SHADOW):**
- CH-C01 — companions/slots have NO live order-intent route (`wire_engine` legacy-gated).
- CH-C02 — companion close = `persist_companion_clip` (CSV + desk export only), bypasses
  gateway/ledger/cooldown/protection.
- Audit lives in vault: `Memory-Chimera/wiki/entities/ExternalEngineAudit20260714.md`.
  No standalone doc. CH-H02 (shutdown core-dump) already FIXED build 468597c.
- §12 acceptance criteria: 0 fully satisfied, 4 partial (reuse infra: registration,
  deterministic replay, catastrophe-flatten, restart-reconcile), 5 open (order callbacks,
  central close routing, partial-fill reconcile, authoritative cost model, forced win/loss).

**9 primitives — reuse / extend / build:**
- REUSE (3): OFI+agg-buy-share+CVD (`AggressiveFlowEngine`, `L2Book::imbalance_top`,
  `ImbalanceVelocity`, `DerivativesSignals.cvd_div`); perp confirm feed (`DerivativesSignals`,
  `LiquidationCascadeDetector`, funding data); 1m spot klines w/ taker-buy vol
  (`data/klines_spot/{BTC,ETH}USDT_1m.csv`, ~1yr).
- EXTEND (3): cost model (`CryptoCostLedger`+`CryptoOpportunityGate` have the exact 3×-cost
  hurdle; add p95/p99 exit-slip + fee decomp + live-fill feed); regime gate (EdgeEngine
  per-instance trend states + D1 mtf bool + BTC-corr suppressor; add 1h+4h pair + clean
  BTC-positive boolean); market structure (`TrendPullbackReclaimEngine` daily EMA reclaim;
  add explicit swing HH/HL pivot detector).
- BUILD (3): depth-adjusted full-qty liquidation (`L2Book` ladder exists, nothing walks it;
  = `CoreNetLiquidationPnL`/`MimicLossPerUnitAtFloor`; UN-BACKTESTABLE, forward-only);
  event-anchored VWAP (only session VWAP today); 5m-ATR-bps adaptive trail (only daily-ATR
  + bp-step tables today).

**DATA REALITY (corrected 2026-07-15l — earlier "un-backtestable" was WRONG):**
Free Binance Vision dumps cover our 2025-05..2026-05 window:
- **perp `bookDepth`** (daily, ~0.5MB/day) — ±0.2/1/2/3/4/5% cumulative bid/ask depth+notional,
  ~30s cadence. Walk the negative bands to price a full-qty sell = depth-adjusted liquidation
  (§2/§5). PERP-book proxy (spot has NO free depth), but BTC/ETH perp is deepest/tightest-basis
  → good proxy. **The protection arithmetic IS backtestable** (on the proxy), not forward-only.
- **spot `aggTrades`** (monthly) — every trade w/ isBuyerMaker → tick-resolution tape aggression,
  OFI, CVD (§3/§7). Fully free-backtestable.
- 1m spot klines (already local) carry `taker_buy_base` → per-minute agg-buy-share (enough for
  Phase-1 without the tick tape).
Pulled by `tools/fetch_binance_depth_flow.sh` → data/bookdepth_perp, data/aggtrades_spot (gitignored).
Residual limits: perp≠spot book (basis-close for BTC/ETH); ~30s depth cadence (fine for a
per-minute campaign controller); TRUE spot L2 still needs a paid vendor (Tardis) or the Phase-8
forward spot-depth collector — but a legitimate free historical depth proxy now exists.

---

## Phase order (dependency + de-risk-early)

### Phase 1 — CORE EDGE VALIDATION ✅ DONE (2026-07-15l) — CONDITIONAL PASS
**Result: CORE archetype has a real, null-verified structural edge on ETH + XRP; dead on BTC +
5 other majors. Instrument universe = ETH+XRP (NOT BTC+ETH). See CORE_MIMIC_PHASE1_FINDINGS.md.**
Harness: Crypto/backtest/core_trigger_bt.cpp. ETH PF2.4-2.6 / XRP PF1.9, both-WF+, 2×-cost PASS,
plateau-backed, randz 96-99th %ile vs random-entry null. Thin freq (7-18 trades/yr/coin). Crude
15m/flat-35bp form → RE-VALIDATE under Phase-2 finer cost/flow before committing machinery.
(original plan for this phase:)
Build the §3 CORE trigger as a BACKTEST on 1m BTC/ETH klines:
15m compression → range breakout w/ real aggressive spot buying → pullback holds above old
range high / anchored VWAP → higher-low forms → reclaims pullback high → CORE BUY.
- Reuse: agg-buy-share + OFI (#3).
- Build (needed here): swing HH/HL pivot detector (#5), event-anchored VWAP (#4).
- Apply the §3 regime gate (1h+4h up, above VWAP, HH/HL, BTC-positive, spread/depth ok,
  expected move ≥ 3× safe cost). Use flat conservative safe_cost for BT (real depth cost
  unavailable historically — state the assumption).
- **GATE:** does CORE ALONE clear net>0, PF≥1.3, 3×-cost hurdle, both WF halves, on BOTH
  BTC + ETH? Long-only, omit 2022, show bleed don't gate it.
  - PASS → proceed to Phase 2.
  - FAIL → STOP. Report. No campaign machinery gets built on a CORE with no edge.
  This phase de-risks the entire multi-week effort for ~days of work.

### Phase 2 — COST MODEL + DEPTH-LIQUIDATION PRIMITIVES
- Extend `CryptoCostLedger`: p95/p99 exit-slip, buy/sell-fee decomposition, fill-telemetry
  feed. `safe_cost_bps` per §2 (dynamic, not flat 20bp).
- Build depth-adjusted full-qty liquidation: walk `L2Book` bid ladder → sell FULL qty →
  `CoreNetLiquidationPnL`, `MimicLossPerUnitAtFloor`. Unit-test vs synthetic ladders;
  forward-measure live (un-backtestable). This is the authoritative BE/protection math.

### Phase 3 — CORE 3-STATE COST MACHINE (§4)
CORE_OPEN_RISK → CORE_COST_COVERED (persist across N updates / dwell) → CORE_PROTECTED
(install acknowledged positive floor +5..10bp net after stress). Structural risk-sized stop.
Built on Phase-2 net-liquidation.

### Phase 4 — FUNDED MIMIC sizing + trigger (§5, §6)
- QmMax = (CoreProfitAtFloor − reserve) / MimicLossPerUnitAtFloor; caps: ≤0.5×Qc, cfg,
  portfolio, liquidity. Recompute every candidate entry.
- MIMIC trigger = second independent continuation (protected + funded + fresh higher-low +
  rebreak + orderflow re-accelerates + remaining move ≥ 2–3× mimic cost). One mimic/campaign.
- Reuse `CryptoCampaignManager` mimic-lot scaffold (flip `mimic_enabled`, rewire sizing to
  the funding equation).

### Phase 5 — SHARED CAMPAIGN CONTROLLER + INVENTORY AUTHORITY (§7, §9, §10)
- 11-state machine IDLE→CORE_PENDING→…→BOTH_OPEN_PROTECTED→EXITING→COOLDOWN + mandatory
  MIMIC_PENDING guards.
- ONE inventory authority: core_remaining + mimic_remaining = exchange_owned; single
  liquidation instruction; pro-rata fill allocation back to legs.
- Two exit layers: (1) hard campaign-stress-PnL floor (no indicators) + (2) reversal
  (structural/giveback) AND ≥1 orderflow confirmation.
- Build 5m-ATR-bps adaptive trail (#7): max(vol_noise_floor, exec_noise_floor, structural).
- Reuse CryptoCampaignManager state scaffold.

### Phase 6 — §12 LIVE-PATH REPAIR (HARD GATE before ANY live) 🔴
- Route campaign close through `ExecutionGateway::submit → SpotExecutor → ExchangeLedger`
  (NOT persist_companion_clip). Fixes CH-C01 + CH-C02.
- Central close/accounting/cooldown/protection routing. Partial-fill reconciliation.
  Mandatory unique engine registration (enforce, not just log). One authoritative cost model.
- Tests: deterministic replay, forced win/loss, catastrophe flatten, restart reconcile.
- Extend `SpotExecutor`/`BinanceREST` with pegged limit + server-side catastrophic stop +
  OCO trailing + OPO/OPOCO (§8) — currently only market + maker-limit + emergency-flatten.

### Phase 7 — BINANCE EXEC SPECIFICS + FAILURE CASES (§8, §11)
Exchange-truth driving (request≠accepted≠filled≠fully-filled; API-timeout=unknown-state →
recover from user-data stream before re-sending). All §11 failure cases: partial fills,
fee-in-base/quote/BNB, min-notional/dust, stale/out-of-order data, stream disconnect, stop
reject, cancel/replace race, restart-while-open, balance-vs-inventory mismatch, double exit
trigger, mimic-fills-after-reversal, spread/depth deterioration between trigger and fill.

### Phase 8 — SHADOW FORWARD → LIVE CUTOVER
Run full campaign in SHADOW forward on BTC/ETH. Forward-measure the un-backtestable depth
liquidation. Verify §12 green (all acceptance tests pass). THEN operator authorizes live.
Vault update on deploy (mandate). Start BTC+ETH only; SOL/smaller disabled until each
independently clears the same gate.

---

## Standing constraints (carry into every phase)
- Companion/mimic judged STANDALONE, never vs WIDE (`feedback-companion-independent-engine`).
  BUT this spec's campaign-protection once BOTH open is explicitly in-scope (§ combined exit)
  — independence is entry/qty/cost/PnL/IDs; protection is campaign-level.
- Long-only spot; NO 200DMA anywhere; omit 2022 from any gate; ALL relevant symbols one pass
  (here = BTC+ETH from the start, not one then the other).
- Exec venue = Binance (restore SpotExecutor real path), NOT IBKR. C++, no Python in live.
- Every new engine ships a backtested loss-protection verdict (here: the campaign floor is it,
  but the CORE structural stop needs its own backtested verdict in Phase 1/3).
- Deploy not done until Memory-Chimera vault reflects it.
