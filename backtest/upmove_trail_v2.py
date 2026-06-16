#!/usr/bin/env python3
"""
v2: up-move + hard-trail + LOSS-MITIGATION levers, full basket, 4 WF windows.
Levers tested to answer "preserve edge, negate losses":
  - hard initial stop  (stop_m * ATR below entry)
  - breakeven arm      (after be_arm*ATR favorable, stop -> entry+cost) == "BE at worst"
  - time stop          (exit if still underwater after ts_bars)
  - trend filter       (none / ema200 / dual / golden)
Long-only spot, 17bp round-trip. Reuses data fetch from upmove_trail.
"""
import sys, os
sys.path.insert(0,os.path.dirname(__file__))
from upmove_trail import series, atr, ema, COST_BP, ms_of
from datetime import datetime, timezone

def backtest(bars, don_n, atr_n, trail_m, cost_k, trend_filter,
             stop_m=None, be_arm=None, ts_bars=None, struct=False, t0=None, t1=None):
    e200=ema(bars,200); e50=ema(bars,50)
    def pf(i,c):
        if trend_filter=="none": return True
        if trend_filter=="ema200": return c>e200[i]
        if trend_filter=="dual": return c>e50[i] and (c>e200[i] or e200[i]>e200[i-24])
        if trend_filter=="golden": return e50[i]>e200[i] and c>e50[i]
        return True
    trades=[]; pos=None; cost=COST_BP/10000.0
    for i in range(210,len(bars)):
        b=bars[i]; c=b[4]; ms=b[0]; a=atr(bars,atr_n,i)
        if a is None: continue
        if pos is None:
            if (t0 and ms<t0) or (t1 and ms>t1): continue
            donhi=max(bars[j][2] for j in range(i-don_n,i))
            if a < cost_k*(c*cost): continue
            if not pf(i,c): continue
            if c>donhi:
                pos={'entry':c,'maxc':c,'eidx':i,'stop':(c-stop_m*a) if stop_m else None,'armed':False,
                     'lvl':donhi,'blow':bars[i][3]}  # breakout level + breakout-bar low (structural invalidation)
        else:
            entry=pos['entry']; pos['maxc']=max(pos['maxc'],c)
            exit_px=None
            # breakeven arm
            if be_arm and not pos['armed'] and c>=entry+be_arm*a:
                pos['armed']=True; pos['stop']=entry+entry*cost  # cover the round-trip
            # structural invalidation: close back below breakout-bar low = setup failed
            if struct and c<pos['blow']:
                exit_px=c
            # hard/BE stop
            elif pos['stop'] is not None and b[3]<=pos['stop']:
                exit_px=pos['stop']
            # time stop
            elif ts_bars and (i-pos['eidx'])>=ts_bars and c<entry:
                exit_px=c
            else:
                trail=pos['maxc']-trail_m*a
                if c<trail or i==len(bars)-1: exit_px=c
            if exit_px is not None:
                net=(exit_px-entry)/entry-cost
                trades.append(net); pos=None
    return trades

def stats(tr):
    if not tr: return (0,0.0,0.0,0.0,0.0)
    w=[x for x in tr if x>0]; l=[x for x in tr if x<=0]
    gp=sum(w); gl=-sum(l); eq=0;peak=0;mdd=0
    for x in tr: eq+=x;peak=max(peak,eq);mdd=min(mdd,eq-peak)
    return (len(tr), round(sum(tr)*100,1), round(gp/gl,2) if gl>0 else 99.9,
            round(100*len(w)/len(tr)), round(mdd*100,1))

SYMS=["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","LINK","DOT","LTC","DOGE","ATOM","UNI","FIL","NEAR"]
WINDOWS=[("2021bull",ms_of(2021,1,1),ms_of(2021,12,31)),
         ("2022BEAR",ms_of(2022,1,1),ms_of(2022,12,31)),
         ("2023rec", ms_of(2023,1,1),ms_of(2023,12,31)),
         ("2024-25", ms_of(2024,1,1),ms_of(2025,12,31)),
         ("JUN2026", ms_of(2026,6,1),ms_of(2026,7,1))]

if __name__=="__main__":
    print("loading basket...",flush=True)
    data={}
    for s in SYMS:
        try: data[s]=series(s)
        except Exception as e: print("skip",s,e)
    print(f"loaded {len(data)} syms, BTC last={data['BTC'][-1][4]:.0f}\n")
    base=dict(don_n=24,atr_n=24,trail_m=2.5,cost_k=3.0)
    LEVERS=[
        ("golden RAW",              dict(trend_filter="golden")),
        ("golden +BE(1ATR)",        dict(trend_filter="golden",be_arm=1.0)),
        ("golden +stop2ATR",        dict(trend_filter="golden",stop_m=2.0)),
        ("golden +stop2 +BE1",      dict(trend_filter="golden",stop_m=2.0,be_arm=1.0)),
        ("golden +STRUCT (setup-fail)", dict(trend_filter="golden",struct=True)),
        ("golden +STRUCT +BE1",     dict(trend_filter="golden",struct=True,be_arm=1.0)),
        ("dual RAW",                dict(trend_filter="dual")),
    ]
    hdr="%-26s | "%"" + " ".join(f"{w[0]:>9}" for w in WINDOWS)
    print(hdr); print("-"*len(hdr))
    for name,lv in LEVERS:
        cells=[]
        for _,a,z in WINDOWS:
            agg=[]
            for s in data: agg+=backtest(data[s],t0=a,t1=z,**base,**lv)
            n,net,p,wr,mdd=stats(agg)
            cells.append(f"{net:+6.0f}%" if n else "   -  ")
        print(f"{name:26s} | " + " ".join(f"{c:>9}" for c in cells))
    print("\n(net % = sum of trade returns across basket, cost-incl. Want: JUN2026 +, 2022BEAR ~flat)")
    # detail row for the best candidate
    print("\n--- detail: golden +stop2+BE1+ts24 ---")
    for wn,a,z in WINDOWS:
        agg=[]
        for s in data: agg+=backtest(data[s],t0=a,t1=z,**base,trend_filter="golden",stop_m=2.0,be_arm=1.0,ts_bars=24)
        n,net,p,wr,mdd=stats(agg)
        print(f"  {wn:9s} n={n:4d} net={net:+7.1f}% PF={p:5} WR={wr:3}% maxDD={mdd:+7.1f}%")
