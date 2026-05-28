#!/usr/bin/env python3
import subprocess, csv, json, sys

rows = list(csv.DictReader(open("/tmp/edge_candidates.csv")))
WIN_DAYS = [180, 365, 730]
HDR = ("SYM", "TF", "STRAT", "lb", "hold", "sl", "5yr_PF", "365d", "730d", "180d", "verdict")
print("{:10} {:>6} {:14} {:>3} {:>4} {:>4} {:>8} {:>7} {:>7} {:>7} {}".format(*HDR))

keepers = []
for r in rows:
    sym, tf, strat = r["sym"], int(r["tf"]), r["strat"]
    lb, hold = int(r["lb"]), int(r["hold"])
    sl, tarm = float(r["sl"]), float(r["tarm"])
    base_pf = float(r["pf"])
    wf = {}
    fail = False
    for d in WIN_DAYS:
        proc = subprocess.run(
            ["./backtest_mac", "--sweep", f"{sym}:{tf}:{strat}",
             "--preset", "prod_tiered", "--last-days", str(d)],
            capture_output=True, text=True, timeout=180)
        pf = None; n = 0; bp = 0
        for line in proc.stdout.splitlines()[1:]:
            try:
                c = line.split(",")
                if (int(c[0]) == lb and int(c[1]) == hold
                        and abs(float(c[2]) - sl) < 0.01
                        and abs(float(c[3]) - tarm) < 0.01):
                    pf = float(c[9]); n = int(c[5]); bp = float(c[8]); break
            except (ValueError, IndexError):
                continue
        wf[d] = (pf, n, bp)
        if pf is not None and n >= 10 and pf < 1.3:
            fail = True

    f = {d: (f"{wf[d][0]:.2f}" if wf[d][0] else "-") for d in WIN_DAYS}
    verdict = "FRAGILE" if fail else ("ADD" if base_pf >= 2.5 else "MARGINAL")
    if verdict == "ADD":
        keepers.append({
            "sym": sym, "tf": tf, "strat": strat,
            "lb": lb, "hold": hold, "sl": sl, "tarm": tarm,
            "pf_5yr": base_pf,
            "pf_365": wf[365][0], "pf_730": wf[730][0], "pf_180": wf[180][0],
            "n_5yr": int(r["n"]), "bp_5yr": float(r["bp"]),
        })
    print(f"{sym:10} {tf:>6} {strat:14} {lb:>3} {hold:>4} {sl:>4.1f} {base_pf:>8.2f} {f[365]:>7} {f[730]:>7} {f[180]:>7} {verdict}")

print(f"\nKEEPERS: {len(keepers)}")
json.dump(keepers, open("/tmp/keepers.json", "w"), indent=2)
for k in keepers:
    print(f"  {k['sym']}-{k['strat']}-{k['tf']}: lb={k['lb']} hold={k['hold']} sl={k['sl']} | 5yr PF={k['pf_5yr']:.2f} 365d={k['pf_365']} 730d={k['pf_730']} 180d={k['pf_180']}")
