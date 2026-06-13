#!/usr/bin/env python3
"""Held-out WF gate for D1 DONCHIAN-pyramid (Seykota crypto-long) candidates.
IS window [-1460,-1095], OOS window [-1095,-730]. Pick by IS PF, validate OOS.
cols: lb,hold,sl,trail_arm,trail_dist,trades,wins,wr,net_bp,PF,sharpe,x"""
import subprocess, sys
HARN="./backtest/backtest_mac"
PRESET="prod_tiered_pyramid_elite"
COINS=["btcusdt","ethusdt","solusdt","bnbusdt","linkusdt","adausdt",
       "avaxusdt","xrpusdt","dogeusdt"]

def sweep(coin, end_days):
    cmd=[HARN,"--sweep",f"{coin}:86400:DONCHIAN","--preset",PRESET,
         "--fine-fill","--regime-gate","--last-days","365","--end-days-ago",str(end_days)]
    try:
        out=subprocess.run(cmd,capture_output=True,text=True,timeout=300).stdout
    except Exception as e:
        return {}
    rows={}
    for ln in out.splitlines():
        p=ln.split(",")
        if len(p)>=11:
            try:
                lb,hold,sl=int(p[0]),int(p[1]),float(p[2])
                trades=int(p[5]); net=float(p[8]); pf=float(p[9]); sh=float(p[10])
                rows[(lb,hold,sl)]=(trades,net,pf,sh)
            except: pass
    return rows

print(f"{'coin':9s} {'IS(lb,hd,sl)':14s} {'IS:PF/n':12s} | {'OOS:PF':7s} {'OOS:n':6s} {'OOS:net':9s} {'OOS:Sh':7s}  verdict")
for c in COINS:
    isr=sweep(c,1095); oosr=sweep(c,730)
    if not isr or not oosr:
        print(f"{c:9s} NO DATA"); continue
    # pick best IS by PF with n>=10
    cand=[(k,v) for k,v in isr.items() if v[0]>=10]
    if not cand: cand=list(isr.items())
    best=max(cand,key=lambda kv: kv[1][2])
    k=best[0]; ist=best[1]
    oo=oosr.get(k)
    if oo is None:
        print(f"{c:9s} {str(k):14s} PF{ist[2]:.2f}/n{ist[0]:<3} | OOS config missing"); continue
    pf,n,net,sh=oo[2],oo[0],oo[1],oo[3]
    verdict="PASS-STD" if (pf>=1.3 and n>=8) else ("PASS-MARG" if pf>=1.0 and n>=8 else "FAIL")
    print(f"{c:9s} {str(k):14s} PF{ist[2]:.2f}/n{ist[0]:<4} | {pf:6.2f} {n:<6} {net:>8.1f} {sh:>6.2f}  {verdict}")
