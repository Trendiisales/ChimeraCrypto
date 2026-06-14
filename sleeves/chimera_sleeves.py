#!/usr/bin/env python3
"""
Chimera portfolio SHADOW sleeves — production module for the two validated edges.

Same code path produces the backtest AND the live shadow target (no backtest/live
drift). Two sleeves, locked from the timeframe+lever sweep (2026-06-14):

  MOMENTUM : cross-sectional, lb30 trailing return, top-5, rebal 14d, inverse-vol,
             BTC>200d macro gate.  bull-Sharpe 1.53, 2025 holdout +0.65.
  BREAKOUT : Donchian N40 + 2x volume confirm, top-5, rebal 14d, equal weight,
             BTC>200d macro gate.  bull-Sharpe 1.02, 2025 holdout +0.91.

corr(momentum, breakout) ~0.53 -> combined Sharpe 1.54 > both, DD 50% < both.
Both sleeves long-only spot, macro-gated (flat in bear). Run modes:
  confirm  : re-run the OOS gate on each sleeve + 50/50 blend (proves build==backtest)
  shadow   : compute TODAY's target portfolio for each sleeve, append to shadow ledger
"""
import os, sys, math, datetime, csv
from breakout_portfolio import (load_daily, run, WINS, COST_BP,
                                 sma, rvol, breakout_score, momentum_score)

SLEEVES = {
    # locked from sweep: momentum best at K=3 (holdout +0.94), breakout best at K=5.
    "MOMENTUM": dict(kind="momentum", K=3, rebal=14, weighting="invvol", lb=30),
    "BREAKOUT": dict(kind="breakout", K=5, rebal=14, weighting="equal", N=40, vmult=2.0),
}
SHADOW_LEDGER = "data/shadow_sleeves_ledger.csv"

def full_stats(dr,a=None,b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<20: return None
    tot=eq=pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    return dict(ret=(tot-1)*100, sharpe=(m/sd)*math.sqrt(365), dd=dd*100)

def series_for(days,syms,close,vol,btc,name):
    s=SLEEVES[name]
    extra={k:v for k,v in s.items() if k not in ("kind","K","rebal","weighting")}
    return run(days,syms,close,vol,btc,s["kind"],s["K"],s["rebal"],s["weighting"],**extra)

def compute_target(days,syms,close,vol,btc,name,i):
    """Point-in-time target weights for sleeve `name` as of bar index i. Live shadow."""
    s=SLEEVES[name]
    m=sma(btc,i,200)
    bull=(m is not None and btc[i]==btc[i] and btc[i]>m)
    if not bull: return {}, False
    sc=[]
    for sym in syms:
        if s["kind"]=="breakout": v=breakout_score(close[sym],vol[sym],i,s["N"],s["vmult"])
        else: v=momentum_score(close[sym],i,s["lb"])
        if v is None or v<=0: continue
        sc.append((v,sym))
    sc.sort(reverse=True); picks=[x for _,x in sc[:s["K"]]]
    w={}
    if picks:
        if s["weighting"]=="invvol":
            iv={p:(1.0/rvol(close[p],i,30) if rvol(close[p],i,30) else 0.0) for p in picks}
            tot=sum(iv.values())
            for p in picks: w[p]= iv[p]/tot if tot>0 else 1.0/len(picks)
        else:
            for p in picks: w[p]=1.0/len(picks)
    return w, True

def confirm():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    print(f"=== CONFIRM (build==backtest) — universe={len(syms)} cost={COST_BP}bp ===\n")
    ser={n:series_for(days,syms,close,vol,btc,n) for n in SLEEVES}
    da={d:r for d,r in ser["MOMENTUM"]}
    comb=[(d, 0.5*da.get(d,0.0)+0.5*r) for d,r in ser["BREAKOUT"]]
    ser["COMBINED"]=comb
    cols=list(ser.keys())
    print(f"{'window':<10}"+"".join(f"| {c:^24}" for c in cols))
    print("-"*(10+26*len(cols)))
    for wn,a,b in [("ALL",None,None)]+[(w[0],w[1],w[2]) for w in WINS]:
        line=f"{wn:<10}"
        for c in cols:
            st=full_stats(ser[c],a,b)
            line+= f"| {st['ret']:+7.0f}% Sh{st['sharpe']:+.2f} DD{st['dd']:.0f}% " if st else "| "+" "*23
        print(line)
    print("-"*(10+26*len(cols)))
    # gate check per sleeve
    print("\nGATE (Sharpe>=1 + positive in 2021 & 2023 & 2024):")
    for n in SLEEVES:
        st={w[0]:full_stats(ser[n],w[1],w[2]) for w in WINS}
        ok=all(st[w] and st[w]["sharpe"]>=1.0 and st[w]["ret"]>0 for w in ["21bull","23rec","24bull"])
        print(f"  {n:<10} {'PASS' if ok else 'FAIL'}  "
              f"(21={st['21bull']['sharpe']:.2f} 23={st['23rec']['sharpe']:.2f} "
              f"24={st['24bull']['sharpe']:.2f} hold25={st['25hold']['sharpe']:+.2f})")

def shadow():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    i=len(days)-1
    asof=datetime.datetime.utcfromtimestamp(days[i]*86400).strftime("%Y-%m-%d")
    new=os.path.exists(SHADOW_LEDGER)
    with open(SHADOW_LEDGER,"a",newline="") as fh:
        w=csv.writer(fh)
        if not new: w.writerow(["asof","sleeve","bull","symbol","weight"])
        for n in SLEEVES:
            tgt,bull=compute_target(days,syms,close,vol,btc,n,i)
            if not tgt:
                w.writerow([asof,n,int(bull),"CASH",1.0])
                print(f"[SHADOW {asof}] {n}: {'no picks' if bull else 'BEAR-GATE -> CASH'}")
            else:
                for sym,wt in sorted(tgt.items(),key=lambda x:-x[1]):
                    w.writerow([asof,n,int(bull),sym,round(wt,4)])
                picks=", ".join(f"{s}:{wt:.0%}" for s,wt in sorted(tgt.items(),key=lambda x:-x[1]))
                print(f"[SHADOW {asof}] {n} (bull): {picks}")
    print(f"-> appended to {SHADOW_LEDGER}")

if __name__=="__main__":
    mode=sys.argv[1] if len(sys.argv)>1 else "confirm"
    (confirm if mode=="confirm" else shadow)()
