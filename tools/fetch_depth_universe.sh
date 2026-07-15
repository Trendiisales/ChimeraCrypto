#!/usr/bin/env bash
# fetch_depth_universe.sh — perp bookDepth ONLY for the remaining universe coins
# (SOL/BNB/DOGE/AVAX/LINK) so the crypto cost table covers ALL 8 coins, not half.
# Cost model needs ONLY depth (not aggTrades). data.binance.vision = data-only source.
# Idempotent + checksum-verified + resumable. Run in background.
set -uo pipefail
BASE="https://data.binance.vision"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BD_DIR="$ROOT/data/bookdepth_perp"
COINS=(SOLUSDT BNBUSDT DOGEUSDT AVAXUSDT LINKUSDT)
START_DAY="2025-05-10"; END_DAY="2026-05-10"
mkdir -p "$BD_DIR"
log(){ printf '[%s] %s\n' "$(date '+%H:%M:%S')" "$*"; }
verify_sha(){ local got; got=$(shasum -a 256 "$1" | awk '{print $1}'); [[ "$got" == "$2" ]]; }
fetch_checked(){ local url="$1" out="$2" tries=0
  while (( tries < 3 )); do
    curl -fsS -o "$out" "$url" || { ((tries++)); sleep 2; continue; }
    local exp; exp=$(curl -fsS "$url.CHECKSUM" 2>/dev/null | awk '{print $1}')
    if [[ -z "$exp" ]] || verify_sha "$out" "$exp"; then return 0; fi
    rm -f "$out"; ((tries++)); sleep 2
  done; return 1; }
for coin in "${COINS[@]}"; do
  ok=0; fail=0; d="$START_DAY"
  while [[ "$d" < "$END_DAY" || "$d" == "$END_DAY" ]]; do
    csv="$BD_DIR/$coin-bookDepth-$d.csv"; zip="$BD_DIR/$coin-bookDepth-$d.zip"
    if [[ -f "$csv" ]]; then ((ok++)); d=$(date -j -v+1d -f '%Y-%m-%d' "$d" '+%Y-%m-%d'); continue; fi
    url="$BASE/data/futures/um/daily/bookDepth/$coin/$coin-bookDepth-$d.zip"
    if fetch_checked "$url" "$zip"; then unzip -o -q "$zip" -d "$BD_DIR" && rm -f "$zip" && ((ok++)) || ((fail++)); else ((fail++)); fi
    d=$(date -j -v+1d -f '%Y-%m-%d' "$d" '+%Y-%m-%d')
  done
  log "$coin depth: ok=$ok fail=$fail"
done
log "UNIVERSE DEPTH DONE. total csv=$(ls "$BD_DIR"/*.csv 2>/dev/null|wc -l|tr -d ' ')"
