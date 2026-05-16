#!/usr/bin/env python3
"""
extend_h1_data.py — Download maximum H1 kline history from Binance
for all 12 Chimera symbols, extending existing data to present.

Usage: python3 extend_h1_data.py [--data-dir ./data]
"""

import json, os, sys, time, glob, argparse
from urllib.request import urlopen, Request
from datetime import datetime

SYMBOLS = {
    # sym_key: (binance_symbol, file_prefix)
    "BTC":  ("BTCUSDT",  "btc_h1_part"),
    "ETH":  ("ETHUSDT",  "ethusdt_h1_part"),
    "SOL":  ("SOLUSDT",  "solusdt_h1_part"),
    "BNB":  ("BNBUSDT",  "bnbusdt_h1_part"),
    "AVAX": ("AVAXUSDT", "avaxusdt_h1_part"),
    "LINK": ("LINKUSDT", "linkusdt_h1_part"),
    "XRP":  ("XRPUSDT",  "xrpusdt_h1_part"),
    "DOGE": ("DOGEUSDT", "dogeusdt_h1_part"),
    "SUI":  ("SUIUSDT",  "suiusdt_h1_part"),
    "APT":  ("APTUSDT",  "aptusdt_h1_part"),
    "NEAR": ("NEARUSDT", "nearusdt_h1_part"),
    "ARB":  ("ARBUSDT",  "arbusdt_h1_part"),
}

# Binance listing dates (approximate, H1 data available from)
LISTING_DATES_MS = {
    "BTC":  1502928000000,  # 2017-08-17
    "ETH":  1502928000000,  # 2017-08-17
    "SOL":  1597276800000,  # 2020-08-13
    "BNB":  1509408000000,  # 2017-10-31
    "AVAX": 1600905600000,  # 2020-09-24
    "LINK": 1547510400000,  # 2019-01-15
    "XRP":  1525824000000,  # 2018-05-09
    "DOGE": 1562284800000,  # 2019-07-05
    "SUI":  1683072000000,  # 2023-05-03
    "APT":  1666137600000,  # 2022-10-19
    "NEAR": 1602547200000,  # 2020-10-13
    "ARB":  1679529600000,  # 2023-03-23
}

BARS_PER_FILE = 1000
API_BASE = "https://api.binance.com/api/v3/klines"
RATE_LIMIT_SLEEP = 0.12  # seconds between API calls


def fetch_klines(symbol, start_ms, end_ms=None, limit=1000):
    """Fetch up to 1000 H1 klines from Binance."""
    url = f"{API_BASE}?symbol={symbol}&interval=1h&startTime={start_ms}&limit={limit}"
    if end_ms:
        url += f"&endTime={end_ms}"
    req = Request(url, headers={"User-Agent": "ChimeraDataExtender/1.0"})
    try:
        resp = urlopen(req, timeout=30)
        return json.loads(resp.read())
    except Exception as e:
        print(f"  [WARN] API error: {e}, retrying in 5s...")
        time.sleep(5)
        try:
            resp = urlopen(req, timeout=30)
            return json.loads(resp.read())
        except Exception as e2:
            print(f"  [ERROR] Retry failed: {e2}")
            return []


def get_existing_coverage(data_dir, prefix):
    """Find the last timestamp in existing data files."""
    files = sorted(glob.glob(os.path.join(data_dir, f"{prefix}*.json")))
    if not files:
        return None, 0, 0
    
    total_bars = 0
    last_ts = None
    max_part = -1
    
    for f in files:
        part_num = int(os.path.basename(f).replace(prefix, "").replace(".json", ""))
        max_part = max(max_part, part_num)
        with open(f) as fh:
            data = json.load(fh)
            total_bars += len(data)
            if data:
                last_ts = data[-1][0]
    
    return last_ts, total_bars, max_part


def download_symbol(sym_key, data_dir, backfill=True):
    """Download H1 data for a symbol, extending from where existing data ends."""
    binance_sym, prefix = SYMBOLS[sym_key]
    
    last_ts, existing_bars, max_part = get_existing_coverage(data_dir, prefix)
    
    if last_ts:
        last_date = datetime.fromtimestamp(last_ts / 1000).strftime("%Y-%m-%d %H:%M")
        print(f"\n{'='*60}")
        print(f"[{sym_key}] Existing: {existing_bars} bars ending {last_date}, {max_part+1} parts")
        
        # Forward extension: from last timestamp + 1h to now
        start_ms = last_ts + 3600000  # +1 hour
        now_ms = int(time.time() * 1000)
        
        if start_ms >= now_ms:
            print(f"  Already up to date!")
            # Still try backfill
            if backfill:
                return backfill_symbol(sym_key, data_dir, prefix, existing_bars, max_part)
            return existing_bars, 0
        
        print(f"  Forward extending from {datetime.fromtimestamp(start_ms/1000).strftime('%Y-%m-%d')} to now...")
        
        new_bars = []
        cursor = start_ms
        while cursor < now_ms:
            batch = fetch_klines(binance_sym, cursor, now_ms)
            if not batch:
                break
            new_bars.extend(batch)
            cursor = batch[-1][0] + 3600000
            time.sleep(RATE_LIMIT_SLEEP)
            if len(batch) < 1000:
                break
        
        if new_bars:
            # Remove any duplicates
            seen = set()
            deduped = []
            for b in new_bars:
                if b[0] not in seen:
                    seen.add(b[0])
                    deduped.append(b)
            new_bars = sorted(deduped, key=lambda x: x[0])
            
            # Save in part files
            part_num = max_part + 1
            for i in range(0, len(new_bars), BARS_PER_FILE):
                chunk = new_bars[i:i+BARS_PER_FILE]
                fname = os.path.join(data_dir, f"{prefix}{part_num}.json")
                with open(fname, "w") as fh:
                    json.dump(chunk, fh)
                part_num += 1
            
            end_date = datetime.fromtimestamp(new_bars[-1][0]/1000).strftime("%Y-%m-%d")
            print(f"  Forward: +{len(new_bars)} bars to {end_date} ({part_num - max_part - 1} new parts)")
        else:
            print(f"  No new forward data available")
        
        forward_count = len(new_bars) if new_bars else 0
        
        # Backfill: get data BEFORE existing start
        if backfill:
            back_count = backfill_symbol(sym_key, data_dir, prefix, existing_bars + forward_count, max_part + (forward_count // BARS_PER_FILE + (1 if forward_count % BARS_PER_FILE else 0)))
            return existing_bars + forward_count, back_count
        
        return existing_bars + forward_count, 0
    
    else:
        # No existing data — full download from listing date
        print(f"\n{'='*60}")
        print(f"[{sym_key}] No existing data, full download from listing")
        listing_ms = LISTING_DATES_MS.get(sym_key, 1640995200000)
        now_ms = int(time.time() * 1000)
        
        all_bars = []
        cursor = listing_ms
        while cursor < now_ms:
            batch = fetch_klines(binance_sym, cursor, now_ms)
            if not batch:
                break
            all_bars.extend(batch)
            cursor = batch[-1][0] + 3600000
            time.sleep(RATE_LIMIT_SLEEP)
            if len(batch) < 1000:
                break
        
        if all_bars:
            seen = set()
            deduped = []
            for b in all_bars:
                if b[0] not in seen:
                    seen.add(b[0])
                    deduped.append(b)
            all_bars = sorted(deduped, key=lambda x: x[0])
            
            part_num = 0
            for i in range(0, len(all_bars), BARS_PER_FILE):
                chunk = all_bars[i:i+BARS_PER_FILE]
                fname = os.path.join(data_dir, f"{prefix}{part_num}.json")
                with open(fname, "w") as fh:
                    json.dump(chunk, fh)
                part_num += 1
            
            start_date = datetime.fromtimestamp(all_bars[0][0]/1000).strftime("%Y-%m-%d")
            end_date = datetime.fromtimestamp(all_bars[-1][0]/1000).strftime("%Y-%m-%d")
            print(f"  Downloaded {len(all_bars)} bars: {start_date} → {end_date} ({part_num} parts)")
        
        return len(all_bars) if all_bars else 0, 0


def backfill_symbol(sym_key, data_dir, prefix, current_total, current_max_part):
    """Try to get data BEFORE the existing start date."""
    binance_sym = SYMBOLS[sym_key][0]
    listing_ms = LISTING_DATES_MS.get(sym_key, 1640995200000)
    
    # Find earliest existing timestamp
    files = sorted(glob.glob(os.path.join(data_dir, f"{prefix}*.json")))
    if not files:
        return 0
    
    with open(files[0]) as fh:
        first_data = json.load(fh)
        if not first_data:
            return 0
        earliest_ts = first_data[0][0]
    
    earliest_date = datetime.fromtimestamp(earliest_ts/1000).strftime("%Y-%m-%d")
    listing_date = datetime.fromtimestamp(listing_ms/1000).strftime("%Y-%m-%d")
    
    if earliest_ts <= listing_ms + 3600000:
        print(f"  Backfill: already at listing date ({listing_date})")
        return 0
    
    print(f"  Backfill: fetching {listing_date} → {earliest_date}...")
    
    all_bars = []
    cursor = listing_ms
    end_ms = earliest_ts - 1
    
    while cursor < end_ms:
        batch = fetch_klines(binance_sym, cursor, end_ms)
        if not batch:
            break
        all_bars.extend(batch)
        cursor = batch[-1][0] + 3600000
        time.sleep(RATE_LIMIT_SLEEP)
        if len(batch) < 1000:
            break
    
    if not all_bars:
        print(f"  Backfill: no additional data found")
        return 0
    
    # Dedupe
    seen = set()
    deduped = []
    for b in all_bars:
        if b[0] not in seen and b[0] < earliest_ts:
            seen.add(b[0])
            deduped.append(b)
    all_bars = sorted(deduped, key=lambda x: x[0])
    
    if not all_bars:
        print(f"  Backfill: no new bars after dedup")
        return 0
    
    # We need to prepend this data. Strategy: rename existing parts to make room,
    # then write backfill parts at the beginning.
    # Simpler approach: write backfill as "backfill" parts, then renumber everything.
    
    # Load ALL existing data
    existing_all = []
    for f in sorted(files, key=lambda x: int(os.path.basename(x).replace(prefix, "").replace(".json", ""))):
        with open(f) as fh:
            existing_all.extend(json.load(fh))
    
    # Also include any forward-extended parts (numbered > current_max_part from original session)
    # They're already in files, so existing_all has them
    
    # Combine: backfill + existing
    combined = all_bars + existing_all
    combined.sort(key=lambda x: x[0])
    
    # Dedupe combined
    seen = set()
    final = []
    for b in combined:
        if b[0] not in seen:
            seen.add(b[0])
            final.append(b)
    
    # Remove old files
    for f in glob.glob(os.path.join(data_dir, f"{prefix}*.json")):
        os.remove(f)
    
    # Write new files
    part_num = 0
    for i in range(0, len(final), BARS_PER_FILE):
        chunk = final[i:i+BARS_PER_FILE]
        fname = os.path.join(data_dir, f"{prefix}{part_num}.json")
        with open(fname, "w") as fh:
            json.dump(chunk, fh)
        part_num += 1
    
    backfill_start = datetime.fromtimestamp(all_bars[0][0]/1000).strftime("%Y-%m-%d")
    print(f"  Backfill: +{len(all_bars)} bars from {backfill_start} (total now {len(final)}, {part_num} parts)")
    return len(all_bars)


def main():
    parser = argparse.ArgumentParser(description="Extend Chimera H1 kline data")
    parser.add_argument("--data-dir", default="./data", help="Path to backtest/data directory")
    parser.add_argument("--symbols", nargs="*", default=None, help="Specific symbols to download (default: all)")
    parser.add_argument("--no-backfill", action="store_true", help="Skip backfill, only extend forward")
    args = parser.parse_args()
    
    data_dir = args.data_dir
    if not os.path.isdir(data_dir):
        print(f"[ERROR] Data directory not found: {data_dir}")
        sys.exit(1)
    
    symbols = args.symbols if args.symbols else list(SYMBOLS.keys())
    
    print(f"Chimera H1 Data Extender")
    print(f"Data directory: {data_dir}")
    print(f"Symbols: {', '.join(symbols)}")
    print(f"Backfill: {'no' if args.no_backfill else 'yes'}")
    
    results = {}
    for sym in symbols:
        if sym not in SYMBOLS:
            print(f"[WARN] Unknown symbol: {sym}")
            continue
        total, backfilled = download_symbol(sym, data_dir, backfill=not args.no_backfill)
        results[sym] = (total, backfilled)
    
    print(f"\n{'='*60}")
    print(f"SUMMARY")
    print(f"{'='*60}")
    for sym in symbols:
        if sym in results:
            total, back = results[sym]
            # Re-read to get final stats
            _, prefix = SYMBOLS[sym]
            last_ts, final_bars, max_part = get_existing_coverage(data_dir, prefix)
            if last_ts:
                files_list = sorted(glob.glob(os.path.join(data_dir, f"{prefix}*.json")))
                with open(files_list[0]) as fh:
                    first = json.load(fh)
                start = datetime.fromtimestamp(first[0][0]/1000).strftime("%Y-%m-%d")
                end = datetime.fromtimestamp(last_ts/1000).strftime("%Y-%m-%d")
                years = (last_ts - first[0][0]) / (1000*86400*365.25)
                print(f"  {sym:5s}: {final_bars:>6} bars | {start} → {end} | {years:.1f} yrs | {max_part+1} parts")


if __name__ == "__main__":
    main()
