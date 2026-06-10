#!/usr/bin/env python3
"""Extend backtest/data/{sym}_h1_part*.json to now via Binance REST.

Usage: python3 refresh_parts.py            # all symbols found in data/
       python3 refresh_parts.py solusdt    # specific prefixes
"""
import json, os, sys, time, glob, re
from urllib.request import urlopen, Request
from datetime import datetime, timezone

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "data")
HOUR_MS = 3600000
BARS_PER_FILE = 1000

def fetch(symbol, start_ms):
    url = (f"https://api.binance.com/api/v3/klines?symbol={symbol}&interval=1h"
           f"&startTime={start_ms}&limit=1000")
    req = Request(url, headers={"User-Agent": "ChimeraRefresh/1.0"})
    for attempt in range(4):
        try:
            with urlopen(req, timeout=30) as r:
                return json.loads(r.read())
        except Exception as e:
            print(f"  [WARN] {symbol} {e}, retry {attempt+1}")
            time.sleep(3 * (attempt + 1))
    raise RuntimeError(f"{symbol}: fetch failed")

def refresh(prefix):
    files = glob.glob(os.path.join(DATA_DIR, f"{prefix}_h1_part*.json"))
    if not files:
        print(f"[{prefix}] no part files, skip")
        return 0
    max_ts, max_part = 0, -1
    for f in files:
        m = re.search(r"_part(\d+)\.json$", f)
        if m:
            max_part = max(max_part, int(m.group(1)))
        try:
            d = json.load(open(f))
            if d and d[-1][0] > max_ts:
                max_ts = d[-1][0]
        except Exception:
            pass
    now_ms = int(time.time() * 1000)
    cutoff = (now_ms // HOUR_MS) * HOUR_MS  # exclude still-open hour
    if max_ts >= cutoff - HOUR_MS:
        print(f"[{prefix}] fresh")
        return 0
    binance_sym = prefix.upper()
    if not binance_sym.endswith("USDT"):
        binance_sym += "USDT"  # short prefixes like "btc"
    rows, cursor = [], max_ts + HOUR_MS
    while cursor < cutoff:
        batch = fetch(binance_sym, cursor)
        if not batch:
            break
        rows.extend(batch)
        cursor = batch[-1][0] + HOUR_MS
        time.sleep(0.15)
        if len(batch) < 1000:
            break
    rows = [b for b in rows if max_ts < b[0] < cutoff]
    if not rows:
        print(f"[{prefix}] no new bars")
        return 0
    part = max_part + 1
    for i in range(0, len(rows), BARS_PER_FILE):
        json.dump(rows[i:i+BARS_PER_FILE],
                  open(os.path.join(DATA_DIR, f"{prefix}_h1_part{part}.json"), "w"))
        part += 1
    last = datetime.fromtimestamp(rows[-1][0]/1000, tz=timezone.utc)
    print(f"[{prefix}] +{len(rows)} bars -> {last:%Y-%m-%d %H:%M} UTC ({part-max_part-1} parts)")
    return len(rows)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        prefixes = [s.lower() for s in sys.argv[1:]]
    else:
        prefixes = sorted({re.sub(r"_h1_part\d+\.json$", "", os.path.basename(p))
                           for p in glob.glob(os.path.join(DATA_DIR, "*_h1_part*.json"))})
    total = 0
    for p in prefixes:
        try:
            total += refresh(p)
        except Exception as e:
            print(f"[{p}] FAILED: {e}")
    print(f"DONE total {total} bars across {len(prefixes)} symbols")
