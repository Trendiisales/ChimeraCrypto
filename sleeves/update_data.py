#!/usr/bin/env python3
"""Incremental 15m data updater for the sleeve universe. Appends only new bars
since each file's last timestamp. Full-fetches a missing symbol from 2020. Keeps
data/multiyr consistent + current so the shadow sleeves run on fresh prices daily."""
import json, urllib.request, time, sys, csv, os, glob
OUT = os.path.join(os.path.dirname(__file__), "data", "multiyr")
os.makedirs(OUT, exist_ok=True)
START_2020 = 1577836800000
INTERVAL = "15m"
def now_ms(): return int(time.time()*1000)
def universe():
    # symbols already present; plus the validated default set on first run
    have = {os.path.basename(f).split("USDT")[0] for f in glob.glob(os.path.join(OUT,"*_15m.csv"))}
    default = ("BTC ETH BNB XRP ADA DOGE SOL DOT AVAX LINK LTC ATOM UNI AAVE FIL TRX ETC XLM ALGO "
               "NEAR SAND MANA CRV SUSHI COMP SNX GRT CHZ ENJ BAT ZEC DASH ZIL ONE HBAR EGLD THETA "
               "VET NEO QTUM AXS ICP RUNE KSM GALA APE LDO INJ").split()
    return sorted(have | set(default))
def last_ts(f):
    last=None
    with open(f) as fh:
        for ln in fh: last=ln
    try: return int(last.split(",")[0])
    except Exception: return None
def fetch(base, start):
    rows=[]
    while start < now_ms():
        u=f"https://api.binance.com/api/v3/klines?symbol={base}&interval={INTERVAL}&startTime={start}&limit=1000"
        try: d=json.loads(urllib.request.urlopen(u,timeout=25).read())
        except Exception as e:
            if "400" in str(e) or "451" in str(e): return None
            time.sleep(0.5); continue
        if not d: break
        rows.extend(d)
        if len(d)<1000: break
        start=d[-1][6]+1
    return rows
def main():
    for s in universe():
        f=os.path.join(OUT, f"{s}USDT_15m.csv")
        if os.path.exists(f):
            lt=last_ts(f)
            if lt is None: continue
            if now_ms()-lt < 15*60*1000: continue   # already current
            r=fetch(s+"USDT", lt+1)
            if not r: print(f"skip {s}", file=sys.stderr); continue
            with open(f,"a",newline="") as fh:
                w=csv.writer(fh)
                for x in r: w.writerow(x)
            print(f"{s}: +{len(r)} bars", file=sys.stderr)
        else:
            r=fetch(s+"USDT", START_2020)
            if not r: print(f"skip new {s}", file=sys.stderr); continue
            with open(f,"w",newline="") as fh:
                w=csv.writer(fh); w.writerow(["open_time_ms","o","h","l","c","v","ct","qv","n","tb","tq","ig"])
                for x in r: w.writerow(x)
            print(f"{s}: NEW {len(r)} bars", file=sys.stderr)
    print("UPDATE DONE", file=sys.stderr)
if __name__=="__main__": main()
