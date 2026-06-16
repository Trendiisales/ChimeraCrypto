#!/usr/bin/env python3
"""Same WF-confirmed up-move config across timeframes 5m..4h. Bars-native params.
Cost 17bp held constant (UNDERSTATES 5m/15m slippage -> fast-TF results optimistic).
Span Jun2025->now (bull + the real 2026 -28.6% bear). Cached to /tmp/mtf_cache."""
import json,os,time,urllib.request,csv
from datetime import datetime,timezone
SYMS=["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","DOGE","NEAR","LINK"]
TFS=["5m","15m","1h","4h"]
TF_MS={"5m":300_000,"15m":900_000,"1h":3600_000,"4h":14400_000}
START=int(datetime(2025,6,1,tzinfo=timezone.utc).timestamp()*1000)
COST_BP=17.0
DON_N,ATR_N,TRAIL_M,STOP_M,TS_BARS,COST_K=24,24,5.0,3.0,48,3.0
CACHE="/tmp/mtf_cache"; os.makedirs(CACHE,exist_ok=True)

def fetch(sym,tf):
    cp=f"{CACHE}/{sym}_{tf}.csv"
    if os.path.exists(cp):
        return [[int(r[0])]+[float(x) for x in r[1:]] for r in csv.reader(open(cp))]
    base="https://api.binance.com/api/v3/klines"; out=[];cur=START
    for _ in range(2000):
        url=f"{base}?symbol={sym}USDT&interval={tf}&startTime={cur}&limit=1000"
        d=json.load(urllib.request.urlopen(url,timeout=25))
        if not d: break
        for k in d: out.append([int(k[0]),float(k[1]),float(k[2]),float(k[3]),float(k[4])])
        cur=d[-1][0]+1
        if len(d)<1000: break
        time.sleep(0.15)
    now=time.time()*1000; out=[b for b in out if b[0]+TF_MS[tf]<=now]
    csv.writer(open(cp,"w")).writerows(out)
    return out

def atr(bars,n,i):
    if i<n: return None
    s=0.0
    for j in range(i-n+1,i+1):
        h,l,pc=bars[j][2],bars[j][3],bars[j-1][4]; s+=max(h-l,abs(h-pc),abs(l-pc))
    return s/n
def ema(bars,p):
    k=2/(p+1);e=[None]*len(bars);pv=bars[0][4]
    for i,b in enumerate(bars): pv=b[4]*k+pv*(1-k);e[i]=pv
    return e
def bt(bars,e50,e200,t0,t1):
    cost=COST_BP/10000.0;tr=[];pos=None
    for i in range(max(DON_N,200),len(bars)):
        b=bars[i];c=b[4];a=atr(bars,ATR_N,i)
        if a is None: continue
        if pos is None:
            if b[0]<t0 or b[0]>t1: continue
            if a<COST_K*(c*cost): continue
            if not(e50[i]>e200[i] and c>e50[i]): continue
            if c>max(bars[j][2] for j in range(i-DON_N,i)): pos=dict(e=c,mx=c,i=i,stop=c-STOP_M*a)
        else:
            pos['mx']=max(pos['mx'],c);ex=None
            if b[3]<=pos['stop']: ex=pos['stop']
            elif (i-pos['i'])>=TS_BARS and c<pos['e']: ex=c
            elif c<pos['mx']-TRAIL_M*a: ex=c
            elif i==len(bars)-1: ex=c
            if ex is not None: tr.append((ex-pos['e'])/pos['e']-cost);pos=None
    return tr
def stats(tr):
    if not tr: return (0,0.0,0.0,0,0.0)
    w=[x for x in tr if x>0];l=[x for x in tr if x<=0];gp=sum(w);gl=-sum(l)
    eq=p=mdd=0
    for x in tr: eq+=x;p=max(p,eq);mdd=min(mdd,eq-p)
    return (len(tr),round(sum(tr)*100,1),round(gp/gl,2) if gl>0 else 99.9,round(100*len(w)/len(tr)),round(mdd*100,1))
def ms(y,m,d): return int(datetime(y,m,d,tzinfo=timezone.utc).timestamp()*1000)

FULL=(START,int(time.time()*1000)); BULL25=(ms(2025,6,1),ms(2026,1,1)); BEAR26=(ms(2026,5,6),ms(2026,6,16))
if __name__=="__main__":
    print(f"basket {len(SYMS)} syms, span Jun2025->now, cost {COST_BP}bp (5m/15m optimistic)\n")
    print(f"{'TF':>4} | {'FULL net':>9} {'#':>5} {'PF':>5} {'WR':>4} {'mDD':>7} | {'H2-25bull':>9} | {'2026bear':>8}")
    print('-'*72)
    for tf in TFS:
        data={};
        for s in SYMS:
            try: data[s]=fetch(s,tf)
            except Exception as e: print("fetch fail",s,tf,e)
        EM={s:(ema(b,50),ema(b,200)) for s,b in data.items()}
        def agg(win):
            allt=[]
            for s in data: allt+=bt(data[s],*EM[s],win[0],win[1])
            return allt
        f=stats(agg(FULL)); bl=stats(agg(BULL25)); br=stats(agg(BEAR26))
        print(f"{tf:>4} | {f[1]:>+8.0f}% {f[0]:>5} {f[2]:>5} {f[3]:>3}% {f[4]:>+6.0f}% | {bl[1]:>+8.0f}% | {br[1]:>+7.0f}%")
    print("\nnet = basket sum per-trade net%, cost-incl. Watch trade count explode at 5m (cost drag).")
