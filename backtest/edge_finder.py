#!/usr/bin/env python3
"""Run targeted sweeps, collect top results, filter by quality bar."""
import subprocess, csv, sys, pathlib
from collections import defaultdict

BIN = "./backtest_mac"
PRESET = "prod_tiered"

# (symbol, tf_secs, strategy)
COMBOS = []
# ICHIMOKU D1 on all liquid alts
for sym in ["solusdt","bnbusdt","linkusdt","avaxusdt","dogeusdt","xrpusdt",
            "nearusdt","suiusdt","aptusdt","arbusdt"]:
    COMBOS.append((sym, 86400, "ICHIMOKU"))
# ICHIMOKU H12 on majors
for sym in ["btcusdt","ethusdt","solusdt","bnbusdt","linkusdt","dogeusdt","xrpusdt"]:
    COMBOS.append((sym, 43200, "ICHIMOKU"))
# ICHIMOKU H8 on majors
for sym in ["btcusdt","ethusdt","solusdt","bnbusdt"]:
    COMBOS.append((sym, 28800, "ICHIMOKU"))
# KELTNER_REVERT H6/H12 on majors (mean-revert in chop)
for sym in ["btcusdt","ethusdt","solusdt","linkusdt","bnbusdt"]:
    for tf in [21600, 43200]:
        COMBOS.append((sym, tf, "KELTNER_REVERT"))

# Quality bar
MIN_PF = 2.0
MIN_SHARPE = 1.5
MIN_TRADES = 25

candidates = []
for sym, tf, strat in COMBOS:
    spec = f"{sym}:{tf}:{strat}"
    print(f"[SWEEP] {spec}", file=sys.stderr)
    r = subprocess.run([BIN, "--sweep", spec, "--preset", PRESET],
                       capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print(f"  ERR rc={r.returncode}", file=sys.stderr)
        continue
    # Parse CSV output (skip header)
    best = None
    for line in r.stdout.splitlines()[1:]:
        try:
            cols = line.split(",")
            lb, hold, sl, tarm, tdist = int(cols[0]), int(cols[1]), float(cols[2]), float(cols[3]), float(cols[4])
            trades, wins = int(cols[5]), int(cols[6])
            wr, bp, pf, sharpe, dd = float(cols[7]), float(cols[8]), float(cols[9]), float(cols[10]), float(cols[11])
        except (ValueError, IndexError):
            continue
        if trades < MIN_TRADES: continue
        if pf < MIN_PF or sharpe < MIN_SHARPE: continue
        # Deduplicate identical results (trail dist often noop) — keep best PF
        key = (lb, hold, sl, tarm)
        if best is None or pf > best[7]:
            best = (sym, tf, strat, lb, hold, sl, tarm, pf, sharpe, trades, wr, bp, dd)
    if best:
        candidates.append(best)
        print(f"  WINNER lb={best[3]} hold={best[4]} sl={best[5]} pf={best[7]:.2f} Sh={best[8]:.2f} n={best[9]} bp={best[11]:.0f}")

# Sort by bp desc
candidates.sort(key=lambda c: -c[11])
print("\n========== EDGE CANDIDATES (PF>=2.0 Sh>=1.5 n>=25) ==========")
print(f"{'SYM':10} {'TF':6} {'STRAT':12} {'lb':>3} {'hold':>5} {'sl':>5} {'tarm':>5} {'PF':>6} {'Sh':>6} {'N':>5} {'WR':>5} {'bp':>10}")
for c in candidates:
    sym, tf, strat, lb, hold, sl, tarm, pf, sh, n, wr, bp, dd = c
    print(f"{sym:10} {tf:6} {strat:12} {lb:>3} {hold:>5} {sl:>5.1f} {tarm:>5.1f} {pf:>6.2f} {sh:>6.2f} {n:>5} {wr:>4.1f}% {bp:>10.0f}")

# Save for next step
with open("/tmp/edge_candidates.csv","w",newline="") as f:
    w = csv.writer(f)
    w.writerow(["symbol","tf_secs","strategy","lookback","hold_bars","sl_atr","trail_arm",
                "pf","sharpe","trades","wr","total_bp","maxdd_bp"])
    for c in candidates: w.writerow(c)
print(f"\nSaved {len(candidates)} candidates to /tmp/edge_candidates.csv")
