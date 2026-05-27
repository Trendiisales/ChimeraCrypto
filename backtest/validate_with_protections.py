"""
validate_with_protections.py — Backtest with full production protection stack.

Mirrors EdgeEngine.hpp apply_safety_preset() defaults:
  hard_floor_bp     = -50.0    (max single-trade loss)
  early_kill_bp     = -25.0    (DOA cut if MFE < +15bp)
  early_kill_mfe    = +15.0
  ratchet_start_bp  = round_trip_bp
  be_arm_bp         = round_trip_bp + 10
  ratchet_lock_pct  = 0.75
  prog_lock_pct_2/3/4 = 0.85 / 0.90 / 0.95   (mfe 100-200 / 200-300 / 300+)
  giveback_arm_bp   = round_trip_bp + 20
  giveback_pct      = 0.15

Also applies blowoff guard (mom > threshold → skip entry).

Outputs per-engine deep stats: PF, Sharpe, MDD_bp, worst trade,
max losing streak, avg loser, recovery time.
"""
import json, sys, math, argparse
from pathlib import Path
import numpy as np
import pandas as pd

ROOT = Path("/Users/jo/ChimeraCrypto/backtest")
DATA = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
OUT  = ROOT / "validate_with_protections_results.csv"

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

def confirmation_signal(entries, kind, df, lb, k=None, period=None, thr=None, n_confirm=2):
    """Require N consecutive bars where signal would fire (S34 confirmation_bars=2)."""
    if n_confirm <= 1: return entries
    if kind in ("TSMOM",):
        # TSMOM: close[i] > close[i-lb]. Confirm bar i-1 also true.
        c = df.close.values
        prev = np.zeros(len(df), dtype=bool)
        if lb < len(df):
            prev[lb+1:] = c[lb+1:] > c[1:-lb]  # close[i-1] > close[i-1-lb]
        return entries & prev
    if kind in ("DONCHIAN",):
        # Donchian back-confirm only supported for these kinds in prod.
        c = df.close.values
        h = df.high.values
        if lb + 1 >= len(df): return entries
        # prior bar's close > prior 20-bar high (excluding prior bar)
        prior_h_prev = pd.Series(h).rolling(lb).max().shift(2).values
        prev = np.zeros(len(df), dtype=bool)
        idx_valid = ~np.isnan(prior_h_prev)
        prev[idx_valid] = c[np.arange(len(df))[idx_valid]-1] > prior_h_prev[idx_valid]
        return entries & prev
    return entries  # BOLLINGER, RSI_REVERT — confirmation unsupported

def backtest_engine(df, eng, blowoff_pct=None, use_protections=True, confirm_bars=2):
    """Run engine. With protection stack from apply_safety_preset()."""
    kind = eng["kind"]
    lb   = eng["lookback"]
    hold = eng["hold_bars"]
    sl_m = eng["sl_atr_mult"]
    ap   = eng["atr_period"]
    bbk  = eng["bb_k"]
    rsit = eng["rsi_threshold"]
    cost = eng["round_trip_bp"] / 10_000.0
    rt_bp = eng["round_trip_bp"]
    arm  = eng["trail_arm_atr"]
    tdist= eng["trail_dist_atr"]
    tarm = eng["trail_tighten_atr"]
    tdis2= eng["trail_tighten_dist_atr"]

    # Protection stack (apply_safety_preset)
    if use_protections:
        hard_floor_bp   = -50.0
        early_kill_bp   = -25.0
        early_kill_mfe  = 15.0
        ratchet_start_bp = rt_bp
        be_arm_bp        = rt_bp + 10.0
        ratchet_lock_pct = 0.75
        prog2 = 0.85; prog3 = 0.90; prog4 = 0.95
        giveback_arm_bp = rt_bp + 20.0
        giveback_pct    = 0.15
    else:
        hard_floor_bp = -1e6  # disabled
        early_kill_bp = -1e6
        early_kill_mfe = 1e6
        ratchet_start_bp = 1e6
        be_arm_bp = 1e6
        ratchet_lock_pct = 0
        prog2 = prog3 = prog4 = 0
        giveback_arm_bp = 1e9
        giveback_pct = 0

    if kind == "TSMOM":         entries = sig_tsmom(df, lb)
    elif kind == "DONCHIAN":    entries = sig_donchian(df, lb)
    elif kind == "BOLLINGER":   entries = sig_bollinger(df, lb, bbk)
    elif kind == "RSI_REVERT":  entries = sig_rsi_revert(df, ap, rsit)
    else: return None, 0

    if use_protections and confirm_bars >= 2:
        entries = confirmation_signal(entries, kind, df, lb, n_confirm=confirm_bars)

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
    peak_mfe_bp = 0.0
    exit_reason = ""

    for i in range(1, n):
        if in_pos:
            bars_held += 1
            high_since = max(high_since, h[i])
            # Use intra-bar high for MFE estimate; trail and ratchet check intra-bar lows.
            mfe = high_since - entry_px
            mfe_bp = (high_since / entry_px - 1.0) * 1e4
            peak_mfe_bp = max(peak_mfe_bp, mfe_bp)

            # ATR trail
            if not trail_armed and mfe >= arm * entry_atr:
                trail_armed = True
            if trail_armed:
                dist = tdis2 if (tarm > 0 and mfe >= tarm * entry_atr) else tdist
                trail_stop = max(trail_stop, high_since - dist * entry_atr)

            # BP-based staged ratchet floor (computed each bar; intra-bar low triggers)
            if peak_mfe_bp < ratchet_start_bp:
                # Stage 1: hard floor
                ratchet_px = entry_px * (1.0 + hard_floor_bp / 1e4)
            elif peak_mfe_bp < be_arm_bp:
                # Stage 2: linear ramp from hard_floor to 0 across [ratchet_start, be_arm]
                frac = (peak_mfe_bp - ratchet_start_bp) / max(1e-9, be_arm_bp - ratchet_start_bp)
                lock_bp = hard_floor_bp * (1.0 - frac)  # ramps from hard_floor → 0
                ratchet_px = entry_px * (1.0 + lock_bp / 1e4)
            else:
                # Stage 3: locked = rt_bp + (mfe - be_arm) * lock_pct (progressive)
                if peak_mfe_bp < 100:
                    lp = ratchet_lock_pct
                elif peak_mfe_bp < 200:
                    lp = prog2
                elif peak_mfe_bp < 300:
                    lp = prog3
                else:
                    lp = prog4
                lock_bp = rt_bp + (peak_mfe_bp - be_arm_bp) * lp
                ratchet_px = entry_px * (1.0 + lock_bp / 1e4)

            # Effective stop = max of all protection stops
            stop = sl_px
            if trail_armed: stop = max(stop, trail_stop)
            stop = max(stop, ratchet_px)

            exit_px = None
            # Early kill (DOA): unrealised at bar's low < early_kill_bp AND mfe never crossed early_kill_mfe
            cur_low_bp = (l[i] / entry_px - 1.0) * 1e4
            if peak_mfe_bp < early_kill_mfe and cur_low_bp < early_kill_bp:
                # exit at early_kill price level
                exit_px = entry_px * (1.0 + early_kill_bp / 1e4)
                exit_reason = "EARLY_KILL"
            # Giveback (only after MFE crossed arm threshold)
            if exit_px is None and peak_mfe_bp >= giveback_arm_bp:
                giveback_trigger_bp = peak_mfe_bp * (1.0 - giveback_pct)
                # if intra-bar drop below this -> exit
                if cur_low_bp <= giveback_trigger_bp:
                    exit_px = entry_px * (1.0 + giveback_trigger_bp / 1e4)
                    exit_reason = "GIVEBACK"
            # Stop hit
            if exit_px is None and l[i] <= stop:
                exit_px = stop
                exit_reason = "SL" if not trail_armed else "TRAIL"
            # Time exit
            if exit_px is None and bars_held >= hold:
                exit_px = c[i]
                exit_reason = "TIME"

            if exit_px is not None:
                gross = (exit_px - entry_px) / entry_px
                net = gross - cost
                trades.append((entry_i, i, gross, net, bars_held, peak_mfe_bp, exit_reason))
                in_pos = False; trail_armed = False; trail_stop = 0.0; peak_mfe_bp = 0.0
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
            peak_mfe_bp = 0.0
    return trades, blowoff_skips

def metrics(trades, n_bars, bars_per_year):
    if not trades:
        return None
    nets = np.array([t[3] for t in trades])
    nets_bp = nets * 1e4
    wins = nets[nets > 0]
    losses = nets[nets <= 0]
    pf = (wins.sum() / -losses.sum()) if losses.sum() < 0 else float("inf")
    win_rate = len(wins) / len(nets)
    avg = nets.mean()
    sd = nets.std()
    eq = np.cumprod(1 + nets)
    peak = np.maximum.accumulate(eq)
    mdd_cmp = (eq / peak - 1).min()
    # bp-based fixed-size cumulative
    cum_bp = np.cumsum(nets_bp)
    peak_bp = np.maximum.accumulate(cum_bp)
    mdd_bp = (cum_bp - peak_bp).min()
    bars_in_trade = sum(t[4] for t in trades)
    expo = bars_in_trade / n_bars
    yrs = n_bars / bars_per_year
    cagr = eq[-1] ** (1/yrs) - 1 if eq[-1] > 0 else -1
    tpy = len(trades) / yrs
    sharpe = (avg / sd) * math.sqrt(tpy) if sd > 0 else 0
    # Worst single trade in bp
    worst_bp = nets_bp.min()
    # Worst losing streak (count)
    streak = max_streak = 0
    for x in nets_bp:
        if x <= 0: streak += 1; max_streak = max(max_streak, streak)
        else: streak = 0
    # Max consecutive loss bp (sum)
    max_run_loss_bp = 0
    cur = 0
    for x in nets_bp:
        cur = min(0, cur + x) if x <= 0 else 0
        max_run_loss_bp = min(max_run_loss_bp, cur)
    avg_loss_bp = losses.mean() * 1e4 if len(losses) else 0
    avg_win_bp = wins.mean() * 1e4 if len(wins) else 0
    # Exit reason breakdown
    reasons = {}
    for t in trades:
        r = t[6]; reasons[r] = reasons.get(r, 0) + 1
    # Recovery time (bars to recover from MDD)
    dd_bar_count = 0
    return dict(
        trades=len(trades), pf=pf, win=win_rate, sharpe=sharpe,
        mdd_cmp=mdd_cmp, mdd_bp=mdd_bp, cum_bp=cum_bp[-1],
        cagr=cagr, expo=expo, final=eq[-1],
        avg_loss_bp=avg_loss_bp, avg_win_bp=avg_win_bp, worst_bp=worst_bp,
        max_loss_streak=max_streak, max_run_loss_bp=max_run_loss_bp,
        n_sl=reasons.get("SL",0)+reasons.get("TRAIL",0),
        n_time=reasons.get("TIME",0),
        n_kill=reasons.get("EARLY_KILL",0),
        n_giveback=reasons.get("GIVEBACK",0),
    )

def passes(m):
    if m is None: return False
    pf = m["pf"]
    if math.isinf(pf): pf = 999
    return pf >= 1.3 and m["sharpe"] >= 1.0 and m["trades"] >= 50

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--threshold", type=float, default=80.0)
    ap.add_argument("--source", choices=["disabled","active"], default="disabled")
    args = ap.parse_args()

    src = ROOT / ("disabled_engines.json" if args.source=="disabled" else "engines.json")
    engines = json.loads(src.read_text())
    print(f"loaded {len(engines)} engines", file=sys.stderr)

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

        # With FULL production protections + blowoff guard
        t_prot, skips = backtest_engine(df, e, blowoff_pct=args.threshold,
                                         use_protections=True, confirm_bars=2)
        m = metrics(t_prot, len(df), bars_per_year)

        def col(k, default=0): return (m.get(k, default) if m else default)
        rows.append(dict(
            tag=e["tag"], symbol=e["symbol"], kind=e["kind"], tf=rule,
            n_bars=len(df), blowoff_skips=skips,
            trades=col("trades"), pf=col("pf"), sharpe=col("sharpe"),
            win=col("win"), mdd_cmp=col("mdd_cmp"), mdd_bp=col("mdd_bp"),
            cum_bp=col("cum_bp"), cagr=col("cagr"),
            avg_loss_bp=col("avg_loss_bp"), avg_win_bp=col("avg_win_bp"),
            worst_bp=col("worst_bp"), max_loss_streak=col("max_loss_streak"),
            max_run_loss_bp=col("max_run_loss_bp"),
            n_sl=col("n_sl"), n_time=col("n_time"),
            n_kill=col("n_kill"), n_giveback=col("n_giveback"),
            passes=passes(m),
        ))

    df_out = pd.DataFrame(rows)
    df_out["pf"] = df_out["pf"].replace([np.inf], 999)
    df_out.to_csv(OUT, index=False)

    surv = df_out[df_out.passes].sort_values("sharpe", ascending=False)
    print(f"\n=== WITH FULL PROTECTIONS (blowoff={args.threshold}%, confirm=2 bars) ===")
    print(f"Total tested : {len(df_out)}")
    print(f"Pass criteria: PF>=1.3, Sharpe>=1.0, trades>=50")
    print(f"Survivors    : {len(surv)}")
    print()
    if len(surv) == 0: return
    cols = ["tag","tf","trades","pf","sharpe","mdd_bp","cum_bp",
            "worst_bp","avg_loss_bp","avg_win_bp","max_loss_streak","max_run_loss_bp",
            "n_sl","n_time","n_kill","n_giveback"]
    print(surv[cols].head(30).to_string(index=False, formatters={
        "pf":"{:.2f}".format,"sharpe":"{:.2f}".format,
        "mdd_bp":"{:,.0f}".format,"cum_bp":"{:,.0f}".format,
        "worst_bp":"{:,.0f}".format,"avg_loss_bp":"{:,.0f}".format,
        "avg_win_bp":"{:,.0f}".format,"max_run_loss_bp":"{:,.0f}".format,
    }))

if __name__ == "__main__":
    main()
