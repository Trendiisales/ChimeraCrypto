#!/usr/bin/env python3
"""Quick analyzer: filter sweep CSV for scalp + general candidates."""
import csv, sys, glob, os

HEADER = ["lookback","hold","sl_atr","trail_arm","trail_dist",
          "trades","wins","wr","net_bp","pf","sharpe","maxdd_bp"]

def parse_rows(path):
    rows = []
    with open(path) as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) != 12: continue
            try:
                r = {k: float(v) if "." in v or k in ("wr","net_bp","pf","sharpe","maxdd_bp","sl_atr","trail_arm","trail_dist") else int(v) for k,v in zip(HEADER, parts)}
                rows.append(r)
            except ValueError:
                continue
    return rows

def fmt(r):
    return f"  lb={int(r['lookback']):2d} hold={int(r['hold']):2d} sl={r['sl_atr']:.1f} | n={int(r['trades']):4d} wr={r['wr']:.1f}% | net={r['net_bp']:+8.1f}bp pf={r['pf']:.3f} sharpe={r['sharpe']:+.2f} dd={r['maxdd_bp']:.0f}"

def main():
    pat = sys.argv[1] if len(sys.argv) > 1 else "sweep_btc_*.csv"
    files = sorted(glob.glob(pat))
    if not files:
        print(f"no files match {pat}")
        return
    for fp in files:
        strat = os.path.basename(fp).replace("sweep_","").replace(".csv","")
        rows = parse_rows(fp)
        # dedup trail combos: take first per (lookback,hold,sl_atr)
        seen = set()
        ded = []
        for r in rows:
            k = (r["lookback"], r["hold"], r["sl_atr"])
            if k in seen: continue
            seen.add(k); ded.append(r)
        print(f"\n=== {strat} ({len(rows)} raw / {len(ded)} dedup) ===")
        # Scalp: lookback<=10 hold<=4 n>=30
        scalp = [r for r in ded if r["lookback"]<=10 and r["hold"]<=4 and r["trades"]>=30]
        scalp.sort(key=lambda x: x["net_bp"], reverse=True)
        print(" SCALP (lookback<=10 hold<=4 n>=30):")
        for r in scalp[:3]: print(fmt(r))
        if not scalp: print("  (none)")
        # All positive PF
        pos = [r for r in ded if r["pf"]>=1.0 and r["trades"]>=30]
        pos.sort(key=lambda x: x["pf"], reverse=True)
        print(" ALL POSITIVE PF (n>=30):")
        for r in pos[:5]: print(fmt(r))
        if not pos: print("  (none)")

if __name__ == "__main__":
    main()
