#!/usr/bin/env python3
"""Download H1 klines from Binance public API for new symbols."""
import json, time, urllib.request, os, sys

DATA_DIR = "/sessions/epic-inspiring-meitner/mnt/ChimeraCrypto/backtest/data"
LIMIT = 1000
INTERVAL = "1h"
START_MS = 1640995200000  # 2022-01-01 00:00 UTC (match existing data)

def download_symbol(symbol):
    """Download all H1 klines for symbol from START_MS to now."""
    prefix = f"{symbol.lower()}_h1"
    part = 0
    start_time = START_MS
    total_bars = 0
    
    while True:
        url = (f"https://api.binance.com/api/v3/klines?"
               f"symbol={symbol.upper()}&interval={INTERVAL}"
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
        print(f"  Part {part}: {len(data)} bars (total: {total_bars})")
        
        if len(data) < LIMIT:
            break
        
        # Next start = last bar close time + 1ms
        start_time = data[-1][6] + 1
        part += 1
        time.sleep(0.2)  # Rate limit courtesy
    
    print(f"  DONE: {symbol} — {total_bars} H1 bars in {part+1} files")
    return total_bars

if __name__ == "__main__":
    symbols = sys.argv[1:] if len(sys.argv) > 1 else ["SUIUSDT", "APTUSDT", "NEARUSDT", "ARBUSDT"]
    
    for sym in symbols:
        print(f"\n[DOWNLOADING] {sym}...")
        download_symbol(sym)
