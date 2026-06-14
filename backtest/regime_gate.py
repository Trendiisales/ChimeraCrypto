#!/usr/bin/env python3
"""Regime-gate optimization. The 200d-SMA gate is slow — misses early recoveries
(BTC 61k->64k). Test faster/smarter gates on the combined momentum+breakout sleeve:
per-cycle performance + BEAR protection (2022/2025 DD) + does the gate trade NOW.
Goal: catch recoveries early WITHOUT bleeding in true bear."""
import math, os, datetime
from breakout_portfolio import load_daily, WINS, sma, rvol
from chimera_sleeves import momentum_score, breakout_score
COST_BP=float(os.environ.get("COST_BP","15"))

def ema(series,i,n):
    if i<n: return None
    k=2/(n+1); e=None
    for j in range(i-n,i+1):
        v=series[j]
        if v!=v: continue
        e=v if e is None else v*k+e*(1-k)
    return e

def gate_open(btc,i,kind):
    """Is the regime gate open at bar i?"""
    px=btc[i]
    if px!=px: return False
    if kind=="sma50_rising":
        m=sma(btc,i,50); mp=sma(btc,i-10,50) if i>=60 else None
        return m is not None and mp is not None and px>m and m>mp
    if kind=="dual_50_200":   # above fast AND not deep-bear (above 200 OR 200 rising-from-below)
        m50=sma(btc,i,50); m200=sma(btc,i,200)
        if m50 is None or m200 is None: return False
        return px>m50 and (px>m200 or (sma(btc,i-20,200) and m200>sma(btc,i-20,200)))
    if kind=="sma100_or_mom": # above 100d OR strong recent 30d momentum
        m=sma(btc,i,100); mo=(btc[i]/btc[i-30]-1) if i>=30 and btc[i-30]>0 else 0
        return (m is not None and px>m) or mo>0.10
    if kind.startswith("sma"):
        n=int(kind[3:]); m=sma(btc,i,n); return m is not None and px>m
    if kind.startswith("ema"):
        n=int(kind[3:]); e=ema(btc,i,n); return e is not None and px>e
    return False

def run(days,syms,close,vol,btc,gate):
    n=len(days); wt={s:0.0 for s in syms}; dr=[]; last=-10**9
    for i in range(1,n):
        r=0.0
        for s in syms:
            if wt[s]<=0: continue
            a,b=close[s][i-1],close[s][i]
            if a==a and b==b and a>0: r+=wt[s]*(b/a-1)
        dr.append((days[i],r))
        if days[i]-last<14: continue
        last=days[i]
        bull=gate_open(btc,i,gate)
        nw={s:0.0 for s in syms}
        if bull:
            mp=[]; bp=[]
            for s in syms:
                mo=momentum_score(close[s],i,30)
                if mo and mo>0: mp.append((mo,s))
                bs=breakout_score(close[s],vol[s],i,40,2.0)
                if bs and bs>0: bp.append((bs,s))
            mp.sort(reverse=True); bp.sort(reverse=True)
            picks={}
            for _,s in mp[:3]: picks[s]=picks.get(s,0)+0.5/3      # momentum sleeve 50%
            for _,s in bp[:5]: picks[s]=picks.get(s,0)+0.5/min(5,max(1,len(bp[:5])))
            # vol-weight within, normalize
            tot=sum(picks.values())
            if tot>0:
                for s,w in picks.items(): nw[s]=w/tot
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

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    i=len(days)-1
    print(f"BTC now ${btc[i]:.0f} | SMA50 ${sma(btc,i,50):.0f} | SMA100 ${sma(btc,i,100):.0f} | SMA200 ${sma(btc,i,200):.0f}\n")
    gates=["sma200","sma150","sma100","sma50","ema100","ema50","sma50_rising","dual_50_200","sma100_or_mom"]
    print(f"{'gate':<16}{'trades_NOW':>11}{'21bull':>13}{'23rec':>13}{'24bull':>13}{'25hold':>13}{'2022bear':>13}")
    print("-"*105)
    def c(s): return f"{s['ret']:+5.0f}%Sh{s['sharpe']:+.2f}" if s else " -- "
    for g in gates:
        dr=run(days,syms,close,vol,btc,g)
        st={w[0]:stats(dr,w[1],w[2]) for w in WINS}
        nowopen="YES" if gate_open(btc,i,g) else "no"
        bear=st["22bear"]
        bearstr=f"{bear['ret']:+.0f}% DD{bear['dd']:.0f}%" if bear else "flat"
        print(f"{g:<16}{nowopen:>11}{c(st['21bull']):>13}{c(st['23rec']):>13}{c(st['24bull']):>13}{c(st['25hold']):>13}{bearstr:>13}")
    print("\ntrades_NOW=YES means this gate would be LONG today (BTC recovering 61->64k).")

if __name__=="__main__": main()
