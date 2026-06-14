#!/usr/bin/env python3
"""Funding-carry edge (long-only spot). Funding rate = cost of a long perp; low/
negative funding = under-owned/contrarian-bullish. Test cross-sectional carry
through the same OOS gate + correlation to momentum (is it diversifying?).

Variants:
  CARRY      : long K LOWEST trailing-funding coins (quality gate: 30d momentum>0)
  MOM_LOWFUND: momentum top, tie-broken/filtered to below-median funding
Reads data/multiyr (price) + data/funding_hist (funding). Macro-gated, vol-weighted.
"""
import csv, glob, os, math, datetime, itertools
from breakout_portfolio import load_daily, WINS, sma, rvol
from chimera_sleeves import series_for  # momentum reference for correlation

FUND="data/funding_hist"
COST_BP=float(os.environ.get("COST_BP","15"))

def load_funding(days):
    """funding[sym] = list aligned to `days`: mean daily funding rate, fwd-filled."""
    didx={d:i for i,d in enumerate(days)}
    out={}
    for f in glob.glob(os.path.join(FUND,"*.csv")):
        sym=os.path.basename(f).split(".")[0]
        acc={}
        with open(f) as fh:
            r=csv.reader(fh); next(r,None)
            for x in r:
                try: ts=int(x[0]); fr=float(x[1])
                except: continue
                d=ts//86400000
                acc.setdefault(d,[]).append(fr)
        arr=[float('nan')]*len(days); last=float('nan')
        for i,d in enumerate(days):
            if d in acc: last=sum(acc[d])/len(acc[d])
            arr[i]=last
        out[sym]=arr
    return out

def mom(close,i,lb=30):
    if i<lb: return None
    a,b=close[i-lb],close[i]
    return (b/a-1.0) if (a==a and b==b and a>0) else None
def trail_fund(fund,i,n=14):
    if sym_missing(fund,i,n): return None
    w=[fund[j] for j in range(i-n,i) if fund[j]==fund[j]]
    return sum(w)/len(w) if len(w)>=n*0.5 else None
def sym_missing(fund,i,n): return i<n

def run(days,syms,close,fund,btc,kind,K=5,rebal=14):
    n=len(days); wt={s:0.0 for s in syms}; dr=[]; last=-10**9
    for i in range(1,n):
        r=0.0
        for s in syms:
            if wt[s]<=0: continue
            a,b=close[s][i-1],close[s][i]
            if a==a and b==b and a>0: r+=wt[s]*(b/a-1)
        dr.append((days[i],r))
        if days[i]-last<rebal: continue
        last=days[i]
        m=sma(btc,i,200); bull=(m is not None and btc[i]==btc[i] and btc[i]>m)
        nw={s:0.0 for s in syms}
        if bull:
            cand=[]
            for s in syms:
                if s not in fund: continue
                mo=mom(close[s],i); tf=trail_fund(fund[s],i)
                if tf is None: continue
                if kind!="PURE_CARRY" and mo is None: continue
                if kind in ("CARRY","MOM_LOWFUND") and mo<=0: continue
                # PURE_CARRY: no momentum gate (pure contrarian funding)  # quality: only up-trending
                cand.append((s,mo,tf))
            if cand:
                if kind in ("CARRY","PURE_CARRY"):
                    cand.sort(key=lambda x:x[2])            # lowest funding first
                elif kind=="MOM_LOWFUND":
                    med=sorted(c[2] for c in cand)[len(cand)//2]
                    cand=[c for c in cand if c[2]<=med] or cand
                    cand.sort(key=lambda x:-x[1])           # then momentum
                picks=[c[0] for c in cand[:K]]
                iv={p:(1.0/rvol(close[p],i,30) if rvol(close[p],i,30) else 0.0) for p in picks}
                tot=sum(iv.values())
                for p in picks: nw[p]= iv[p]/tot if tot>0 else 1.0/len(picks)
        turn=sum(abs(nw[s]-wt[s]) for s in syms); d_i,r_i=dr[-1]; dr[-1]=(d_i,r_i-turn*COST_BP/10000.0)
        wt=nw
    return dr

def stats(dr,a=None,b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<10: return None
    tot=eq=pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    return dict(ret=(tot-1)*100,sharpe=(m/sd)*math.sqrt(365),dd=dd*100)
def corr(a,b):
    da={d:r for d,r in a}; c=[(da[d],r) for d,r in b if d in da]
    if len(c)<50: return float('nan')
    xs=[x for x,_ in c]; ys=[y for _,y in c]; mx=sum(xs)/len(xs); my=sum(ys)/len(ys)
    cov=sum((x-mx)*(y-my) for x,y in c); sx=math.sqrt(sum((x-mx)**2 for x in xs)); sy=math.sqrt(sum((y-my)**2 for y in ys))
    return cov/(sx*sy) if sx>0 and sy>0 else float('nan')

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    fund=load_funding(days)
    print(f"price {len(syms)} syms, funding {len(fund)} syms, cost {COST_BP}bp\n")
    momref=series_for(days,syms,close,vol,btc,"MOMENTUM")
    print(f"{'strategy':<13}{'21bull':>14}{'23rec':>14}{'24bull':>14}{'25hold':>14}  corr_mom GATE")
    print("-"*92)
    def c(s): return f"{s['ret']:+5.0f}% Sh{s['sharpe']:+.2f}" if s else "  --"
    for kind in ["CARRY","MOM_LOWFUND","PURE_CARRY"]:
        dr=run(days,syms,close,fund,btc,kind)
        st={w[0]:stats(dr,w[1],w[2]) for w in WINS}
        if not all(st[w] for w in ["21bull","23rec","24bull"]): print(f"{kind:<13} insufficient funding history in early windows"); continue
        gate=all(st[w]["sharpe"]>=1.0 and st[w]["ret"]>0 for w in ["21bull","23rec","24bull"])
        print(f"{kind:<13}{c(st['21bull']):>14}{c(st['23rec']):>14}{c(st['24bull']):>14}{c(st['25hold']):>14}  {corr(dr,momref):+.2f}  {'**PASS**' if gate else ''}")

if __name__=="__main__": main()
