"""
DOGE-RSI30-H3 standalone backtest on extended data.

Engine logic (from include/core/EdgeEngine.hpp RSI_REVERT path):
  Entry: RSI(14) crosses UP through 30 (r_prev<=30, r_now>30). LONG.
  Initial SL: entry - 3.5 * ATR(14)
  Trailing stop:
    arm when MFE >= +0.5 * ATR; trail at (high - 0.3 * ATR)
    tighten when MFE >= +1.5 * ATR; trail at (high - 0.15 * ATR)
  Time exit: bars_held >= 20
  Cost: 22 bp round-trip
"""
import sys, math
from pathlib import Path
import numpy as np
import pandas as pd

CSV = Path("/Users/jo/ChimeraCrypto/data/klines_spot/DOGEUSDT_1h_extended.csv")
COST_BPS = 22.0
RSI_PERIOD = 14
RSI_THRESH = 30.0
HOLD_BARS = 20
SL_ATR = 3.5
TRAIL_ARM_ATR = 0.5
TRAIL_DIST_ATR = 0.3
TRAIL_TIGHTEN_ATR = 1.5
TRAIL_TIGHTEN_DIST_ATR = 0.15


def load() -> pd.DataFrame:
    df = pd.read_csv(CSV, usecols=["open_time_ms","open","high","low","close","volume"])
    df["ts"] = pd.to_datetime(df["open_time_ms"], unit="ms", utc=True)
    return df.set_index("ts").drop(columns=["open_time_ms"]).sort_index()


def resample_h3(h1: pd.DataFrame) -> pd.DataFrame:
    o = h1["open"].resample("3h").first()
    h = h1["high"].resample("3h").max()
    l = h1["low"].resample("3h").min()
    c = h1["close"].resample("3h").last()
    v = h1["volume"].resample("3h").sum()
    out = pd.concat([o,h,l,c,v], axis=1).dropna()
    out.columns = ["open","high","low","close","volume"]
    return out


def rsi(close, n=14):
    d = close.diff()
    up = d.clip(lower=0).ewm(alpha=1/n, adjust=False).mean()
    dn = (-d.clip(upper=0)).ewm(alpha=1/n, adjust=False).mean()
    rs = up / dn.replace(0, np.nan)
    return 100 - 100 / (1 + rs)


def atr(df, n=14):
    h, l, c = df["high"], df["low"], df["close"]
    pc = c.shift(1)
    tr = pd.concat([(h-l),(h-pc).abs(),(l-pc).abs()], axis=1).max(axis=1)
    return tr.ewm(alpha=1/n, adjust=False).mean()


def backtest(df: pd.DataFrame):
    r = rsi(df["close"], RSI_PERIOD).values
    a = atr(df, RSI_PERIOD).values
    o = df["open"].values
    h = df["high"].values
    l = df["low"].values
    c = df["close"].values

    trades = []
    pos_open = False
    entry_px = sl_px = trail_arm_px = trail_stop_px = entry_atr = 0.0
    bars_held = 0
    trail_armed = False
    trail_tightened = False
    high_since_entry = 0.0
    entry_idx = -1

    for i in range(1, len(df)):
        if np.isnan(r[i]) or np.isnan(r[i-1]) or np.isnan(a[i]):
            continue

        if pos_open:
            bars_held += 1
            bar_h, bar_l = h[i], l[i]
            high_since_entry = max(high_since_entry, bar_h)
            mfe = high_since_entry - entry_px
            if mfe >= TRAIL_ARM_ATR * entry_atr and not trail_armed:
                trail_armed = True
            if trail_armed:
                dist = TRAIL_TIGHTEN_DIST_ATR if mfe >= TRAIL_TIGHTEN_ATR * entry_atr else TRAIL_DIST_ATR
                new_trail = high_since_entry - dist * entry_atr
                trail_stop_px = max(trail_stop_px, new_trail)
                if not trail_tightened and mfe >= TRAIL_TIGHTEN_ATR * entry_atr:
                    trail_tightened = True
            stop_level = max(sl_px, trail_stop_px) if trail_armed else sl_px
            exit_px = None
            exit_reason = None
            if bar_l <= stop_level:
                exit_px = stop_level
                exit_reason = "TRAIL" if trail_armed and stop_level == trail_stop_px else "SL"
            elif bars_held >= HOLD_BARS:
                exit_px = c[i]
                exit_reason = "TIME"
            if exit_px is not None:
                gross_ret = (exit_px - entry_px) / entry_px
                net_ret = gross_ret - (COST_BPS / 10_000.0)
                trades.append({
                    "entry_idx": entry_idx, "exit_idx": i,
                    "entry_px": entry_px, "exit_px": exit_px,
                    "gross_ret": gross_ret, "net_ret": net_ret,
                    "reason": exit_reason, "bars": bars_held,
                })
                pos_open = False
                trail_armed = trail_tightened = False
                trail_stop_px = 0.0
            continue

        if r[i-1] <= RSI_THRESH and r[i] > RSI_THRESH:
            entry_px = c[i]
            entry_atr = a[i]
            if entry_atr <= 0:
                continue
            sl_px = entry_px - SL_ATR * entry_atr
            high_since_entry = entry_px
            bars_held = 0
            entry_idx = i
            pos_open = True

    return trades


def metrics(trades, bars_total, bars_per_year):
    if not trades:
        return dict(trades=0, pf=0, win=0, avg=0, sharpe=0, mdd=0, cagr=0, expo=0)
    n = len(trades)
    nets = np.array([t["net_ret"] for t in trades])
    wins = nets[nets > 0]
    losses = nets[nets <= 0]
    pf = wins.sum() / -losses.sum() if len(losses) and losses.sum() < 0 else float("inf")
    win = len(wins) / n
    avg = nets.mean()
    # build equity curve at trade timestamps
    eq = np.cumprod(1 + nets)
    peak = np.maximum.accumulate(eq)
    mdd = (eq / peak - 1).min()
    bars_in_trade = sum(t["bars"] for t in trades)
    expo = bars_in_trade / bars_total
    # Sharpe (annualize per-trade returns by trades/year)
    trades_per_year = n / (bars_total / bars_per_year)
    sharpe = nets.mean() / nets.std() * math.sqrt(trades_per_year) if nets.std() > 0 else 0
    yrs = bars_total / bars_per_year
    cagr = eq[-1] ** (1/yrs) - 1 if eq[-1] > 0 else -1
    return dict(trades=n, pf=pf, win=win, avg=avg, sharpe=sharpe, mdd=mdd, cagr=cagr, expo=expo, eq_end=eq[-1])


def main():
    h1 = load()
    print(f"H1 range: {h1.index[0]} -> {h1.index[-1]}  bars={len(h1)}", file=sys.stderr)
    h3 = resample_h3(h1)
    print(f"H3 bars: {len(h3)}", file=sys.stderr)

    # Walk forward by year for stability check
    years = sorted(set(h3.index.year))
    print("\nPer-year backtest:")
    print(f'{"Year":6}{"Trd":>5}{"PF":>8}{"Win%":>7}{"Avg%":>8}{"Sharpe":>9}{"MDD%":>8}{"CAGR%":>9}{"Expo%":>7}')
    for y in years:
        sub = h3[h3.index.year == y]
        if len(sub) < 200: continue
        t = backtest(sub)
        m = metrics(t, len(sub), 365*8)  # H3 = 8 bars/day
        print(f'{y:6}{m["trades"]:>5}{m["pf"] if m["pf"]!=float("inf") else 999:>8.2f}'
              f'{m["win"]*100:>7.1f}{m["avg"]*100:>8.2f}{m["sharpe"]:>9.2f}'
              f'{m["mdd"]*100:>8.1f}{m["cagr"]*100:>9.1f}{m["expo"]*100:>7.1f}')

    # Full sample
    t_all = backtest(h3)
    m_all = metrics(t_all, len(h3), 365*8)
    print(f"\nFULL SAMPLE: {h3.index[0].date()} -> {h3.index[-1].date()}  ({len(h3)} H3 bars)")
    print(f"  Trades:    {m_all['trades']}")
    print(f"  PF:        {m_all['pf']:.2f}" + ("  (inf — no losers)" if m_all['pf']==float('inf') else ""))
    print(f"  WinRate:   {m_all['win']*100:.1f}%")
    print(f"  Avg trade: {m_all['avg']*100:+.2f}%")
    print(f"  Sharpe:    {m_all['sharpe']:.2f}")
    print(f"  MaxDD:     {m_all['mdd']*100:.1f}%")
    print(f"  CAGR:      {m_all['cagr']*100:+.1f}%")
    print(f"  Exposure:  {m_all['expo']*100:.1f}%")
    print(f"  Final eq:  {m_all['eq_end']:.3f}x")

    exits = {}
    for t in t_all:
        exits[t["reason"]] = exits.get(t["reason"], 0) + 1
    print(f"  Exits:     {exits}")

    pass_sh = m_all["sharpe"] >= 1.5
    pass_pf = m_all["pf"] >= 1.5
    pass_mdd = m_all["mdd"] >= -0.25
    pass_n = m_all["trades"] >= 50
    print()
    print("Pass criteria:")
    print(f"  Sharpe >= 1.5     : {'PASS' if pass_sh else 'FAIL'}  ({m_all['sharpe']:.2f})")
    print(f"  PF     >= 1.5     : {'PASS' if pass_pf else 'FAIL'}  ({m_all['pf']:.2f})")
    print(f"  MDD    >= -25%    : {'PASS' if pass_mdd else 'FAIL'}  ({m_all['mdd']*100:.1f}%)")
    print(f"  Trades >= 50      : {'PASS' if pass_n else 'FAIL'}  ({m_all['trades']})")
    verdict = pass_sh and pass_pf and pass_mdd and pass_n
    print()
    print(f"VERDICT: {'ADD' if verdict else 'DUMP'}")
    return 0 if verdict else 1


if __name__ == "__main__":
    sys.exit(main())
