"""
refresh_h1.py — Incremental H1 kline refresh.

Reads each *_1h_extended.csv, finds the last open_time_ms, then fetches
all Binance klines after that timestamp through "now-1h" and appends.
"""
import csv, json, time, sys, urllib.request
from pathlib import Path

SYMBOLS = ["BTCUSDT","ETHUSDT","BNBUSDT","SOLUSDT","XRPUSDT","DOGEUSDT",
           "LINKUSDT","AVAXUSDT","APTUSDT","ARBUSDT","NEARUSDT","SUIUSDT"]
OUT_DIR = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
INTERVAL = "1h"
LIMIT = 1000
HOUR_MS = 3_600_000

def fetch(url, attempts=4):
    for k in range(attempts):
        try:
            with urllib.request.urlopen(urllib.request.Request(url), timeout=30) as r:
                return json.loads(r.read().decode())
        except Exception as e:
            print(f"  err {e}, retry {k+1}", file=sys.stderr); time.sleep(1.5)
    return None

def last_open_ts(p):
    """Read last open_time_ms from CSV without loading whole file."""
    with p.open("rb") as f:
        f.seek(0, 2)
        size = f.tell()
        block = min(size, 4096)
        f.seek(-block, 2)
        tail = f.read(block).decode(errors="ignore").strip().splitlines()
    for line in reversed(tail):
        if not line or line.startswith("open_time_ms"): continue
        try:
            return int(line.split(",", 1)[0])
        except ValueError:
            continue
    return None

def refresh(sym):
    out = OUT_DIR / f"{sym}_1h_extended.csv"
    if not out.exists():
        print(f"  {sym}: no existing file, skip", file=sys.stderr); return 0
    last_ts = last_open_ts(out)
    if last_ts is None:
        print(f"  {sym}: cannot parse last ts, skip", file=sys.stderr); return 0
    start = last_ts + HOUR_MS  # fetch from next bar
    now_ms = int(time.time() * 1000)
    # don't fetch the currently-open bar
    end_cap = now_ms - (now_ms % HOUR_MS)  # most recent closed hour start
    if start >= end_cap:
        print(f"  {sym}: already up to date (last={last_ts})", file=sys.stderr); return 0
    new_rows = []
    cursor = start
    while cursor < end_cap:
        url = (f"https://api.binance.com/api/v3/klines?symbol={sym}"
               f"&interval={INTERVAL}&startTime={cursor}&limit={LIMIT}")
        data = fetch(url)
        if not data: break
        for r in data:
            # exclude the bar that is still open (close_time_ms > now)
            if r[6] >= now_ms: continue
            new_rows.append(r)
        if len(data) < LIMIT: break
        cursor = data[-1][6] + 1
        time.sleep(0.1)
    if not new_rows:
        print(f"  {sym}: 0 new bars", file=sys.stderr); return 0
    with out.open("a", newline="") as f:
        w = csv.writer(f)
        for r in new_rows: w.writerow(r)
    print(f"  {sym}: +{len(new_rows)} new bars (last_old={last_ts}, last_new={new_rows[-1][0]})", file=sys.stderr)
    return len(new_rows)

def main():
    total = 0
    for s in SYMBOLS:
        print(f"== {s}", file=sys.stderr)
        total += refresh(s)
    print(f"done. total new bars: {total}")

if __name__ == "__main__":
    main()
