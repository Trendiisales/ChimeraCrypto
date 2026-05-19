"""
Validate pyramid logic on tier-A DONCHIAN engines.

For each engine: run baseline (no pyramid) + pyramid variant. Compare.
Pyramid spec matches EdgeEngine.hpp Session 31:
  - Only fires after trail armed (BE-lock active)
  - First add at MFE >= entry + pyramid_arm_atr * ATR_at_entry
  - Subsequent adds every +pyramid_step_atr from prev add
  - Each add = pyramid_size_mult * base notional
  - All legs share the same trail stop, exit together
  - Cost charged per leg
"""
import sys, math
from pathlib import Path
import numpy as np
import pandas as pd

DATA = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
TF_MAP = {21600:"6h", 57600:"16h"}

# 3 tier-A DONCHIAN engines (matching main.cpp configs)
ENGINES = [
    {"tag":"SUI-DONCH-H6",  "symbol":"suiusdt",  "tf":21600, "lookback":40, "hold":24, "sl_atr":3.5,
     "trail_arm":0.8, "trail_dist":0.3, "tight_arm":1.5, "tight_dist":0.15, "cost_bp":22,
     "pyr_arm":1.0, "pyr_step":0.8, "pyr_size":0.5, "pyr_max":3},
    {"tag":"BNB-DONCH-H16", "symbol":"bnbusdt",  "tf":57600, "lookback":40, "hold":6,  "sl_atr":3.0,
     "trail_arm":0.5, "trail_dist":0.4, "tight_arm":1.5, "tight_dist":0.15, "cost_bp":22,
     "pyr_arm":1.0, "pyr_step":0.8, "pyr_size":0.5, "pyr_max":3},
    {"tag":"BTC-DONCH-H16", "symbol":"btcusdt",  "tf":57600, "lookback":40, "hold":20, "sl_atr":3.5,
     "trail_arm":0.5, "trail_dist":0.3, "tight_arm":1.5, "tight_dist":0.15, "cost_bp":22,
     "pyr_arm":1.0, "pyr_step":0.8, "pyr_size":0.5, "pyr_max":3},
]


def load(sym):
    p = DATA / f"{sym.upper()}_1h_extended.csv"
    df = pd.read_csv(p, usecols=["open_time_ms","open","high","low","close"])
    df["ts"] = pd.to_datetime(df["open_time_ms"], unit="ms", utc=True)
    return df.set_index("ts").drop(columns=["open_time_ms"]).sort_index()


def resample(h1, rule):
    o = h1["open"].resample(rule).first()
    h = h1["high"].resample(rule).max()
    l = h1["low"].resample(rule).min()
    c = h1["close"].resample(rule).last()
    out = pd.concat([o,h,l,c], axis=1).dropna()
    out.columns = ["open","high","low","close"]
    return out


def atr(df, n=14):
    h, l, c = df["high"], df["low"], df["close"]
    pc = c.shift(1)
    tr = pd.concat([(h-l),(h-pc).abs(),(l-pc).abs()], axis=1).max(axis=1)
    return tr.ewm(alpha=1/n, adjust=False).mean()


def donchian_signal(df, lb):
    h = df["high"].values; c = df["close"].values
    prior_high = pd.Series(h).rolling(lb).max().shift(1).values
    return (c > prior_high) & ~np.isnan(prior_high)


def backtest(df, eng, with_pyramid: bool):
    entries = donchian_signal(df, eng["lookback"])
    a = atr(df, 14).values
    h = df["high"].values; l = df["low"].values; c = df["close"].values
    cost = eng["cost_bp"] / 10_000.0
    sl_m = eng["sl_atr"]
    arm  = eng["trail_arm"]; tdist = eng["trail_dist"]
    tarm = eng["tight_arm"]; tdis2 = eng["tight_dist"]
    pyr_on = with_pyramid
    pyr_arm  = eng["pyr_arm"]
    pyr_step = eng["pyr_step"]
    pyr_size = eng["pyr_size"]
    pyr_max  = eng["pyr_max"]

    trades = []
    in_pos = False
    entry_px = entry_atr = sl_px = trail_stop = 0.0
    high_since = 0.0
    bars_held = 0
    trail_armed = False
    pyr_legs = []  # list of (entry_px, size_mult)
    pyr_next_atr = pyr_arm  # next trigger in ATR units above entry
    pyr_count = 0
    entry_i = 0

    for i in range(1, len(df)):
        if in_pos:
            bars_held += 1
            high_since = max(high_since, h[i])
            mfe = high_since - entry_px

            # trail arm + BE-lock
            if not trail_armed and mfe >= arm * entry_atr:
                trail_armed = True
                # initial trail with BE floor
                be_px = entry_px * (1.0 + cost)
                raw = high_since - tdist * entry_atr
                trail_stop = max(raw, be_px)
                if pyr_on:
                    pyr_next_atr = pyr_arm  # eligibility starts now
            if trail_armed:
                dist = tdis2 if (tarm > 0 and mfe >= tarm * entry_atr) else tdist
                new = high_since - dist * entry_atr
                be_px = entry_px * (1.0 + cost)
                trail_stop = max(trail_stop, max(new, be_px))

            # pyramid: only if armed + enabled + under max
            if pyr_on and trail_armed and pyr_count < pyr_max:
                cur_profit_atr = (c[i] - entry_px) / entry_atr
                if cur_profit_atr >= pyr_next_atr:
                    add_px = c[i]  # add on bar close (sim of next-bar fill)
                    pyr_legs.append((add_px, pyr_size))
                    pyr_count += 1
                    pyr_next_atr = cur_profit_atr + pyr_step

            # exit: trail/SL hit, or time
            stop = max(sl_px, trail_stop) if trail_armed else sl_px
            exit_px = None
            if l[i] <= stop:
                exit_px = stop
            elif bars_held >= eng["hold"]:
                exit_px = c[i]
            if exit_px is not None:
                # base leg P&L
                base_gross = (exit_px - entry_px) / entry_px
                base_net = base_gross - cost
                # weighted total: base counts 1.0, each leg counts size_mult
                total_size = 1.0
                weighted_net = base_net
                for pe_px, pe_size in pyr_legs:
                    leg_gross = (exit_px - pe_px) / pe_px
                    leg_net = leg_gross - cost
                    weighted_net += leg_net * pe_size
                    total_size += pe_size
                # report as return on (base size) — i.e., excess vs single-shot
                trades.append({
                    "entry_i": entry_i, "exit_i": i,
                    "base_net": base_net,
                    "total_net": weighted_net,  # sum of returns weighted by size, normalized to base
                    "total_size": total_size,
                    "bars": bars_held,
                    "pyr_adds": len(pyr_legs),
                })
                in_pos = False; trail_armed = False; trail_stop = 0.0
                pyr_legs = []; pyr_count = 0; pyr_next_atr = pyr_arm
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


def metrics(trades, n_bars, bars_per_year, key):
    if not trades:
        return None
    nets = np.array([t[key] for t in trades])
    wins = nets[nets > 0]; losses = nets[nets <= 0]
    pf = (wins.sum() / -losses.sum()) if losses.sum() < 0 else float("inf")
    win = len(wins) / len(nets)
    eq = np.cumprod(1 + nets)
    peak = np.maximum.accumulate(eq)
    mdd = (eq / peak - 1).min()
    yrs = n_bars / bars_per_year
    cagr = eq[-1] ** (1/yrs) - 1 if eq[-1] > 0 else -1
    tpy = len(trades) / yrs
    sharpe = (nets.mean()/nets.std())*math.sqrt(tpy) if nets.std()>0 else 0
    bars_in = sum(t["bars"] for t in trades)
    expo = bars_in / n_bars
    avg_pyr = np.mean([t["pyr_adds"] for t in trades])
    return dict(trades=len(trades), pf=pf, win=win, sharpe=sharpe, mdd=mdd, cagr=cagr,
                expo=expo, final=eq[-1], avg_pyr=avg_pyr)


def main():
    print(f'{"Engine":18}{"Mode":12}{"trd":>5}{"PF":>7}{"Win%":>7}{"Shrp":>7}{"MDD%":>8}{"CAGR%":>9}{"Expo%":>7}{"AvgAdds":>9}')
    print("-" * 96)
    decisions = []
    for eng in ENGINES:
        h1 = load(eng["symbol"])
        df = resample(h1, TF_MAP[eng["tf"]])
        bpy = (365 * 24 * 3600) / eng["tf"]
        # baseline: no pyramid
        base_trades = backtest(df, eng, with_pyramid=False)
        m_base = metrics(base_trades, len(df), bpy, "base_net")
        # pyramid
        pyr_trades = backtest(df, eng, with_pyramid=True)
        m_pyr = metrics(pyr_trades, len(df), bpy, "total_net")
        if m_base is None or m_pyr is None:
            print(f'{eng["tag"]:18} NO TRADES'); continue
        for label, m in [("baseline", m_base), ("PYRAMID", m_pyr)]:
            pf = m["pf"] if m["pf"] != float("inf") else 999
            print(f'{eng["tag"]:18}{label:12}{m["trades"]:>5}{pf:>7.2f}{m["win"]*100:>7.1f}'
                  f'{m["sharpe"]:>7.2f}{m["mdd"]*100:>8.1f}{m["cagr"]*100:>9.1f}'
                  f'{m["expo"]*100:>7.1f}{m["avg_pyr"]:>9.2f}')
        # decision: pyramid keep if (CAGR up >=10% relative) AND (MDD not worse than -30%) AND (Sharpe not worse by >0.2)
        cagr_lift = (m_pyr["cagr"] - m_base["cagr"]) / max(abs(m_base["cagr"]), 0.01)
        mdd_ok = m_pyr["mdd"] >= -0.30
        sharpe_drop_ok = (m_pyr["sharpe"] - m_base["sharpe"]) >= -0.2
        keep = cagr_lift >= 0.10 and mdd_ok and sharpe_drop_ok
        decisions.append((eng["tag"], keep, m_base, m_pyr))
        print(f'{"":18}{"-> KEEP PYRAMID" if keep else "-> DISABLE PYRAMID"}  '
              f'(CAGR lift {cagr_lift*100:+.0f}%, MDD {"OK" if mdd_ok else "FAIL"}, '
              f'Sharpe Δ{m_pyr["sharpe"]-m_base["sharpe"]:+.2f})')
        print()
    print("=" * 96)
    print("DECISIONS:")
    for tag, keep, mb, mp in decisions:
        print(f'  {tag:18} {"KEEP" if keep else "DISABLE"}  '
              f'baseline=Sharpe {mb["sharpe"]:.2f}/CAGR {mb["cagr"]*100:.1f}%  '
              f'pyramid=Sharpe {mp["sharpe"]:.2f}/CAGR {mp["cagr"]*100:.1f}%')


if __name__ == "__main__":
    main()
