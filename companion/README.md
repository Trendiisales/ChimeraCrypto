# CryptoUpJump Companion Clip — STANDALONE ADDITIVE paper book

Staged 2026-07-03 alongside the native C++ `*-UPJUMP-H1` engines (EdgeEngine
`StrategyKind::UPJUMP`, ride-to-flip, no trade-level stops).

## What this is

The companion is a **separate, independent paper book** that runs its OWN clip
contract alongside the UpJump parent. It does **NOT** modify, close, move, or
shrink the real UpJump position — the real engine rides to symmetric down-jump
flip (WIDE) regardless. The two COEXIST and are ADDITIVE.

## HARD RULE — judge STANDALONE, never vs-WIDE

**NEVER** evaluate this companion by comparing its clip return to riding WIDE
("beats WIDE", "clip vs WIDE", dominance test). They do NOT compete for the same
capital. Its true alternative is *"don't take it at all"*, not "ride wide".

Judge it STANDALONE: is its own book net-positive after costs, WF both halves,
both regimes? Each of the 5 staged legs passed all-6 gates standalone (per-coin
BT net/PF in the cron-line comments). Canonical rule:
`Memory-Omega/wiki/entities/CompanionDominanceError.md` +
auto-memory `feedback-companion-independent-engine`.

## Status: STAGED REFERENCE ONLY — not wired, not running on this box

- `_upjump_clip_cron_5legs.txt` — the 5 live-feed legs (BTC/ETH/SOL/DOGE/BNB).
  PAPER-only (`COMPANION_PUSH_STATE=0`), cold-loss OFF (protection = the clip's
  own rev/stall/reclip), `STALL_TF_HOURS=1` (24×1h up-jump parent).
- `stall_accountant.py` — the companion runner (verbatim from
  `/Users/jo/stall-accountant/stall_accountant.py`).
- The cron paths point at the Mac Crypto intraday `state.json`. The companion
  currently runs PAPER against that Crypto intraday state, NOT ChimeraCrypto's
  ledger (different state format). Auto-wire to CC's ledger is DEFERRED.
- **Do NOT start these crons on the live box.** Do NOT flip
  `COMPANION_PUSH_STATE=1` without explicit operator go.

## Deferred legs

The 6 no-feed legs (ADA/TRX/AAVE/NEAR/OP) are omitted here — they need Binance
feed subscriptions added to ChimeraCrypto first. AAVE uses an INVERSE lever
(reclip OFF). OP is parent-only (companion not viable any lever).
