"""
cap_sweep.py — Simulate live session window with different MAX_PER_SYMBOL caps.

Picks: all NEAR + SOL + LINK + AVAX + SUI engines that pass validation,
plus the 4 newly-revived. Apply concurrent-position cap per symbol.
"""
import json, sys, math
from pathlib import Path
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, backtest_engine, TF_MAP
)

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")
SESSION_START_MS = 1779177600000
SESSION_END_MS   = 1779844200000

# All active+revived NEAR/SOL/LINK/SUI/AVAX engines that survived validation
ENGINES = [
    "NEAR-TSMOM-H4","NEAR-TSMOM-H8","NEAR-TSMOM-H12","NEAR-DONCH-H8","NEAR-DONCH-H12","NEAR-DONCH-H16",
    "SOL-TSMOM-H4","SOL-TSMOM-H6","SOL-DONCH-H16",
    "LINK-TSMOM-H6","LINK-TSMOM-H8","LINK-TSMOM-H12","LINK-DONCH-H16",
    "AVAX-TSMOM-H6","AVAX-TSMOM-H8",
    "SUI-TSMOM-H4","SUI-TSMOM-H8",
]

def main():
    engines_all = (json.loads((ROOT/"engines.json").read_text())
                 + json.loads((ROOT/"disabled_engines.json").read_text()))
    by_tag = {e["tag"]: e for e in engines_all}

    # Collect per-engine trades in window: (entry_ms, exit_ms, net_bp, tag, symbol)
    all_trades = []
    for tag in ENGINES:
        e = by_tag.get(tag)
        if not e: continue
        rule = TF_MAP.get(e["tf_secs"])
        h1 = load_h1(e["symbol"])
        if h1 is None: continue
        df = resample(h1, rule)
        trades, _ = backtest_engine(df, e, blowoff_pct=80.0, use_protections=True, confirm_bars=2)
        idx_ms = (df.index.view("int64") // 1_000_000).astype("int64")
        for t in trades:
            entry_ms = int(idx_ms[t[0]])
            exit_ms = int(idx_ms[t[1]])
            if entry_ms >= SESSION_START_MS and entry_ms <= SESSION_END_MS:
                all_trades.append({
                    "entry_ms": entry_ms, "exit_ms": exit_ms,
                    "net_bp": t[3] * 1e4, "peak_mfe_bp": t[5],
                    "tag": tag, "symbol": e["symbol"], "reason": t[6],
                })

    df = pd.DataFrame(all_trades).sort_values("entry_ms").reset_index(drop=True)

    def apply_cap(cap):
        """Walk trades chronologically. Skip if symbol already has `cap` open positions."""
        open_by_sym = {}
        accepted = []
        for _, t in df.iterrows():
            sym = t.symbol; ent = t.entry_ms; exi = t.exit_ms
            # close any positions on this symbol whose exit_ms <= entry_ms
            open_by_sym[sym] = [x for x in open_by_sym.get(sym, []) if x > ent]
            if len(open_by_sym[sym]) >= cap:
                continue
            open_by_sym.setdefault(sym, []).append(exi)
            accepted.append(t.to_dict())
        return pd.DataFrame(accepted)

    print(f"Total signal-firings in window (no cap): {len(df)}")
    print(f"Total raw net_bp standalone           : {df.net_bp.sum():+,.1f}\n")

    rows = []
    for cap in [1, 2, 3, 5, 99]:
        acc = apply_cap(cap)
        tot = acc.net_bp.sum()
        # max bp lost in single bar (cluster risk)
        cluster_loss_by_bar = acc.groupby("entry_ms").net_bp.sum()
        worst_cluster = cluster_loss_by_bar.min() if len(cluster_loss_by_bar) else 0
        wins = (acc.net_bp > 0).sum()
        rows.append({
            "cap": cap, "trades": len(acc), "wins": wins,
            "win_rate": wins / max(1, len(acc)),
            "net_bp": tot, "worst_cluster_bp": worst_cluster,
            "biggest_single_loss_bp": acc.net_bp.min() if len(acc) else 0,
        })

    out = pd.DataFrame(rows)
    print("=== MAX_PER_SYMBOL sweep over current session window ===")
    print(out.to_string(index=False, formatters={
        "net_bp": "{:+,.1f}".format,
        "worst_cluster_bp": "{:+,.1f}".format,
        "biggest_single_loss_bp": "{:+,.1f}".format,
        "win_rate": "{:.1%}".format,
    }))
    print()

    # Show NEAR-specific overlap clusters
    near = df[df.symbol == "nearusdt"].copy()
    cluster_bp = near.groupby("entry_ms").agg(
        n=("tag","count"), bp_sum=("net_bp","sum"), tags=("tag",lambda x:",".join(x))
    ).sort_values("entry_ms")
    print(f"=== NEAR bar-level concurrency clusters (top 15 by trade count) ===")
    big_clusters = cluster_bp[cluster_bp.n >= 2].sort_values("n", ascending=False).head(15)
    big_clusters["ts"] = [pd.Timestamp(ms, unit="ms", tz="UTC").strftime("%m-%d %H:%M") for ms in big_clusters.index]
    print(big_clusters[["ts","n","bp_sum","tags"]].to_string(index=False, formatters={"bp_sum":"{:+,.1f}".format}))

if __name__ == "__main__":
    main()
