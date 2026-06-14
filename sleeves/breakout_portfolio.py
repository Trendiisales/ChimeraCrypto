#!/usr/bin/env python3
"""
Volume-confirmed breakout PORTFOLIO — second-edge candidate.
Each rebalance: a symbol is eligible if its close is above its prior N-day high
(Donchian breakout) AND the recent volume confirms (vol > vol_mult * avg). Rank
eligible by breakout strength, long top-K, vol-scaled, BTC-200d macro gate.
Tested through the SAME OOS gate as momentum + reports CORRELATION to momentum's
daily returns (an uncorrelated edge diversifies; a correlated one is redundant).
Reads validated data/multiyr/*_15m.csv. Long-only spot.
"""
import csv, glob, os, math, datetime, itertools, sys

DATADIR = "data/multiyr"
COST_BP = float(os.environ.get("COST_BP","15"))
def vday(ms): return ms // 86400000

def load_daily():
    px = {}
    for f in sorted(glob.glob(os.path.join(DATADIR, "*_15m.csv"))):
        sym = os.path.basename(f).split("USDT")[0]
        dl = {}
        with open(f) as fh:
            r = csv.reader(fh); next(r, None)
            for x in r:
                try: ts=int(x[0]); c=float(x[4]); v=float(x[5])
                except Exception: continue
                d=vday(ts)
                if d in dl: c0,v0=dl[d]; dl[d]=(c, v0+v)
                else: dl[d]=(c, v)
        px[sym]=dl
    days = sorted({d for s in px for d in px[s]})
    syms = sorted(px.keys())
    close = {s:[px[s].get(d,(float('nan'),0.0))[0] for d in days] for s in syms}
    vol   = {s:[px[s].get(d,(float('nan'),0.0))[1] for d in days] for s in syms}
    return days, syms, close, vol

def dms(y,m,d): return int(datetime.datetime(y,m,d,tzinfo=datetime.timezone.utc).timestamp()*1000)
WINS = [("21bull",vday(dms(2021,1,1)),vday(dms(2021,11,10))),
        ("22bear",vday(dms(2022,1,1)),vday(dms(2022,12,31))),
        ("23rec", vday(dms(2023,1,1)),vday(dms(2023,12,31))),
        ("24bull",vday(dms(2024,1,1)),vday(dms(2024,12,31))),
        ("25hold",vday(dms(2025,1,1)),vday(dms(2026,7,1)))]

def sma(s,i,n):
    if i<n: return None
    v=[s[j] for j in range(i-n,i) if s[j]==s[j]]
    return sum(v)/len(v) if len(v)>=n*0.8 else None
def rvol(s,i,n=30):
    if i<n+1: return None
    rs=[s[j]/s[j-1]-1 for j in range(i-n,i) if s[j]==s[j] and s[j-1]==s[j-1] and s[j-1]>0]
    if len(rs)<n*0.6: return None
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs)
    return math.sqrt(var) if var>0 else None

def breakout_score(close,vol,i,N,vmult):
    """Eligible (return score>0) iff close>prior N-day high AND volume-confirmed."""
    if i<N+1: return None
    hi=max([close[j] for j in range(i-N,i) if close[j]==close[j]] or [float('nan')])
    if hi!=hi or close[i]!=close[i] or hi<=0: return None
    if close[i] <= hi: return None
    vavg=sma(vol,i,N)
    if vavg is None or vavg<=0 or vol[i]!=vol[i]: return None
    if vol[i] < vmult*vavg: return None
    return close[i]/hi - 1.0

def momentum_score(close,i,lb):
    if i<lb: return None
    a,b=close[i-lb],close[i]
    if a!=a or b!=b or a<=0: return None
    return b/a-1.0

def run(days,syms,close,vol,btc,kind,K,rebal,weighting,**p):
    n=len(days); weights={s:0.0 for s in syms}; dr=[]; last=-10**9
    for i in range(1,n):
        r=0.0
        for s in syms:
            w=weights[s]
            if w<=0: continue
            a,b=close[s][i-1],close[s][i]
            if a==a and b==b and a>0: r+=w*(b/a-1)
        dr.append((days[i],r))
        if days[i]-last<rebal: continue
        last=days[i]
        bull=True
        if btc is not None:
            bull=macro_bull(btc,i)
        nw={s:0.0 for s in syms}
        if bull:
            sc=[]
            for s in syms:
                if kind=="breakout": v=breakout_score(close[s],vol[s],i,p["N"],p["vmult"])
                else: v=momentum_score(close[s],i,p["lb"])
                if v is None or v<=0: continue
                sc.append((v,s))
            sc.sort(reverse=True); picks=[s for _,s in sc[:K]]
            if picks:
                if weighting=="invvol":
                    iv={s:(1.0/rvol(close[s],i,30) if rvol(close[s],i,30) else 0.0) for s in picks}
                    tot=sum(iv.values())
                    for s in picks: nw[s]= iv[s]/tot if tot>0 else 1.0/len(picks)
                else:
                    for s in picks: nw[s]=1.0/len(picks)
        turn=sum(abs(nw[s]-weights[s]) for s in syms)
        d_i,r_i=dr[-1]; dr[-1]=(d_i, r_i - turn*COST_BP/10000.0)
        weights=nw
    return dr

def wstats(dr,a,b):
    rs=[r for d,r in dr if a<=d<b]
    if len(rs)<20: return None
    tot=1.0
    for r in rs: tot*=(1+r)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) if var>0 else 1e-9
    return dict(ret=(tot-1)*100, sharpe=(m/sd)*math.sqrt(365), n=len(rs))

def corr(a,b):
    da={d:r for d,r in a}; common=[(da[d],r) for d,r in b if d in da]
    if len(common)<50: return float('nan')
    xs=[x for x,_ in common]; ys=[y for _,y in common]
    mx=sum(xs)/len(xs); my=sum(ys)/len(ys)
    cov=sum((x-mx)*(y-my) for x,y in common)
    sx=math.sqrt(sum((x-mx)**2 for x in xs)); sy=math.sqrt(sum((y-my)**2 for y in ys))
    return cov/(sx*sy) if sx>0 and sy>0 else float('nan')

def main():
    days,syms,close,vol=load_daily()
    btc=close.get("BTC")
    print(f"universe={len(syms)} cost={COST_BP}bp\n")
    # reference momentum series (the validated edge) for correlation
    mom=run(days,syms,close,vol,btc,"momentum",3,14,"invvol",lb=30)
    rows=[]
    for N,vm,K,rb,wt in itertools.product([20,40,55],[1.5,2.0],[5,8],[7,14],["equal","invvol"]):
        dr=run(days,syms,close,vol,btc,"breakout",K,rb,wt,N=N,vmult=vm)
        st={w[0]:wstats(dr,w[1],w[2]) for w in WINS}
        if not all(st[w] for w in ["21bull","23rec","24bull"]): continue
        gate=all(st[w]["sharpe"]>=1.0 and st[w]["ret"]>0 for w in ["21bull","23rec","24bull"])
        rows.append((N,vm,K,rb,wt,st,gate,corr(dr,mom)))
    print(f"{'N':>3}{'vm':>5}{'K':>3}{'rb':>4} {'wt':<7} | {'21bull':>14}{'23rec':>14}{'24bull':>14} | {'25hold':>14}  corr_mom GATE")
    print("-"*112)
    def c(s): return f"{s['ret']:+5.0f}% Sh{s['sharpe']:+.2f}" if s else "--"
    rows.sort(key=lambda r:-min(r[5]["21bull"]["sharpe"],r[5]["23rec"]["sharpe"],r[5]["24bull"]["sharpe"]))
    npass=0
    for N,vm,K,rb,wt,st,gate,cm in rows:
        if gate: npass+=1
        print(f"{N:>3}{vm:>5}{K:>3}{rb:>4} {wt:<7} | {c(st['21bull']):>14}{c(st['23rec']):>14}{c(st['24bull']):>14} | {c(st['25hold']):>14}  {cm:+.2f}   {'**PASS**' if gate else ''}")
    print("-"*112)
    print(f"{npass} breakout configs PASS the gate. corr_mom = daily-return correlation to validated momentum (low = diversifying).")

if __name__=="__main__": main()
