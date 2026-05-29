#!/usr/bin/env python3
"""Fast parallel H1 download. Validates syms, hard-fails on Invalid, threads."""
import json, os, sys, time, urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "data")
LIMIT = 1000
INTERVAL = "1h"
START_MS = 1502928000000  # 2017-08

def fetch_chunk(symbol, start_ms):
    url = (f"https://api.binance.com/api/v3/klines?"
           f"symbol={symbol.upper()}&interval={INTERVAL}"
           f"&startTime={start_ms}&limit={LIMIT}")
    with urllib.request.urlopen(url, timeout=20) as r:
        return json.loads(r.read())

def get_valid_syms(syms):
    url = "https://api.binance.com/api/v3/exchangeInfo"
    with urllib.request.urlopen(url, timeout=30) as r:
        info = json.loads(r.read())
    status = {s["symbol"]: s["status"] for s in info["symbols"]}
    valid, invalid = [], []
    for s in syms:
        if status.get(s) == "TRADING":
            valid.append(s)
        else:
            invalid.append((s, status.get(s, "NOT LISTED")))
    return valid, invalid

def download_symbol(symbol):
    os.makedirs(DATA_DIR, exist_ok=True)
    prefix = f"{symbol.lower()}_h1"
    # Skip if part0 already exists and is fresh enough
    p0 = os.path.join(DATA_DIR, f"{prefix}_part0.json")
    if os.path.exists(p0):
        # Check last part for freshness
        parts = sorted([f for f in os.listdir(DATA_DIR) if f.startswith(prefix + "_part")])
        if parts:
            last = json.load(open(os.path.join(DATA_DIR, parts[-1])))
            if last and last[-1][0] > (time.time() - 86400) * 1000:
                return symbol, len(parts), "skip-fresh"
    part = 0
    start = START_MS
    total = 0
    while True:
        try:
            data = fetch_chunk(symbol, start)
        except Exception as e:
            time.sleep(1)
            try:
                data = fetch_chunk(symbol, start)
            except Exception as e2:
                return symbol, total, f"FAIL: {e2}"
        if not data:
            break
        with open(os.path.join(DATA_DIR, f"{prefix}_part{part}.json"), "w") as f:
            json.dump(data, f)
        total += len(data)
        if len(data) < LIMIT:
            break
        start = data[-1][6] + 1
        part += 1
    return symbol, total, "ok"

def main():
    syms = [s.upper() for s in sys.argv[1:]]
    print(f"validating {len(syms)} syms via exchangeInfo...", flush=True)
    valid, invalid = get_valid_syms(syms)
    print(f"  valid: {len(valid)}", flush=True)
    for s, st in invalid:
        print(f"  SKIP {s}: {st}", flush=True)
    print(f"downloading {len(valid)} syms in parallel (8 workers)...", flush=True)
    t0 = time.time()
    with ThreadPoolExecutor(max_workers=8) as pool:
        futs = {pool.submit(download_symbol, s): s for s in valid}
        for fut in as_completed(futs):
            sym, total, status = fut.result()
            print(f"  [{time.time()-t0:5.1f}s] {sym:12} {total:>6} bars  {status}", flush=True)
    print(f"DONE in {time.time()-t0:.1f}s", flush=True)

if __name__ == "__main__":
    main()
