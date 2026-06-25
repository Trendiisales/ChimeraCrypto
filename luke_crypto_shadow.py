#!/usr/bin/env python3
"""luke_crypto_shadow.py — LIVE SHADOW executor for the LukeCryptoMomentum engine.

Self-contained (fetches own daily klines from Binance), manages open positions across daily runs,
paper-fills the validated C (+B) arm-and-wait entries, rides wide (exit on first daily close<9EMA),
BTC>200MA regime-gated. Logs entries+exits to a shadow ledger -> the BACKTEST_TRUTH verdict.
Matches the upmove_shadow.py convention. Schedule daily (launchd). SHADOW: zero capital risk.

  python3 luke_crypto_shadow.py          # one daily run: manage + maybe enter, append ledger
State:  luke_crypto_positions.json   Ledger: luke_crypto_shadow_ledger.csv
"""
import os, json, time, urllib.request, datetime as dt
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
POS  = os.path.join(HERE, "luke_crypto_positions.json")
LED  = os.path.join(HERE, "luke_crypto_shadow_ledger.csv")
UNIV = "BTC ETH SOL BNB XRP ADA AVAX DOGE LINK LTC DOT ATOM NEAR APT ARB OP INJ SUI TIA SEI".split()
EQUITY0, RISK_PCT = 100000.0, 0.01
ADR_MIN, MAX_STOPW, STOP_BUF = 4.0, 0.06, 0.003
MAX_CONCURRENT = 5

def fetch(sym, n=320):
    url=f"https://api.binance.com/api/v3/klines?symbol={sym}USDT&interval=1d&limit={n}"
    try:
        d=json.load(urllib.request.urlopen(url, timeout=20))
        return [(int(k[0]), float(k[1]), float(k[2]), float(k[3]), float(k[4]), float(k[5])) for k in d]
    except Exception:
        return None

def ema(x, span):
    a=2/(span+1); e=x[0]
    out=[e]
    for v in x[1:]: e=v*a+e*(1-a); out.append(e)
    return out

def indicators(bars):
    o=[b[1] for b in bars]; h=[b[2] for b in bars]; l=[b[3] for b in bars]; c=[b[4] for b in bars]; v=[b[5] for b in bars]
    e9=ema(c,9); e21=ema(c,21); e50=ema(c,50)
    adr=[np.nan]*len(c)
    for i in range(20,len(c)): adr[i]=np.mean([(h[j]-l[j])/c[j]*100 for j in range(i-19,i+1)])
    # anchored vwap from recent swing low
    tp=[(h[i]+l[i]+c[i])/3 for i in range(len(c))]; av=[np.nan]*len(c); cpv=cv=0.0
    for i in range(len(c)):
        sw = 2<=i<len(c)-2 and l[i]==min(l[max(0,i-40):i+1]) and l[i]<l[i-1] and l[i]<l[i+1]
        if sw: cpv=tp[i]*v[i]; cv=v[i]
        else: cpv+=tp[i]*v[i]; cv+=v[i]
        av[i]=cpv/cv if cv>0 else np.nan
    return dict(o=o,h=h,l=l,c=c,e9=e9,e21=e21,e50=e50,adr=adr,av=av,sl=[e21[i]-e21[i-5] if i>=5 else 0 for i in range(len(c))])

def setup(ind, i):
    """C (inside-day/VCP) primary + B (avwap cluster) secondary on decision bar i. Returns (kind,trig,stop) or None."""
    h,l,c,e9,e21,e50,adr,av,sl=ind['h'],ind['l'],ind['c'],ind['e9'],ind['e21'],ind['e50'],ind['adr'],ind['av'],ind['sl']
    if np.isnan(e50[i]) or np.isnan(adr[i]) or adr[i]<ADR_MIN: return None
    up=sl[i]>0
    if up and e9[i]>e21[i] and c[i]>e21[i]:
        in1=h[i]<h[i-1] and l[i]>l[i-1]; in2=in1 and h[i-1]<h[i-2] and l[i-1]>l[i-2]
        if in1 or in2:
            trig=h[i]; stop=l[i]*(1-STOP_BUF)
            if trig>stop and (trig-stop)/trig<=MAX_STOPW: return ('C',trig,stop)
    if up and c[i]>e50[i] and not np.isnan(av[i]):
        if abs(l[i]-av[i])/c[i]<0.02 and c[i]>av[i]:
            band=0.02*c[i]; ncl=sum(1 for x in (av[i],e21[i],e9[i],round(c[i])) if abs(x-l[i])<=band)
            if ncl>=2:
                trig=h[i]; stop=min(av[i],l[i])*(1-STOP_BUF)
                if trig>stop and (trig-stop)/trig<=MAX_STOPW: return ('B',trig,stop)
    return None

def main():
    pos=json.load(open(POS)) if os.path.exists(POS) else {}
    data={s:fetch(s) for s in UNIV}; data={s:d for s,d in data.items() if d and len(d)>60}
    if 'BTC' not in data: print("no BTC data — abort"); return
    btc=indicators(data['BTC']); regime_ok = btc['c'][-1] > btc['e50'][-1] and btc['c'][-1] > np.mean(btc['c'][-200:])
    # actually BTC>200d SMA:
    btc_sma200=np.mean(btc['c'][-200:]); regime_ok = btc['c'][-1] > btc_sma200
    now=int(time.time()*1000); newrows=[]
    inds={s:indicators(d) for s,d in data.items()}
    # 1) manage open positions on the latest bar
    for s in list(pos.keys()):
        if s not in inds: continue
        ind=inds[s]; i=len(ind['c'])-1; p=pos[s]
        lo,cl,e9=ind['l'][i],ind['c'][i],ind['e9'][i]
        if lo<=p['stop']:
            px=p['stop']; pnl=p['shares']*(px-p['entry'])
            newrows.append([now,'EXIT',s,p['setup'],round(p['entry'],4),round(px,4),round(pnl,2),'STOP']); del pos[s]
        elif cl<e9:
            px=cl; pnl=p['shares']*(px-p['entry'])
            newrows.append([now,'EXIT',s,p['setup'],round(p['entry'],4),round(px,4),round(pnl,2),'CLOSE<9EMA']); del pos[s]
    # 2) entries: arm setup on prior bar, fire on today's break (arm-and-wait), gated
    if regime_ok:
        cands=[]
        for s,ind in inds.items():
            if s in pos: continue
            i=len(ind['c'])-2  # decision bar = yesterday (armed); today = break check
            sig=setup(ind,i)
            if not sig: continue
            kind,trig,stop=sig
            today_h=ind['h'][-1]; today_o=ind['o'][-1]
            if today_h>=trig and ind['l'][-1]>stop:   # broke the armed trigger today
                cands.append((s,kind,max(trig,today_o),stop))
        cands.sort(key=lambda x:(x[2]-x[3])/x[2])  # tightest stop first
        eq=EQUITY0  # shadow notional base
        for s,kind,fill,stop in cands:
            if len(pos)>=MAX_CONCURRENT: break
            sw=fill-stop
            if sw<=0: continue
            shares=int(eq*RISK_PCT/sw)
            if shares<=0: continue
            pos[s]=dict(entry=fill,stop=stop,shares=shares,setup=kind,opened=now)
            newrows.append([now,'ENTRY',s,kind,round(fill,4),0,0,f'LUKE-BREAK gate=BTC>200MA'])
    json.dump(pos, open(POS,'w'), indent=1)
    new=not os.path.exists(LED)
    with open(LED,'a') as f:
        if new: f.write("ts_ms,action,sym,setup,entry,exit,pnl,reason\n")
        for r in newrows: f.write(",".join(str(x) for x in r)+"\n")
    ts=dt.datetime.utcnow().strftime("%Y-%m-%d %H:%M")
    print(f"[{ts}] LukeCryptoShadow: regime_ok={regime_ok} (BTC {btc['c'][-1]:.0f} vs 200d {btc_sma200:.0f}) "
          f"open={len(pos)} new_events={len(newrows)} {[r[1]+':'+r[2] for r in newrows]}")

if __name__=='__main__':
    main()
