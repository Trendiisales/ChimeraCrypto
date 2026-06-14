#!/usr/bin/env python3
"""Vol-target overlay on the combined sleeve — cut the ~50% DD without killing
return. Scale next-day exposure by target_vol/trailing_realized_vol (lagged, no
lookahead), capped. Compare per cycle. Same data, same gate windows."""
import math
from breakout_portfolio import load_daily, run, WINS
from chimera_sleeves import series_for, SLEEVES

def realized_vol(rs, i, n=30):
    if i<n: return None
    w=rs[i-n:i]; m=sum(w)/len(w); var=sum((x-m)**2 for x in w)/len(w)
    return math.sqrt(var)*math.sqrt(365) if var>0 else None

def apply_vt(dr, tgt, lo=0.3, hi=1.5):
    rs=[r for _,r in dr]; ds=[d for d,_ in dr]; out=[]; scal=1.0
    for i in range(len(rs)):
        out.append((ds[i], scal*rs[i]))
        rv=realized_vol(rs,i+1,30)
        scal = max(lo,min(hi, tgt/rv)) if rv and rv>0 else 1.0
    return out

def stats(dr,a=None,b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<10: return None
    tot=eq=pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    yrs=len(rs)/365; cagr=((tot)**(1/yrs)-1)*100 if yrs>0 and tot>0 else 0
    return dict(ret=(tot-1)*100,sharpe=(m/sd)*math.sqrt(365),dd=dd*100,cagr=cagr)

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    da={d:r for d,r in series_for(days,syms,close,vol,btc,"MOMENTUM")}
    comb=[(d,0.5*da.get(d,0.0)+0.5*r) for d,r in series_for(days,syms,close,vol,btc,"BREAKOUT")]
    print("Vol-target overlay on COMBINED 50/50 (lagged, cap 0.3-1.5x):\n")
    print(f"{'config':<16}{'CAGR':>7}{'Sharpe':>8}{'MaxDD':>7}  | per-cycle Sharpe (21/23/24/25hold)")
    variants=[("base (none)",None)]+[(f"target {int(t*100)}% vol",t) for t in [0.80,0.60,0.40]]
    for name,t in variants:
        s = comb if t is None else apply_vt(comb,t)
        o=stats(s); cyc=[stats(s,w[1],w[2]) for w in WINS]
        cy=" ".join(f"{c['sharpe']:+.2f}" if c else " -- " for c in [cyc[0],cyc[2],cyc[3],cyc[4]])
        print(f"{name:<16}{o['cagr']:>6.0f}%{o['sharpe']:>8.2f}{o['dd']:>6.0f}%  | {cy}")

if __name__=="__main__": main()
