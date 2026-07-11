#!/usr/bin/env python3
# fetch_klines.py — download Binance spot klines for the 200DMA-veto missed-trades
# replay (S-2026-07-12 audit). 1h from 2026-03-01 (engine warmup) and 5m from
# 2026-05-30 (10-min short-rally regime proxy) for every symbol in the active
# non-UPJUMP slot roster + BTC (global regime).
# Output: data/<SYMBOL>_1h.csv and data/<SYMBOL>_5m.csv  (ts_ms,o,h,l,c)
import json, time, urllib.request, os, sys

SYMS = ["BTCUSDT","BNBUSDT","DOGEUSDT","LINKUSDT","SOLUSDT","NEARUSDT","SUIUSDT",
        "AVAXUSDT","XRPUSDT","FETUSDT","TIAUSDT","INJUSDT","SEIUSDT","APTUSDT"]

H1_START = 1772323200000   # 2026-03-01 00:00 UTC
M5_START = 1780099200000   # 2026-05-30 00:00 UTC
END      = 1783900800000   # 2026-07-12 00:00 UTC

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

def fetch(sym, interval, start, end):
    rows = []
    t = start
    while t < end:
        url = (f"https://api.binance.com/api/v3/klines?symbol={sym}"
               f"&interval={interval}&startTime={t}&endTime={end}&limit=1000")
        with urllib.request.urlopen(url, timeout=30) as r:
            data = json.loads(r.read())
        if not data:
            break
        for k in data:
            rows.append((k[0], k[1], k[2], k[3], k[4]))
        t = data[-1][0] + 1
        time.sleep(0.15)
    return rows

def main():
    os.makedirs(OUT, exist_ok=True)
    for sym in SYMS:
        for interval, start in (("1h", H1_START), ("5m", M5_START)):
            path = os.path.join(OUT, f"{sym}_{interval}.csv")
            if os.path.exists(path) and os.path.getsize(path) > 1000:
                print(f"skip {path} (exists)"); continue
            rows = fetch(sym, interval, start, END)
            with open(path, "w") as f:
                f.write("ts_ms,o,h,l,c\n")
                for r in rows:
                    f.write(f"{r[0]},{r[1]},{r[2]},{r[3]},{r[4]}\n")
            print(f"{path}: {len(rows)} bars")

if __name__ == "__main__":
    main()
