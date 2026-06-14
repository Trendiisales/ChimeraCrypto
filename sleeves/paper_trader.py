#!/usr/bin/env python3
"""
Paper-trade emulator for the validated sleeves — simulates real fills and writes
them to the ledger in the SAME format as the old Chimera system (trades.json +
open_positions.json), so it reads like live trading.

Each sleeve holds up to K concurrent positions (one per picked coin). On each
14-day rebalance: coins dropped from the top-K are SOLD (closed trade booked),
new top-K coins are BOUGHT (position opened). Macro gate bear -> close all
(MACRO_FLAT). Deterministic full replay each run (no incremental state to drift).

Trade record (old schema): engine, symbol, reason, entry_ts, exit_ts, entry_px,
exit_px, net_bp, mfe_bp.  net_bp = price move (bp) - round-trip cost.
NO real orders — paper only.
"""
import json, os, math
from breakout_portfolio import load_daily, sma
from chimera_sleeves import SLEEVES, compute_target

DATADIR = os.path.join(os.path.dirname(__file__), "data")
COST_RT_BP = 30.0   # 15bp/side round trip
REBAL_DAYS = 14
WARMUP = 205        # need 200d SMA + lookback
# LIVE paper ledger starts FRESH at go-live — only book trades on/after this date.
# Position state still warms from full history; we just don't record old fills.
# (Override with env LIVE_START_MS to backfill.) 2026-06-14 = fresh start.
LIVE_START_MS = int(os.environ.get("LIVE_START_MS", "1781395200000"))

def picks_for(days, syms, close, vol, btc, name, i):
    w,_ = compute_target(days, syms, close, vol, btc, name, i)
    return set(w.keys())

def main():
    days, syms, close, vol = load_daily()
    btc = close.get("BTC")
    open_pos = {}          # "SLEEVE-SYM" -> dict(sleeve,sym,entry_px,entry_ts,mfe_bp)
    trades = []
    last_rebal = -10**9
    for i in range(WARMUP, len(days)):
        ts = days[i]*86400000
        # mark-to-market: update MFE on every open position
        for k,p in open_pos.items():
            px = close[p["sym"]][i]
            if px==px and p["entry_px"]>0:
                bp=(px/p["entry_px"]-1)*1e4
                if bp>p["mfe_bp"]: p["mfe_bp"]=bp
        if days[i]-last_rebal < REBAL_DAYS:
            continue
        last_rebal = days[i]
        m=sma(btc,i,200); bull=(m is not None and btc[i]==btc[i] and btc[i]>m)
        desired={}
        for name in SLEEVES:
            for sym in (picks_for(days,syms,close,vol,btc,name,i) if bull else set()):
                desired[f"{name[:3]}-{sym}"]=(name,sym)
        # CLOSE positions no longer desired
        for k in list(open_pos):
            if k not in desired:
                p=open_pos.pop(k); px=close[p["sym"]][i]
                if px!=px: px=p["entry_px"]
                gross=(px/p["entry_px"]-1)*1e4
                if ts >= LIVE_START_MS:   # fresh live ledger — only book go-live-forward fills
                    trades.append(dict(engine=k, symbol=p["sym"],
                        reason=("MACRO_FLAT" if not bull else "REBAL"),
                        entry_ts=p["entry_ts"], exit_ts=ts,
                        entry_px=p["entry_px"], exit_px=round(px,8),
                        net_bp=round(gross-COST_RT_BP,1), mfe_bp=round(p["mfe_bp"],1)))
        # OPEN new desired positions
        for k,(name,sym) in desired.items():
            if k not in open_pos:
                px=close[sym][i]
                if px==px and px>0:
                    open_pos[k]=dict(sleeve=name, sym=sym, entry_px=round(px,8),
                                     entry_ts=ts, mfe_bp=0.0)
    # persist ledger (old format) + open positions (mark-to-market unrealized)
    last=len(days)-1; lts=days[last]*86400000
    op=[]
    for k,p in open_pos.items():
        px=close[p["sym"]][last]; unr=(px/p["entry_px"]-1)*1e4 if px==px and p["entry_px"]>0 else 0
        op.append(dict(engine=k, symbol=p["sym"], entry_px=p["entry_px"], spot=round(px,8) if px==px else 0,
            sl_px=0, entry_ts=p["entry_ts"], mfe_bp=round(p["mfe_bp"],1),
            unreal_bp=round(unr,1)))
    os.makedirs(DATADIR, exist_ok=True)
    with open(os.path.join(DATADIR,"trades.json"),"w") as f: json.dump(trades,f)
    with open(os.path.join(DATADIR,"open_positions.json"),"w") as f:
        json.dump(dict(snapshot_ts=lts, open_count=len(op), positions=op), f)
    # summary
    net=sum(t["net_bp"] for t in trades); wins=sum(1 for t in trades if t["net_bp"]>0)
    gw=sum(t["net_bp"] for t in trades if t["net_bp"]>0); gl=-sum(t["net_bp"] for t in trades if t["net_bp"]<=0)
    pf=gw/gl if gl>0 else 0
    print(f"PAPER LEDGER: {len(trades)} trades, net {net:.0f}bp, WR {100*wins/max(1,len(trades)):.0f}%, "
          f"PF {pf:.2f}, {len(op)} open. -> data/trades.json + open_positions.json")

if __name__=="__main__": main()
