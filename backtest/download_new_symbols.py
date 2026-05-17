#!/usr/bin/env python3
"""
Session 30: Download H1 klines for new symbols (PEPE, WIF, FET, ONDO, TIA).
Run from the backtest/ directory.

Usage:
    python3 download_new_symbols.py
    python3 download_new_symbols.py PEPEUSDT WIFUSDT   # specific symbols only
"""
import json, time, urllib.request, os, sys

# Data dir relative to this script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "data")
LIMIT = 1000
INTERVAL = "1h"
START_MS = 1640995200000  # 2022-01-01 00:00 UTC (match existing data)

# New Session 30 symbols
NEW_SYMBOLS = ["PEPEUSDT", "WIFUSDT", "FETUSDT", "ONDOUSDT", "TIAUSDT"]


def download_symbol(symbol):
    """Download all H1 klines for symbol from START_MS to now."""
    os.makedirs(DATA_DIR, exist_ok=True)

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


def download_spot_1m(symbol):
    """Download recent 1m spot klines for live feed warm-start."""
    spot_dir = os.path.join(SCRIPT_DIR, "..", "data", "klines_spot")
    os.makedirs(spot_dir, exist_ok=True)

    outpath = os.path.join(spot_dir, f"{symbol.upper()}_1m.csv")
    # Only need last ~7 days of 1m data for warm-start
    import datetime
    end_ms = int(time.time() * 1000)
    start_ms = end_ms - 7 * 24 * 3600 * 1000  # 7 days ago

    all_bars = []
    current_start = start_ms

    while current_start < end_ms:
        url = (f"https://api.binance.com/api/v3/klines?"
               f"symbol={symbol.upper()}&interval=1m"
               f"&startTime={current_start}&limit={LIMIT}")
        try:
            req = urllib.request.Request(url)
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read().decode())
        except Exception as e:
            print(f"  [1m ERROR] {e}")
            time.sleep(2)
            continue

        if not data:
            break

        all_bars.extend(data)
        if len(data) < LIMIT:
            break
        current_start = data[-1][6] + 1
        time.sleep(0.1)

    # Write as CSV: open_time,open,high,low,close,volume
    with open(outpath, 'w') as f:
        f.write("open_time,open,high,low,close,volume\n")
        for bar in all_bars:
            f.write(f"{bar[0]},{bar[1]},{bar[2]},{bar[3]},{bar[4]},{bar[5]}\n")

    print(f"  1m spot data: {len(all_bars)} bars → {outpath}")


if __name__ == "__main__":
    symbols = sys.argv[1:] if len(sys.argv) > 1 else NEW_SYMBOLS

    print("=" * 60)
    print("Session 30: Downloading H1 klines for new symbols")
    print("=" * 60)

    for sym in symbols:
        print(f"\n[DOWNLOADING H1] {sym}...")
        bars = download_symbol(sym)
        if bars > 0:
            print(f"[DOWNLOADING 1m spot] {sym}...")
            download_spot_1m(sym)

    print("\n" + "=" * 60)
    print("DONE. Next steps:")
    print("  1. Run optimizer_general on each symbol+strategy combo")
    print("  2. Run walk_forward_validate on passing combos")
    print("  3. Add deploy-grade engines to main.cpp g_slots")
    print("=" * 60)
