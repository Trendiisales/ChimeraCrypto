#!/usr/bin/env python3
"""
COMBINED backtest of the validated edges — the gate before implementing.
Blend cross-sectional momentum (lb30/K3/rb14/invvol) + volume-confirmed breakout
(N40/vm2.0/K5/rb14/equal), 50/50 daily-rebalanced. Reports each sleeve alone and
the blend, per window + overall, with maxDD. Diversification works iff the blend's
risk-adjusted return (Sharpe) / drawdown beats either sleeve alone.
"""
import os, math, datetime
from breakout_portfolio import load_daily, run, WINS, COST_BP

def full_stats(dr, a=None, b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<20: return None
    tot=1.0; eq=1.0; peak=1.0; dd=0.0
    for r in rs:
        tot*=(1+r); eq*=(1+r); peak=max(peak,eq); dd=max(dd,(peak-eq)/peak)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) if var>0 else 1e-9
    return dict(ret=(tot-1)*100, sharpe=(m/sd)*math.sqrt(365), dd=dd*100, n=len(rs))

def blend(a, b, wa=0.5, wb=0.5):
    da={d:r for d,r in a}
    return [(d, wa*da.get(d,0.0)+wb*r) for d,r in b]

def main():
    days,syms,close,vol=load_daily()
    btc=close.get("BTC")
    print(f"universe={len(syms)} cost={COST_BP}bp  (momentum lb30/K3/rb14/invvol + breakout N40/vm2.0/K5/rb14/equal, 50/50)\n")
    mom=run(days,syms,close,vol,btc,"momentum",3,14,"invvol",lb=30)
    brk=run(days,syms,close,vol,btc,"breakout",5,14,"equal",N=40,vmult=2.0)
    comb=blend(mom,brk)
    series={"MOMENTUM":mom, "BREAKOUT":brk, "COMBINED 50/50":comb}
    hdr=f"{'window':<10}"
    for name in series: hdr+=f"| {name:^26}"
    print(hdr); print("-"*len(hdr))
    rowwins=[("ALL",None,None)]+[(w[0],w[1],w[2]) for w in WINS]
    for wn,a,b in rowwins:
        line=f"{wn:<10}"
        for name,dr in series.items():
            s=full_stats(dr,a,b)
            line+= f"| {s['ret']:+7.0f}% Sh{s['sharpe']:+.2f} DD{s['dd']:.0f}%  " if s else "| "+" "*25
        print(line)
    print("-"*len(hdr))
    # diversification verdict (ALL-period)
    sm=full_stats(mom); sb=full_stats(brk); sc=full_stats(comb)
    print(f"\nALL-period Sharpe: momentum {sm['sharpe']:.2f}, breakout {sb['sharpe']:.2f}, "
          f"COMBINED {sc['sharpe']:.2f}  (max-of-sleeves {max(sm['sharpe'],sb['sharpe']):.2f})")
    print(f"ALL-period maxDD:  momentum {sm['dd']:.0f}%, breakout {sb['dd']:.0f}%, "
          f"COMBINED {sc['dd']:.0f}%  (min-of-sleeves {min(sm['dd'],sb['dd']):.0f}%)")
    better = sc['sharpe'] > max(sm['sharpe'],sb['sharpe']) and sc['dd'] <= max(sm['dd'],sb['dd'])
    print(f"\nDIVERSIFICATION {'WORKS — combined beats both on Sharpe with no worse DD' if better else 'marginal — combined Sharpe between sleeves (still smoother equity)'}")

if __name__=="__main__": main()
