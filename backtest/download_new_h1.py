"""Download H1 klines for TIA/ONDO/FET/PEPE/WIF — full history from listing."""
import csv, json, time, sys, urllib.request
from pathlib import Path

SYMBOLS = ["TIAUSDT","ONDOUSDT","FETUSDT","PEPEUSDT","WIFUSDT"]
OUT_DIR = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
INTERVAL = "1h"
LIMIT = 1000
START_MS = 1609459200000  # 2021-01-01 (Binance returns first available)

def fetch(url, attempts=4):
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
        print(f"  {sym}: already exists, skip", file=sys.stderr); return
    rows = []
    start = START_MS
    while True:
        url = f"https://api.binance.com/api/v3/klines?symbol={sym}&interval={INTERVAL}&startTime={start}&limit={LIMIT}"
        data = fetch(url)
        if data is None: print(f"  {sym}: fail"); return
        if not data: break
        rows.extend(data)
        if len(data) < LIMIT: break
        start = data[-1][6] + 1
        time.sleep(0.1)
    if not rows: print(f"  {sym}: empty"); return
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["open_time_ms","open","high","low","close","volume","close_time_ms","quote_volume","trades","taker_buy_base","taker_buy_quote","ignore"])
        for r in rows: w.writerow(r)
    print(f"  {sym} -> {len(rows)} bars (first ts={rows[0][0]}, last={rows[-1][0]})")

for s in SYMBOLS:
    print(f"== {s}")
    download(s)
print("done")
