#!/usr/bin/env bash
# fetch_binance_depth_flow.sh — pull FREE Binance Vision historical data for the
# CORE/MIMIC build (2026-07-15l). data.binance.vision is a data-only public source
# (clear of the "no bulk pulls via production gateway" rule — that's the exec gateway).
#
# Pulls, for ETHUSDT + XRPUSDT (Phase-1 passing universe), over the window matching
# data/klines_spot (2025-05 .. 2026-05):
#   1. perp bookDepth (daily)   — ±0.2/1/2/3/4/5% cumulative depth+notional, ~30s cadence.
#      = the depth-adjusted-liquidation proxy (spec §2/§5). SPOT has no free depth;
#        BTC/ETH PERP book is the deepest, tightest-basis proxy. ~0.5MB/day, unzipped.
#   2. spot aggTrades (monthly) — every trade w/ isBuyerMaker flag = true tape aggression,
#      CVD, OFI at tick resolution (spec §3/§7). ~300-800MB/mo zipped; KEPT ZIPPED
#      (unzipped is ~4-6x — stream-process per month, don't explode all at once).
#
# Idempotent + SHA256-checksum-verified + resumable. Safe to re-run.
# Data lands in gitignored dirs (data/bookdepth_perp, data/aggtrades_spot).
set -uo pipefail

BASE="https://data.binance.vision"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BD_DIR="$ROOT/data/bookdepth_perp"
AT_DIR="$ROOT/data/aggtrades_spot"
# Universe = ETH + XRP (Phase-1 passing set; BTC DEAD/within-null, excluded). BTC perp
# bookDepth already on disk (366d) and kept for the BTC-regime gate (price-only); no BTC
# aggTrades needed. ETH depth already present -> skips instantly; XRP depth+flow fetched fresh.
COINS=(ETHUSDT XRPUSDT)
START_YM="2025-05"; END_YM="2026-05"          # monthly window (aggTrades)
START_DAY="2025-05-10"; END_DAY="2026-05-10"  # daily window (bookDepth)
mkdir -p "$BD_DIR" "$AT_DIR"

log(){ printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"; }

# sha256 helper (macOS shasum)
verify_sha(){ # $1=file $2=expected_sha
  local got; got=$(shasum -a 256 "$1" | awk '{print $1}'); [[ "$got" == "$2" ]]; }

fetch_checked(){ # $1=url $2=outfile  -> download + verify against url.CHECKSUM, retry once
  local url="$1" out="$2" tries=0
  while (( tries < 3 )); do
    if [[ -f "$out" ]]; then
      local exp; exp=$(curl -fsS "$url.CHECKSUM" 2>/dev/null | awk '{print $1}')
      if [[ -n "$exp" ]] && verify_sha "$out" "$exp"; then return 0; fi
      rm -f "$out"   # corrupt/partial -> redownload
    fi
    curl -fsS -o "$out" "$url" || { ((tries++)); sleep 2; continue; }
    local exp; exp=$(curl -fsS "$url.CHECKSUM" 2>/dev/null | awk '{print $1}')
    if [[ -z "$exp" ]] || verify_sha "$out" "$exp"; then return 0; fi
    rm -f "$out"; ((tries++)); sleep 2
  done
  log "FAIL $url"; return 1
}

# ---- 1. perp bookDepth (daily) -------------------------------------------------
log "=== bookDepth (perp) $START_DAY .. $END_DAY ==="
bd_ok=0; bd_fail=0
for coin in "${COINS[@]}"; do
  d="$START_DAY"
  while [[ "$d" < "$END_DAY" || "$d" == "$END_DAY" ]]; do
    url="$BASE/data/futures/um/daily/bookDepth/$coin/$coin-bookDepth-$d.zip"
    zip="$BD_DIR/$coin-bookDepth-$d.zip"
    csv="$BD_DIR/$coin-bookDepth-$d.csv"
    if [[ -f "$csv" ]]; then ((bd_ok++)); d=$(date -j -v+1d -f '%Y-%m-%d' "$d" '+%Y-%m-%d'); continue; fi
    if fetch_checked "$url" "$zip"; then
      unzip -o -q "$zip" -d "$BD_DIR" && rm -f "$zip" && ((bd_ok++)) || ((bd_fail++))
    else ((bd_fail++)); fi
    d=$(date -j -v+1d -f '%Y-%m-%d' "$d" '+%Y-%m-%d')
  done
done
log "bookDepth done: ok=$bd_ok fail=$bd_fail  (csv in $BD_DIR)"

# ---- 2. spot aggTrades (monthly, kept zipped) ---------------------------------
log "=== aggTrades (spot) $START_YM .. $END_YM ==="
at_ok=0; at_fail=0
for coin in "${COINS[@]}"; do
  ym="$START_YM"
  while [[ "$ym" < "$END_YM" || "$ym" == "$END_YM" ]]; do
    url="$BASE/data/spot/monthly/aggTrades/$coin/$coin-aggTrades-$ym.zip"
    zip="$AT_DIR/$coin-aggTrades-$ym.zip"
    if fetch_checked "$url" "$zip"; then ((at_ok++)); log "  got $coin $ym ($(du -h "$zip"|cut -f1))"; else ((at_fail++)); fi
    ym=$(date -j -v+1m -f '%Y-%m' "$ym" '+%Y-%m')
  done
done
log "aggTrades done: ok=$at_ok fail=$at_fail  (zips in $AT_DIR)"
log "ALL DONE. bookDepth csv=$(ls "$BD_DIR"/*.csv 2>/dev/null|wc -l|tr -d ' ')  aggTrades zip=$(ls "$AT_DIR"/*.zip 2>/dev/null|wc -l|tr -d ' ')"
