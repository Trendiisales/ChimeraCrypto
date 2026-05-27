"""
validate_disabled_blowoff.py — Re-test disabled engines with BLOWOFF GUARD.

Compares each disabled engine's 5yr backtest in two modes:
  baseline: no extra filter (mirrors original validator)
  blowoff:  skip entries when momentum_pct(close vs close[-lookback]) > threshold

Threshold mirrors backend constant in main.cpp: BLOWOFF_THRESHOLD_PCT = 80.

Pass criteria (same as batch_validate.py):
  Sharpe >= 1.0
  PF     >= 1.3
  MDD    >= -30%
  trades >= 50

Usage:
  python3 validate_disabled_blowoff.py [--threshold 80] [--source disabled]
    --source disabled  -> read disabled_engines.json (default)
    --source active    -> read engines.json (sanity check on live roster)
"""
import json, sys, math, argparse
from pathlib import Path
import numpy as np
import pandas as pd

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")
DATA = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
OUT  = ROOT / "validate_disabled_blowoff_results.csv"

SUPPORTED = {"TSMOM", "DONCHIAN", "BOLLINGER", "RSI_REVERT"}
TF_MAP = {3600:"1h", 7200:"2h", 10800:"3h", 14400:"4h", 21600:"6h", 28800:"8h",
          43200:"12h", 57600:"16h", 86400:"1D", 172800:"2D", 259200:"3D"}

_h1_cache = {}
def load_h1(sym):
    if sym in _h1_cache: return _h1_cache[sym]
    p = DATA / f"{sym.upper()}_1h_extended.csv"
    if not p.exists():
        _h1_cache[sym] = None; return None
    df = pd.read_csv(p, usecols=["open_time_ms","open","high","low","close"])
    df["ts"] = pd.to_datetime(df.open_time_ms, unit="ms", utc=True)
    df = df.set_index("ts").drop(columns=["open_time_ms"]).sort_index()
    _h1_cache[sym] = df
    return df

def resample(h1, rule):
    o = h1["open"].resample(rule).first()
    h = h1["high"].resample(rule).max()
    l = h1["low"].resample(rule).min()
    c = h1["close"].resample(rule).last()
    out = pd.concat([o,h,l,c], axis=1).dropna()
    out.columns = ["open","high","low","close"]
    return out

def rsi(close, n):
    d = close.diff()
    up = d.clip(lower=0).ewm(alpha=1/n, adjust=False).mean()
    dn = (-d.clip(upper=0)).ewm(alpha=1/n, adjust=False).mean()
    rs = up / dn.replace(0, np.nan)
    return 100 - 100 / (1 + rs)

def atr(df, n):
    h, l, c = df.high, df.low, df.close
    pc = c.shift(1)
    tr = pd.concat([(h-l),(h-pc).abs(),(l-pc).abs()], axis=1).max(axis=1)
    return tr.ewm(alpha=1/n, adjust=False).mean()

def bb_lower(close, n, k):
    m = close.rolling(n).mean()
    s = close.rolling(n).std(ddof=0)
    return m - k * s

def sig_tsmom(df, lb):
    c = df.close.values
    out = np.zeros(len(df), dtype=bool)
    out[lb:] = c[lb:] > c[:-lb]
    return out

def sig_donchian(df, lb):
    h = df.high.values; c = df.close.values
    if lb >= len(df): return np.zeros(len(df), dtype=bool)
    prior_high = pd.Series(h).rolling(lb).max().shift(1).values
    out = (c > prior_high) & ~np.isnan(prior_high)
    return out

def sig_bollinger(df, lb, k):
    low = df.low.values; cls = df.close.values
    lower = bb_lower(df.close, lb, k).values
    return (low <= lower) & (cls > lower) & ~np.isnan(lower)

def sig_rsi_revert(df, period, thr):
    r = rsi(df.close, period).values
    out = np.zeros(len(df), dtype=bool)
    out[1:] = (r[:-1] <= thr) & (r[1:] > thr) & ~np.isnan(r[:-1]) & ~np.isnan(r[1:])
    return out

def backtest_engine(df, eng, blowoff_pct=None):
    """Run engine. If blowoff_pct given, skip entries where 100*(c[i]/c[i-lb] - 1) > blowoff_pct."""
    kind = eng["kind"]
    lb   = eng["lookback"]
    hold = eng["hold_bars"]
    sl_m = eng["sl_atr_mult"]
    ap   = eng["atr_period"]
    bbk  = eng["bb_k"]
    rsit = eng["rsi_threshold"]
    cost = eng["round_trip_bp"] / 10_000.0
    arm  = eng["trail_arm_atr"]
    tdist= eng["trail_dist_atr"]
    tarm = eng["trail_tighten_atr"]
    tdis2= eng["trail_tighten_dist_atr"]
    if kind == "TSMOM":         entries = sig_tsmom(df, lb)
    elif kind == "DONCHIAN":    entries = sig_donchian(df, lb)
    elif kind == "BOLLINGER":   entries = sig_bollinger(df, lb, bbk)
    elif kind == "RSI_REVERT":  entries = sig_rsi_revert(df, ap, rsit)
    else: return None, 0
    a = atr(df, ap).values
    c = df.close.values; h = df.high.values; l = df.low.values
    n = len(df)
    blowoff_skips = 0
    trades = []
    in_pos = False
    entry_px = entry_atr = sl_px = trail_stop = 0.0
    bars_held = 0
    high_since = 0.0
    trail_armed = False
    entry_i = 0
    for i in range(1, n):
        if in_pos:
            bars_held += 1
            high_since = max(high_since, h[i])
            mfe = high_since - entry_px
            if not trail_armed and mfe >= arm * entry_atr:
                trail_armed = True
            if trail_armed:
                dist = tdis2 if (tarm > 0 and mfe >= tarm * entry_atr) else tdist
                trail_stop = max(trail_stop, high_since - dist * entry_atr)
            stop = max(sl_px, trail_stop) if trail_armed else sl_px
            exit_px = None
            if l[i] <= stop:
                exit_px = stop
            elif bars_held >= hold:
                exit_px = c[i]
            if exit_px is not None:
                gross = (exit_px - entry_px) / entry_px
                net = gross - cost
                trades.append((entry_i, i, gross, net, bars_held))
                in_pos = False; trail_armed = False; trail_stop = 0.0
            continue
        if entries[i] and not np.isnan(a[i]) and a[i] > 0:
            if blowoff_pct is not None and i >= lb:
                lb_close = c[i - lb]
                if lb_close > 0:
                    mom = (c[i] / lb_close - 1.0) * 100.0
                    if mom > blowoff_pct:
                        blowoff_skips += 1
                        continue
            in_pos = True
            entry_px = c[i]
            entry_atr = a[i]
            sl_px = entry_px - sl_m * entry_atr
            high_since = entry_px
            bars_held = 0
            entry_i = i
    return trades, blowoff_skips

def metrics(trades, n_bars, bars_per_year):
    if not trades:
        return None
    nets = np.array([t[3] for t in trades])
    wins = nets[nets > 0]
    losses = nets[nets <= 0]
    pf = (wins.sum() / -losses.sum()) if losses.sum() < 0 else float("inf")
    win_rate = len(wins) / len(nets)
    avg = nets.mean()
    sd = nets.std()
    # Compound equity (sized as % of equity each trade).
    eq = np.cumprod(1 + nets)
    peak = np.maximum.accumulate(eq)
    mdd_cmp = (eq / peak - 1).min()
    # Fixed-size bp equity (mirrors production: each trade sized off fixed capital).
    bp = nets * 10_000.0
    cum_bp = np.cumsum(bp)
    peak_bp = np.maximum.accumulate(cum_bp)
    mdd_bp = (cum_bp - peak_bp).min()
    bars_in_trade = sum(t[4] for t in trades)
    expo = bars_in_trade / n_bars
    yrs = n_bars / bars_per_year
    cagr = eq[-1] ** (1/yrs) - 1 if eq[-1] > 0 else -1
    tpy = len(trades) / yrs
    sharpe = (avg / sd) * math.sqrt(tpy) if sd > 0 else 0
    return dict(trades=len(trades), pf=pf, win=win_rate, avg=avg,
                sharpe=sharpe, mdd=mdd_cmp, mdd_bp=mdd_bp, cum_bp=cum_bp[-1],
                cagr=cagr, expo=expo, final=eq[-1])

def passes(m):
    if m is None: return False
    pf = m["pf"]
    if math.isinf(pf): pf = 999
    return m["sharpe"] >= 1.0 and pf >= 1.3 and m["mdd"] >= -0.30 and m["trades"] >= 50

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--threshold", type=float, default=80.0)
    ap.add_argument("--source", choices=["disabled","active"], default="disabled")
    args = ap.parse_args()

    src = ROOT / ("disabled_engines.json" if args.source == "disabled" else "engines.json")
    engines = json.loads(src.read_text())
    print(f"loaded {len(engines)} engines from {src.name}", file=sys.stderr)
    print(f"blowoff threshold = {args.threshold}%", file=sys.stderr)

    rows = []
    for e in engines:
        if e["kind"] not in SUPPORTED: continue
        if not e["symbol"] or not e["tf_secs"]: continue
        rule = TF_MAP.get(e["tf_secs"])
        if not rule: continue
        h1 = load_h1(e["symbol"])
        if h1 is None: continue
        df = resample(h1, rule)
        if len(df) < 100: continue
        bars_per_year = (365 * 24 * 3600) / e["tf_secs"]

        t_base, _   = backtest_engine(df, e, blowoff_pct=None)
        t_blow, skips = backtest_engine(df, e, blowoff_pct=args.threshold)
        m_base = metrics(t_base, len(df), bars_per_year)
        m_blow = metrics(t_blow, len(df), bars_per_year)

        def col(m, k, default=0):
            return (m.get(k, default) if m else default)

        rows.append(dict(
            tag=e["tag"], symbol=e["symbol"], kind=e["kind"], tf=rule,
            n_bars=len(df), blowoff_skips=skips,
            base_trades=col(m_base,"trades"), base_pf=col(m_base,"pf"),
            base_win=col(m_base,"win"), base_sharpe=col(m_base,"sharpe"),
            base_mdd=col(m_base,"mdd"), base_mdd_bp=col(m_base,"mdd_bp"),
            base_cum_bp=col(m_base,"cum_bp"), base_cagr=col(m_base,"cagr"),
            base_pass=passes(m_base),
            blow_trades=col(m_blow,"trades"), blow_pf=col(m_blow,"pf"),
            blow_win=col(m_blow,"win"), blow_sharpe=col(m_blow,"sharpe"),
            blow_mdd=col(m_blow,"mdd"), blow_mdd_bp=col(m_blow,"mdd_bp"),
            blow_cum_bp=col(m_blow,"cum_bp"), blow_cagr=col(m_blow,"cagr"),
            blow_pass=passes(m_blow),
        ))

    df_out = pd.DataFrame(rows)
    # replace inf pf for display
    for col in ("base_pf","blow_pf"):
        df_out[col] = df_out[col].replace([np.inf], 999)
    df_out.to_csv(OUT, index=False)
    print(f"wrote {len(df_out)} rows -> {OUT}", file=sys.stderr)
    print()

    base_pass = df_out[df_out.base_pass]
    blow_pass = df_out[df_out.blow_pass]
    blow_only = df_out[df_out.blow_pass & ~df_out.base_pass]
    both_pass = df_out[df_out.blow_pass & df_out.base_pass]
    blow_fail_base_pass = df_out[df_out.base_pass & ~df_out.blow_pass]

    print(f"Total engines tested        : {len(df_out)}")
    print(f"Pass baseline (no filter)   : {len(base_pass)}")
    print(f"Pass blowoff (mom <= {int(args.threshold)}%) : {len(blow_pass)}")
    print(f"Newly survives w/ blowoff   : {len(blow_only)}")
    print(f"Survives in both modes      : {len(both_pass)}")
    print(f"Lost survival under blowoff : {len(blow_fail_base_pass)}")
    print()

    def show(title, sub):
        if len(sub) == 0:
            print(f"{title}: none\n"); return
        print(f"{title} ({len(sub)}):")
        cols = ["tag","symbol","tf","blowoff_skips",
                "base_trades","base_pf","base_sharpe","base_mdd","base_cagr","base_pass",
                "blow_trades","blow_pf","blow_sharpe","blow_mdd","blow_cagr","blow_pass"]
        print(sub.sort_values("blow_sharpe", ascending=False)[cols].to_string(index=False,
              formatters={"base_pf":"{:.2f}".format, "blow_pf":"{:.2f}".format,
                          "base_sharpe":"{:.2f}".format, "blow_sharpe":"{:.2f}".format,
                          "base_mdd":"{:.2%}".format, "blow_mdd":"{:.2%}".format,
                          "base_cagr":"{:.2%}".format, "blow_cagr":"{:.2%}".format,
                          "base_win":"{:.2%}".format, "blow_win":"{:.2%}".format,}))
        print()

    show("NEW SURVIVORS (only with blowoff guard)", blow_only)
    show("SURVIVES IN BOTH MODES", both_pass)
    show("LOST SURVIVAL AFTER BLOWOFF", blow_fail_base_pass)

if __name__ == "__main__":
    main()
