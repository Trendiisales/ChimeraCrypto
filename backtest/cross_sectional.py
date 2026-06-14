#!/usr/bin/env python3
"""
Cross-sectional momentum portfolio backtest on VALIDATED multi-cycle data.
Strongest-evidence crypto edge: rank universe by trailing return, long top-K,
vol-scaled, regime-gated to bull. Tested through the same OOS gate as everything
else: must be positive + Sharpe>1 in 2021 AND 2023 AND 2024 (independent bull
windows); 2025 reported as holdout.

Long-only spot, macro-gated (BTC>200d SMA -> else cash). Cost = bp per side on
turnover. Reads data/multiyr/*_15m.csv (must pass tools/validate_dataset.py first).
"""
import csv, glob, os, math, datetime, itertools, json

DATADIR = "data/multiyr"
COST_BP = float(__import__("os").environ.get("COST_BP","15"))
def vday(ms): return ms // 86400000  # UTC day index

def load_daily():
    """Return (days sorted, {sym: {day: (close, vol)}})."""
    px = {}
    for f in sorted(glob.glob(os.path.join(DATADIR, "*_15m.csv"))):
        sym = os.path.basename(f).split("USDT")[0]
        day_last = {}
        with open(f) as fh:
            r = csv.reader(fh); next(r, None)
            for x in r:
                try:
                    ts = int(x[0]); c = float(x[4]); v = float(x[5])
                except Exception: continue
                d = vday(ts)
                # keep last close of day, accumulate volume
                if d in day_last:
                    pc, pv = day_last[d]; day_last[d] = (c, pv + v)
                else:
                    day_last[d] = (c, v)
        px[sym] = day_last
    alldays = sorted({d for s in px for d in px[s]})
    return alldays, px

def daily_close_matrix(days, px):
    syms = sorted(px.keys())
    close = {s: [px[s].get(d, (float('nan'), 0.0))[0] for d in days] for s in syms}
    return syms, close

# windows (UTC day index ranges)
def dms(y,m,d): return int(datetime.datetime(y,m,d,tzinfo=datetime.timezone.utc).timestamp()*1000)
WINS = [
    ("21bull", vday(dms(2021,1,1)),  vday(dms(2021,11,10)), True),
    ("22bear", vday(dms(2022,1,1)),  vday(dms(2022,12,31)), False),
    ("23rec",  vday(dms(2023,1,1)),  vday(dms(2023,12,31)), True),
    ("24bull", vday(dms(2024,1,1)),  vday(dms(2024,12,31)), True),
    ("25hold", vday(dms(2025,1,1)),  vday(dms(2026,7,1)),   False),
]

def sma(series, i, n):
    if i < n: return None
    s = [series[j] for j in range(i-n, i) if series[j]==series[j]]  # skip nan
    if len(s) < n*0.8: return None
    return sum(s)/len(s)

def trailing_ret(series, i, lb):
    if i < lb: return None
    a, b = series[i-lb], series[i]
    if a!=a or b!=b or a<=0: return None
    return b/a - 1.0

def realized_vol(series, i, n=30):
    if i < n+1: return None
    rs=[]
    for j in range(i-n, i):
        a,b = series[j-1], series[j]
        if a==a and b==b and a>0: rs.append(b/a-1.0)
    if len(rs) < n*0.6: return None
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs)
    return math.sqrt(var) if var>0 else None

def run(days, syms, close, lb, K, rebal, weighting, macro_gate):
    n = len(days)
    btc = close.get("BTC")
    weights = {s: 0.0 for s in syms}   # current portfolio weights
    eq = 1.0
    daily_rets = []   # (day, ret)
    last_rebal = -10**9
    for i in range(1, n):
        # daily P&L from yesterday's weights
        r = 0.0
        for s in syms:
            w = weights[s]
            if w <= 0: continue
            a, b = close[s][i-1], close[s][i]
            if a==a and b==b and a>0: r += w * (b/a - 1.0)
        daily_rets.append((days[i], r))
        eq *= (1.0 + r)
        # rebalance?
        if days[i] - last_rebal < rebal: continue
        last_rebal = days[i]
        # macro gate
        bull = True
        if macro_gate and btc is not None:
            m = sma(btc, i, 200)
            bull = (m is not None and btc[i]==btc[i] and btc[i] > m)
        new_w = {s: 0.0 for s in syms}
        if bull:
            scores = []
            for s in syms:
                tr = trailing_ret(close[s], i, lb)
                if tr is None or tr <= 0: continue   # long-only: only positive momentum
                scores.append((tr, s))
            scores.sort(reverse=True)
            picks = [s for _, s in scores[:K]]
            if picks:
                if weighting == "invvol":
                    iv = {}
                    for s in picks:
                        v = realized_vol(close[s], i, 30)
                        iv[s] = (1.0/v) if v else 0.0
                    tot = sum(iv.values())
                    if tot > 0:
                        for s in picks: new_w[s] = iv[s]/tot
                    else:
                        for s in picks: new_w[s] = 1.0/len(picks)
                else:
                    for s in picks: new_w[s] = 1.0/len(picks)
        # turnover cost — MUST hit daily_rets (the series window_stats reads),
        # not a throwaway eq. Subtract it from this rebalance day's return.
        turn = sum(abs(new_w[s]-weights[s]) for s in syms)
        cost = turn * COST_BP/10000.0
        d_i, r_i = daily_rets[-1]
        daily_rets[-1] = (d_i, r_i - cost)
        weights = new_w
    return daily_rets

def window_stats(daily_rets, a, b):
    rs = [r for d, r in daily_rets if a <= d < b]
    if len(rs) < 20: return None
    tot = 1.0
    for r in rs: tot *= (1+r)
    tot -= 1
    m = sum(rs)/len(rs); var = sum((x-m)**2 for x in rs)/len(rs)
    sd = math.sqrt(var) if var>0 else 1e-9
    sharpe = (m/sd)*math.sqrt(365)
    # maxDD
    eq=1.0; peak=1.0; dd=0.0
    for r in rs:
        eq*=(1+r); peak=max(peak,eq); dd=max(dd,(peak-eq)/peak)
    return dict(ret=tot*100, sharpe=sharpe, dd=dd*100, n=len(rs))

def main():
    days, px = load_daily()
    syms, close = daily_close_matrix(days, px)
    print(f"loaded {len(syms)} symbols, {len(days)} days "
          f"({datetime.datetime.utcfromtimestamp(days[0]*86400).date()} -> "
          f"{datetime.datetime.utcfromtimestamp(days[-1]*86400).date()})\n")
    grid = list(itertools.product(
        [14,30,60,90],          # lookback days
        [3,5,8],                # top-K
        [7,14],                 # rebalance days
        ["equal","invvol"],     # weighting
        [True],                 # macro gate (always on for bull-only mandate)
    ))
    rows = []
    for lb,K,rb,wt,mg in grid:
        dr = run(days, syms, close, lb, K, rb, wt, mg)
        st = {w[0]: window_stats(dr, w[1], w[2]) for w in WINS}
        if not all(st[w] for w in ["21bull","23rec","24bull"]): continue
        gate = all(st[w]["sharpe"]>=1.0 and st[w]["ret"]>0 for w in ["21bull","23rec","24bull"])
        rows.append((lb,K,rb,wt,mg,st,gate))
    # report
    print(f"{'lb':>3}{'K':>3}{'rb':>4} {'wt':<7} | "
          f"{'21bull':>16}{'23rec':>16}{'24bull':>16} | {'25hold':>16} GATE")
    print("-"*110)
    def cell(s): return f"{s['ret']:+5.0f}% Sh{s['sharpe']:+.2f}" if s else "  --"
    rows.sort(key=lambda r: -min(r[5]["21bull"]["sharpe"], r[5]["23rec"]["sharpe"], r[5]["24bull"]["sharpe"]))
    passes=0
    for lb,K,rb,wt,mg,st,gate in rows:
        flag = "** PASS **" if gate else ""
        if gate: passes+=1
        print(f"{lb:>3}{K:>3}{rb:>4} {wt:<7} | {cell(st['21bull']):>16}{cell(st['23rec']):>16}"
              f"{cell(st['24bull']):>16} | {cell(st['25hold']):>16} {flag}")
    print("-"*110)
    print(f"{passes} configs PASS the 3-bull-window OOS gate (Sharpe>=1 + positive each)")

if __name__ == "__main__":
    main()
