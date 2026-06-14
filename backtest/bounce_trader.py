#!/usr/bin/env python3
"""Tactical bounce-trader: trade up-moves even in macro-bear, with a FAST-EXIT
mechanism (move to breakeven after +X, then trail to lock profit). Tests the
operator's thesis: can we catch bounces and bail before the turn, WITHOUT the
slow macro gate destroying the edge? Per-symbol, short timeframe, no macro gate.
Focus: ETH + the better names; measure BEAR-window survival specifically.

Entry  : close breaks above prior N-bar high (bounce/breakout trigger)
Stop   : entry - sl_atr*ATR ; after +be_arm*ATR move stop to BREAKEVEN ;
         after +trail_arm*ATR trail by trail_dist*ATR (locks profit, exits on turn)
"""
import csv, glob, os, math, datetime, itertools, sys

DATADIR="data/multiyr"
COST_BP=float(os.environ.get("COST_BP","15"))

def load_hourly(sym, hours=1):
    """Aggregate 15m -> N-hour bars. Returns list of (ts, o,h,l,c)."""
    f=os.path.join(DATADIR, f"{sym}USDT_15m.csv")
    if not os.path.exists(f): return []
    raw=[]
    with open(f) as fh:
        r=csv.reader(fh); next(r,None)
        for x in r:
            try: raw.append((int(x[0]),float(x[1]),float(x[2]),float(x[3]),float(x[4])))
            except: pass
    bars=[]; span=hours*3600000
    cur=None
    for ts,o,h,l,c in raw:
        b=ts//span
        if cur is None or b!=cur[0]:
            if cur: bars.append(cur[1])
            cur=(b,[ts,o,h,l,c])
        else:
            cur[1][2]=max(cur[1][2],h); cur[1][3]=min(cur[1][3],l); cur[1][4]=c
    if cur: bars.append(cur[1])
    return bars

def atr(bars,i,n=14):
    if i<n: return None
    s=0
    for j in range(i-n,i):
        tr=max(bars[j][2]-bars[j][3], abs(bars[j][2]-bars[j-1][4]), abs(bars[j][3]-bars[j-1][4]))
        s+=tr
    return s/n

def run_symbol(bars, N, sl_atr, be_arm, trail_arm, trail_dist, hold_max):
    """Returns list of trades: (entry_ts,exit_ts,entry,exit,net_bp,reason)."""
    trades=[]; pos=None
    for i in range(N+15, len(bars)):
        ts,o,h,l,c=bars[i]
        a=atr(bars,i)
        if a is None or a<=0: continue
        if pos:
            # manage: update stop (BE / trail), check exit on this bar's L/H
            entry,stop,be_done,trail_on,peak,ets,held=pos
            # mark peak
            peak=max(peak,h)
            prof_atr=(peak-entry)/a
            if not be_done and prof_atr>=be_arm:
                stop=max(stop,entry); be_done=True   # move to breakeven
            if prof_atr>=trail_arm:
                stop=max(stop, peak-trail_dist*a); trail_on=True
            # exit if low breaches stop (fill at stop, or gap)
            exitpx=None; reason=None
            if l<=stop:
                exitpx=min(stop, o); reason=("TRAIL" if trail_on else ("BE" if be_done else "SL"))
            elif held>=hold_max:
                exitpx=c; reason="TIME"
            if exitpx is not None:
                net=(exitpx/entry-1)*1e4 - COST_BP*2
                trades.append((ets,ts,entry,exitpx,net,reason)); pos=None
            else:
                pos=(entry,stop,be_done,trail_on,peak,ets,held+1)
            continue
        # entry: breakout of prior N-bar high
        hi=max(bars[j][2] for j in range(i-N,i))
        if c>hi:
            entry=c; stop=entry-sl_atr*a
            pos=(entry,stop,False,False,entry,ts,0)
    return trades

def windows():
    def dms(y,m,d): return int(datetime.datetime(y,m,d,tzinfo=datetime.timezone.utc).timestamp()*1000)
    return [("21bull",dms(2021,1,1),dms(2021,11,10)),("22BEAR",dms(2022,1,1),dms(2023,1,1)),
            ("23rec",dms(2023,1,1),dms(2024,1,1)),("24bull",dms(2024,1,1),dms(2025,1,1)),
            ("25hold",dms(2025,1,1),dms(2026,7,1))]

def stats(trades,a=None,b=None):
    t=[x for x in trades if (a is None or a<=x[1]<b)]
    if not t: return None
    net=sum(x[4] for x in t); w=sum(1 for x in t if x[4]>0)
    gw=sum(x[4] for x in t if x[4]>0); gl=-sum(x[4] for x in t if x[4]<=0)
    pf=gw/gl if gl>0 else (99 if gw>0 else 0)
    eq=0;peak=0;dd=0
    for x in t: eq+=x[4]; peak=max(peak,eq); dd=max(dd,peak-eq)
    return dict(n=len(t),net=net,wr=100*w/len(t),pf=pf,dd=dd,worst=min(x[4] for x in t))

def main():
    syms=sys.argv[1].split(",") if len(sys.argv)>1 else ["ETH","SOL","BNB","LINK","BTC","ADA","AVAX","XRP"]
    hours=int(os.environ.get("HOURS","4"))
    # tactical config: tight stop, BE fast, trail to lock
    N=int(os.environ.get("N","20")); sl=float(os.environ.get("SL","1.5"))
    be=float(os.environ.get("BE","1.0")); ta=float(os.environ.get("TA","1.5")); td=float(os.environ.get("TD","1.5"))
    hm=int(os.environ.get("HOLD","60"))
    print(f"BOUNCE TRADER (no macro gate) — {hours}h bars, breakout N={N}, SL={sl}ATR, BE-arm={be}ATR, trail {ta}/{td}ATR, cost {COST_BP}bp/side\n")
    print(f"{'sym':<6}{'ALL net%':>10}{'PF':>6}{'WR':>5}{'tr':>5} | {'22BEAR net%':>12}{'PF':>6}{'WR':>5}{'tr':>5}{'maxDD%':>8}")
    print("-"*76)
    W=windows()
    agg_bear=[]; agg_all=[]
    for s in syms:
        bars=load_hourly(s,hours)
        if len(bars)<300: continue
        tr=run_symbol(bars,N,sl,be,ta,td,hm)
        al=stats(tr); be_w=stats(tr,W[1][1],W[1][2])
        if not al: continue
        agg_all+=tr
        bs=f"{be_w['net']/100:+8.0f}%{be_w['pf']:>6.2f}{be_w['wr']:>4.0f}%{be_w['n']:>5}{be_w['dd']/100:>7.0f}%" if be_w else f"{'no trades':>33}"
        print(f"{s:<6}{al['net']/100:>9.0f}%{al['pf']:>6.2f}{al['wr']:>4.0f}%{al['n']:>5} | {bs}")
    print("-"*76)
    pool=stats(agg_all); pb=stats(agg_all,W[1][1],W[1][2])
    if pool: print(f"{'POOL':<6}{pool['net']/100:>9.0f}%{pool['pf']:>6.2f}{pool['wr']:>4.0f}%{pool['n']:>5} | "
                   f"{pb['net']/100:+8.0f}%{pb['pf']:>6.2f}{pb['wr']:>4.0f}%{pb['n']:>5}{pb['dd']/100:>7.0f}%" if pb else "")
    print("\nKey question: is 22BEAR PF>1? If yes, BE+trail lets us trade bounces in bear. If <1, it chops.")

if __name__=="__main__": main()
