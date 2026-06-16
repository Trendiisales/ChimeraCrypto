#!/usr/bin/env python3
"""
Walk-forward / curve-fit confirmation for the min-loss up-move config.
TRAIN = 2021 + 2023 bull + 2022 bear  (selection happens HERE only)
TEST  = 2024-25 + June-2026          (blind; never used to choose config)
Checks: (1) train-selected winner's blind OOS, (2) Spearman(train,test) over all
72 configs, (3) plateau around winner, (4) per-symbol breadth, (5) 2022 H1/H2.
"""
import sys,os
sys.path.insert(0,os.path.dirname(__file__))
from upmove_trail import series, COST_BP, ms_of
from upmove_minloss_sweep import precompute, bt

SYMS=["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","LINK","DOT","LTC","DOGE","ATOM","UNI","FIL","NEAR"]
def win(a,b): return (ms_of(*a),ms_of(*b))
TRAIN_BULL=[win((2021,1,1),(2021,12,31)), win((2023,1,1),(2023,12,31))]
BEAR    = win((2022,1,1),(2022,12,31))
BEAR_H1 = win((2022,1,1),(2022,6,30))
BEAR_H2 = win((2022,7,1),(2022,12,31))
TEST    =[win((2024,1,1),(2025,12,31)), win((2026,6,1),(2026,7,1))]

def net_over(data,PC,cfg,wins,persym=False):
    if persym:
        out={}
        for s in data:
            t=0
            for (a,b) in wins: t+=sum(bt(data[s],*PC[s],t0=a,t1=b,**cfg))
            out[s]=round(t*100,1)
        return out
    tot=0
    for s in data:
        for (a,b) in wins: tot+=sum(bt(data[s],*PC[s],t0=a,t1=b,**cfg))
    return round(tot*100,1)

def spearman(x,y):
    def rank(v):
        idx=sorted(range(len(v)),key=lambda i:v[i]); r=[0]*len(v)
        for pos,i in enumerate(idx): r[i]=pos
        return r
    rx,ry=rank(x),rank(y); n=len(x)
    d2=sum((rx[i]-ry[i])**2 for i in range(n))
    return round(1-6*d2/(n*(n*n-1)),3)

if __name__=="__main__":
    print("loading+precompute...",flush=True)
    data={}; PC={}
    for s in SYMS: data[s]=series(s); PC[s]=precompute(data[s])
    print(f"{len(data)} syms BTC={data['BTC'][-1][4]:.0f}\n")
    grid=[]
    for trail_m in [1.5,2.5,3.5,5.0]:
        for stop_m in [None,3.0]:
            for ts_bars in [None,24,48]:
                for cost_k in [3.0,5.0,8.0]:
                    grid.append(dict(don_n=24,trail_m=trail_m,cost_k=cost_k,stop_m=stop_m,ts_bars=ts_bars))
    # score every config on TRAIN (bull + bounded bear) and TEST (blind)
    res=[]
    for cfg in grid:
        tb=net_over(data,PC,cfg,TRAIN_BULL)       # train bull
        bear=net_over(data,PC,cfg,[BEAR])         # train bear (in-sample)
        test=net_over(data,PC,cfg,TEST)           # OOS
        train_score=tb+bear                       # selection metric: total train net
        res.append(dict(cfg=cfg,tb=tb,bear=bear,test=test,train=train_score))
    # ---- selection: pick winner by TRAIN ONLY ----
    res.sort(key=lambda r:-r['train'])
    print("=== TOP 8 BY TRAIN SCORE (2021+2023 bull + 2022 bear) -> blind TEST ===")
    print(f"{'trail':>5}{'stop':>5}{'ts':>5}{'ck':>4} | {'TRAINbull':>9}{'bear22':>8}{'trainScr':>9} || {'TESToos':>8}")
    for r in res[:8]:
        c=r['cfg']
        print(f"{c['trail_m']:>5}{str(c['stop_m']):>5}{str(c['ts_bars']):>5}{c['cost_k']:>4} | "
              f"{r['tb']:>+9.0f}{r['bear']:>+8.0f}{r['train']:>+9.0f} || {r['test']:>+8.0f}")
    win=res[0]
    print(f"\nTRAIN-WINNER = {win['cfg']}")
    print(f"  blind OOS (2024-25+Jun) net = {win['test']:+.0f}   (positive => edge generalises)")
    # ---- generalization: rank corr ----
    sc=spearman([r['train'] for r in res],[r['test'] for r in res])
    print(f"\nSpearman(train_score, test_oos) over all 72 configs = {sc}")
    print("  >0.5 = levers transfer (NOT curve-fit) | ~0/neg = curve-fit")
    # how does the *previously eyeballed* recommended config rank?
    rec=next(r for r in res if r['cfg']['trail_m']==5.0 and r['cfg']['stop_m']==3.0 and r['cfg']['ts_bars']==48 and r['cfg']['cost_k']==3.0)
    rank=sorted(range(len(res)),key=lambda i:-res[i]['test'])
    rec_test_rank=[res[i] for i in rank].index(rec)+1
    print(f"\nprev 'recommended' (5.0/3.0/48/3.0): train={rec['train']:+.0f} bear={rec['bear']:+.0f} TESToos={rec['test']:+.0f}  -> OOS rank {rec_test_rank}/72")
    # ---- plateau: neighbours of train winner (same trail, vary the rest) ----
    print(f"\n=== PLATEAU: all configs with trail={win['cfg']['trail_m']} (winner's trail) ===")
    nb=[r for r in res if r['cfg']['trail_m']==win['cfg']['trail_m']]
    pos=sum(1 for r in nb if r['test']>0)
    print(f"  {pos}/{len(nb)} have positive blind OOS; OOS range {min(r['test'] for r in nb):+.0f}..{max(r['test'] for r in nb):+.0f}")
    # ---- breadth: per-symbol for winner ----
    ps_bull=net_over(data,PC,win['cfg'],TEST,persym=True)
    ps_bear=net_over(data,PC,win['cfg'],[BEAR],persym=True)
    bw=sum(1 for v in ps_bull.values() if v>0)
    print(f"\n=== BREADTH (train-winner) ===")
    print(f"  OOS bull positive on {bw}/{len(ps_bull)} symbols")
    print(f"  best/worst OOS sym: {max(ps_bull,key=ps_bull.get)} {max(ps_bull.values()):+.0f} / {min(ps_bull,key=ps_bull.get)} {min(ps_bull.values()):+.0f}")
    # ---- within-bear robustness ----
    h1=net_over(data,PC,win['cfg'],[BEAR_H1]); h2=net_over(data,PC,win['cfg'],[BEAR_H2])
    print(f"\n=== 2022 BEAR robustness (train-winner) === H1={h1:+.0f}  H2={h2:+.0f}  (both bounded/neg = consistent, not one-spike)")
