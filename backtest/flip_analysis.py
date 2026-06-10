#!/usr/bin/env python3
"""S56: bull-flip readiness analysis.
1. BTC 200dMA hysteresis flip events (MacroBase enter MA*1.03 / exit MA*0.97):
   dates, whipsaws, lag from local bottom, capture of following bull leg.
2. Where the flip sits TODAY (price needed to arm).
3. Breadth-confirmed variant (>=50% of long-history alts above own 200dMA):
   does it cut whipsaws without giving up capture?
Data: backtest/data/{sym}_h1_part*.json (2017->now for BTC).
"""
import json, glob, os, re, math
from datetime import datetime, timezone

D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

def load_h1(sym):
    files = sorted(glob.glob(f"{D}/{sym}_h1_part*.json"),
                   key=lambda f: int(re.search(r"part(\d+)", f).group(1)))
    bars = []
    for f in files:
        try: bars.extend(json.load(open(f)))
        except Exception: pass
    bars.sort(key=lambda b: b[0])
    out, seen = [], set()
    for b in bars:
        if b[0] not in seen:
            seen.add(b[0]); out.append((b[0], float(b[4])))  # ts, close
    return out

def to_daily(h1):
    days = {}
    for ts, c in h1:
        days[ts // 86400000] = c  # last close of day wins
    ks = sorted(days)
    return ks, [days[k] for k in ks]

def dstr(day): return datetime.fromtimestamp(day*86400, tz=timezone.utc).strftime("%Y-%m-%d")

btc_days, btc_close = to_daily(load_h1("btc"))
n = len(btc_close)
ma = [None]*n
s = 0.0
for i, c in enumerate(btc_close):
    s += c
    if i >= 200: s -= btc_close[i-200]
    if i >= 199: ma[i] = s/200

# --- 1. hysteresis flip events (MacroBase logic) ---
def flips(enter_mult=1.03, exit_mult=0.97, extra_ok=None):
    inv = False; ev = []
    for i in range(200, n):
        if not inv and btc_close[i] > ma[i]*enter_mult and (extra_ok is None or extra_ok[i]):
            inv = True; ev.append(("IN", i))
        elif inv and btc_close[i] < ma[i]*exit_mult:
            inv = False; ev.append(("OUT", i))
    return ev

def stats(ev, label):
    print(f"\n=== {label} ===")
    legs = []
    for j in range(len(ev)):
        if ev[j][0] == "IN":
            i_in = ev[j][1]
            i_out = ev[j+1][1] if j+1 < len(ev) else n-1
            ret = btc_close[i_out]/btc_close[i_in] - 1
            days_held = btc_days[i_out] - btc_days[i_in]
            # lag: local bottom in the 250d before entry
            lo = min(range(max(200, i_in-250), i_in+1), key=lambda k: btc_close[k])
            lag = btc_days[i_in] - btc_days[lo]
            rally_missed = btc_close[i_in]/btc_close[lo] - 1
            legs.append((i_in, i_out, ret, days_held, lag, rally_missed))
            print(f"  IN {dstr(btc_days[i_in])} @{btc_close[i_in]:8.0f}  OUT {dstr(btc_days[i_out])} "
                  f"@{btc_close[i_out]:8.0f}  ret {ret*100:+6.1f}%  held {days_held:4d}d  "
                  f"lag-from-bottom {lag:3d}d (missed {rally_missed*100:+.0f}%)")
    wins = [l for l in legs if l[2] > 0]
    whip = [l for l in legs if l[2] <= 0 and l[3] < 60]
    tot = 1.0
    for l in legs: tot *= (1+l[2])
    print(f"  legs={len(legs)} wins={len(wins)} whipsaws(<60d,neg)={len(whip)} "
          f"compound={tot:.2f}x  avg lag={sum(l[4] for l in legs)/max(1,len(legs)):.0f}d")
    return legs

ev = flips()
stats(ev, "BTC 200dMA hysteresis 3% (MacroBase logic, 2017->now)")

# --- 2. today ---
i = n-1
print(f"\nTODAY {dstr(btc_days[i])}: BTC {btc_close[i]:.0f}, 200dMA {ma[i]:.0f}, "
      f"arm at {ma[i]*1.03:.0f} = {(ma[i]*1.03/btc_close[i]-1)*100:+.1f}% rally needed (MA falling daily)")

# --- 3. breadth variant ---
alts = ["ethusdt","xrpusdt","ltcusdt","bnbusdt","adausdt","linkusdt","dogeusdt","bchusdt","xlmusdt","trxusdt"]
breadth_ok = [None]*n
series = {}
for a in alts:
    d, c = to_daily(load_h1(a))
    m = {}
    s2 = 0.0
    for j, cc in enumerate(c):
        s2 += cc
        if j >= 200: s2 -= c[j-200]
        if j >= 199: m[d[j]] = (cc, s2/200)
    series[a] = m
for i in range(200, n):
    day = btc_days[i]
    above = avail = 0
    for a in alts:
        if day in series[a]:
            avail += 1
            cc, mm = series[a][day]
            if cc > mm: above += 1
    breadth_ok[i] = (avail >= 5 and above/avail >= 0.5)
ev2 = flips(extra_ok=breadth_ok)
stats(ev2, "BTC 200dMA hyst 3% + breadth>=50% alts above own 200dMA")
