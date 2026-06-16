#!/usr/bin/env python3
"""
Up-move + hard-trail engine test (operator spec, 2026-06-16).
Entry  : Donchian N-bar high breakout (pure "market moving up", NO EMA required).
Cost   : only fire when ATR/price clears k * round-trip cost (edge > fees).
Exit   : hard ATR chandelier trail = max_close_since_entry - m*ATR.
Knob   : trend_filter OFF (operator ask) vs ON (close>EMA200) to show the delta.
Cost   : 17 bp round-trip applied entry+exit. Long-only spot.
Windows: JUNE 2026 (this month, in-sample tune) + 2022 bear (OOS ruin check).
"""
import csv, json, urllib.request, time, sys, os
from datetime import datetime, timezone

KL = os.path.expanduser("~/ChimeraCrypto/data/klines_spot")
COST_BP = 17.0  # round-trip bp

def load_local(sym):
    p = f"{KL}/{sym}USDT_1h_extended.csv"
    out=[]
    with open(p) as f:
        r=csv.reader(f); next(r)
        for row in r:
            out.append([int(row[0]),float(row[1]),float(row[2]),float(row[3]),float(row[4])])
    return out  # [ms,o,h,l,c]

def fetch_tail(sym, start_ms):
    base="https://api.binance.com/api/v3/klines"
    out=[]; cur=start_ms
    while True:
        url=f"{base}?symbol={sym}USDT&interval=1h&startTime={cur}&limit=1000"
        try:
            d=json.load(urllib.request.urlopen(url,timeout=20))
        except Exception as e:
            print("fetch err",e); break
        if not d: break
        for k in d:
            out.append([int(k[0]),float(k[1]),float(k[2]),float(k[3]),float(k[4])])
        cur=d[-1][0]+1
        if len(d)<1000: break
        time.sleep(0.25)
    return out

def series(sym):
    loc=load_local(sym)
    last=loc[-1][0]
    tail=fetch_tail(sym,last+1)
    bars=loc+tail
    # dedupe by ms
    seen=set(); clean=[]
    for b in bars:
        if b[0] in seen: continue
        seen.add(b[0]); clean.append(b)
    clean.sort(key=lambda x:x[0])
    return clean

def atr(bars,n,i):
    if i<n: return None
    s=0.0
    for j in range(i-n+1,i+1):
        h,l,pc=bars[j][2],bars[j][3],bars[j-1][4]
        s+=max(h-l,abs(h-pc),abs(l-pc))
    return s/n

def ema(bars,n):
    k=2/(n+1); e=[None]*len(bars); prev=bars[0][4]
    for i,b in enumerate(bars):
        prev=b[4]*k+prev*(1-k); e[i]=prev
    return e

def backtest(bars, don_n, atr_n, trail_m, cost_k, trend_filter, t0=None, t1=None):
    e200=ema(bars,200); e50=ema(bars,50)
    def pass_filter(i,c):
        if trend_filter=="none": return True
        if trend_filter=="ema200": return c>e200[i]
        if trend_filter=="ema50_rising": return c>e50[i] and e50[i]>e50[i-24]
        if trend_filter=="dual": return c>e50[i] and (c>e200[i] or e200[i]>e200[i-24])
        if trend_filter=="golden": return e50[i]>e200[i] and c>e50[i]
        return True
    trades=[]; pos=None
    for i in range(max(don_n,atr_n,210),len(bars)):
        b=bars[i]; c=b[4]; ms=b[0]
        a=atr(bars,atr_n,i)
        if a is None: continue
        if pos is None:
            if t0 and ms<t0: continue
            if t1 and ms>t1: continue
            donhi=max(bars[j][2] for j in range(i-don_n,i))
            # cost gate: ATR (move room) must exceed cost_k * round-trip cost in price terms
            cost_px=c*COST_BP/10000.0
            if a < cost_k*cost_px:   # not enough vol to clear fees + edge
                continue
            if not pass_filter(i,c):
                continue
            if c>donhi:  # up-move breakout
                pos={'entry':c,'ems':ms,'maxc':c,'eidx':i}
        else:
            pos['maxc']=max(pos['maxc'],c)
            trail=pos['maxc']-trail_m*a
            if c<trail or i==len(bars)-1:
                gross=(c-pos['entry'])/pos['entry']
                net=gross-COST_BP/10000.0
                trades.append({'ret':net,'bars':i-pos['eidx'],'ems':pos['ems']})
                pos=None
    return trades

def stats(trades):
    if not trades: return dict(n=0,net=0,pf=0,wr=0,avg=0,mdd=0)
    wins=[t['ret'] for t in trades if t['ret']>0]; losses=[t['ret'] for t in trades if t['ret']<=0]
    gp=sum(wins); gl=-sum(losses)
    eq=0; peak=0; mdd=0
    for t in trades:
        eq+=t['ret']; peak=max(peak,eq); mdd=min(mdd,eq-peak)
    return dict(n=len(trades), net=round(sum(t['ret'] for t in trades)*100,1),
                pf=round(gp/gl,2) if gl>0 else 99.9,
                wr=round(100*len(wins)/len(trades),0),
                avg=round(sum(t['ret'] for t in trades)/len(trades)*100,2),
                mdd=round(mdd*100,1))

def ms_of(y,m,d): return int(datetime(y,m,d,tzinfo=timezone.utc).timestamp()*1000)

if __name__=="__main__":
    syms=sys.argv[1].split(",") if len(sys.argv)>1 else ["BTC"]
    JUN0=ms_of(2026,6,1); JUNX=ms_of(2026,7,1)
    BEAR0=ms_of(2022,1,1); BEARX=ms_of(2022,12,31)
    data={s:series(s) for s in syms}
    for s in syms:
        b=data[s]
        print(f"\n### {s}  bars={len(b)} last_close={b[-1][4]:.0f} last={datetime.fromtimestamp(b[-1][0]/1000,timezone.utc):%Y-%m-%d %H:%M}")
    # default config
    cfg=dict(don_n=24, atr_n=24, trail_m=2.5, cost_k=3.0)
    print(f"\ncfg={cfg} cost={COST_BP}bp")
    for label,tf in [("none (operator)","none"),("ema200","ema200"),("ema50_rising","ema50_rising"),("dual_50_200","dual"),("golden_cross","golden")]:
        print(f"\n== filter={label} ==")
        for win,(a,z) in [("JUNE-2026",(JUN0,JUNX)),("2022-BEAR",(BEAR0,BEARX))]:
            agg=[]
            for s in syms:
                tr=backtest(data[s],trend_filter=tf,t0=a,t1=z,**cfg)
                agg+=tr
            st=stats(agg)
            print(f"  {win:10s} n={st['n']:3d} net={st['net']:7.1f}% PF={st['pf']:5} WR={st['wr']:4}% avgR={st['avg']:6}% maxDD={st['mdd']:7.1f}%")
