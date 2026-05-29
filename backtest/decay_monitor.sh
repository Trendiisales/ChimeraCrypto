#!/bin/bash
# decay_monitor.sh — weekly rolling 60d OOS panel on every live engine.
# Writes a per-week snapshot CSV under data/decay/. Diffs vs prior snapshot
# and flags engines whose rolling-60d PF dropped below 1.1 OR whose PF
# fell by >40% week-over-week.
#
# Run via cron weekly (Sundays 03:00 UTC):
#   0 3 * * 0  /home/jo/ChimeraCrypto/backtest/decay_monitor.sh

set -e
cd "$(dirname "$0")/.."
ts=$(date -u +%Y%m%d_%H%M%S)
out="data/decay/decay_${ts}.csv"
mkdir -p data/decay

# Re-extract live roster from current main.cpp + s43*/s43b cpp files
python3 /tmp/build_full_roster.py >/dev/null 2>&1 || {
    # Fallback: rebuild roster from compiled binary's strings
    strings build/chimera | grep -E '^[A-Z][A-Z0-9]+-[A-Z]+-[A-Z0-9]+$' | sort -u > /tmp/_live_tags.txt
    echo "WARN: roster regeneration fell back to binary strings — config params unknown" >&2
}

if [ ! -f /tmp/full_live_roster.csv ]; then
    echo "ERROR: no live roster CSV available — abort" >&2
    exit 1
fi

# 60d window
./backtest/backtest --roster /tmp/full_live_roster.csv \
    --preset prod_tiered_pyramid_elite \
    --last-days 60 2>/dev/null \
    | grep -E "^[A-Z][A-Z0-9_-]+,[a-z]+,[A-Z_]+," > "$out"

n=$(wc -l < "$out")
echo "[decay] wrote $out ($n engines)"

# Find prior snapshot
prior=$(ls -t data/decay/decay_*.csv 2>/dev/null | sed -n 2p)
if [ -z "$prior" ]; then
    echo "[decay] no prior snapshot — first run, no diff"
    exit 0
fi
echo "[decay] comparing $out  vs  $prior"

# S44f: track per-engine decay-week streak in data/decay/streak.json.
# After 3 consecutive weeks of DECAY, auto-cull the engine via cull list.
STREAK_FILE=data/decay/streak.json
[ -f "$STREAK_FILE" ] || echo "{}" > "$STREAK_FILE"

python3 - <<PY
import csv, pathlib, sys
def load(p):
    out = {}
    for line in pathlib.Path(p).read_text().splitlines():
        c = line.split(",")
        if len(c) < 12: continue
        try:
            out[c[0]] = dict(pf=float(c[8]), trades=int(c[4]),
                             bp=float(c[7]), dd=float(c[10]))
        except ValueError:
            pass
    return out
cur = load("$out")
prev = load("$prior")
alerts = []
for tag in sorted(set(cur) & set(prev)):
    c = cur[tag]; p = prev[tag]
    # Decay alert if PF < 1.1 AND >=20 trades (enough signal)
    if c["pf"] < 1.1 and c["trades"] >= 20:
        alerts.append(f"DECAY  {tag:24s} PF={c['pf']:5.2f} trades={c['trades']} bp={c['bp']:+.0f}")
    # Drop alert if PF dropped >40% week-over-week
    elif p["pf"] > 0 and c["pf"] / p["pf"] < 0.6 and c["pf"] < 1.5:
        alerts.append(f"DROP   {tag:24s} PF {p['pf']:.2f} -> {c['pf']:.2f} (-{100*(1-c['pf']/p['pf']):.0f}%)")
if not alerts:
    print("[decay] no alerts — all engines steady")
else:
    print(f"[decay] {len(alerts)} ALERT(S):")
    for a in alerts: print(f"  {a}")

# S44f: streak tracker + auto-cull after 3 consecutive DECAY weeks
import json
streak_path = "$STREAK_FILE"
try:
    streak = json.loads(open(streak_path).read())
except Exception:
    streak = {}
decay_tags = set()
for a in alerts:
    if a.startswith("DECAY"):
        # "DECAY  TAG-NAME pf=..." -> extract tag (column 1)
        parts = a.split()
        if len(parts) >= 2: decay_tags.add(parts[1])
# Increment streaks for engines in decay this week
# Reset streaks for engines NOT in decay
new_streak = {}
to_cull = []
for tag in set(list(streak.keys()) + list(decay_tags)):
    if tag in decay_tags:
        new_streak[tag] = streak.get(tag, 0) + 1
        if new_streak[tag] >= 3:
            to_cull.append((tag, new_streak[tag]))
    # else: omit (resets to 0)
open(streak_path, "w").write(json.dumps(new_streak, indent=2))
if to_cull:
    cull_path = "data/decay/autocull.txt"
    with open(cull_path, "a") as f:
        for tag, weeks in to_cull:
            f.write(f"{tag}\n")
            print(f"[decay] AUTOCULL {tag} (decay {weeks} weeks straight) -> appended to {cull_path}")
    print(f"[decay] Run: python3 /tmp/cull_v3.py {cull_path} && rebuild + restart")
PY
