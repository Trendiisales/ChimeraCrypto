"""
test_per_tf_blowoff.py — Per-TF blowoff threshold validation.

Current prod: uniform 80% across all TFs.
Hypothesis: short TFs (H4-H12) accommodate higher mom safely. Test
threshold map: D-bars 80%, H4-H12 150%, H1-H3 200%.

Compares net_bp for each disabled engine under:
  A. Uniform 80% (baseline)
  B. Per-TF (proposed: D=80, H>=24=80, H12-H6=150, H<6=200)
"""
import json, sys, math
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, backtest_engine, metrics, passes, TF_MAP
)

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")

# Per-TF threshold logic.
# D2+: blowoff likely real (slow lookbacks, 100%+ moves = top). Keep 80.
# D1-H16: trend rallies common. 100% lookback move still trade-able. Raise to 120.
# H4-H12: short-term momentum. Raise to 150.
# H1-H3: noisy. Raise to 200.
def per_tf_threshold(tf_secs):
    if tf_secs >= 172800:  return 80.0    # D2+
    if tf_secs >= 86400:   return 120.0   # D1
    if tf_secs >= 28800:   return 150.0   # H8-H16
    if tf_secs >= 14400:   return 150.0   # H4-H6
    return 200.0                          # H1-H3

def main():
    engines = json.loads((ROOT / "disabled_engines.json").read_text())
    rows = []
    for e in engines:
        if e["kind"] not in {"TSMOM","DONCHIAN","BOLLINGER","RSI_REVERT"}: continue
        rule = TF_MAP.get(e["tf_secs"])
        if not rule: continue
        h1 = load_h1(e["symbol"])
        if h1 is None: continue
        df = resample(h1, rule)
        if len(df) < 150: continue
        bpy = (365 * 24 * 3600) / e["tf_secs"]

        # A: uniform 80
        t_uni, _ = backtest_engine(df, e, blowoff_pct=80.0, use_protections=True, confirm_bars=2)
        m_uni = metrics(t_uni, len(df), bpy)
        # B: per-TF
        thr_b = per_tf_threshold(e["tf_secs"])
        t_per, _ = backtest_engine(df, e, blowoff_pct=thr_b, use_protections=True, confirm_bars=2)
        m_per = metrics(t_per, len(df), bpy)

        def g(m, k): return (m.get(k) if m else 0) or 0
        rows.append({
            "tag": e["tag"], "tf": rule, "thr_per": thr_b,
            "uni_trades": g(m_uni,"trades"), "uni_pf": g(m_uni,"pf"),
            "uni_sharpe": g(m_uni,"sharpe"), "uni_cum_bp": g(m_uni,"cum_bp"),
            "uni_mdd_bp": g(m_uni,"mdd_bp"),
            "uni_pass": passes(m_uni),
            "per_trades": g(m_per,"trades"), "per_pf": g(m_per,"pf"),
            "per_sharpe": g(m_per,"sharpe"), "per_cum_bp": g(m_per,"cum_bp"),
            "per_mdd_bp": g(m_per,"mdd_bp"),
            "per_pass": passes(m_per),
        })

    df = pd.DataFrame(rows)
    for col in ("uni_pf","per_pf"): df[col] = df[col].replace([np.inf], 999)

    print(f"=== Per-TF blowoff vs uniform 80% (n={len(df)} disabled engines) ===")
    print(f"\nPass uniform 80%    : {df.uni_pass.sum()}")
    print(f"Pass per-TF threshold: {df.per_pass.sum()}")
    print(f"Newly survives via per-TF: {(df.per_pass & ~df.uni_pass).sum()}")
    print(f"Lost survival via per-TF : {(df.uni_pass & ~df.per_pass).sum()}")
    print()
    print(f"Total cum_bp uniform 80% : {df[df.uni_pass].uni_cum_bp.sum():+,.0f} bp")
    print(f"Total cum_bp per-TF      : {df[df.per_pass].per_cum_bp.sum():+,.0f} bp")
    delta = df[df.per_pass].per_cum_bp.sum() - df[df.uni_pass].uni_cum_bp.sum()
    print(f"Net delta (survivors)    : {delta:+,.0f} bp")
    print()

    # Top engines where per-TF MATERIALLY improved
    df["sharpe_delta"] = df.per_sharpe - df.uni_sharpe
    df["cum_delta"] = df.per_cum_bp - df.uni_cum_bp
    helped = df[(df.per_pass) & (df.sharpe_delta > 0.05)].sort_values("cum_delta", ascending=False)
    print(f"=== TOP 15 engines where per-TF blowoff HELPS ===")
    cols = ["tag","tf","thr_per","uni_pf","per_pf","uni_sharpe","per_sharpe","uni_cum_bp","per_cum_bp","cum_delta"]
    print(helped[cols].head(15).to_string(index=False, formatters={
        "uni_pf":"{:.2f}".format,"per_pf":"{:.2f}".format,
        "uni_sharpe":"{:.2f}".format,"per_sharpe":"{:.2f}".format,
        "uni_cum_bp":"{:,.0f}".format,"per_cum_bp":"{:,.0f}".format,
        "cum_delta":"{:+,.0f}".format,
    }))
    print()

    # Top engines where per-TF HURT
    hurt = df[(df.uni_pass) & (df.sharpe_delta < -0.1)].sort_values("cum_delta")
    print(f"=== TOP 10 engines where per-TF blowoff HURTS ===")
    print(hurt[cols].head(10).to_string(index=False, formatters={
        "uni_pf":"{:.2f}".format,"per_pf":"{:.2f}".format,
        "uni_sharpe":"{:.2f}".format,"per_sharpe":"{:.2f}".format,
        "uni_cum_bp":"{:,.0f}".format,"per_cum_bp":"{:,.0f}".format,
        "cum_delta":"{:+,.0f}".format,
    }))

if __name__ == "__main__":
    main()
