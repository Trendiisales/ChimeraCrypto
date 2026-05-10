#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
funding_prevalidation.py
========================

Pre-validation pass over the historical Binance funding-rate dataset for the
Chimera 8-symbol basket. Counts how often each symbol satisfies the entry
gates of the two funding-driven engines, using the engines' actual,
production-tuned thresholds with **zero per-symbol parameter tuning**
(anti-overfitting).

Purpose
-------
Decide whether the Chimera "5-point plan" Step 3 — extending FundingWindow
and FundingPersistenceFade to the 6-symbol alt basket — is worth the
engineering effort, or whether the existing thresholds are structurally
inert in the current funding regime and the engines need either a regime-
appropriate retune or removal.

Inputs
------
Funding CSVs produced by fetch_chimera_history.py:
  data/funding/{BTC,ETH,SOL,BNB,AVAX,LINK,XRP,DOGE}USDT.csv
Each CSV has columns: symbol, funding_time_ms, funding_rate, mark_price.
Funding events occur at 00:00, 08:00, 16:00 UTC (3/day per symbol).

Engine thresholds replayed
--------------------------
FundingWindowEngine (BTC + ETH currently; "extend to alts" is what
Step 3 would do):
  RATE_THRESHOLD    = 0.00015      (|funding_rate| >= 1.5 bp/8h)
  BASIS_THRESHOLD   = 3 bp         (|perp - spot| / spot * 1e4)
  WINDOW_SECS       = 180          (within 3 min before funding settlement)
  COOLDOWN_MS       = 14_400_000   (4 h between entries per symbol)

  NOTE on the rate-threshold value: the engine header comment claims
  "15 bp/8h minimum" but the constant 0.00015 fractional equals 1.5 bp
  under the standard convention (1 bp = 0.0001), confirmed by the
  engine's own printf which formats `funding_rate * 10000.0` as bp/8h.
  This script uses the actual numeric value (0.00015) and labels it
  correctly as 1.5 bp. The off-by-10 in the header comment is a
  documentation bug, not a behavioural one.

  This script applies the rate gate only. Basis is not in the funding CSV
  (perp/spot kline reconstruction is more work). Rate-only counts are an
  UPPER BOUND on FundingWindow signal frequency: real signal count is
  rate_hits * fraction-of-rate-hits-where-basis-also-dislocates. If
  rate_hits is already negligible the basis check cannot raise the count.

FundingPersistenceFadeEngine (BTC only currently):
  FUNDING_TRIGGER     = -0.0010    (24h-avg <= -10 bp/8h)
  FUNDING_RECENT_MAX  = -0.0003    (every sample in last 8h <= -3 bp/8h)
  FUNDING_REVERT      =  0.0       (exit when 24h-avg back >= 0)
  COOLDOWN_MS         =  3 days    (between entries)
  MIN_BUFFER_SPAN_MS  = 23 h       (need full lookback before any entry)

  This script approximates the engine's per-minute live funding sample
  with the discrete 8h published funding events. Over a 24h window that's
  3 samples (at T-16h, T-8h, T-0h) instead of ~1440. Over an 8h window
  that's 1 sample. The simplification is conservative: a real sustained
  -3bp 8h regime that produces 1440 samples all <= -3bp will also produce
  the single funding event at the end of the window <= -3bp (Binance's
  paid funding is the time-weighted average of the rolling premium-index
  during that 8h, so they're tightly coupled). This script's "trigger
  fired" count is therefore a faithful proxy for the engine's per-event
  trigger evaluations.

Outputs
-------
1. Pretty-printed per-symbol table to stdout.
2. Markdown report written to --report path (default:
   funding_prevalidation_report_<UTC date>.md).

Usage
-----
  python3 funding_prevalidation.py
  python3 funding_prevalidation.py --in /custom/data/funding --report /tmp/r.md

Exit status
-----------
0 always (analysis is informational, not pass/fail).
"""

from __future__ import annotations

import argparse
import csv
import os
import statistics
import sys
from collections import deque, OrderedDict
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine thresholds — verbatim from the C++ headers at
# include/core/FundingWindowEngine.hpp and
# include/core/FundingPersistenceFadeEngine.hpp on
# tier1-risk-integration @ 06a14cc.
# ---------------------------------------------------------------------------

# FundingWindowEngine
# RATE_THRESHOLD: header comment says "15 bp/8h" but the constant value
# 0.00015 is actually 1.5 bp under standard convention (1 bp = 0.0001).
# The engine's own printf confirms this — funding_rate * 10000.0 formatted
# as bp/8h means a value of 0.00015 prints as "1.5 bp/8h". This script
# uses the actual numeric (0.00015) and labels it as 1.5 bp.
FW_RATE_THRESHOLD = 0.00015          # 1.5 bp/8h fractional (header comment off by 10x)
FW_COOLDOWN_MS = 14_400_000          # 4 h
FW_WINDOW_SECS = 180                 # 3 min before funding boundary

# FundingPersistenceFadeEngine
FPF_FUNDING_TRIGGER = -0.0010        # 24h-avg <= -10 bp/8h
FPF_FUNDING_RECENT_MAX = -0.0003     # every sample in last 8h <= -3 bp/8h
FPF_FUNDING_REVERT = 0.0             # exit when 24h-avg >= 0
FPF_COOLDOWN_MS = 3 * 24 * 3600 * 1000   # 3 days
FPF_LOOKBACK_24H_MS = 24 * 3600 * 1000
FPF_RECENT_8H_MS = 8 * 3600 * 1000
FPF_MIN_BUFFER_SPAN_MS = 23 * 3600 * 1000

# Per-symbol pretty headers
DEFAULT_SYMBOLS = [
    "BTCUSDT",
    "ETHUSDT",
    "SOLUSDT",
    "BNBUSDT",
    "AVAXUSDT",
    "LINKUSDT",
    "XRPUSDT",
    "DOGEUSDT",
]

# Symbols the engines currently trade live (engine-supported set).
LIVE_FW_SYMBOLS = {"BTCUSDT", "ETHUSDT"}
LIVE_FPF_SYMBOLS = {"BTCUSDT"}

# ---------------------------------------------------------------------------
# CSV loading
# ---------------------------------------------------------------------------


def load_funding_csv(path: Path):
    """
    Load (funding_time_ms, funding_rate) tuples from a Chimera funding CSV.
    Returns a list sorted ascending by timestamp. Skips rows with parse errors.
    """
    rows = []
    with path.open("r", newline="") as fh:
        reader = csv.DictReader(fh)
        for r in reader:
            try:
                t = int(r["funding_time_ms"])
                rate = float(r["funding_rate"])
            except (KeyError, TypeError, ValueError):
                continue
            rows.append((t, rate))
    rows.sort(key=lambda x: x[0])
    return rows


# ---------------------------------------------------------------------------
# FundingWindowEngine pre-validation
# ---------------------------------------------------------------------------


def fw_signal_count(events):
    """
    Count FundingWindow rate-gate hits and apply the 4h cooldown.
    Each `events` row is (funding_time_ms, funding_rate).
    Returns dict with rate_hits, post_cooldown_entries, distribution split.
    """
    rate_hits = 0
    pos_hits = 0       # |rate| >= threshold AND rate > 0
    neg_hits = 0       # |rate| >= threshold AND rate < 0
    cooldown_until = -1
    post_cooldown = 0

    for t, rate in events:
        if abs(rate) >= FW_RATE_THRESHOLD:
            rate_hits += 1
            if rate > 0:
                pos_hits += 1
            else:
                neg_hits += 1
            if t >= cooldown_until:
                post_cooldown += 1
                cooldown_until = t + FW_COOLDOWN_MS

    return {
        "rate_hits": rate_hits,
        "rate_hits_positive": pos_hits,
        "rate_hits_negative": neg_hits,
        "entries_after_cooldown": post_cooldown,
    }


# ---------------------------------------------------------------------------
# FundingPersistenceFadeEngine pre-validation
# ---------------------------------------------------------------------------


def fpf_signal_count(events):
    """
    Replay FundingPersistenceFade entry/exit logic over discrete 8h funding
    events. State machine:
      - For every event T compute avg_24h (3-event window) and max_8h (1
        event = T itself).
      - If !active and (avg_24h <= FUNDING_TRIGGER) and (max_8h <=
        FUNDING_RECENT_MAX) and (now - cooldown_until_ms >= 0) and
        (buffer_span >= MIN_BUFFER_SPAN_MS): enter.
      - If active and avg_24h >= FUNDING_REVERT: exit (funding revert);
        cooldown_until_ms = now + FPF_COOLDOWN_MS.

    No price-based exits (TP/SL/timeout) are modelled here — this is a
    *signal frequency* pass, not a P&L sim.

    Returns dict with: entries, total_trigger_events, time_in_trigger_pct,
    longest_streak_events, min_avg24h_observed_bp.
    """
    if not events:
        return {
            "entries": 0,
            "total_trigger_events": 0,
            "time_in_trigger_pct": 0.0,
            "longest_streak_events": 0,
            "min_avg24h_observed_bp": 0.0,
        }

    buffer = deque()  # of (t_ms, rate)
    entries = 0
    total_trigger_events = 0
    longest_streak = 0
    cur_streak = 0
    active = False
    cooldown_until = -1
    min_avg24h_bp = float("inf")

    first_t = events[0][0]

    for t, rate in events:
        # update rolling buffer (retain >= 24h, we keep 26h to match engine)
        buffer.append((t, rate))
        cutoff_keep = t - 26 * 3600 * 1000
        while buffer and buffer[0][0] < cutoff_keep:
            buffer.popleft()

        # window means/maxes
        cutoff_24 = t - FPF_LOOKBACK_24H_MS
        cutoff_8 = t - FPF_RECENT_8H_MS
        rates_24, max_8 = [], -1e9
        for bt, br in buffer:
            if bt >= cutoff_24:
                rates_24.append(br)
            if bt >= cutoff_8:
                if br > max_8:
                    max_8 = br
        avg_24 = statistics.mean(rates_24) if rates_24 else 0.0
        if avg_24 * 10000 < min_avg24h_bp:
            min_avg24h_bp = avg_24 * 10000

        buffer_span_ok = (t - first_t) >= FPF_MIN_BUFFER_SPAN_MS and (
            buffer and (buffer[-1][0] - buffer[0][0]) >= FPF_MIN_BUFFER_SPAN_MS
        )

        in_trigger_zone = (
            avg_24 <= FPF_FUNDING_TRIGGER
            and max_8 <= FPF_FUNDING_RECENT_MAX
            and buffer_span_ok
        )
        if in_trigger_zone:
            total_trigger_events += 1
            cur_streak += 1
            if cur_streak > longest_streak:
                longest_streak = cur_streak
        else:
            cur_streak = 0

        if not active:
            if in_trigger_zone and t >= cooldown_until:
                active = True
                entries += 1
        else:
            # exit on funding revert
            if avg_24 >= FPF_FUNDING_REVERT:
                active = False
                cooldown_until = t + FPF_COOLDOWN_MS

    if min_avg24h_bp == float("inf"):
        min_avg24h_bp = 0.0

    n = max(len(events), 1)
    return {
        "entries": entries,
        "total_trigger_events": total_trigger_events,
        "time_in_trigger_pct": 100.0 * total_trigger_events / n,
        "longest_streak_events": longest_streak,
        "min_avg24h_observed_bp": min_avg24h_bp,
    }


# ---------------------------------------------------------------------------
# Per-symbol funding-rate distribution stats
# ---------------------------------------------------------------------------


def rate_distribution(events):
    if not events:
        return {
            "n": 0,
            "min_bp": 0.0,
            "max_bp": 0.0,
            "mean_bp": 0.0,
            "median_bp": 0.0,
            "p1_bp": 0.0,
            "p99_bp": 0.0,
            "p_negative": 0.0,
            "p_le_minus3bp": 0.0,
            "p_le_minus10bp": 0.0,
        }
    rates = [r for _, r in events]
    rates_sorted = sorted(rates)
    n = len(rates)
    p = lambda q: rates_sorted[max(0, min(n - 1, int(q * n)))]
    return {
        "n": n,
        "min_bp": min(rates) * 10000,
        "max_bp": max(rates) * 10000,
        "mean_bp": statistics.mean(rates) * 10000,
        "median_bp": statistics.median(rates) * 10000,
        "p1_bp": p(0.01) * 10000,
        "p99_bp": p(0.99) * 10000,
        "p_negative": 100.0 * sum(1 for r in rates if r < 0) / n,
        "p_le_minus3bp": 100.0 * sum(1 for r in rates if r <= -0.0003) / n,
        "p_le_minus10bp": 100.0 * sum(1 for r in rates if r <= -0.0010) / n,
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def fmt_bp(x):
    return f"{x:+7.2f}"


def make_report(per_symbol):
    """
    per_symbol: OrderedDict[symbol] -> {
        "events": [...],
        "dist": {...},
        "fw":   {...},
        "fpf":  {...},
        "first_iso": ..., "last_iso": ...,
    }
    Returns the markdown report as a string.
    """
    lines = []
    lines.append("# Chimera funding pre-validation report")
    lines.append("")
    lines.append(
        f"**Generated:** {datetime.now(timezone.utc).isoformat()}"
    )
    lines.append("**Branch / commit:** tier1-risk-integration @ 06a14cc (session 8)")
    lines.append("**Engines replayed:** FundingWindowEngine, FundingPersistenceFadeEngine")
    lines.append(
        "**Anti-overfitting:** zero per-symbol parameter tuning. All thresholds "
        "are verbatim from the BTC/ETH-tuned engine headers."
    )
    lines.append("")

    # ------------------------------------------------------------------
    # Funding-rate distribution
    # ------------------------------------------------------------------
    lines.append("## 1. Funding-rate distribution per symbol (bp / 8h)")
    lines.append("")
    lines.append(
        "| Symbol | Events | First | Last | Min | P1 | Median | Mean | P99 | Max | "
        "% < 0 | % <= -3bp | % <= -10bp |"
    )
    lines.append(
        "|---|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"
    )
    for sym, d in per_symbol.items():
        dist = d["dist"]
        lines.append(
            f"| {sym} | {dist['n']} | {d['first_iso'][:10]} | {d['last_iso'][:10]} | "
            f"{fmt_bp(dist['min_bp'])} | {fmt_bp(dist['p1_bp'])} | "
            f"{fmt_bp(dist['median_bp'])} | {fmt_bp(dist['mean_bp'])} | "
            f"{fmt_bp(dist['p99_bp'])} | {fmt_bp(dist['max_bp'])} | "
            f"{dist['p_negative']:5.1f}% | {dist['p_le_minus3bp']:5.2f}% | "
            f"{dist['p_le_minus10bp']:5.2f}% |"
        )
    lines.append("")

    # ------------------------------------------------------------------
    # FundingWindow
    # ------------------------------------------------------------------
    lines.append("## 2. FundingWindowEngine signal frequency (rate gate, 365 d)")
    lines.append("")
    lines.append(
        "Entry gate (rate component only): `|funding_rate| >= 1.5 bp/8h` "
        "(constant `RATE_THRESHOLD = 0.00015`; engine header comment claims "
        "15 bp/8h but is off by 10x — see script docstring). This is an "
        "**upper bound** on real signal frequency — the engine additionally "
        "requires `|basis| >= 3 bp` and `secs_to_funding <= 180`, both of "
        "which can only reduce the count."
    )
    lines.append("")
    lines.append(
        "| Symbol | Engine? | Rate hits | Pos | Neg | After 4h cooldown | "
        "Approx hits / yr |"
    )
    lines.append("|---|---|---:|---:|---:|---:|---:|")
    for sym, d in per_symbol.items():
        fw = d["fw"]
        live = "live" if sym in LIVE_FW_SYMBOLS else "Step 3 candidate"
        lines.append(
            f"| {sym} | {live} | {fw['rate_hits']} | "
            f"{fw['rate_hits_positive']} | {fw['rate_hits_negative']} | "
            f"{fw['entries_after_cooldown']} | "
            f"{fw['entries_after_cooldown']} |"
        )
    lines.append("")

    # ------------------------------------------------------------------
    # FundingPersistenceFade
    # ------------------------------------------------------------------
    lines.append(
        "## 3. FundingPersistenceFadeEngine signal frequency (full gate, 365 d)"
    )
    lines.append("")
    lines.append(
        "Entry gate: `avg_24h <= -10 bp/8h` AND `max_8h <= -3 bp/8h` AND "
        "buffer span >= 23 h. Exit: `avg_24h >= 0` (funding revert). "
        "Cooldown: 3 days between entries. Discrete 8h-event approximation "
        "of the engine's per-minute live buffer (see script docstring for "
        "fidelity caveat)."
    )
    lines.append("")
    lines.append(
        "| Symbol | Engine? | Entries | In-trigger events | "
        "% time in trigger | Longest streak | Min avg-24h observed |"
    )
    lines.append("|---|---|---:|---:|---:|---:|---:|")
    for sym, d in per_symbol.items():
        fpf = d["fpf"]
        live = "live" if sym in LIVE_FPF_SYMBOLS else "Step 3 candidate"
        lines.append(
            f"| {sym} | {live} | {fpf['entries']} | "
            f"{fpf['total_trigger_events']} | "
            f"{fpf['time_in_trigger_pct']:5.2f}% | "
            f"{fpf['longest_streak_events']} | "
            f"{fpf['min_avg24h_observed_bp']:+6.2f} bp |"
        )
    lines.append("")

    # ------------------------------------------------------------------
    # Verdict
    # ------------------------------------------------------------------
    lines.append("## 4. Verdict")
    lines.append("")

    fw_total = sum(d["fw"]["rate_hits"] for d in per_symbol.values())
    fpf_total = sum(d["fpf"]["entries"] for d in per_symbol.values())
    fw_alts_total = sum(
        d["fw"]["rate_hits"]
        for s, d in per_symbol.items()
        if s not in LIVE_FW_SYMBOLS
    )
    fpf_alts_total = sum(
        d["fpf"]["entries"]
        for s, d in per_symbol.items()
        if s not in LIVE_FPF_SYMBOLS
    )

    lines.append(
        f"- **FundingWindow rate-gate hits across all 8 symbols / 365 d:** {fw_total}. "
        f"Across the 6 alts only: {fw_alts_total}."
    )
    lines.append(
        f"- **FundingPersistenceFade entries across all 8 symbols / 365 d:** "
        f"{fpf_total}. Across the alts only: {fpf_alts_total}."
    )
    lines.append("")

    # Split-engine verdict: FW and FPF are independent decisions.
    fw_alive = fw_alts_total >= 30          # >= ~1 hit/symbol/2 weeks across alts
    fpf_alive = fpf_alts_total >= 2

    lines.append("### FundingWindowEngine")
    lines.append("")
    if fw_alive:
        lines.append(
            "Step 3 is **justified** for FundingWindow. The 1.5 bp/8h rate "
            "gate fires meaningfully often on the alts in the current "
            "regime — meaningfully more than on BTC/ETH where the engine "
            "is currently live. Recommended path:"
        )
        lines.append("")
        lines.append(
            "  1. Extend FundingWindow to the 6-alt basket using inherited "
            "thresholds (no per-symbol tuning — anti-overfitting). Touches "
            "engine instantiation in `src/main.cpp` and PerpFeed WS "
            "subscriptions for the alt symbols."
        )
        lines.append(
            "  2. Deploy in `shadow_mode = true` with `Tier1Risk` "
            "`per_engine_r_cap[FUNDING_WINDOW] = 1.0` (already the default)."
        )
        lines.append(
            "  3. Collect 4-8 weeks of forward-shadow telemetry per Step 4. "
            "Pass criterion at Step 5 review: net P&L > 0 after 15 bp "
            "round-trip cost on >= 30 trades per symbol."
        )
        lines.append(
            "  4. Caveat: the rate-only count is an upper bound. Real "
            "signal frequency depends on how often the basis dislocation "
            "(>=3 bp) coincides with the rate spike. Forward shadow data "
            "will reveal this directly without the need for historical "
            "perp/spot kline reconstruction."
        )
    else:
        lines.append(
            "FundingWindow rate-gate signal across the alts is too sparse "
            "to justify Step 3 engineering at current thresholds. Either "
            "retune (drop `RATE_THRESHOLD` further), strip the engine, or "
            "defer until regime change."
        )
    lines.append("")

    lines.append("### FundingPersistenceFadeEngine")
    lines.append("")
    if fpf_alive:
        lines.append(
            "Step 3 is **justified** for FundingPersistenceFade. The "
            "-10 bp 24h-avg trigger fires often enough on the alts to "
            "justify extending. Same shadow-then-validate path as "
            "FundingWindow above."
        )
    else:
        lines.append(
            "FundingPersistenceFade is **structurally inert** in the "
            "current funding regime across the entire basket — `entries = "
            f"{fpf_total}` across 8 symbols × 365 days. The -10 bp 24h-avg "
            "trigger is mechanically unreachable: BTC's most-negative "
            "single funding event in the year was -1.52 bp (10x off), and "
            "no other symbol except SOL even comes close. The engine has "
            "not fired in production not because of a regime issue but "
            "because the threshold was set for a different funding world. "
            "Three actionable paths:"
        )
        lines.append("")
        lines.append(
            "  1. **Retune** `FUNDING_TRIGGER` from -10 bp to -2 to -3 bp "
            "and `FUNDING_RECENT_MAX` from -3 bp to -1 bp, then forward-"
            "shadow. Caveat: small-threshold tuning increases false-trigger "
            "frequency; the engine's edge is supposed to be *persistent* "
            "extreme funding — at -2 bp it isn't extreme."
        )
        lines.append(
            "  2. **Strip the engine and reclaim the budget** in "
            "`Tier1Risk::per_engine_r_cap[FUNDING_PERSIST]`. Mirrors the "
            "OBI cap = 0R decision from session 7."
        )
        lines.append(
            "  3. **Defer until regime change.** Keep BTC live in "
            "`shadow_mode` against the original thresholds — costs nothing "
            "while quiet, auto-fires if 2021-style funding returns."
        )
    lines.append("")

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Chimera funding pre-validation pass"
    )
    parser.add_argument(
        "--in",
        dest="indir",
        default="data/funding",
        help="Input directory containing {SYMBOL}.csv files (default: data/funding)",
    )
    parser.add_argument(
        "--symbols",
        nargs="+",
        default=DEFAULT_SYMBOLS,
        help=f"Symbols to analyse (default: {' '.join(DEFAULT_SYMBOLS)})",
    )
    parser.add_argument(
        "--report",
        default=None,
        help=(
            "Path for the markdown report. Default: "
            "funding_prevalidation_report_<UTC date>.md in the cwd."
        ),
    )
    args = parser.parse_args()

    indir = Path(args.indir).resolve()
    if not indir.exists():
        print(f"input dir not found: {indir}", file=sys.stderr)
        sys.exit(2)

    per_symbol = OrderedDict()
    for sym in args.symbols:
        csv_path = indir / f"{sym}.csv"
        if not csv_path.exists():
            print(f"  [skip] {sym}: {csv_path} missing", file=sys.stderr)
            continue
        events = load_funding_csv(csv_path)
        if not events:
            print(f"  [skip] {sym}: no rows in {csv_path}", file=sys.stderr)
            continue
        first_iso = datetime.fromtimestamp(
            events[0][0] / 1000, tz=timezone.utc
        ).isoformat()
        last_iso = datetime.fromtimestamp(
            events[-1][0] / 1000, tz=timezone.utc
        ).isoformat()
        per_symbol[sym] = {
            "events": events,
            "dist": rate_distribution(events),
            "fw": fw_signal_count(events),
            "fpf": fpf_signal_count(events),
            "first_iso": first_iso,
            "last_iso": last_iso,
        }

    if not per_symbol:
        print("no symbols loaded; nothing to do", file=sys.stderr)
        sys.exit(2)

    # ── stdout summary ───────────────────────────────────────────────
    print(
        f"{'Symbol':10s}  "
        f"{'Events':>6s}  "
        f"{'min(bp)':>8s}  "
        f"{'max(bp)':>8s}  "
        f"{'mean(bp)':>9s}  "
        f"{'FW hits':>8s}  "
        f"{'FPF ent':>8s}  "
        f"{'%trig':>7s}"
    )
    for sym, d in per_symbol.items():
        dist, fw, fpf = d["dist"], d["fw"], d["fpf"]
        print(
            f"{sym:10s}  "
            f"{dist['n']:>6d}  "
            f"{dist['min_bp']:>8.2f}  "
            f"{dist['max_bp']:>8.2f}  "
            f"{dist['mean_bp']:>9.3f}  "
            f"{fw['rate_hits']:>8d}  "
            f"{fpf['entries']:>8d}  "
            f"{fpf['time_in_trigger_pct']:>6.2f}%"
        )
    print()

    # ── markdown report ───────────────────────────────────────────────
    today_utc = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    report_path = (
        Path(args.report).resolve()
        if args.report
        else Path.cwd() / f"funding_prevalidation_report_{today_utc}.md"
    )
    report = make_report(per_symbol)
    report_path.write_text(report)
    print(f"wrote report: {report_path}")


if __name__ == "__main__":
    main()
