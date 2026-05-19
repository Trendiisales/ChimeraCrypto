"""Download H1 klines for all 12 roster symbols, 5yr each, from Binance."""
import json, time, urllib.request, sys, csv
from pathlib import Path

SYMBOLS = ["BTCUSDT","ETHUSDT","BNBUSDT","SOLUSDT","XRPUSDT","DOGEUSDT",
           "LINKUSDT","AVAXUSDT","APTUSDT","ARBUSDT","NEARUSDT","SUIUSDT"]
OUT_DIR = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
INTERVAL = "1h"
LIMIT = 1000
START_MS = 1609459200000  # 2021-01-01

def fetch_url(url, attempts=4):
    for k in range(attempts):
        try:
            with urllib.request.urlopen(urllib.request.Request(url), timeout=30) as r:
                return json.loads(r.read().decode())
        except Exception as e:
            print(f"  err {e}, retry {k+1}", file=sys.stderr); time.sleep(1.5)
    return None

def download(sym):
    out = OUT_DIR / f"{sym}_1h_extended.csv"
    if out.exists() and out.stat().st_size > 100_000:
        print(f"  {sym} already have {out}", file=sys.stderr); return
    rows = []
    start = START_MS
    while True:
        url = f"https://api.binance.com/api/v3/klines?symbol={sym}&interval={INTERVAL}&startTime={start}&limit={LIMIT}"
        data = fetch_url(url)
        if data is None:
            print(f"  {sym} fail", file=sys.stderr); return
        if not data: break
        rows.extend(data)
        if len(data) < LIMIT: break
        start = data[-1][6] + 1
        time.sleep(0.1)
    if not rows:
        print(f"  {sym} EMPTY (not listed?)", file=sys.stderr); return
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["open_time_ms","open","high","low","close","volume","close_time_ms","quote_volume","trades","taker_buy_base","taker_buy_quote","ignore"])
        for r in rows: w.writerow(r)
    print(f"  {sym} -> {len(rows)} bars", file=sys.stderr)

def main():
    for s in SYMBOLS:
        print(f"== {s}", file=sys.stderr)
        download(s)
    print("done")

if __name__ == "__main__":
    main()
