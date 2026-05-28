#!/usr/bin/env python3
"""Broader sweep — strategies and TFs not yet covered."""
import subprocess, csv, sys

BIN = "./backtest_mac"
PRESET = "prod_tiered"

SYMS_MAJOR = ["btcusdt","ethusdt","solusdt","bnbusdt","linkusdt","xrpusdt"]
SYMS_ALT   = ["dogeusdt","avaxusdt","nearusdt","suiusdt","aptusdt","arbusdt"]
SYMS_NEW   = ["fetusdt","tiausdt","ondousdt","pepeusdt","wifusdt"]

COMBOS = []
# WILLIAMS_R — untested at H8/H12/D1
for sym in SYMS_MAJOR + SYMS_ALT:
    for tf in [28800, 43200, 86400]:
        COMBOS.append((sym, tf, "WILLIAMS_R"))
# DUAL_THRUST — try H8/H12/D1
for sym in SYMS_MAJOR + ["dogeusdt","avaxusdt","nearusdt"]:
    for tf in [28800, 43200, 86400]:
        COMBOS.append((sym, tf, "DUAL_THRUST"))
# SUPERTREND — H8/H12 (only H4/D1 done)
for sym in SYMS_MAJOR + SYMS_ALT:
    for tf in [28800, 43200]:
        COMBOS.append((sym, tf, "SUPERTREND"))
# ICHIMOKU on remaining alts H12/H8
for sym in ["nearusdt","suiusdt","aptusdt","arbusdt","dogeusdt"]:
    for tf in [28800, 43200]:
        COMBOS.append((sym, tf, "ICHIMOKU"))
# ICHIMOKU on new-data symbols D1
for sym in SYMS_NEW:
    COMBOS.append((sym, 86400, "ICHIMOKU"))
# KELTNER_REVERT on alts H6/H12
for sym in ["dogeusdt","avaxusdt","nearusdt","suiusdt","xrpusdt","bnbusdt"]:
    for tf in [21600, 43200]:
        COMBOS.append((sym, tf, "KELTNER_REVERT"))

MIN_PF, MIN_SH, MIN_N = 2.5, 1.5, 30

candidates = []
for i, (sym, tf, strat) in enumerate(COMBOS):
    spec = f"{sym}:{tf}:{strat}"
    print(f"[{i+1}/{len(COMBOS)}] {spec}", file=sys.stderr)
    r = subprocess.run([BIN, "--sweep", spec, "--preset", PRESET],
                       capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        continue
    best = None
    for line in r.stdout.splitlines()[1:]:
        try:
            c = line.split(",")
            lb, hold = int(c[0]), int(c[1])
            sl, tarm, tdist = float(c[2]), float(c[3]), float(c[4])
            trades, wins = int(c[5]), int(c[6])
            wr, bp, pf, sh, dd = float(c[7]), float(c[8]), float(c[9]), float(c[10]), float(c[11])
        except (ValueError, IndexError):
            continue
        if trades < MIN_N or pf < MIN_PF or sh < MIN_SH:
            continue
        if best is None or bp > best[11]:
            best = (sym, tf, strat, lb, hold, sl, tarm, pf, sh, trades, wr, bp, dd)
    if best:
        candidates.append(best)
        print(f"  WIN {best[2]} lb={best[3]} sl={best[5]} pf={best[7]:.2f} bp={best[11]:.0f}", file=sys.stderr)

candidates.sort(key=lambda c: -c[11])
print("\n========== BROADER SWEEP CANDIDATES ==========")
print(f"{'SYM':10} {'TF':>6} {'STRAT':14} {'lb':>3} {'hold':>4} {'sl':>4} {'PF':>6} {'Sh':>6} {'N':>4} {'WR':>5} {'bp':>9}")
for c in candidates:
    sym, tf, strat, lb, hold, sl, tarm, pf, sh, n, wr, bp, dd = c
    print(f"{sym:10} {tf:>6} {strat:14} {lb:>3} {hold:>4} {sl:>4.1f} {pf:>6.2f} {sh:>6.2f} {n:>4} {wr:>4.1f}% {bp:>9.0f}")

with open("/tmp/edge_candidates2.csv","w",newline="") as f:
    w = csv.writer(f)
    w.writerow(["sym","tf","strat","lb","hold","sl","tarm","pf","sh","n","wr","bp","dd"])
    for c in candidates: w.writerow(c)
print(f"\nSaved {len(candidates)} candidates")
