#!/usr/bin/env python3
"""
Min-loss sweep (operator 2026-06-16): accept the loss, MINIMISE it.
Objective: keep bull edge (2021+2024-25) while making 2022 BEAR loss as small
as possible. Sweep the full exit space + entry-selectivity, rank by frontier.
Precomputes ATR/EMA per symbol for speed. golden entry filter fixed (best gate).
Levers:
  trail_m  - chandelier trail width (wider = fewer exits/re-entries)
  stop_m   - hard initial ATR stop (None = trail only)
  ts_bars  - "been negative too long" time stop (exit if underwater >= N bars)
  cost_k   - entry selectivity (higher = only fire on bigger moves = fewer trades)
"""
import sys,os
sys.path.insert(0,os.path.dirname(__file__))
from upmove_trail import series, COST_BP, ms_of

def precompute(bars, atr_n=24):
    n=len(bars); A=[None]*n
    for i in range(n):
        if i<atr_n: continue
        s=0.0
        for j in range(i-atr_n+1,i+1):
            h,l,pc=bars[j][2],bars[j][3],bars[j-1][4]
            s+=max(h-l,abs(h-pc),abs(l-pc))
        A[i]=s/atr_n
    def emaarr(p):
        k=2/(p+1); e=[None]*n; prev=bars[0][4]
        for i,b in enumerate(bars): prev=b[4]*k+prev*(1-k); e[i]=prev
        return e
    return A, emaarr(50), emaarr(200)

def bt(bars,A,e50,e200,don_n,trail_m,cost_k,stop_m,ts_bars,t0,t1):
    cost=COST_BP/10000.0; trades=[]; pos=None
    for i in range(210,len(bars)):
        b=bars[i]; c=b[4]; ms=b[0]; a=A[i]
        if a is None: continue
        if pos is None:
            if ms<t0 or ms>t1: continue
            if a < cost_k*(c*cost): continue
            if not (e50[i]>e200[i] and c>e50[i]): continue   # golden filter
            donhi=max(bars[j][2] for j in range(i-don_n,i))
            if c>donhi:
                pos={'e':c,'mx':c,'i':i,'stop':(c-stop_m*a) if stop_m else None}
        else:
            pos['mx']=max(pos['mx'],c); ex=None
            if pos['stop'] is not None and b[3]<=pos['stop']: ex=pos['stop']
            elif ts_bars and (i-pos['i'])>=ts_bars and c<pos['e']: ex=c
            else:
                tr=pos['mx']-trail_m*a
                if c<tr or i==len(bars)-1: ex=c
            if ex is not None:
                trades.append((ex-pos['e'])/pos['e']-cost); pos=None
    return trades

def agg_net(data,PC,cfg,t0,t1):
    tot=0.0; n=0
    for s in data:
        tr=bt(data[s],*PC[s],t0=t0,t1=t1,**cfg)
        tot+=sum(tr); n+=len(tr)
    return round(tot*100,1), n

SYMS=["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","LINK","DOT","LTC","DOGE","ATOM","UNI","FIL","NEAR"]
W={'2021':(ms_of(2021,1,1),ms_of(2021,12,31)),
   '2022BEAR':(ms_of(2022,1,1),ms_of(2022,12,31)),
   '2024_25':(ms_of(2024,1,1),ms_of(2025,12,31)),
   'JUN26':(ms_of(2026,6,1),ms_of(2026,7,1))}

if __name__=="__main__":
    print("loading + precomputing...",flush=True)
    data={}; PC={}
    for s in SYMS:
        data[s]=series(s); PC[s]=precompute(data[s])
    print(f"{len(data)} syms, BTC={data['BTC'][-1][4]:.0f}\n")
    grid=[]
    for trail_m in [1.5,2.5,3.5,5.0]:
        for stop_m in [None,3.0]:
            for ts_bars in [None,24,48]:
                for cost_k in [3.0,5.0,8.0]:
                    grid.append(dict(don_n=24,trail_m=trail_m,cost_k=cost_k,stop_m=stop_m,ts_bars=ts_bars))
    rows=[]
    for cfg in grid:
        r={k:agg_net(data,PC,cfg,*W[k]) for k in W}
        bull=r['2021'][0]+r['2024_25'][0]
        rows.append((r['2022BEAR'][0], bull, r, cfg))
    # rank by smallest bear loss (closest to 0)
    rows.sort(key=lambda x:-x[0])  # bear net: less negative first
    hdr=f"{'trail':>5} {'stop':>4} {'tstop':>5} {'ck':>3} | {'2022BEAR':>9} {'bullsum':>8} {'2021':>7} {'2024_25':>8} {'JUN26':>6} {'#bear':>5}"
    print(hdr); print('-'*len(hdr))
    for bear,bull,r,cfg in rows:
        print(f"{cfg['trail_m']:>5} {str(cfg['stop_m']):>4} {str(cfg['ts_bars']):>5} {cfg['cost_k']:>3} | "
              f"{bear:>+9.0f} {bull:>+8.0f} {r['2021'][0]:>+7.0f} {r['2024_25'][0]:>+8.0f} {r['JUN26'][0]:>+6.0f} {r['2022BEAR'][1]:>5}")
    print("\nGoal: 2022BEAR closest to 0 WHILE bullsum stays large + JUN26 positive.")
    print("(#bear = trade count in 2022 — watch how loss tracks trade count, not stop tightness)")
