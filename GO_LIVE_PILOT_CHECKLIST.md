# GO-LIVE PILOT CHECKLIST — Binance SpotExecutor tiny live pilot (2026-07-18)

Scope wired this session: **LIVE PILOT SCOPE** at the ExecutionGateway.
In `mode=live`, entries restricted to `live_pilot_symbols` (BTCUSDT, ETHUSDT),
per-order notional clamped to `live_pilot_max_order_usd` ($12), aggregate open
live gross capped at `live_pilot_max_gross_usd` ($50). Exits NEVER blocked.
Inert in shadow (byte-identical research record). `mode=live` with an empty
pilot list and no explicit `live_full=true` = **hard startup abort** — the
first live window is bounded by construction.

What can trade in the pilot: only gateway-routed sleeves (XSEC-BTC / XSEC-BR /
XSEC2 / RipRider / legacy-gated EdgeEngine intents) and only on BTC/ETH.
The 78-cell UpJump grid + all mimic/cascade companions have their OWN shadow
books, never route through the gateway, and are untouched.

## OPERATOR STEPS (cannot be done by AI — real credentials)

1. **Create the Binance API key** (binance.com → API Management):
   - Enable: *Enable Reading* + *Enable Spot & Margin Trading* ONLY.
   - **Withdrawals: DISABLED** (never enable).
   - IP access restriction: **restrict to `143.198.89.54`** (josgp1).
2. **On josgp1** edit `~/ChimeraCrypto/config/binance_credentials.json`:
   ```json
   { "api_key": "<key>", "secret_key": "<secret>", "shadow_mode": false }
   ```
3. **On josgp1** edit `~/ChimeraCrypto/config/live_config.json`:
   - `"shadow_mode": false`
   - add `"mode": "live"` (cross-check key; contradiction = abort)
   - `"portfolio_cash_usd": 60` (cash reservation ENFORCES; ≥ pilot gross cap)
   - `"portfolio_alloc_mode": "hardcap"` (Stage-2 safety caps ON)
   - keep the `live_pilot_*` keys as committed.
4. `sudo systemctl restart chimera`

## VERIFY (boot log `~/ChimeraCrypto/logs/chimera.log`)

- `[STARTUP] RUNTIME MODE = LIVE (live_config.shadow_mode=0 creds_shadow=0)`
- `[PILOT-SCOPE] LIVE pilot ACTIVE: 2 symbol(s) [BTCUSDT,ETHUSDT] max_order=$12.00 max_gross=$50.00`
- `[EXECUTOR] Ready. shadow=NO (LIVE)` (a real balance fetch succeeded)
- StartupReconciler `PASS` (first live boot reconciles vs the REAL account —
  a mismatch BLOCKS entries until clean; expected behaviour, not a bug)
- `[ALLOC] ... mode=HARDCAP`, `[MIMIC-FLOOR-GATE] ... 0 VIOLATION(refused)`
- User-stream heartbeat live (8G auto-halt now armed for real — first live arming)

## ROLLBACK (any doubt = do this)

- `shadow_mode: true` in BOTH config files, remove/`"mode":"shadow"`, restart.
- Emergency: GUI kill button (nginx-injected token) or the `/api/kill` endpoint
  (tokened), then `emergency_flatten` closes pilot legs market.

## KNOWN LIMITS OF THE PILOT (accepted, documented)

- CH-C02 still open: slot-engine closes bypass central protection accounting —
  pilot sleeves route via the gateway so the pilot itself is covered.
- Worst case bounded: ≤$50 gross, ≤$12/order, 2 majors, kill switch + daily-loss
  halts + heartbeat auto-halt + clock-drift halt all armed.
- Real-fill forward record (net_bp_real dual column) is ~11d into its 30d
  window — pilot is the instrument that matures it, sized accordingly.
