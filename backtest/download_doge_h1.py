"""Download DOGEUSDT H1 from Binance 2021-01-01 -> now. Save CSV."""
import json, time, urllib.request, sys, csv
from pathlib import Path

OUT = Path("/Users/jo/ChimeraCrypto/data/klines_spot/DOGEUSDT_1h_extended.csv")
SYMBOL = "DOGEUSDT"
INTERVAL = "1h"
LIMIT = 1000
START_MS = 1609459200000  # 2021-01-01 UTC

def fetch(start_ms):
    url = (f"https://api.binance.com/api/v3/klines?"
           f"symbol={SYMBOL}&interval={INTERVAL}&startTime={start_ms}&limit={LIMIT}")
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.loads(r.read().decode())

def main():
    rows = []
    start = START_MS
    while True:
        try:
            data = fetch(start)
        except Exception as e:
            print(f"err {e}, retry in 2s", file=sys.stderr); time.sleep(2); continue
        if not data:
            break
        rows.extend(data)
        print(f"got {len(data)} bars, total {len(rows)}, last={data[-1][0]}", file=sys.stderr)
        if len(data) < LIMIT:
            break
        start = data[-1][6] + 1
        time.sleep(0.15)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["open_time_ms","open","high","low","close","volume","close_time_ms","quote_volume","trades","taker_buy_base","taker_buy_quote","ignore"])
        for r in rows:
            w.writerow(r)
    print(f"wrote {len(rows)} rows -> {OUT}")

if __name__ == "__main__":
    main()
