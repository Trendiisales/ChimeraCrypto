"""
session_what_if.py — Simulate Tier-1/2 picks over current live session window.

Window: 2026-05-20 08:00 UTC → 2026-05-27 02:00 UTC (from trade log).
Applies same full protections + blowoff guard as validate_with_protections.py.
Reports cum_bp + trade detail per engine.
"""
import json, sys, math
from pathlib import Path
import numpy as np
import pandas as pd
sys.path.insert(0, str(Path(__file__).parent))
from validate_with_protections import (
    load_h1, resample, backtest_engine, metrics, TF_MAP
)

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")
SESSION_START_MS = 1779177600000  # 2026-05-20 08:00 UTC (first trade entry)
SESSION_END_MS   = 1779844200000  # 2026-05-27 02:10 UTC (latest data we have)

PICKS = [
    "LINK-TSMOM-H8", "SOL-TSMOM-H4", "SUI-TSMOM-H4", "NEAR-TSMOM-H4",
    # also include the 6 already-active picks for reference
    "NEAR-TSMOM-H8", "NEAR-TSMOM-H12", "NEAR-DONCH-H8",
    "SOL-TSMOM-H6", "AVAX-TSMOM-H6", "LINK-TSMOM-H6",
]

def fmt_ts(ms): return pd.Timestamp(ms, unit="ms", tz="UTC").strftime("%m-%d %H:%M")

def main():
    engines = json.loads((ROOT / "engines.json").read_text())
    # disabled_engines.json also has the picks since some were disabled
    disabled = json.loads((ROOT / "disabled_engines.json").read_text())
    all_engines = {e["tag"]: e for e in engines + disabled}

    print(f"Window: {fmt_ts(SESSION_START_MS)} → {fmt_ts(SESSION_END_MS)} UTC\n")
    rows = []
    all_trades = []
    for tag in PICKS:
        e = all_engines.get(tag)
        if not e:
            print(f"  {tag}: cfg not found"); continue
        rule = TF_MAP.get(e["tf_secs"])
        h1 = load_h1(e["symbol"])
        if h1 is None: continue
        # Resample full history (to warm-up indicators), THEN filter window for trades
        df_full = resample(h1, rule)
        # Run full backtest, then filter trades to window
        # Index of bars whose bar-close ts >= SESSION_START
        # backtest_engine works on bar arrays — easiest: run full, then post-filter trades.
        trades, _ = backtest_engine(df_full, e, blowoff_pct=80.0,
                                     use_protections=True, confirm_bars=2)
        # trades: (entry_i, exit_i, gross, net, bars_held, peak_mfe_bp, reason)
        # bar at index i has close-time ms = df_full.index[i].value//1_000_000 + tf_secs*1000
        idx_ms = (df_full.index.view("int64") // 1_000_000).astype("int64")
        window_trades = []
        for t in trades:
            entry_ms = int(idx_ms[t[0]])
            exit_ms  = int(idx_ms[t[1]])
            if entry_ms >= SESSION_START_MS and entry_ms <= SESSION_END_MS:
                window_trades.append((t, entry_ms, exit_ms))
        net_bp_sum = sum(t[3] * 1e4 for t, _, _ in window_trades)
        n = len(window_trades)
        wins = sum(1 for t, _, _ in window_trades if t[3] > 0)
        rows.append(dict(tag=tag, tf=rule, window_trades=n, window_wins=wins,
                         window_net_bp=net_bp_sum,
                         was_active_pre=tag in {"NEAR-TSMOM-H8","NEAR-TSMOM-H12","NEAR-DONCH-H8",
                                                "SOL-TSMOM-H6","AVAX-TSMOM-H6","LINK-TSMOM-H6"}))
        for t, ent_ms, exi_ms in window_trades:
            all_trades.append(dict(tag=tag, entry=fmt_ts(ent_ms), exit=fmt_ts(exi_ms),
                                    net_bp=t[3]*1e4, peak_mfe_bp=t[5], reason=t[6]))

    df_sum = pd.DataFrame(rows)
    df_trades = pd.DataFrame(all_trades).sort_values("entry") if all_trades else pd.DataFrame()

    print("=== Per-engine contribution over live session window ===")
    print(df_sum.to_string(index=False, formatters={
        "window_net_bp": "{:+,.1f}".format,
    }))
    print()
    new_only = df_sum[~df_sum.was_active_pre]
    already_active = df_sum[df_sum.was_active_pre]
    print(f"NEWLY-REVIVED 4 contribution : {new_only.window_net_bp.sum():+,.1f} bp ({new_only.window_trades.sum()} trades)")
    print(f"ALREADY-ACTIVE 6 reference   : {already_active.window_net_bp.sum():+,.1f} bp ({already_active.window_trades.sum()} trades)")
    print()
    print("=== Per-trade detail in window ===")
    if len(df_trades):
        print(df_trades.to_string(index=False, formatters={
            "net_bp": "{:+.1f}".format, "peak_mfe_bp": "{:+.1f}".format,
        }))

if __name__ == "__main__":
    main()
