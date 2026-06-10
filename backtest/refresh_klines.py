#!/usr/bin/env python3
"""Refresh/extend data/klines_spot/{SYM}_1h_extended.csv to now via Binance REST.

Usage:
    python3 refresh_klines.py                 # update every existing *_1h_extended.csv
    python3 refresh_klines.py HBARUSDT ...    # update-or-create specific symbols
"""
import json, os, sys, time, glob
from urllib.request import urlopen, Request

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "..", "data", "klines_spot")
HEADER = "open_time_ms,open,high,low,close,volume,close_time_ms,quote_volume,trades,taker_buy_base,taker_buy_quote,ignore"
DEFAULT_START_MS = 1609459200000  # 2021-01-01, matches BTC_1h_extended start
HOUR_MS = 3600000

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
    raise RuntimeError(f"{symbol}: fetch failed after retries")

def refresh(symbol):
    path = os.path.join(DATA_DIR, f"{symbol}_1h_extended.csv")
    exists = os.path.exists(path)
    last_ts = None
    if exists:
        size = os.path.getsize(path)
        with open(path, "rb") as f:
            f.seek(max(0, size - 4096))
            tail = f.read().decode(errors="ignore").strip().splitlines()
        for line in reversed(tail):
            tok = line.split(",")[0]
            if tok.isdigit():
                last_ts = int(tok)
                break
    start = (last_ts + HOUR_MS) if last_ts else DEFAULT_START_MS
    now_ms = int(time.time() * 1000)
    if start >= now_ms - HOUR_MS:
        print(f"[{symbol}] fresh, skip")
        return 0
    rows = []
    cursor = start
    while cursor < now_ms - HOUR_MS:
        batch = fetch(symbol, cursor)
        if not batch:
            break
        rows.extend(batch)
        cursor = batch[-1][0] + HOUR_MS
        time.sleep(0.15)
        if len(batch) < 1000:
            break
    # drop the still-open current hour bar
    cutoff = (now_ms // HOUR_MS) * HOUR_MS
    rows = [b for b in rows if b[0] < cutoff and (last_ts is None or b[0] > last_ts)]
    if not rows:
        print(f"[{symbol}] no new bars")
        return 0
    mode = "a" if exists else "w"
    with open(path, mode) as f:
        if not exists:
            f.write(HEADER + "\n")
        for b in rows:
            f.write(",".join(str(x) for x in b[:12]) + "\n")
    print(f"[{symbol}] +{len(rows)} bars -> {time.strftime('%Y-%m-%d %H:%M', time.gmtime(rows[-1][0]/1000))} UTC")
    return len(rows)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        symbols = [s.upper() for s in sys.argv[1:]]
    else:
        symbols = sorted(os.path.basename(p).replace("_1h_extended.csv", "")
                         for p in glob.glob(os.path.join(DATA_DIR, "*_1h_extended.csv")))
    total = 0
    for s in symbols:
        try:
            total += refresh(s)
        except Exception as e:
            print(f"[{s}] FAILED: {e}")
    print(f"DONE total {total} bars")
