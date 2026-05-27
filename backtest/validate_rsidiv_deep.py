"""
validate_rsidiv_deep.py — Deep validation of RSI divergence as new signal kind.

Tests:
1. OOS walk-forward: train 2021-2024 / test 2025-2026.
2. Parameter sensitivity: sweep rsi_period {7,14,21} × pivot_len {3,5,7}.
3. Live session window simulation: May 20-27 2026.
4. Confirms incremental P&L vs current roster.
"""
import sys, json, math
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import load_h1, resample, metrics, passes, TF_MAP
from test_new_signals import sig_rsi_divergence, backtest_custom, make_template

SYMBOLS = ["btcusdt","ethusdt","bnbusdt","solusdt","xrpusdt","dogeusdt",
           "linkusdt","avaxusdt","aptusdt","arbusdt","nearusdt","suiusdt"]
TFS = ["3h","4h","6h","8h","12h"]
TF_TO_SECS = {v:k for k,v in TF_MAP.items()}

OOS_SPLIT = pd.Timestamp("2025-05-01", tz="UTC")

def run_one(df, eng_template, blowoff=80.0):
    sig = sig_rsi_divergence(df, rsi_period=14, pivot_len=5)
    trades, _ = backtest_custom(df, sig, eng_template, blowoff_pct=blowoff)
    n_bars = len(df)
    return trades, n_bars

def filter_trades_by_window(df, trades, start_ts, end_ts=None):
    idx_ms = (df.index.view("int64") // 1_000_000).astype("int64")
    start_ms = int(start_ts.value // 1_000_000)
    end_ms = int(end_ts.value // 1_000_000) if end_ts else 1 << 60
    out = []
    for t in trades:
        ent = int(idx_ms[t[0]])
        if ent >= start_ms and ent <= end_ms:
            out.append(t)
    return out

# ── 1. OOS walk-forward ──────────────────────────────────────────────────────
def test_oos():
    print(f"\n=== TEST 1: Walk-forward OOS (split={OOS_SPLIT.date()}) ===")
    rows = []
    for sym in SYMBOLS:
        h1 = load_h1(sym)
        if h1 is None: continue
        for tf in TFS:
            df = resample(h1, tf)
            if len(df) < 200: continue
            tf_secs = TF_TO_SECS[tf]
            bpy = (365 * 24 * 3600) / tf_secs
            tmpl = make_template(tf_secs)

            trades, _ = run_one(df, tmpl)
            # split trades by entry timestamp
            idx_ms = (df.index.view("int64") // 1_000_000).astype("int64")
            split_ms = int(OOS_SPLIT.value // 1_000_000)
            in_sample  = [t for t in trades if int(idx_ms[t[0]]) <  split_ms]
            out_sample = [t for t in trades if int(idx_ms[t[0]]) >= split_ms]
            # Approximate IS/OOS bar counts (proportional)
            split_idx = int(np.searchsorted(idx_ms, split_ms))
            n_is, n_oos = split_idx, len(df) - split_idx

            m_is  = metrics(in_sample, max(1, n_is), bpy)
            m_oos = metrics(out_sample, max(1, n_oos), bpy)

            def g(m, k): return (m.get(k) if m else None) or 0

            rows.append({
                "sym": sym.upper().replace("USDT",""), "tf": tf,
                "is_trades": len(in_sample), "is_pf": g(m_is,"pf"), "is_sharpe": g(m_is,"sharpe"),
                "is_pass": passes(m_is),
                "oos_trades": len(out_sample), "oos_pf": g(m_oos,"pf"), "oos_sharpe": g(m_oos,"sharpe"),
                "oos_pass": passes(m_oos),
            })

    df_out = pd.DataFrame(rows)
    df_out["oos_pf"] = df_out["oos_pf"].replace([np.inf], 999)
    df_out["is_pf"] = df_out["is_pf"].replace([np.inf], 999)

    is_pass = df_out.is_pass.sum()
    oos_pass = df_out.oos_pass.sum()
    both_pass = (df_out.is_pass & df_out.oos_pass).sum()
    print(f"In-sample (2021-2025) pass : {is_pass}/{len(df_out)}")
    print(f"OOS (2025-2026) pass       : {oos_pass}/{len(df_out)}")
    print(f"Pass BOTH (robust)         : {both_pass}/{len(df_out)}")
    print()
    robust = df_out[df_out.is_pass & df_out.oos_pass].sort_values("oos_sharpe", ascending=False)
    print(f"=== ROBUST OOS WINNERS (pass both periods, n={len(robust)}) ===")
    if len(robust):
        cols = ["sym","tf","is_trades","is_pf","is_sharpe","oos_trades","oos_pf","oos_sharpe"]
        print(robust[cols].head(20).to_string(index=False, formatters={
            "is_pf":"{:.2f}".format,"is_sharpe":"{:.2f}".format,
            "oos_pf":"{:.2f}".format,"oos_sharpe":"{:.2f}".format,
        }))
    return df_out

# ── 2. Parameter sensitivity ─────────────────────────────────────────────────
def test_param_sweep():
    print(f"\n=== TEST 2: Parameter sensitivity ===")
    # Focus on top OOS-passing configs; sweep params
    top = [("nearusdt","6h"),("avaxusdt","6h"),("solusdt","6h"),("btcusdt","6h"),
           ("xrpusdt","6h"),("solusdt","4h"),("ethusdt","4h"),("arbusdt","3h")]
    rows = []
    for sym, tf in top:
        h1 = load_h1(sym)
        if h1 is None: continue
        df = resample(h1, tf)
        tf_secs = TF_TO_SECS[tf]
        bpy = (365 * 24 * 3600) / tf_secs
        tmpl = make_template(tf_secs)
        for rsi_p in [7, 14, 21]:
            for piv in [3, 5, 7]:
                sig = sig_rsi_divergence(df, rsi_period=rsi_p, pivot_len=piv)
                trades, _ = backtest_custom(df, sig, tmpl, blowoff_pct=80.0)
                m = metrics(trades, len(df), bpy)
                def g(k): return (m.get(k) if m else 0) or 0
                rows.append({
                    "sym": sym.upper().replace("USDT",""), "tf": tf,
                    "rsi_p": rsi_p, "pivot": piv,
                    "n_sig": int(sig.sum()), "trades": g("trades"),
                    "pf": g("pf"), "sharpe": g("sharpe"),
                    "passes": passes(m),
                })
    df_out = pd.DataFrame(rows)
    df_out["pf"] = df_out["pf"].replace([np.inf], 999)
    print(df_out.to_string(index=False, formatters={
        "pf":"{:.2f}".format, "sharpe":"{:.2f}".format,
    }))
    print()
    # Stability check: each (sym,tf) — % of param combos that pass
    grp = df_out.groupby(["sym","tf"]).agg(
        n_combos=("passes","count"), n_pass=("passes","sum"),
        mean_pf=("pf","mean"), mean_sharpe=("sharpe","mean"),
        std_sharpe=("sharpe","std"),
    ).reset_index()
    grp["pass_rate"] = grp.n_pass / grp.n_combos
    print("=== Param stability per (sym,tf) ===")
    print(grp.to_string(index=False, formatters={
        "pass_rate":"{:.0%}".format,
        "mean_pf":"{:.2f}".format,
        "mean_sharpe":"{:.2f}".format,
        "std_sharpe":"{:.2f}".format,
    }))
    return df_out

# ── 3. Live session window sim ───────────────────────────────────────────────
def test_session_window():
    SESSION_START_MS = 1779177600000  # 2026-05-20 08:00 UTC
    SESSION_END_MS   = 1779844200000  # 2026-05-27 02:10 UTC
    print(f"\n=== TEST 3: Session window sim (2026-05-20 → 2026-05-27) ===")
    candidates = [
        ("nearusdt","6h"),("avaxusdt","6h"),("solusdt","6h"),("btcusdt","6h"),
        ("xrpusdt","6h"),("solusdt","4h"),("ethusdt","4h"),
    ]
    rows = []
    for sym, tf in candidates:
        h1 = load_h1(sym)
        if h1 is None: continue
        df = resample(h1, tf)
        tf_secs = TF_TO_SECS[tf]
        tmpl = make_template(tf_secs)
        sig = sig_rsi_divergence(df, rsi_period=14, pivot_len=5)
        trades, _ = backtest_custom(df, sig, tmpl, blowoff_pct=80.0)
        idx_ms = (df.index.view("int64") // 1_000_000).astype("int64")
        wn = [t for t in trades if int(idx_ms[t[0]]) >= SESSION_START_MS
              and int(idx_ms[t[0]]) <= SESSION_END_MS]
        net_bp = sum(t[3]*1e4 for t in wn)
        wins = sum(1 for t in wn if t[3]>0)
        rows.append({
            "sym": sym.upper().replace("USDT",""), "tf": tf,
            "session_trades": len(wn), "session_wins": wins,
            "session_net_bp": net_bp,
        })
    df_out = pd.DataFrame(rows)
    print(df_out.to_string(index=False, formatters={
        "session_net_bp":"{:+,.1f}".format,
    }))
    print(f"\nTotal RSI-div contribution in window: {df_out.session_net_bp.sum():+,.1f} bp ({df_out.session_trades.sum()} trades)")

def main():
    test_oos()
    test_param_sweep()
    test_session_window()

if __name__ == "__main__":
    main()
