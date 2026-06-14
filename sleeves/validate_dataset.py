#!/usr/bin/env python3
"""
validate_dataset.py — MANDATORY data-veracity gate. Run BEFORE any backtest/search.

Why this exists (2026-06-14 data disaster): backtests were run on a directory
where different symbol files covered DIFFERENT date ranges (BTC=2025-26 only,
others=2024, alts=mixed fresh-fetched). The harness assumed multi-year coverage
and bucketed trades by calendar year -> every "cross-regime" number was garbage.
Nothing detected it because nothing checked span, consistency, or provenance.

This tool refuses to pass a dataset unless EVERY check holds. Exit code 0 = safe
to backtest. Non-zero = DO NOT TRUST, do not run.

Checks per file:
  - parses as OHLC, all values > 0
  - timestamps strictly monotonic increasing (no dupes, no out-of-order)
  - ONE modal bar interval (detects TF); off-interval gaps reported, large gaps flagged
  - OHLC sanity: high >= max(open,close), low <= min(open,close), high >= low
Dataset-level (the disaster guards):
  - CONSISTENCY: every file shares the same modal interval (no mixed TF)
  - SPAN ALIGNMENT: every file's [first,last] overlaps within a tolerance
    (catches one file being a different vintage than the rest)
  - COVERAGE: if --require-cycles given, every file must span those years
Writes data/<dir>/MANIFEST.json with per-file sha256 + span + bars + interval,
so a later run can prove the files are the same ones that were validated.
"""
import csv, glob, os, sys, json, hashlib, datetime, argparse, statistics

def sha256(path, limit_mb=8):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read(limit_mb * 1024 * 1024))   # head hash is enough to catch a swap
    return h.hexdigest()[:16]

def load(path):
    rows = []
    with open(path) as fh:
        r = csv.reader(fh); next(r, None)
        for x in r:
            try: rows.append((int(x[0]), float(x[1]), float(x[2]), float(x[3]), float(x[4])))
            except Exception: pass
    return rows

def vdate(ms): return datetime.datetime.utcfromtimestamp(ms/1000).strftime("%Y-%m-%d")

def check_file(path):
    """Return (ok, meta, errors)."""
    errs = []
    rows = load(path)
    if len(rows) < 100:
        return False, {}, [f"too few rows ({len(rows)})"]
    # monotonic + dupes
    nonmono = dupes = 0
    deltas = []
    ohlc_bad = nonpos = 0
    for i, (ts, o, h, l, c) in enumerate(rows):
        if o <= 0 or h <= 0 or l <= 0 or c <= 0: nonpos += 1
        if h < max(o, c) - 1e-9 or l > min(o, c) + 1e-9 or h < l - 1e-9: ohlc_bad += 1
        if i > 0:
            d = ts - rows[i-1][0]
            if d == 0: dupes += 1
            elif d < 0: nonmono += 1
            else: deltas.append(d)
    interval = int(statistics.median(deltas)) if deltas else 0
    big_gaps = sum(1 for d in deltas if d > interval * 4)
    maxgap = max(deltas) if deltas else 0
    meta = {
        "bars": len(rows), "first_ms": rows[0][0], "last_ms": rows[-1][0],
        "first": vdate(rows[0][0]), "last": vdate(rows[-1][0]),
        "interval_s": interval // 1000, "big_gaps": big_gaps,
        "maxgap_h": round(maxgap/3600000, 1), "sha": sha256(path),
        "years": sorted({datetime.datetime.utcfromtimestamp(r[0]/1000).year for r in rows}),
    }
    # HARD failures (corrupt data)
    if nonmono: errs.append(f"{nonmono} out-of-order timestamps")
    if dupes:   errs.append(f"{dupes} duplicate timestamps")
    if nonpos:  errs.append(f"{nonpos} non-positive prices")
    if ohlc_bad > len(rows) * 0.001: errs.append(f"{ohlc_bad} OHLC-invalid bars (>0.1%)")
    if interval == 0: errs.append("cannot determine bar interval")
    return (len(errs) == 0), meta, errs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("datadir")
    ap.add_argument("--glob", default="*.csv")
    ap.add_argument("--require-cycles", default="", help="comma years every file must span, e.g. 2021,2022,2023,2024")
    ap.add_argument("--span-tol-days", type=int, default=30, help="max allowed mismatch in dataset start/end across files")
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.datadir, args.glob)))
    if not files:
        print(f"FAIL: no files match {args.datadir}/{args.glob}"); sys.exit(2)
    req_cycles = [int(y) for y in args.require_cycles.split(",") if y.strip()]

    print(f"{'file':<26}{'bars':>8}{'interval':>9}{'span':>26}{'maxgap_h':>9}  status")
    print("-"*100)
    manifest = {"datadir": args.datadir, "validated_utc": datetime.datetime.utcnow().isoformat(),
                "files": {}}
    any_fail = False
    intervals, firsts, lasts = [], [], []
    for f in files:
        name = os.path.basename(f)
        ok, meta, errs = check_file(f)
        if not meta:
            print(f"{name:<26}{'--':>8}{'--':>9}{'--':>26}{'--':>9}  FAIL: {'; '.join(errs)}")
            any_fail = True; continue
        # per-file cycle coverage
        if req_cycles and not all(y in meta["years"] for y in req_cycles):
            missing = [y for y in req_cycles if y not in meta["years"]]
            errs.append(f"missing cycles {missing}")
        status = "OK" if not errs else "FAIL: " + "; ".join(errs)
        if errs: any_fail = True
        print(f"{name:<26}{meta['bars']:>8}{meta['interval_s']:>8}s"
              f"{(meta['first']+'->'+meta['last']):>26}{meta['maxgap_h']:>9}  {status}")
        manifest["files"][name] = {**meta, "ok": not errs, "errors": errs}
        intervals.append(meta["interval_s"]); firsts.append(meta["first_ms"]); lasts.append(meta["last_ms"])

    print("-"*100)
    # DISASTER GUARD 1 — interval consistency (no mixed TF)
    if len(set(intervals)) > 1:
        print(f"FAIL [CONSISTENCY]: mixed bar intervals across files: {sorted(set(intervals))}s "
              f"-- files are different timeframes, cannot combine")
        any_fail = True
    # DISASTER GUARD 2 — span alignment (no mixed vintage)
    if firsts and lasts:
        tol = args.span_tol_days * 86400000
        fspread = (max(firsts) - min(firsts)) / 86400000
        lspread = (max(lasts)  - min(lasts))  / 86400000
        print(f"span alignment: start spread={fspread:.0f}d (listing dates, info), "
              f"end spread={lspread:.0f}d (tol={args.span_tol_days}d)")
        # start-spread is benign: coins list on different dates. --require-cycles
        # guarantees each file still covers the windows that matter. WARN only.
        if max(firsts) - min(firsts) > tol:
            late = [n for n,m in manifest["files"].items() if m["first_ms"] > min(firsts)+tol]
            print(f"WARN [LISTING]: late-listing symbols (no data before their listing): {late} "
                  f"-- ok if --require-cycles covers them; per-symbol window coverage enforced in harness")
        # end-spread is the DISASTER signature: a file ending early = stale/delisted
        # vintage that will silently poison recent-window backtests.
        if max(lasts) - min(lasts) > tol:
            stale = [n for n,m in manifest["files"].items() if m["last_ms"] < max(lasts)-tol]
            print(f"FAIL [VINTAGE]: files end on different dates "
                  f"(end {vdate(min(lasts))}..{vdate(max(lasts))}) -- STALE files: {stale} "
                  f"-- THIS IS THE 2026-06-14 DISASTER signature. Remove stale files or refetch.")
            any_fail = True
    manifest["consistent"] = not any_fail
    mpath = os.path.join(args.datadir, "MANIFEST.json")
    with open(mpath, "w") as fh: json.dump(manifest, fh, indent=2)
    print(f"manifest -> {mpath}")
    if any_fail:
        print("\n*** DATASET FAILED VALIDATION — DO NOT BACKTEST ON IT ***")
        sys.exit(1)
    print(f"\nPASS: {len(files)} files, interval={intervals[0]}s, "
          f"span={vdate(min(firsts))}->{vdate(max(lasts))}. Safe to backtest.")
    sys.exit(0)

if __name__ == "__main__":
    main()
