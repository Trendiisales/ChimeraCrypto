#!/usr/bin/env python3
"""Generate gui_data.json for the sleeves dashboard: equity curves, metrics,
per-cycle, current target portfolio, macro-gate state, monthly returns."""
import json, math, datetime, os
from breakout_portfolio import load_daily, WINS, sma
from chimera_sleeves import SLEEVES, series_for, compute_target

OUT = os.path.join(os.path.dirname(__file__), "gui_data.json")
DATADIR = os.path.join(os.path.dirname(__file__), "data")

def load_ledger():
    """Read the paper-trade ledger (trades.json + open_positions.json) and build
    the live-trading summary, matching the old system's Net P&L / PF / WR."""
    tp=os.path.join(DATADIR,"trades.json"); op=os.path.join(DATADIR,"open_positions.json")
    trades=json.load(open(tp)) if os.path.exists(tp) else []
    opos=json.load(open(op)) if os.path.exists(op) else {"positions":[],"open_count":0}
    net=sum(t.get("net_bp",0) for t in trades)
    wins=sum(1 for t in trades if t.get("net_bp",0)>0)
    gw=sum(t["net_bp"] for t in trades if t.get("net_bp",0)>0)
    gl=-sum(t["net_bp"] for t in trades if t.get("net_bp",0)<=0)
    pf=round(gw/gl,2) if gl>0 else (99 if gw>0 else 0)
    recent=sorted(trades,key=lambda t:t.get("exit_ts",0),reverse=True)[:60]
    return dict(
        net_bp=round(net), n_trades=len(trades),
        wr=round(100*wins/max(1,len(trades))), pf=pf,
        open_count=opos.get("open_count",0),
        positions=opos.get("positions",[]),
        recent=recent,
        winloss=[round(t.get("net_bp",0),1) for t in sorted(trades,key=lambda t:t.get("exit_ts",0))[-20:]],
    )

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
        ledger=load_ledger(),
    )
    with open(OUT,"w") as f: json.dump(data,f)

    # ── api_state.json: feeds the old GUI's /api/state2 (engines + signals) ──
    from breakout_portfolio import momentum_score
    opos={p["engine"]:p for p in load_ledger()["positions"]}
    engines=[]
    SLEEVE_OF={"MOM":"MOMENTUM","BRK":"BREAKOUT"}
    # open positions (in_position=true)
    for tag,p in opos.items():
        engines.append(dict(tag=tag, symbol=p["sym"], in_position=True,
            entry_px=p["entry_px"], sl_px=0, mfe_bp=p.get("mfe_bp",0),
            strategy=SLEEVE_OF.get(tag.split("-")[0],"SLEEVE"), tf_secs=14*86400,
            oos_pf=data["metrics"]["COMBINED"]["pf"], last_close=p.get("spot",0),
            bars_held=None, trail_armed=False, signal_ready=False, momentum_pct=0))
    # per-symbol momentum signals (ready-to-enter candidates)
    for sym in syms:
        mp=momentum_score(close[sym],i,30)
        if mp is None: continue
        engines.append(dict(tag=sym, symbol=sym, in_position=False,
            strategy="MOMENTUM", tf_secs=14*86400, momentum_pct=round(mp*100,1),
            signal_ready=bool(bull and mp>0), halted=not bull,
            next_bar_close_ms=0, entry_px=0, sl_px=0, mfe_bp=0,
            last_close=round(close[sym][i],8) if close[sym][i]==close[sym][i] else 0))
    apist=dict(build=f"sleeves · MACRO {'BULL' if bull else 'BEAR (cash)'}",
        startup_ts=days[i]*86400000, engines=engines,
        spot_local={s.lower():round(close[s][i],8) for s in syms if close[s][i]==close[s][i]})
    with open(os.path.join(os.path.dirname(__file__),"api_state.json"),"w") as f: json.dump(apist,f)
    print(f"wrote {OUT} + api_state.json ({len(engines)} engines, {len(opos)} open, gate={'BULL' if bull else 'BEAR'})")

if __name__=="__main__": main()
