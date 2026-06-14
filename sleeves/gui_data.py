#!/usr/bin/env python3
"""Generate gui_data.json for the sleeves dashboard: equity curves, metrics,
per-cycle, current target portfolio, macro-gate state, monthly returns."""
import json, math, datetime, os
from breakout_portfolio import load_daily, WINS, sma
from chimera_sleeves import SLEEVES, series_for, compute_target

OUT = os.path.join(os.path.dirname(__file__), "gui_data.json")

def metrics(dr,a=None,b=None):
    rs=[r for d,r in dr if (a is None or a<=d<b)]
    if len(rs)<10: return None
    pos=[r for r in rs if r>0]; neg=[r for r in rs if r<0]
    gw=sum(pos); gl=-sum(neg)
    tot=eq=pk=1.0; dd=0.0
    for r in rs: tot*=(1+r); eq*=(1+r); pk=max(pk,eq); dd=max(dd,(pk-eq)/pk)
    m=sum(rs)/len(rs); var=sum((x-m)**2 for x in rs)/len(rs); sd=math.sqrt(var) or 1e-9
    yrs=len(rs)/365
    return dict(pf=round(gw/gl,2) if gl>1e-9 else 99,
                wr=round(100*len(pos)/max(1,len(pos)+len(neg))),
                dd=round(dd*100), sharpe=round((m/sd)*math.sqrt(365),2),
                cagr=round(((tot)**(1/yrs)-1)*100) if yrs>0 and tot>0 else 0,
                ret=round((tot-1)*100))

def equity(dr, step=7):
    """cumulative equity, downsampled every `step` days. returns (dates, vals)."""
    d=[]; v=[]; eq=1.0
    for i,(day,r) in enumerate(dr):
        eq*=(1+r)
        if i%step==0 or i==len(dr)-1:
            d.append(datetime.datetime.utcfromtimestamp(day*86400).strftime("%Y-%m-%d"))
            v.append(round(eq,4))
    return d,v

def monthly(dr):
    buck={}
    for day,r in dr:
        k=datetime.datetime.utcfromtimestamp(day*86400).strftime("%Y-%m")
        buck[k]=buck.get(k,1.0)*(1+r)
    return [{"m":k,"ret":round((v-1)*100,1)} for k,v in sorted(buck.items())]

def main():
    days,syms,close,vol=load_daily(); btc=close.get("BTC")
    mom=series_for(days,syms,close,vol,btc,"MOMENTUM")
    brk=series_for(days,syms,close,vol,btc,"BREAKOUT")
    da={d:r for d,r in mom}
    comb=[(d,0.5*da.get(d,0.0)+0.5*r) for d,r in brk]
    i=len(days)-1
    m200=sma(btc,i,200)
    bull = (m200 is not None and btc[i]==btc[i] and btc[i]>m200)
    tgt={}
    for n in SLEEVES:
        w,_=compute_target(days,syms,close,vol,btc,n,i)
        tgt[n]=[{"sym":s,"weight":round(wt,3)} for s,wt in sorted(w.items(),key=lambda x:-x[1])] or "CASH"
    lbl={"21bull":"2021 bull","22bear":"2022 bear","23rec":"2023 recovery","24bull":"2024 bull","25hold":"2025 holdout"}
    data=dict(
        generated=datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M UTC"),
        universe=len(syms),
        asof=datetime.datetime.utcfromtimestamp(days[i]*86400).strftime("%Y-%m-%d"),
        macro=dict(bull=bull, btc=round(btc[i],2) if btc[i]==btc[i] else None,
                   sma200=round(m200,2) if m200 else None),
        target=tgt,
        metrics={n:metrics(s) for n,s in [("MOMENTUM",mom),("BREAKOUT",brk),("COMBINED",comb)]},
        cycles=[{"name":lbl[w[0]], **(metrics(comb,w[1],w[2]) or {})} for w in WINS],
        equity={"dates":equity(comb)[0],
                "COMBINED":equity(comb)[1],"MOMENTUM":equity(mom)[1],"BREAKOUT":equity(brk)[1]},
        monthly=monthly(comb),
    )
    with open(OUT,"w") as f: json.dump(data,f)
    print(f"wrote {OUT} ({len(data['equity']['dates'])} equity points, gate={'BULL' if bull else 'BEAR'})")

if __name__=="__main__": main()
