#!/usr/bin/env python3
"""Walk-forward + bear-window stress test.
Bear window: 2026-01-14 to 2026-02-15 (BTC -35% draw).
Days since 2026-01-14: roughly 134 days ago from 2026-05-28.
So --last-days 134 captures bear-onward; we want bear ONLY -> use bp delta."""
import subprocess, csv, sys, json

rows = list(csv.DictReader(open("/tmp/edge_candidates2.csv")))
WIN_DAYS = [134, 180, 365, 730]  # 134 = since bear peak
print(f"{'SYM':10} {'TF':>6} {'STRAT':14} {'lb':>3} {'hold':>4} {'sl':>4} | {'5yr_PF':>7} {'730d':>7} {'365d':>7} {'180d':>7} {'134d':>7} | verdict")
keepers = []
for r in rows:
    sym, tf, strat = r["sym"], int(r["tf"]), r["strat"]
    lb, hold = int(r["lb"]), int(r["hold"])
    sl, tarm = float(r["sl"]), float(r["tarm"])
    base_pf = float(r["pf"])
    base_bp = float(r["bp"])
    wf = {}
    fail = False
    for d in WIN_DAYS:
        proc = subprocess.run(
            ["./backtest_mac", "--sweep", f"{sym}:{tf}:{strat}",
             "--preset", "prod_tiered", "--last-days", str(d)],
            capture_output=True, text=True, timeout=180)
        pf = None; n = 0; bp = 0; dd = 0
        for line in proc.stdout.splitlines()[1:]:
            try:
                c = line.split(",")
                if (int(c[0]) == lb and int(c[1]) == hold
                        and abs(float(c[2]) - sl) < 0.01
                        and abs(float(c[3]) - tarm) < 0.01):
                    pf = float(c[9]); n = int(c[5]); bp = float(c[8]); dd = float(c[11]); break
            except (ValueError, IndexError):
                continue
        wf[d] = {"pf": pf, "n": n, "bp": bp, "dd": dd}
        # Fail if PF<1.3 AND n>=10
        if pf is not None and n >= 10 and pf < 1.3:
            fail = True
        # Fail if drawdown > 50% of 5yr bp earned (proxy: max single-window dd > 0.5 * base_bp)
        if dd > base_bp * 0.5:
            fail = True

    fmt = lambda d: f"{wf[d]['pf']:.2f}" if wf[d]['pf'] else "-"
    verdict = "FRAGILE" if fail else "ADD"
    if verdict == "ADD":
        keepers.append({
            "sym": sym, "tf": tf, "strat": strat,
            "lb": lb, "hold": hold, "sl": sl, "tarm": tarm,
            "pf_5yr": base_pf, "bp_5yr": base_bp,
            "wf": wf,
        })
    print(f"{sym:10} {tf:>6} {strat:14} {lb:>3} {hold:>4} {sl:>4.1f} | {base_pf:>7.2f} {fmt(730):>7} {fmt(365):>7} {fmt(180):>7} {fmt(134):>7} | {verdict}")

print(f"\nKEEPERS: {len(keepers)}")
for k in keepers:
    print(f"  {k['sym']}-{k['strat']}-{k['tf']}: lb={k['lb']} hold={k['hold']} sl={k['sl']}")
    for d in WIN_DAYS:
        w = k['wf'][d]
        pf = f"{w['pf']:.2f}" if w['pf'] else "-"
        print(f"    {d:>4}d: PF={pf:>6} n={w['n']:>3} bp={w['bp']:>+7.0f} dd={w['dd']:>+6.0f}")
json.dump(keepers, open("/tmp/keepers2.json","w"), indent=2)
