"""
test_new_symbols.py — Validate TSMOM + DONCHIAN on new symbols.
TIA, ONDO, FET, PEPE, WIF — newly downloaded H1.
"""
import sys, math, json
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, backtest_engine, metrics, passes, TF_MAP
)

NEW = [("tiausdt","TIA"), ("ondousdt","ONDO"), ("fetusdt","FET"),
       ("pepeusdt","PEPE"), ("wifusdt","WIF")]
TFS = ["3h","4h","6h","8h","12h","1D","2D"]
TF_TO_SECS = {v:k for k,v in TF_MAP.items()}

# Per-TF blowoff (mirrors deploy plan)
def per_tf_blowoff(tf_secs):
    if tf_secs >= 172800:  return 80.0
    if tf_secs >= 86400:   return 120.0
    if tf_secs >= 14400:   return 150.0
    return 200.0

def make_eng(sym, tf_secs, kind, lookback=20):
    return dict(
        symbol=sym, kind=kind, tf_secs=tf_secs, lookback=lookback,
        hold_bars=12, sl_atr_mult=3.0, atr_period=14,
        bb_k=2.0, rsi_threshold=30.0, round_trip_bp=22.0, max_history=64,
        trail_arm_atr=1.0, trail_dist_atr=0.4, trail_tighten_atr=3.0,
        trail_tighten_dist_atr=0.25,
    )

def main():
    rows = []
    for sym, label in NEW:
        h1 = load_h1(sym)
        if h1 is None:
            print(f"{label}: no data"); continue
        first_ts = h1.index[0]
        n_bars = len(h1)
        print(f"{label}: {n_bars} H1 bars from {first_ts.date()}")
        for tf in TFS:
            df = resample(h1, tf)
            if len(df) < 150: continue
            tf_secs = TF_TO_SECS[tf]
            bpy = (365 * 24 * 3600) / tf_secs
            thr = per_tf_blowoff(tf_secs)
            for kind in ["TSMOM","DONCHIAN"]:
                e = make_eng(sym, tf_secs, kind, lookback=20)
                trades, _ = backtest_engine(df, e, blowoff_pct=thr,
                                             use_protections=True, confirm_bars=2)
                m = metrics(trades, len(df), bpy)
                def g(k): return (m.get(k) if m else 0) or 0
                rows.append({
                    "sym": label, "tf": tf, "kind": kind,
                    "blowoff_pct": thr,
                    "trades": g("trades"), "pf": g("pf"), "sharpe": g("sharpe"),
                    "cum_bp": g("cum_bp"), "mdd_bp": g("mdd_bp"),
                    "worst_bp": g("worst_bp"), "avg_loss_bp": g("avg_loss_bp"),
                    "avg_win_bp": g("avg_win_bp"),
                    "passes": passes(m),
                })

    df = pd.DataFrame(rows)
    df["pf"] = df["pf"].replace([np.inf], 999)
    df.to_csv("/Users/jo/ChimeraCrypto/backtest/test_new_symbols_results.csv", index=False)

    print()
    print(f"=== ALL TESTED CONFIGS (n={len(df)}) ===")
    cols=["sym","tf","kind","trades","pf","sharpe","cum_bp","mdd_bp","worst_bp","passes"]
    print(df[cols].to_string(index=False, formatters={
        "pf":"{:.2f}".format,"sharpe":"{:.2f}".format,
        "cum_bp":"{:,.0f}".format,"mdd_bp":"{:,.0f}".format,
        "worst_bp":"{:.0f}".format,
    }))
    print()
    print(f"=== SURVIVORS (PF>=1.3, Sharpe>=1.0, trades>=50) ===")
    surv = df[df.passes].sort_values("sharpe", ascending=False)
    if len(surv):
        print(surv[cols].to_string(index=False, formatters={
            "pf":"{:.2f}".format,"sharpe":"{:.2f}".format,
            "cum_bp":"{:,.0f}".format,"mdd_bp":"{:,.0f}".format,
            "worst_bp":"{:.0f}".format,
        }))
    else:
        print("none")

if __name__ == "__main__":
    main()
