#!/usr/bin/env python3
"""
Chimera Engine Health Monitor
Polls /api/state every 10s and shows exactly why each engine is blocked.
Run: python3 chimera_monitor.py
"""

import urllib.request, json, time, os, subprocess, sys
from datetime import datetime, timezone

API = "http://localhost:8080/api/state"
LOG = os.path.expanduser("~/ChimeraCrypto/build/logs/chimera_2026-04-12.log")
POLL = 10  # seconds

COST_FLOOR_BP = 15.0

def fetch_state():
    try:
        with urllib.request.urlopen(API, timeout=3) as r:
            return json.load(r)
    except Exception as e:
        return None

def tail_log(n=200):
    try:
        result = subprocess.run(['tail', '-n', str(n), LOG], capture_output=True, text=True)
        return result.stdout
    except:
        return ""

def get_rejections(log_tail):
    """Parse last rejection summary from log."""
    lines = log_tail.split('\n')
    rejections = {}
    in_summary = False
    for line in reversed(lines):
        if 'REJECTION-SUMMARY' in line:
            in_summary = True
            continue
        if in_summary:
            if line.strip() == '' or 'REJECTION-SUMMARY' in line:
                break
            # e.g. "  ETH LEADLAG: 593 rejections (no_leadlag_signal)"
            if 'rejections' in line:
                parts = line.strip().split(':')
                if len(parts) >= 2:
                    key = parts[0].strip()
                    reason = parts[1].strip()
                    rejections[key] = reason
    return rejections

def get_last_btc_move(log_tail):
    best = 0.0
    for line in log_tail.split('\n'):
        if 'BTC-MOVE' in line and 'move_1200ms=' in line:
            try:
                val = float(line.split('move_1200ms=')[1].split('bp')[0])
                if abs(val) > abs(best):
                    best = val
            except:
                pass
    return best

def get_last_liq(log_tail):
    liqs = []
    for line in log_tail.split('\n'):
        if '[LIQ]' in line and 'SHORT LIQ' in line:
            liqs.append(line.strip())
    return liqs[-3:] if liqs else []

def clr(code): return f"\033[{code}m"
RESET=clr(0); RED=clr(31); GRN=clr(32); YLW=clr(33); CYN=clr(36); BOLD=clr(1); DIM=clr(2)

def status_line(ok, label, detail=""):
    icon = f"{GRN}✓{RESET}" if ok else f"{RED}✗{RESET}"
    detail_str = f" {DIM}{detail}{RESET}" if detail else ""
    return f"  {icon} {label}{detail_str}"

def check_vwap(sym, sym_data, price):
    issues = []
    regime = sym_data.get('regime_state','?')
    if regime not in ('GRIND','DEAD','NEUTRAL'):
        issues.append(f"regime={regime} (need GRIND/DEAD)")
    vwap_dev = sym_data.get('vwap_deviation_bp', 0)
    if vwap_dev is None or vwap_dev < 15.0:
        issues.append(f"dev={vwap_dev:.1f}bp (need ≥15bp)")
    if not sym_data.get('vwap_ready', False):
        issues.append("VWAP not ready")
    return issues

def check_leadlag(btc_move, sym, sym_data):
    issues = []
    if abs(btc_move) < 5.0:
        issues.append(f"BTC move={btc_move:.2f}bp (need ≥5bp in 1200ms)")
    if btc_move < 0:
        issues.append("BTC moving DOWN → short signal → spot-only blocked")
    return issues

def check_liq(sym, sym_data, recent_liqs):
    sym_upper = sym.replace('usdt','').upper()
    sym_liqs = [l for l in recent_liqs if sym_upper in l]
    if not sym_liqs:
        return ["no qualifying liquidation events recently"]
    notional = 0
    try:
        notional = float(sym_liqs[-1].split('$')[1].split(' ')[0].replace(',',''))
    except:
        pass
    issues = []
    min_not = 200000 if sym in ('btcusdt','ethusdt','solusdt') else 300000
    if notional < min_not:
        issues.append(f"last liq ${notional:.0f} < ${min_not:.0f} threshold")
    return issues

def run():
    os.system('clear')
    print(f"\n{BOLD}{CYN}╔══════════════════════════════════════════════════╗{RESET}")
    print(f"{BOLD}{CYN}║       CHIMERA ENGINE HEALTH MONITOR              ║{RESET}")
    print(f"{BOLD}{CYN}╚══════════════════════════════════════════════════╝{RESET}\n")

    while True:
        now = datetime.now(timezone.utc).strftime('%H:%M:%S UTC')
        state = fetch_state()
        log_tail = tail_log(300)
        rejections = get_rejections(log_tail)
        btc_move = get_last_btc_move(log_tail)
        recent_liqs = get_last_liq(log_tail)

        os.system('clear')
        print(f"{BOLD}{CYN}  CHIMERA MONITOR  {now}{RESET}\n")

        if not state:
            print(f"{RED}  ✗ Cannot reach API at {API}{RESET}")
            time.sleep(POLL)
            continue

        build = state.get('build_ver','?')
        pnl = state.get('pnl', 0)
        trades = state.get('total_trades', 0)
        lat = state.get('latency_p95', 0)
        print(f"  Build: {BOLD}{build}{RESET}  |  PnL: {GRN if pnl>=0 else RED}{pnl:+.2f}bp{RESET}  |  Trades: {trades}  |  Latency p95: {lat:.1f}ms\n")

        # ── BTC MOVE STATUS ──────────────────────────────────────
        print(f"{BOLD}  BTC IMPULSE (LEADLAG trigger){RESET}")
        btc_ok = abs(btc_move) >= 5.0 and btc_move > 0
        print(status_line(btc_ok, f"1200ms move: {btc_move:+.2f}bp", f"need +5bp upward"))
        if btc_move < 0:
            print(f"  {YLW}⚠ Market trending DOWN — all LEADLAG = short = blocked on spot{RESET}")
        print()

        # ── PER SYMBOL ───────────────────────────────────────────
        for sym in ['btcusdt','ethusdt','solusdt']:
            short = sym.replace('usdt','').upper()
            sym_data = state.get(sym, {})
            price = state.get(sym+'_price', 0)
            regime = sym_data.get('regime_state','?')
            vwap_dev = sym_data.get('vwap_deviation_bp') or 0
            day_high = sym_data.get('day_high', 0)
            day_low = sym_data.get('day_low', 0)
            liq_not = sym_data.get('liq_notional', 0)

            print(f"{BOLD}  {short}  ${price:,.2f}  H:${day_high:,.2f} L:${day_low:,.2f}  Regime:{regime}{RESET}")

            # VWAP
            vwap_issues = check_vwap(sym, sym_data, price)
            vwap_ok = len(vwap_issues) == 0
            detail = " | ".join(vwap_issues) if vwap_issues else f"dev={vwap_dev:.1f}bp ✓ ready to fire"
            print(status_line(vwap_ok, "VWAP", detail))

            # LEADLAG
            ll_issues = check_leadlag(btc_move, sym, sym_data)
            print(status_line(len(ll_issues)==0, "LEADLAG", " | ".join(ll_issues) if ll_issues else "ready"))

            # LIQ
            liq_issues = check_liq(sym, sym_data, recent_liqs)
            liq_detail = f"last liq notional: ${liq_not:,.0f}" if liq_not else ("no recent liqs" if liq_issues else "ready")
            print(status_line(len(liq_issues)==0, "LIQ", liq_detail))

            # Rejection summary for this symbol
            sym_rejections = {k:v for k,v in rejections.items() if short in k}
            if sym_rejections:
                for k,v in sym_rejections.items():
                    print(f"    {DIM}↳ {k}: {v}{RESET}")
            print()

        # ── RECENT LIQS ──────────────────────────────────────────
        if recent_liqs:
            print(f"{BOLD}  RECENT LIQUIDATIONS (last 3){RESET}")
            for l in recent_liqs:
                print(f"  {DIM}{l}{RESET}")
            print()

        # ── OVERALL DIAGNOSIS ────────────────────────────────────
        print(f"{BOLD}  DIAGNOSIS{RESET}")
        btc_data = state.get('btcusdt',{})
        eth_data = state.get('ethusdt',{})
        sol_data = state.get('solusdt',{})

        eth_vwap = eth_data.get('vwap_deviation_bp') or 0
        sol_vwap = sol_data.get('vwap_deviation_bp') or 0

        if eth_vwap >= 15.0 or sol_vwap >= 15.0:
            best = 'ETH' if eth_vwap > sol_vwap else 'SOL'
            best_dev = max(eth_vwap, sol_vwap)
            best_regime = (eth_data if best=='ETH' else sol_data).get('regime_state','?')
            if best_regime in ('GRIND','DEAD','NEUTRAL'):
                print(f"  {GRN}→ {best} VWAP should fire! dev={best_dev:.1f}bp regime={best_regime}{RESET}")
            else:
                print(f"  {YLW}→ {best} VWAP dev={best_dev:.1f}bp but regime={best_regime} blocks it{RESET}")
        elif btc_move > 0 and abs(btc_move) >= 3.0:
            print(f"  {YLW}→ BTC moving up {btc_move:.2f}bp — close to LEADLAG threshold (5bp){RESET}")
        else:
            print(f"  {RED}→ No engine conditions met — market too slow/directional for current thresholds{RESET}")
            print(f"  {YLW}→ Consider: lower VWAP entry to 12bp or LEADLAG lookback to 2000ms{RESET}")

        print(f"\n  {DIM}Refreshing in {POLL}s... Ctrl+C to exit{RESET}\n")
        time.sleep(POLL)

if __name__ == '__main__':
    try:
        run()
    except KeyboardInterrupt:
        print("\nMonitor stopped.")
