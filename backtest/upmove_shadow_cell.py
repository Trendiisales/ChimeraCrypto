#!/usr/bin/env python3
"""
UpMoveTrail shadow cell — production-faithful spec + runnable shadow logger.
WF-confirmed up-move/hard-trail engine with trailing-roster cull. SHADOW ONLY:
computes today's target signal per symbol, logs it, places NO orders.

Same signal_state() function drives backtest AND live (one code path).
Ports into sleeves/chimera_sleeves.py as a new sleeve on deploy.

EDGE (WF-confirmed 2026-06-16, see Memory-Chimera UpMoveTrailLossMitigation):
  entry  = own-trend golden (EMA50>EMA200 & close>EMA50)
           + Donchian-24 breakout (close > prior 24-bar high)
           + cost gate (ATR >= 3 * round-trip cost)
  exit   = chandelier trail 5*ATR  |  hard stop 3*ATR  |  time-stop 48 bars-underwater
  gate   = BTC daily regime (dual_50_200) on top  -> whole book flat in bear
  roster = trailing-net cull: a symbol is ACTIVE only if its trailing engine net >= 0
           over the eval window. Launch-benched (chronic laggards): LTC, ATOM, DOT.
           Re-evaluated monthly. Rolling, real-time, NO hindsight (the +625 config).
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(__file__))
from upmove_trail import series, COST_BP
from upmove_minloss_sweep import precompute, bt

# ---- validated config (do not retune without WF) ----
CFG = dict(don_n=24, trail_m=5.0, cost_k=3.0, stop_m=3.0, ts_bars=48)
ATR_N = 24
UNIVERSE = ["BTC","ETH","SOL","BNB","XRP","ADA","AVAX","LINK","DOT","LTC",
            "DOGE","ATOM","UNI","FIL","NEAR"]
ROSTER_EVAL_BARS = 180*24    # trailing 180d window for the justification test
INCLUDE_MARGIN   = 5.0       # trailing net% must CLEAR this (not just >0) to justify use
ROSTER_MIN_TRADES = 8        # need this many trailing trades = enough sample to judge
H1 = 60*60*1000
# No bans, no name-list. A coin is unused until it JUSTIFIES inclusion by trailing
# performance. LTC/ATOM/DOT start out simply because they don't clear the bar today;
# any coin (incl. those) is included the moment its trailing record justifies it,
# and dropped again the moment it stops. Symmetric, real-time, no hindsight.

# ---------- ROSTER (justification-gated, rolling) ----------
def roster_active(data, PC, now_ms):
    """A symbol is USED only if it JUSTIFIES inclusion: trailing-180d engine net
    clears +INCLUDE_MARGIN over >=ROSTER_MIN_TRADES trades. Else it sits out
    (not banned) until its record justifies it. Re-evaluated each run."""
    active, detail = [], {}
    t0 = now_ms - ROSTER_EVAL_BARS*H1
    for s in UNIVERSE:
        tr = bt(data[s], *PC[s], t0=t0, t1=now_ms, **CFG)
        net = round(sum(tr)*100, 1); n = len(tr)
        if n < ROSTER_MIN_TRADES:   reason = "unproven (too few trades)"; ok=False
        elif net < INCLUDE_MARGIN:  reason = f"not justified (net {net:+.0f} < +{INCLUDE_MARGIN:.0f})"; ok=False
        else:                       reason = "justified"; ok=True
        detail[s] = dict(trail_net=net, trail_trades=n, active=ok, reason=reason)
        if ok: active.append(s)
    return active, detail

# ---------- REGIME GATE (BTC daily dual_50_200) ----------
def regime_ok(btc_bars, now_ms):
    # build daily closes from 1h bars up to now
    closes=[b[4] for b in btc_bars if b[0]<=now_ms]
    if len(closes) < 200*24: return False, {}
    daily=closes[::24]
    if len(daily) < 200: return False, {}
    c=daily[-1]; ma50=sum(daily[-50:])/50; ma200=sum(daily[-200:])/200
    ma200_prev=sum(daily[-224:-24])/200 if len(daily)>=224 else ma200
    ok = c>ma50 and (c>ma200 or ma200>ma200_prev)
    return ok, dict(btc=round(c), ma50=round(ma50), ma200=round(ma200), rising=ma200>ma200_prev)

# ---------- SIGNAL STATE (shared by backtest + live) ----------
def signal_state(bars, A, e50, e200, i):
    """Engine view at bar i: 'ENTER' (fresh up-move breakout passing all entry gates),
    else 'FLAT'. Exit/position mgmt handled by the executor on deploy; shadow logs ENTER."""
    c=bars[i][4]; a=A[i]
    if a is None: return "FLAT", {}
    if not (e50[i]>e200[i] and c>e50[i]): return "FLAT", {"reason":"not own-uptrend"}
    if a < CFG['cost_k']*(c*COST_BP/10000.0): return "FLAT", {"reason":"vol<cost-gate"}
    donhi=max(bars[j][2] for j in range(i-CFG['don_n'], i))
    if c>donhi:
        return "ENTER", dict(price=round(c,4), breakout_lvl=round(donhi,4),
                             atr=round(a,4), stop=round(c-CFG['stop_m']*a,4),
                             trail_init=round(c-CFG['trail_m']*a,4))
    return "FLAT", {"reason":"no-breakout"}

# ---------- daily shadow target (NO ORDERS) ----------
def shadow_targets():
    data={s:series(s) for s in UNIVERSE}
    PC={s:precompute(data[s], ATR_N) for s in UNIVERSE}
    now=min(data[s][-1][0] for s in UNIVERSE)
    reg_ok, reg = regime_ok(data["BTC"], now)
    active, roster = roster_active(data, PC, now)
    out=dict(ts_ms=now, mode="SHADOW", regime_ok=reg_ok, regime=reg,
             active_roster=active, benched=[s for s in UNIVERSE if s not in active],
             roster_detail=roster, targets={})
    if reg_ok:
        for s in active:
            b=data[s]; i=len(b)-1
            st,info=signal_state(b,*PC[s],i)
            if st=="ENTER": out["targets"][s]=info
    out["n_entries"]=len(out["targets"])
    return out

if __name__=="__main__":
    t=shadow_targets()
    print(f"=== UpMoveTrail SHADOW  ts={t['ts_ms']}  mode={t['mode']} ===")
    print(f"REGIME: {'RISK-ON' if t['regime_ok'] else 'FLAT (bear gate)'}  {t['regime']}")
    print(f"ROSTER active {len(t['active_roster'])}/{len(UNIVERSE)}: {t['active_roster']}")
    print(f"  benched: {t['benched']}")
    print("  trailing-net cull detail (180d):")
    for s in UNIVERSE:
        d=t['roster_detail'][s]
        print(f"    {s:5} net {d['trail_net']:>+7.1f}  trades {d['trail_trades']:>3}  {'USE   ' if d['active'] else 'sit out'} {d['reason']}")
    print(f"\nTODAY entries ({t['n_entries']}): {list(t['targets'].keys()) or 'none (no fresh breakouts / gate flat)'}")
    for s,info in t['targets'].items(): print(f"  {s}: {info}")
    # write shadow log line
    logp=os.path.join(os.path.dirname(__file__),"upmove_shadow_log.jsonl")
    with open(logp,"a") as f: f.write(json.dumps(t)+"\n")
    print(f"\nlogged -> {logp}  (NO ORDERS placed)")
