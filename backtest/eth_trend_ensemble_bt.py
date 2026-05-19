"""
EthTrendEnsemble backtest — spot, long-only.

Cells:
  A: H4 EMA8/21 cross + ADX(14) > 25
  B: D1 Donchian-20 breakout, exit on 10-day low
  C: H4 pullback (px > EMA200, RSI(14) < 40, touch EMA50, close back > EMA50)

Top-level daily regime gate (any-true => allow trading):
  ADX(14) D1 > 20  AND  ATR(14) D1 > SMA50(ATR)  AND  BTC ADX(14) D1 > 18

Ensemble: inverse realized-vol weights (per-cell 30d returns std), refreshed weekly.

Costs: 10 bps round-trip per trade (Binance taker + slip).

Pass criteria:
  Sharpe >= 1.5, MaxDD >= -20%, ann return beats buy-hold/2.
"""

from __future__ import annotations
import sys, math
from pathlib import Path
import numpy as np
import pandas as pd

DATA = Path("/Users/jo/ChimeraCrypto/data/klines_spot")
COST_BPS = 10.0
INIT_EQUITY = 100_000.0


def load_1m(sym: str) -> pd.DataFrame:
    p = DATA / f"{sym}_1m.csv"
    df = pd.read_csv(p, usecols=["open_time_ms", "open", "high", "low", "close", "volume"])
    df["ts"] = pd.to_datetime(df["open_time_ms"], unit="ms", utc=True)
    df = df.set_index("ts").drop(columns=["open_time_ms"])
    return df.sort_index()


def resample(df: pd.DataFrame, rule: str) -> pd.DataFrame:
    o = df["open"].resample(rule).first()
    h = df["high"].resample(rule).max()
    l = df["low"].resample(rule).min()
    c = df["close"].resample(rule).last()
    v = df["volume"].resample(rule).sum()
    out = pd.concat([o, h, l, c, v], axis=1).dropna()
    out.columns = ["open", "high", "low", "close", "volume"]
    return out


def ema(s: pd.Series, n: int) -> pd.Series:
    return s.ewm(span=n, adjust=False).mean()


def rsi(close: pd.Series, n: int = 14) -> pd.Series:
    d = close.diff()
    up = d.clip(lower=0).ewm(alpha=1 / n, adjust=False).mean()
    dn = (-d.clip(upper=0)).ewm(alpha=1 / n, adjust=False).mean()
    rs = up / dn.replace(0, np.nan)
    return 100 - 100 / (1 + rs)


def atr(df: pd.DataFrame, n: int = 14) -> pd.Series:
    h, l, c = df["high"], df["low"], df["close"]
    pc = c.shift(1)
    tr = pd.concat([(h - l), (h - pc).abs(), (l - pc).abs()], axis=1).max(axis=1)
    return tr.ewm(alpha=1 / n, adjust=False).mean()


def adx(df: pd.DataFrame, n: int = 14) -> pd.Series:
    h, l, c = df["high"], df["low"], df["close"]
    up = h.diff()
    dn = -l.diff()
    plus_dm = np.where((up > dn) & (up > 0), up, 0.0)
    minus_dm = np.where((dn > up) & (dn > 0), dn, 0.0)
    pc = c.shift(1)
    tr = pd.concat([(h - l), (h - pc).abs(), (l - pc).abs()], axis=1).max(axis=1)
    atr_ = tr.ewm(alpha=1 / n, adjust=False).mean()
    plus_di = 100 * pd.Series(plus_dm, index=df.index).ewm(alpha=1 / n, adjust=False).mean() / atr_
    minus_di = 100 * pd.Series(minus_dm, index=df.index).ewm(alpha=1 / n, adjust=False).mean() / atr_
    dx = 100 * (plus_di - minus_di).abs() / (plus_di + minus_di).replace(0, np.nan)
    return dx.ewm(alpha=1 / n, adjust=False).mean()


# ── signals: return target position per bar (0 or 1) ─────────────────────────

def cell_a_signal(h4: pd.DataFrame) -> pd.Series:
    e8 = ema(h4["close"], 8)
    e21 = ema(h4["close"], 21)
    a = adx(h4, 14)
    in_long = pd.Series(0, index=h4.index, dtype=int)
    state = 0
    for i in range(len(h4)):
        if state == 0:
            if e8.iat[i] > e21.iat[i] and a.iat[i] > 25:
                state = 1
        else:
            if e8.iat[i] < e21.iat[i] or a.iat[i] < 20:
                state = 0
        in_long.iat[i] = state
    return in_long


def cell_b_signal(d1: pd.DataFrame) -> pd.Series:
    hi20 = d1["high"].rolling(20).max().shift(1)
    lo10 = d1["low"].rolling(10).min().shift(1)
    in_long = pd.Series(0, index=d1.index, dtype=int)
    state = 0
    for i in range(len(d1)):
        c = d1["close"].iat[i]
        if state == 0:
            if not np.isnan(hi20.iat[i]) and c > hi20.iat[i]:
                state = 1
        else:
            if not np.isnan(lo10.iat[i]) and c < lo10.iat[i]:
                state = 0
        in_long.iat[i] = state
    return in_long


def cell_c_signal(h4: pd.DataFrame) -> pd.Series:
    e50 = ema(h4["close"], 50)
    e200 = ema(h4["close"], 200)
    r = rsi(h4["close"], 14)
    in_long = pd.Series(0, index=h4.index, dtype=int)
    state = 0
    armed = False
    for i in range(len(h4)):
        c = h4["close"].iat[i]
        l = h4["low"].iat[i]
        if state == 0:
            if c > e200.iat[i] and r.iat[i] < 40 and l <= e50.iat[i]:
                armed = True
            if armed and c > e50.iat[i] and r.iat[i] >= 40:
                state = 1
                armed = False
        else:
            if c < e200.iat[i] or c < e50.iat[i] * 0.97:
                state = 0
        in_long.iat[i] = state
    return in_long


# ── regime gate (daily) ──────────────────────────────────────────────────────

def regime_gate(d1_eth: pd.DataFrame, d1_btc: pd.DataFrame) -> pd.Series:
    a_eth = adx(d1_eth, 14)
    a_btc = adx(d1_btc, 14)
    atr_ = atr(d1_eth, 14)
    atr_sma = atr_.rolling(50).mean()
    g = (a_eth > 20) & (atr_ > atr_sma) & (a_btc.reindex(d1_eth.index, method="ffill") > 18)
    return g.astype(int)


# ── backtest engine ──────────────────────────────────────────────────────────

def equity_curve(prices: pd.Series, positions: pd.Series, cost_bps: float) -> pd.Series:
    rets = prices.pct_change().fillna(0.0)
    pos_lag = positions.shift(1).fillna(0.0)
    turn = positions.diff().abs().fillna(positions.abs())
    cost = turn * (cost_bps / 10_000.0)
    strat_ret = pos_lag * rets - cost
    return (1 + strat_ret).cumprod()


def metrics(eq: pd.Series, bars_per_year: float) -> dict:
    r = eq.pct_change().dropna()
    if r.std() == 0 or len(r) == 0:
        return dict(sharpe=0, cagr=0, mdd=0, vol=0, expo=0)
    sharpe = r.mean() / r.std() * math.sqrt(bars_per_year)
    yrs = len(r) / bars_per_year
    cagr = eq.iloc[-1] ** (1 / yrs) - 1 if eq.iloc[-1] > 0 else -1
    peak = eq.cummax()
    mdd = (eq / peak - 1).min()
    vol = r.std() * math.sqrt(bars_per_year)
    return dict(sharpe=sharpe, cagr=cagr, mdd=mdd, vol=vol)


def inverse_vol_weights(rets: pd.DataFrame, lookback_bars: int = 30 * 6) -> pd.DataFrame:
    """rets: per-cell unleveraged returns on H4 grid. Weekly refresh (7d * 6 = 42 H4 bars)."""
    vol = rets.rolling(lookback_bars).std()
    inv = 1.0 / vol.replace(0, np.nan)
    w = inv.div(inv.sum(axis=1), axis=0).fillna(1.0 / rets.shape[1])
    refresh_idx = w.index[::42]
    w_held = w.reindex(refresh_idx).reindex(w.index, method="ffill")
    return w_held.clip(0, 1)


def main():
    print("loading 1m bars...", file=sys.stderr)
    eth1 = load_1m("ETHUSDT")
    btc1 = load_1m("BTCUSDT")

    eth_h4 = resample(eth1, "4h")
    eth_d1 = resample(eth1, "1D")
    btc_d1 = resample(btc1, "1D")

    print(f"H4 bars: {len(eth_h4)}  D1 bars: {len(eth_d1)}", file=sys.stderr)

    sig_a = cell_a_signal(eth_h4)
    sig_c = cell_c_signal(eth_h4)
    sig_b_d1 = cell_b_signal(eth_d1)
    sig_b = sig_b_d1.reindex(eth_h4.index, method="ffill").fillna(0).astype(int)

    gate_d1 = regime_gate(eth_d1, btc_d1)
    gate_h4 = gate_d1.reindex(eth_h4.index, method="ffill").fillna(0).astype(int)

    sig_a *= gate_h4
    sig_b *= gate_h4
    sig_c *= gate_h4

    px = eth_h4["close"]
    rets_h4 = px.pct_change().fillna(0.0)

    cell_rets = pd.DataFrame({
        "A": sig_a.shift(1).fillna(0) * rets_h4 - sig_a.diff().abs().fillna(0) * COST_BPS / 10_000,
        "B": sig_b.shift(1).fillna(0) * rets_h4 - sig_b.diff().abs().fillna(0) * COST_BPS / 10_000,
        "C": sig_c.shift(1).fillna(0) * rets_h4 - sig_c.diff().abs().fillna(0) * COST_BPS / 10_000,
    })

    w = inverse_vol_weights(cell_rets)
    portfolio_target = (w["A"] * sig_a + w["B"] * sig_b + w["C"] * sig_c).clip(0, 1)
    eq = equity_curve(px, portfolio_target, COST_BPS)

    bars_per_year = 6 * 365  # H4
    m = metrics(eq, bars_per_year)
    expo = (portfolio_target > 0).mean()

    bh = (1 + rets_h4).cumprod()
    bh_m = metrics(bh, bars_per_year)

    print()
    print("=" * 60)
    print("EthTrendEnsemble — spot long-only — H4 grid")
    print("=" * 60)
    print(f"Period:        {eth_h4.index[0]} -> {eth_h4.index[-1]}")
    print(f"Bars (H4):     {len(eth_h4)}")
    print()
    print(f"{'':16}{'Ensemble':>14}{'Buy&Hold':>14}")
    print(f"{'Sharpe':16}{m['sharpe']:>14.2f}{bh_m['sharpe']:>14.2f}")
    print(f"{'CAGR':16}{m['cagr']*100:>13.1f}%{bh_m['cagr']*100:>13.1f}%")
    print(f"{'MaxDD':16}{m['mdd']*100:>13.1f}%{bh_m['mdd']*100:>13.1f}%")
    print(f"{'AnnVol':16}{m['vol']*100:>13.1f}%{bh_m['vol']*100:>13.1f}%")
    print(f"{'Exposure':16}{expo*100:>13.1f}%{'100.0%':>14}")
    print()

    print("Per-cell standalone:")
    for k, s in {"A": sig_a, "B": sig_b, "C": sig_c}.items():
        e = equity_curve(px, s, COST_BPS)
        cm = metrics(e, bars_per_year)
        ce = (s > 0).mean()
        print(f"  Cell {k}: Sharpe={cm['sharpe']:+.2f}  CAGR={cm['cagr']*100:+.1f}%  MDD={cm['mdd']*100:+.1f}%  expo={ce*100:.0f}%")

    print()
    pass_sharpe = m["sharpe"] >= 1.5
    pass_mdd = m["mdd"] >= -0.20
    pass_vs_bh = m["cagr"] >= bh_m["cagr"] / 2
    print(f"Pass criteria:")
    print(f"  Sharpe >= 1.5     : {'PASS' if pass_sharpe else 'FAIL'}  ({m['sharpe']:.2f})")
    print(f"  MDD    >= -20%    : {'PASS' if pass_mdd else 'FAIL'}  ({m['mdd']*100:.1f}%)")
    print(f"  CAGR   >= BH/2    : {'PASS' if pass_vs_bh else 'FAIL'}  ({m['cagr']*100:.1f}% vs {bh_m['cagr']/2*100:.1f}%)")
    print()
    verdict = pass_sharpe and pass_mdd and pass_vs_bh
    print(f"VERDICT: {'SHIP' if verdict else 'DUMP'}")
    return 0 if verdict else 1


if __name__ == "__main__":
    sys.exit(main())
