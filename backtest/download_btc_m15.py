#!/usr/bin/env python3
"""Download BTCUSDT M15 klines from Binance public API."""
import json, time, urllib.request, os, sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "data")
LIMIT = 1000
INTERVAL = "15m"
SYMBOL = "BTCUSDT"
START_MS = 1502928000000  # 2017-08-17 (Binance BTCUSDT inception)


def main():
    os.makedirs(DATA_DIR, exist_ok=True)
    prefix = f"{SYMBOL.lower()}_m15"
    part = 0
    start_time = START_MS
    total_bars = 0
    while True:
        url = (f"https://api.binance.com/api/v3/klines?"
               f"symbol={SYMBOL}&interval={INTERVAL}"
               f"&startTime={start_time}&limit={LIMIT}")
        try:
            req = urllib.request.Request(url)
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read().decode())
        except Exception as e:
            print(f"  [ERROR] Part {part}: {e}")
            time.sleep(2)
            continue
        if not data:
            break
        outpath = os.path.join(DATA_DIR, f"{prefix}_part{part}.json")
        with open(outpath, 'w') as f:
            json.dump(data, f)
        total_bars += len(data)
        if part % 20 == 0:
            print(f"  Part {part}: total {total_bars} bars (last_ts={data[-1][0]})")
        if len(data) < LIMIT:
            break
        start_time = data[-1][6] + 1
        part += 1
        time.sleep(0.15)
    print(f"DONE: {SYMBOL} M15 = {total_bars} bars in {part+1} files")


if __name__ == "__main__":
    main()
