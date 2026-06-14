#!/usr/bin/env python3
"""Exhaustive bracket + entry sweep — pull EVERY lever for a bear-tradeable edge.
Entries:  BREAKOUT (trend) | RSI_REVERT (buy oversold, mean-rev) | DIP (pullback)
          | BBAND (lower-band bounce)
Exits:    fixed-TP/SL BRACKET (OCO) across tp_atr x sl_atr grid (small-regular vs
          fat-TP). Conservative fill: if a bar's range hits both TP and SL, assume
          SL first (no look-ahead optimism).
Per-symbol + pooled, ALL period + 2022 BEAR. Find ANY config with bear PF>1.
"""
import csv, glob, os, math, datetime, itertools, sys
DATADIR="data/multiyr"; COST_BP=float(os.environ.get("COST_BP","15"))

def load_bars(sym,hours):
    f=os.path.join(DATADIR,f"{sym}USDT_15m.csv")
    if not os.path.exists(f): return []
    raw=[]
    with open(f) as fh:
        r=csv.reader(fh); next(r,None)
        for x in r:
            try: raw.append((int(x[0]),float(x[1]),float(x[2]),float(x[3]),float(x[4])))
            except: pass
    bars=[]; span=hours*3600000; cur=None
    for ts,o,h,l,c in raw:
        b=ts//span
        if cur is None or b!=cur[0]:
            if cur: bars.append(cur[1])
            cur=(b,[ts,o,h,l,c])
        else: cur[1][2]=max(cur[1][2],h); cur[1][3]=min(cur[1][3],l); cur[1][4]=c
    if cur: bars.append(cur[1])
    return bars

def atr(b,i,n=14):
    if i<n: return None
    s=sum(max(b[j][2]-b[j][3],abs(b[j][2]-b[j-1][4]),abs(b[j][3]-b[j-1][4])) for j in range(i-n,i))
    return s/n
def rsi(b,i,n=14):
    if i<n+1: return None
    g=l=0
    for j in range(i-n,i):
        d=b[j][4]-b[j-1][4]
        if d>0: g+=d
        else: l-=d
    if g+l==0: return 50
    rs=g/(l if l>0 else 1e-9); return 100-100/(1+rs)
def sma(b,i,n):
    if i<n: return None
    return sum(b[j][4] for j in range(i-n,i))/n

def signal(b,i,kind,a):
    c=b[i][4]
    if kind=="BREAKOUT":
        hi=max(b[j][2] for j in range(i-20,i)); return c>hi
    if kind=="RSI_REVERT":
        r0=rsi(b,i-1); r1=rsi(b,i)
        return r0 is not None and r1 is not None and r0<30 and r1>r0   # oversold turning up
    if kind=="DIP":
        m=sma(b,i,50)
        return m is not None and c>m and b[i][3]<=m*1.01 and c>b[i-1][4]  # uptrend pullback to MA, bounce
    if kind=="BBAND":
        m=sma(b,i,20)
        if m is None: return False
        sd=math.sqrt(sum((b[j][4]-m)**2 for j in range(i-20,i))/20)
        return b[i][3]<m-2*sd and c>b[i][3]+0.2*(b[i][2]-b[i][3])  # pierce lower band, close up
    return False

def run(b,kind,tp_atr,sl_atr,hold):
    trades=[]; pos=None
    for i in range(60,len(b)):
        ts,o,h,l,c=b[i]; a=atr(b,i)
        if a is None or a<=0: continue
        if pos:
            entry,tp,sl,ets,held=pos; ex=None;rs=None
            if l<=sl: ex=sl;rs="SL"          # conservative: SL checked first
            elif h>=tp: ex=tp;rs="TP"
            elif held>=hold: ex=c;rs="TIME"
            if ex is not None:
                trades.append((ets,ts,(ex/entry-1)*1e4-COST_BP*2,rs)); pos=None
            else: pos=(entry,tp,sl,ets,held+1)
            continue
        if signal(b,i,kind,a):
            entry=c; pos=(entry,entry+tp_atr*a,entry-sl_atr*a,ts,0)
    return trades

def st(t,a=None,bb=None):
    t=[x for x in t if (a is None or a<=x[1]<bb)]
    if len(t)<10: return None
    net=sum(x[2] for x in t); w=sum(1 for x in t if x[2]>0)
    gw=sum(x[2] for x in t if x[2]>0); gl=-sum(x[2] for x in t if x[2]<=0)
    return dict(n=len(t),net=net,wr=100*w/len(t),pf=gw/gl if gl>0 else (99 if gw>0 else 0))

def main():
    syms=["ETH","SOL","BNB","LINK","ADA","AVAX","BTC","XRP","DOGE","LTC"]
    hours=int(os.environ.get("HOURS","4"))
    def dms(y,m,d): return int(datetime.datetime(y,m,d,tzinfo=datetime.timezone.utc).timestamp()*1000)
    BEAR=(dms(2022,1,1),dms(2023,1,1))
    print(f"BRACKET SWEEP — {hours}h bars, conservative fills, cost {COST_BP}bp/side, pooled over {len(syms)} syms")
    print(f"{'entry':<11}{'TP/SL(atr)':>11}{'ALL: net%':>11}{'PF':>6}{'WR':>5}{'tr':>6} | {'BEAR net%':>10}{'PF':>6}{'WR':>5}{'tr':>5}")
    print("-"*86)
    bars={s:load_bars(s,hours) for s in syms}
    best_bear=None
    for kind in ["BREAKOUT","RSI_REVERT","DIP","BBAND"]:
        for tp,sl in [(1,1),(1,2),(2,1),(2,2),(1,3),(3,1),(0.5,1),(2,3),(3,2)]:
            allt=[]
            for s in syms:
                if len(bars[s])>300: allt+=run(bars[s],kind,tp,sl,40)
            A=st(allt); B=st(allt,*BEAR)
            if not A: continue
            flag=""
            if B and B["pf"]>1.0: flag=" <<< BEAR PF>1"; best_bear=(kind,tp,sl,B)
            bs=f"{B['net']/100:+9.0f}%{B['pf']:>6.2f}{B['wr']:>4.0f}%{B['n']:>5}" if B else f"{'--':>26}"
            print(f"{kind:<11}{f'{tp}/{sl}':>11}{A['net']/100:>10.0f}%{A['pf']:>6.2f}{A['wr']:>4.0f}%{A['n']:>6} | {bs}{flag}")
    print("-"*86)
    print("BEAR PF>1 found!" if best_bear else "NO config achieved bear PF>1 across any entry x bracket. Every lever pulled.")

if __name__=="__main__": main()
