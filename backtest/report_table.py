#!/usr/bin/env python3
"""Operator-readable metrics table for the validated sleeves: PF, win-rate, DD,
return, Sharpe — per sleeve + combined, overall + per cycle. Daily-marked."""
import math
from breakout_portfolio import load_daily, run, WINS
from chimera_sleeves import SLEEVES, series_for

def metrics(dr, a=None, b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<10: return None
    pos=[r for r in rs if r>0]; neg=[r for r in rs if r<0]
    gw=sum(pos); gl=-sum(neg)
    pf= gw/gl if gl>1e-9 else (99 if gw>0 else 0)
    wr= 100*len(pos)/(len(pos)+len(neg)) if (pos or neg) else 0
    tot=eq=pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    yrs=len(rs)/365
    cagr=((tot)**(1/yrs)-1)*100 if yrs>0 and tot>0 else 0
    return dict(pf=pf, wr=wr, dd=dd*100, ret=(tot-1)*100, cagr=cagr, sharpe=(m/sd)*math.sqrt(365))

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    ser={n:series_for(days,syms,close,vol,btc,n) for n in SLEEVES}
    da={d:r for d,r in ser["MOMENTUM"]}
    ser["COMBINED"]=[(d,0.5*da.get(d,0.0)+0.5*r) for d,r in ser["BREAKOUT"]]
    print(f"\nUniverse: {len(syms)} liquid coins | 15m data 2020-2026 | long-only spot | BTC>200d macro gate\n")
    print("OVERALL (2020-2026, cost-inclusive 15bp/side):")
    print(f"  {'sleeve':<10}{'PF':>6}{'WinRate':>9}{'MaxDD':>8}{'CAGR':>8}{'Sharpe':>8}")
    for n in ["MOMENTUM","BREAKOUT","COMBINED"]:
        m=metrics(ser[n]); print(f"  {n:<10}{m['pf']:>6.2f}{m['wr']:>8.0f}%{m['dd']:>7.0f}%{m['cagr']:>7.0f}%{m['sharpe']:>8.2f}")
    print("\nPER CYCLE — COMBINED 50/50 (return | Sharpe | maxDD):")
    print(f"  {'window':<14}{'return':>10}{'Sharpe':>9}{'maxDD':>8}")
    lbl={"21bull":"2021 bull","22bear":"2022 BEAR","23rec":"2023 recov","24bull":"2024 bull","25hold":"2025 HOLDOUT"}
    for w in WINS:
        m=metrics(ser["COMBINED"],w[1],w[2])
        if m: print(f"  {lbl[w[0]]:<14}{m['ret']:>+9.0f}%{m['sharpe']:>9.2f}{m['dd']:>7.0f}%")

if __name__=="__main__": main()
