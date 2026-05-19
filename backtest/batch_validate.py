"""
Batch-validate all parsed engines on 5yr Binance H1 history.

Strategies implemented (long-only, matches include/core/EdgeEngine.hpp):
  TSMOM      close > close[-lookback]
  DONCHIAN   close > prior N-bar high (excl current)
  BOLLINGER  low <= lower BB(lookback, bb_k) AND close > lower BB
  RSI_REVERT RSI(atr_period) crosses up through rsi_threshold

Exit logic (shared):
  Initial SL: entry - sl_atr_mult * ATR
  Trail: arm at MFE >= trail_arm_atr * ATR, dist = trail_dist_atr * ATR
         tighten at MFE >= trail_tighten_atr * ATR (if > 0), dist = trail_tighten_dist_atr * ATR
  Time exit at bars_held >= hold_bars
  Cost: round_trip_bp basis points
"""
import json, sys, math
from pathlib import Path
import numpy as np
import pandas as pd

ENG = Path("/Users/jo/ChimeraCrypto/backtest/engines.json")
DATA = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
OUT  = Path("/Users/jo/ChimeraCrypto/backtest/batch_validate_results.csv")

SUPPORTED = {"TSMOM", "DONCHIAN", "BOLLINGER", "RSI_REVERT"}
TF_MAP = {3600:"1h", 7200:"2h", 10800:"3h", 14400:"4h", 21600:"6h", 28800:"8h",
          43200:"12h", 57600:"16h", 86400:"1D", 172800:"2D", 259200:"3D"}

# ── data loader cache ────────────────────────────────────────────────────────
_h1_cache = {}
def load_h1(sym_lower):
    if sym_lower in _h1_cache: return _h1_cache[sym_lower]
    sym = sym_lower.upper()
    p = DATA / f"{sym}_1h_extended.csv"
    if not p.exists():
        _h1_cache[sym_lower] = None; return None
    df = pd.read_csv(p, usecols=["open_time_ms","open","high","low","close","volume"])
    df["ts"] = pd.to_datetime(df["open_time_ms"], unit="ms", utc=True)
    df = df.set_index("ts").drop(columns=["open_time_ms"]).sort_index()
    _h1_cache[sym_lower] = df
    return df

def resample(h1, rule):
    o = h1["open"].resample(rule).first()
    h = h1["high"].resample(rule).max()
    l = h1["low"].resample(rule).min()
    c = h1["close"].resample(rule).last()
    out = pd.concat([o,h,l,c], axis=1).dropna()
    out.columns = ["open","high","low","close"]
    return out

# ── indicators ───────────────────────────────────────────────────────────────
def rsi(close, n):
    d = close.diff()
    up = d.clip(lower=0).ewm(alpha=1/n, adjust=False).mean()
    dn = (-d.clip(upper=0)).ewm(alpha=1/n, adjust=False).mean()
    rs = up / dn.replace(0, np.nan)
    return 100 - 100 / (1 + rs)

def atr(df, n):
    h, l, c = df["high"], df["low"], df["close"]
    pc = c.shift(1)
    tr = pd.concat([(h-l),(h-pc).abs(),(l-pc).abs()], axis=1).max(axis=1)
    return tr.ewm(alpha=1/n, adjust=False).mean()

def bb_lower(close, n, k):
    m = close.rolling(n).mean()
    s = close.rolling(n).std(ddof=0)
    return m - k * s

# ── signal arrays (bool per bar, evaluated on close[i]) ──────────────────────
def sig_tsmom(df, lb):
    c = df["close"].values
    out = np.zeros(len(df), dtype=bool)
    out[lb:] = c[lb:] > c[:-lb]
    return out

def sig_donchian(df, lb):
    h = df["high"].values
    c = df["close"].values
    out = np.zeros(len(df), dtype=bool)
    # prior N-bar high excludes current bar -> rolling on [i-lb, i-1]
    if lb >= len(df): return out
    prior_high = pd.Series(h).rolling(lb).max().shift(1).values
    out = (c > prior_high) & ~np.isnan(prior_high)
    return out

def sig_bollinger(df, lb, k):
    low = df["low"].values
    cls = df["close"].values
    lower = bb_lower(df["close"], lb, k).values
    out = (low <= lower) & (cls > lower) & ~np.isnan(lower)
    return out

def sig_rsi_revert(df, period, thr):
    r = rsi(df["close"], period).values
    out = np.zeros(len(df), dtype=bool)
    out[1:] = (r[:-1] <= thr) & (r[1:] > thr) & ~np.isnan(r[:-1]) & ~np.isnan(r[1:])
    return out

# ── exec model ───────────────────────────────────────────────────────────────
def backtest_engine(df, eng):
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
    else: return None
    a = atr(df, ap).values
    o = df["open"].values; h = df["high"].values; l = df["low"].values; c = df["close"].values
    n = len(df)
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
            in_pos = True
            entry_px = c[i]
            entry_atr = a[i]
            sl_px = entry_px - sl_m * entry_atr
            high_since = entry_px
            bars_held = 0
            entry_i = i
    return trades

def metrics(trades, n_bars, bars_per_year):
    if not trades or len(trades) < 5:
        return None
    nets = np.array([t[3] for t in trades])
    wins = nets[nets > 0]
    losses = nets[nets <= 0]
    pf = (wins.sum() / -losses.sum()) if losses.sum() < 0 else float("inf")
    win_rate = len(wins) / len(nets)
    avg = nets.mean()
    sd = nets.std()
    eq = np.cumprod(1 + nets)
    peak = np.maximum.accumulate(eq)
    mdd = (eq / peak - 1).min()
    bars_in_trade = sum(t[4] for t in trades)
    expo = bars_in_trade / n_bars
    yrs = n_bars / bars_per_year
    cagr = eq[-1] ** (1/yrs) - 1 if eq[-1] > 0 else -1
    trades_per_year = len(trades) / yrs
    sharpe = (avg / sd) * math.sqrt(trades_per_year) if sd > 0 else 0
    return dict(trades=len(trades), pf=pf, win=win_rate, avg=avg,
                sharpe=sharpe, mdd=mdd, cagr=cagr, expo=expo, final=eq[-1])

def main():
    engines = json.loads(ENG.read_text())
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
        trades = backtest_engine(df, e)
        m = metrics(trades, len(df), bars_per_year)
        if m is None:
            rows.append(dict(tag=e["tag"], symbol=e["symbol"], kind=e["kind"], tf=rule,
                             trades=0, pf=0, win=0, sharpe=0, mdd=0, cagr=0, expo=0, final=1, n_bars=len(df)))
            continue
        rows.append(dict(tag=e["tag"], symbol=e["symbol"], kind=e["kind"], tf=rule,
                         **m, n_bars=len(df)))
    df_out = pd.DataFrame(rows)
    df_out.to_csv(OUT, index=False)
    print(f"wrote {len(df_out)} results -> {OUT}")
    print()
    # pass criteria
    df_out["pf"] = df_out["pf"].replace([np.inf], 999)
    pass_mask = (df_out["sharpe"] >= 1.0) & (df_out["pf"] >= 1.3) & (df_out["mdd"] >= -0.30) & (df_out["trades"] >= 50)
    survivors = df_out[pass_mask].sort_values("sharpe", ascending=False)
    print(f"Total tested: {len(df_out)}")
    print(f"Survivors (Sharpe>=1.0, PF>=1.3, MDD>=-30%, trades>=50): {len(survivors)}")
    print()
    if len(survivors) == 0:
        print("ALL DEAD")
        return
    print("TOP 20 SURVIVORS:")
    print(f'{"tag":24}{"sym":10}{"kind":12}{"tf":5}{"trd":>5}{"PF":>7}{"Win%":>7}{"Shrp":>7}{"MDD%":>7}{"CAGR%":>8}{"Expo%":>7}')
    for _, r in survivors.head(20).iterrows():
        print(f'{r["tag"][:24]:24}{r["symbol"]:10}{r["kind"]:12}{r["tf"]:5}'
              f'{int(r["trades"]):>5}{r["pf"]:>7.2f}{r["win"]*100:>7.1f}'
              f'{r["sharpe"]:>7.2f}{r["mdd"]*100:>7.1f}{r["cagr"]*100:>8.1f}{r["expo"]*100:>7.1f}')

if __name__ == "__main__":
    main()
