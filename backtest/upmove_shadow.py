#!/usr/bin/env python3
"""
UpMoveTrail SHADOW EXECUTOR — stateful, hourly, NO ORDERS.
Faithful 1h deploy of the WF-confirmed up-move/hard-trail engine (build==backtest).
Self-contained: fetches own 1h data from Binance, manages open positions across runs,
logs entries+exits to a shadow ledger. Regime-gated + justification-roster.

Run hourly (systemd timer / cron). Isolated from sleeves/chimera_sleeves.py.
State: upmove_positions.json   Ledger: upmove_shadow_ledger.csv
"""
import json, os, csv, time, urllib.request
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
POS  = os.path.join(HERE, "upmove_positions.json")
LEDG = os.path.join(HERE, "upmove_shadow_ledger.csv")

# ---- validated config (do NOT retune without fresh WF) ----
UNIVERSE = ["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","LINK","DOT","LTC",
            "DOGE","ATOM","UNI","FIL","NEAR"]
DON_N, ATR_N, TRAIL_M, STOP_M, TS_BARS, COST_K = 24, 24, 5.0, 3.0, 48, 3.0
COST_BP = 17.0
FETCH_DAYS = 260                 # enough for EMA200 (1h) + 180d roster
ROSTER_EVAL_BARS = 180*24
INCLUDE_MARGIN, ROSTER_MIN_TRADES = 5.0, 8
H1 = 3600_000

# ---------------- data (self-contained, Binance 1h) ----------------
def fetch(sym, ncalls_days=FETCH_DAYS):
    base="https://api.binance.com/api/v3/klines"
    start=int((time.time()-ncalls_days*86400)*1000)
    out=[]; cur=start
    for _ in range(400):
        url=f"{base}?symbol={sym}USDT&interval=1h&startTime={cur}&limit=1000"
        d=json.load(urllib.request.urlopen(url,timeout=25))
        if not d: break
        for k in d: out.append([int(k[0]),float(k[1]),float(k[2]),float(k[3]),float(k[4])])
        cur=d[-1][0]+1
        if len(d)<1000: break
        time.sleep(0.2)
    # drop the in-progress (not-yet-closed) final bar: keep only fully closed
    now=time.time()*1000
    return [b for b in out if b[0]+H1 <= now]

def atr_arr(bars,n=ATR_N):
    A=[None]*len(bars)
    for i in range(n,len(bars)):
        s=0.0
        for j in range(i-n+1,i+1):
            h,l,pc=bars[j][2],bars[j][3],bars[j-1][4]; s+=max(h-l,abs(h-pc),abs(l-pc))
        A[i]=s/n
    return A
def ema_arr(bars,p):
    k=2/(p+1); e=[None]*len(bars); prev=bars[0][4]
    for i,b in enumerate(bars): prev=b[4]*k+prev*(1-k); e[i]=prev
    return e

# ---------------- engine primitives (identical to backtest) ----------------
def entry_ok(bars,A,e50,e200,i):
    c=bars[i][4]; a=A[i]
    if a is None: return None
    if not (e50[i]>e200[i] and c>e50[i]): return None
    if a < COST_K*(c*COST_BP/10000.0): return None
    donhi=max(bars[j][2] for j in range(i-DON_N,i))
    if c>donhi:
        return dict(entry=round(c,6), atr=round(a,6), lvl=round(donhi,6),
                    stop=round(c-STOP_M*a,6), trail0=round(c-TRAIL_M*a,6))
    return None

def regime_bull(btc):
    closes=[b[4] for b in btc]
    if len(closes)<200*24: return False,{}
    daily=closes[::24]
    if len(daily)<200: return False,{}
    c=daily[-1]; m50=sum(daily[-50:])/50; m200=sum(daily[-200:])/200
    m200p=sum(daily[-224:-24])/200 if len(daily)>=224 else m200
    return (c>m50 and (c>m200 or m200>m200p)), dict(btc=round(c),ma50=round(m50),ma200=round(m200),rising=m200>m200p)

def trailing_net(bars,A,e50,e200,t0):
    # quick backtest over trailing window for the roster justification test
    cost=COST_BP/10000.0; trades=[]; pos=None
    for i in range(ATR_N,len(bars)):
        b=bars[i]; c=b[4]; a=A[i]
        if a is None: continue
        if pos is None:
            if b[0]<t0: continue
            ek=entry_ok(bars,A,e50,e200,i)
            if ek: pos=dict(e=ek['entry'],mx=ek['entry'],i=i,stop=ek['stop'])
        else:
            pos['mx']=max(pos['mx'],c); ex=None
            if b[3]<=pos['stop']: ex=pos['stop']
            elif (i-pos['i'])>=TS_BARS and c<pos['e']: ex=c
            else:
                tr=pos['mx']-TRAIL_M*a
                if c<tr or i==len(bars)-1: ex=c
            if ex is not None: trades.append((ex-pos['e'])/pos['e']-cost); pos=None
    return round(sum(trades)*100,1), len(trades)

# ---------------- ledger ----------------
def log(asof, kind, sym, info):
    new = not os.path.exists(LEDG)
    with open(LEDG,"a",newline="") as f:
        w=csv.writer(f)
        if new: w.writerow(["ts_utc","event","symbol","price","detail"])
        w.writerow([asof, kind, sym, info.get("price",""), json.dumps(info)])

# ---------------- main hourly cycle ----------------
def run():
    asof=datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%MZ")
    data={};
    for s in UNIVERSE:
        try: data[s]=fetch(s)
        except Exception as e: print(f"fetch fail {s}: {e}")
    if "BTC" not in data: print("no BTC data, abort"); return
    AE={s:(atr_arr(b),ema_arr(b,50),ema_arr(b,200)) for s,b in data.items()}
    bull,reg=regime_bull(data["BTC"])
    now_ms=data["BTC"][-1][0]; t0=now_ms-ROSTER_EVAL_BARS*H1
    # roster: justification-gated
    active=[]
    for s in UNIVERSE:
        if s not in data: continue
        net,n=trailing_net(data[s],*AE[s],t0)
        if n>=ROSTER_MIN_TRADES and net>=INCLUDE_MARGIN: active.append(s)
    pos=json.load(open(POS)) if os.path.exists(POS) else {}
    print(f"[{asof}] regime={'BULL' if bull else 'FLAT'} {reg} | roster {len(active)}/{len(UNIVERSE)}: {active}")
    # 1) manage OPEN positions (exits) — runs regardless of regime/roster
    for s in list(pos.keys()):
        if s not in data: continue
        b=data[s]; i=len(b)-1; c=b[i][4]; a=AE[s][0][i]; p=pos[s]
        p["maxc"]=max(p["maxc"],c); ex=None; why=None
        bars_held=int((now_ms-p["entry_ms"])//H1)
        if b[i][3]<=p["stop"]:                       ex,why=p["stop"],"hard-stop"
        elif bars_held>=TS_BARS and c<p["entry"]:    ex,why=c,"timestop"
        elif a is not None and c < p["maxc"]-TRAIL_M*a: ex,why=c,"trail"
        if ex is not None:
            ret=(ex-p["entry"])/p["entry"]-COST_BP/10000.0
            log(asof,"EXIT",s,dict(price=round(ex,6),entry=p["entry"],ret_pct=round(ret*100,2),reason=why,
                                   bars_held=int((now_ms-p["entry_ms"])//H1)))
            print(f"   EXIT {s} @{ex:.4f} ({why}) ret {ret*100:+.2f}%")
            del pos[s]
    # 2) new ENTRIES — only if regime bull + symbol active + no open position
    if bull:
        for s in active:
            if s in pos or s not in data: continue
            b=data[s]; i=len(b)-1
            ek=entry_ok(b,*AE[s],i)
            if ek:
                pos[s]=dict(entry=ek["entry"],maxc=ek["entry"],stop=ek["stop"],
                            entry_ms=b[i][0],atr0=ek["atr"])
                log(asof,"ENTRY",s,dict(price=ek["entry"],stop=ek["stop"],trail0=ek["trail0"],lvl=ek["lvl"]))
                print(f"   ENTRY {s} @{ek['entry']:.4f} stop {ek['stop']:.4f}")
    json.dump(pos,open(POS,"w"),indent=0)
    print(f"   open positions: {list(pos.keys()) or 'none'}  (NO ORDERS placed)")

if __name__=="__main__":
    run()
