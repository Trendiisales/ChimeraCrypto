"""
sim_dynamic_tier.py — Simulate dynamic tier sizing on backtest trades.

Walk through each engine's trade sequence. After each trade, recompute rolling
PF/Sharpe from last 30. Blend with backtest priors. Show tier evolution.

Compares:
  STATIC: tier from backtest oos_sharpe + bt_pf only
  DYNAMIC: Bayesian blend (backtest prior + live rolling)
"""
import json, sys, math
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, backtest_engine, TF_MAP
)

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")

# Mirror of main.cpp tier_sizing_mult().
def tier_static(sharpe, pf):
    if sharpe >= 3.5 and pf >= 2.0: return 1.3, "ELITE"
    if sharpe >= 2.0:              return 1.1, "STRONG"
    if sharpe <  1.0 or pf < 1.5:  return 0.7, "WEAK"
    return 1.0, "BASE"

def live_weight(n):
    if n < 10:  return 0.0
    if n >= 50: return 0.5
    return 0.2 + (n - 10) * (0.30 / 40.0)

def compute_live_stats(nets):
    if len(nets) < 5: return 0.0, 0.0, 0
    arr = np.asarray(nets)
    wins = arr[arr > 0].sum()
    losses = -arr[arr < 0].sum()
    pf = (wins / losses) if losses > 0 else (99.0 if wins > 0 else 0.0)
    mean = arr.mean(); sd = arr.std()
    sharpe = (mean / sd) * math.sqrt(200.0) if sd > 0 else 0.0
    return pf, sharpe, len(arr)

def main():
    # Use validate_with_protections_results.csv for tag→bt_pf/bt_sharpe lookup
    df = pd.read_csv(ROOT / "validate_with_protections_results.csv")
    df["pf"] = df["pf"].replace([np.inf], 999)
    df = df[df.passes].copy()
    bt_lookup = {row.tag: (row.sharpe, row.pf) for _, row in df.iterrows()}

    # Re-run backtest to get per-trade nets for top engines
    engines_json = (json.loads((ROOT / "engines.json").read_text())
                  + json.loads((ROOT / "disabled_engines.json").read_text()))
    by_tag = {e["tag"]: e for e in engines_json}

    # Pick 8 representative engines spanning tier outcomes
    picks = ["NEAR-TSMOM-H4", "NEAR-TSMOM-H8", "NEAR-TSMOM-H12",
             "SOL-TSMOM-H4", "SOL-TSMOM-H6", "LINK-TSMOM-H8",
             "SUI-TSMOM-H4", "DOGE-TSMOM-H12"]

    print(f"=== Dynamic tier evolution simulation ===")
    print(f"(Each engine's tier changes as live trades accumulate)")
    print()

    summary_rows = []
    for tag in picks:
        e = by_tag.get(tag)
        bt = bt_lookup.get(tag)
        if not e or not bt: continue
        bt_sh, bt_pf = bt
        static_mult, static_tier = tier_static(bt_sh, bt_pf)

        rule = TF_MAP.get(e["tf_secs"])
        h1 = load_h1(e["symbol"])
        df_bars = resample(h1, rule)
        trades, _ = backtest_engine(df_bars, e, blowoff_pct=80.0, use_protections=True, confirm_bars=2)
        nets_bp = [t[3] * 1e4 for t in trades]

        # Walk through trades, recompute live every step
        rolling = []
        tier_history = []
        for i, net in enumerate(nets_bp):
            rolling.append(net)
            if len(rolling) > 30: rolling = rolling[-30:]
            live_pf, live_sh, n = compute_live_stats(rolling)
            w = live_weight(n)
            blend_sh = (1 - w) * bt_sh + w * live_sh
            blend_pf = (1 - w) * bt_pf + w * live_pf
            mult, tier = tier_static(blend_sh, blend_pf)
            tier_history.append((i+1, n, w, live_sh, live_pf, blend_sh, blend_pf, mult, tier))

        # Sample tier at n=10,30,50,100,end
        sample_n = [10, 30, 50, 100, len(tier_history)]
        print(f"\n{tag}  (backtest static: sharpe={bt_sh:.2f} pf={bt_pf:.2f} -> {static_tier} {static_mult}x)")
        print(f"{'after_n':>8} {'live_n':>6} {'w':>5} {'live_sh':>8} {'live_pf':>8} {'blend_sh':>9} {'blend_pf':>9} {'tier':>7} {'mult':>5}")
        seen = set()
        for n_target in sample_n:
            if n_target > len(tier_history): continue
            row = tier_history[n_target - 1]
            if row[0] in seen: continue
            seen.add(row[0])
            print(f"{row[0]:>8} {row[1]:>6} {row[2]:>5.2f} {row[3]:>8.2f} {row[4]:>8.2f} {row[5]:>9.2f} {row[6]:>9.2f} {row[8]:>7} {row[7]:>5.2f}")

        # Final tier vs static
        final = tier_history[-1] if tier_history else None
        if final:
            summary_rows.append({
                "tag": tag, "total_trades": len(nets_bp),
                "bt_sharpe": bt_sh, "bt_pf": bt_pf,
                "static_mult": static_mult, "static_tier": static_tier,
                "final_blend_sh": final[5], "final_blend_pf": final[6],
                "final_mult": final[7], "final_tier": final[8],
                "delta_mult": final[7] - static_mult,
            })

    print(f"\n\n=== Summary: static vs dynamic (at end of 5yr backtest) ===")
    s = pd.DataFrame(summary_rows)
    print(s.to_string(index=False, formatters={
        "bt_sharpe":"{:.2f}".format, "bt_pf":"{:.2f}".format,
        "static_mult":"{:.2f}".format, "final_mult":"{:.2f}".format,
        "final_blend_sh":"{:.2f}".format, "final_blend_pf":"{:.2f}".format,
        "delta_mult":"{:+.2f}".format,
    }))

if __name__ == "__main__":
    main()
