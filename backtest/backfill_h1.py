#!/usr/bin/env python3
"""Backfill H1 klines before existing data start date."""
import json, os, sys, time, glob
from urllib.request import urlopen, Request
from datetime import datetime

SYMBOLS = {
    "BTC":  ("BTCUSDT",  "btc_h1_part"),
    "ETH":  ("ETHUSDT",  "ethusdt_h1_part"),
    "SOL":  ("SOLUSDT",  "solusdt_h1_part"),
    "LINK": ("LINKUSDT", "linkusdt_h1_part"),
    "XRP":  ("XRPUSDT",  "xrpusdt_h1_part"),
    "NEAR": ("NEARUSDT", "nearusdt_h1_part"),
}

# How far back each symbol can go on Binance
EARLIEST_MS = {
    "BTC":  1502928000000,  # 2017-08-17
    "ETH":  1502928000000,  # 2017-08-17
    "SOL":  1597276800000,  # 2020-08-13
    "LINK": 1547510400000,  # 2019-01-15
    "XRP":  1525824000000,  # 2018-05-09
    "NEAR": 1602547200000,  # 2020-10-13
}

BARS_PER_FILE = 1000

def fetch_klines(symbol, start_ms, end_ms):
    url = f"https://api.binance.com/api/v3/klines?symbol={symbol}&interval=1h&startTime={start_ms}&endTime={end_ms}&limit=1000"
    req = Request(url, headers={"User-Agent": "ChimeraBackfill/1.0"})
    try:
        resp = urlopen(req, timeout=30)
        return json.loads(resp.read())
    except Exception as e:
        print(f"  [WARN] {e}, retry...")
        time.sleep(5)
        resp = urlopen(req, timeout=30)
        return json.loads(resp.read())

def backfill(sym_key, data_dir):
    binance_sym, prefix = SYMBOLS[sym_key]
    earliest_target = EARLIEST_MS[sym_key]
    
    # Find current earliest timestamp
    files = glob.glob(os.path.join(data_dir, f"{prefix}*.json"))
    if not files:
        print(f"[{sym_key}] No files found")
        return
    
    # Find the actual earliest bar across all files
    min_ts = float('inf')
    max_part = -1
    for f in files:
        part_num = int(os.path.basename(f).replace(prefix, "").replace(".json", ""))
        max_part = max(max_part, part_num)
        with open(f) as fh:
            d = json.load(fh)
            if d and d[0][0] < min_ts:
                min_ts = d[0][0]
    
    current_start = datetime.fromtimestamp(min_ts/1000).strftime("%Y-%m-%d")
    target_start = datetime.fromtimestamp(earliest_target/1000).strftime("%Y-%m-%d")
    
    if min_ts <= earliest_target + 3600000:
        print(f"[{sym_key}] Already at listing date ({target_start}), skip")
        return
    
    gap_hours = (min_ts - earliest_target) / 3600000
    print(f"[{sym_key}] Current start: {current_start}, listing: {target_start}, gap: ~{int(gap_hours)} bars to fetch")
    
    # Fetch the gap
    all_bars = []
    cursor = earliest_target
    end_ms = min_ts - 1
    
    while cursor < end_ms:
        batch = fetch_klines(binance_sym, cursor, end_ms)
        if not batch:
            break
        all_bars.extend(batch)
        cursor = batch[-1][0] + 3600000
        time.sleep(0.12)
        if len(batch) < 1000:
            break
    
    if not all_bars:
        print(f"  No backfill data")
        return
    
    # Dedupe
    seen = set()
    deduped = []
    for b in all_bars:
        if b[0] not in seen and b[0] < min_ts:
            seen.add(b[0])
            deduped.append(b)
    all_bars = sorted(deduped, key=lambda x: x[0])
    
    # Write as new part files starting from max_part+1
    # (optimizer sorts by timestamp, so part order doesn't matter)
    part_num = max_part + 1
    for i in range(0, len(all_bars), BARS_PER_FILE):
        chunk = all_bars[i:i+BARS_PER_FILE]
        fname = os.path.join(data_dir, f"{prefix}{part_num}.json")
        with open(fname, "w") as fh:
            json.dump(chunk, fh)
        part_num += 1
    
    actual_start = datetime.fromtimestamp(all_bars[0][0]/1000).strftime("%Y-%m-%d")
    print(f"  +{len(all_bars)} bars from {actual_start} ({part_num - max_part - 1} new parts)")

data_dir = "./data"
syms = sys.argv[1:] if len(sys.argv) > 1 else list(SYMBOLS.keys())
for s in syms:
    if s in SYMBOLS:
        backfill(s, data_dir)
