#!/usr/bin/env python3
"""
Timeframe + lever sweep for the two validated edges. Answers:
 - which TIMEFRAMES work (lookback x rebalance grid)
 - which LEVERS matter (K, weighting, vol_mult)
 - best settings per edge (gate pass + 2025 holdout)
 - MULTI-TIMEFRAME bonus: are fast vs slow configs low-correlated (= separate sleeves)?
Reuses validated machinery from breakout_portfolio. Same OOS gate, clean 48-sym data.
"""
import os, math, itertools
from breakout_portfolio import load_daily, run, WINS, COST_BP

def stats(dr,a,b):
    rs=[r for d,r in dr if a<=d<b]
    if len(rs)<20: return None
    tot=1.0; eq=1.0; pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    return dict(ret=(tot-1)*100, sharpe=(m/sd)*math.sqrt(365), dd=dd*100)

def gate_min_sharpe(st):
    return min(st["21bull"]["sharpe"], st["23rec"]["sharpe"], st["24bull"]["sharpe"])

def corr(a,b):
    da={d:r for d,r in a}; c=[(da[d],r) for d,r in b if d in da]
    if len(c)<50: return float('nan')
    xs=[x for x,_ in c]; ys=[y for _,y in c]; mx=sum(xs)/len(xs); my=sum(ys)/len(ys)
    cov=sum((x-mx)*(y-my) for x,y in c); sx=math.sqrt(sum((x-mx)**2 for x in xs)); sy=math.sqrt(sum((y-my)**2 for y in ys))
    return cov/(sx*sy) if sx>0 and sy>0 else float('nan')

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    print(f"universe={len(syms)} cost={COST_BP}bp  (K=5, invvol fixed for the timeframe grid)\n")
    keep={}  # name -> series, for correlation
    # ---- MOMENTUM timeframe grid: lookback x rebalance ----
    print("MOMENTUM — min-bull-Sharpe (gate>=1.0) | 2025 holdout Sharpe")
    print(f"{'lb\\rebal':>8}" + "".join(f"{rb:>10}" for rb in [3,7,14,21,30]))
    for lb in [7,14,30,60,90,120]:
        row=f"{lb:>8}"
        for rb in [3,7,14,21,30]:
            dr=run(days,syms,close,vol,btc,"momentum",5,rb,"invvol",lb=lb)
            st={w[0]:stats(dr,w[1],w[2]) for w in WINS}
            if not all(st[w] for w in ["21bull","23rec","24bull","25hold"]): row+=f"{'--':>10}"; continue
            g=gate_min_sharpe(st); h=st["25hold"]["sharpe"]
            mark="*" if g>=1.0 else " "
            row+=f"{mark}{g:.2f}|{h:+.2f}".rjust(10)
            keep[f"M-lb{lb}-rb{rb}"]=dr
        print(row)
    print("  (cell = min-bull-Sharpe | 2025-holdout-Sharpe; * = passes gate)\n")
    # ---- BREAKOUT timeframe grid: N x rebalance ----
    print("BREAKOUT (vmult=2.0) — min-bull-Sharpe | 2025 holdout Sharpe")
    print(f"{'N\\rebal':>8}" + "".join(f"{rb:>10}" for rb in [3,7,14,21,30]))
    for N in [20,40,55,90]:
        row=f"{N:>8}"
        for rb in [3,7,14,21,30]:
            dr=run(days,syms,close,vol,btc,"breakout",5,rb,"equal",N=N,vmult=2.0)
            st={w[0]:stats(dr,w[1],w[2]) for w in WINS}
            if not all(st[w] for w in ["21bull","23rec","24bull","25hold"]): row+=f"{'--':>10}"; continue
            g=gate_min_sharpe(st); h=st["25hold"]["sharpe"]
            mark="*" if g>=1.0 else " "
            row+=f"{mark}{g:.2f}|{h:+.2f}".rjust(10)
            keep[f"B-N{N}-rb{rb}"]=dr
        print(row)
    print()
    # ---- MULTI-TIMEFRAME correlation: fast vs medium vs slow ----
    reps=["M-lb14-rb7","M-lb30-rb14","M-lb90-rb30","B-N40-rb14","B-N20-rb7"]
    reps=[r for r in reps if r in keep]
    print("CROSS-CORRELATION (daily returns) — low between two passers = separate sleeves:")
    print(f"{'':>14}"+"".join(f"{r:>14}" for r in reps))
    for a in reps:
        line=f"{a:>14}"
        for b in reps: line+=f"{corr(keep[a],keep[b]):>14.2f}"
        print(line)

if __name__=="__main__": main()
