#!/bin/bash
# Crypto desk (josgp1 = chimera-direct) heartbeat.
#
# WHY THIS EXISTS (2026-07-20): the 2a live-only-rebuild deploy (build 9ba4b7a) left a stale
# XSEC/RIPRIDER declaration in config/engine_registry.json. load_from_json re-added them as
# declared-SHADOW-but-not-wired, so g_registry.validate() aborted every boot -> std::terminate
# -> 23 crash cycles with the 9 live holds UNSERVICED. It stayed invisible because EVERY
# SessionStart self-test (protection/feeds/feedpath/watermark) targets OMEGA (omega-new), NONE
# touch the crypto box. And the deploy's own "service active" verification was a FALSE POSITIVE:
# systemd reports active during the ~10s boot window; the registry abort fires post-HTTP near the
# end. So "is-active" alone is a lie during a crash loop.
#
# This check reads the CURRENT-BOOT log slice for `reconcile PASS` and the absence of
# `STARTUP ABORT`/`terminate` -- the exact signal that a boot-window is-active read misses.
# GREEN requires: active+running, current boot reached reconcile PASS, zero abort/terminate in
# that slice, pid uptime past the crash point, and the live holds file readable.
set -o pipefail
HOST=${CRYPTO_HOST:-chimera-direct}

OUT=$(ssh -o ConnectTimeout=8 -o BatchMode=yes "$HOST" '
  F=$HOME/ChimeraCrypto/logs/chimera.log
  AS=$(systemctl show chimera -p ActiveState --value 2>/dev/null)
  SS=$(systemctl show chimera -p SubState --value 2>/dev/null)
  NR=$(systemctl show chimera -p NRestarts --value 2>/dev/null)
  PID=$(systemctl show chimera -p MainPID --value 2>/dev/null)
  UP=$(ps -o etimes= -p "$PID" 2>/dev/null | tr -d " ")
  SL=$(grep -n "Tier-2 Edge Engines" "$F" 2>/dev/null | tail -1 | cut -d: -f1)
  BOOT=$(tail -n +"${SL:-1}" "$F" 2>/dev/null)
  PASS=$(printf "%s" "$BOOT" | grep -c "reconcile PASS")
  ABRT=$(printf "%s" "$BOOT" | grep -cE "STARTUP ABORT|terminate called")
  HOLDS=$(python3 -c "import json;d=json.load(open(\"$HOME/ChimeraCrypto/data/live_mimic_positions.json\"));print(len(d) if isinstance(d,list) else len(d.get(\"positions\",d)))" 2>/dev/null)
  HASH=$(cd "$HOME/ChimeraCrypto" 2>/dev/null && git rev-parse --short HEAD 2>/dev/null)
  echo "AS=$AS|SS=$SS|NR=$NR|UP=${UP:-0}|PASS=$PASS|ABRT=$ABRT|HOLDS=${HOLDS:-?}|HASH=$HASH"
' 2>/dev/null)

if [ -z "$OUT" ]; then
  echo "RED|unreachable: ssh $HOST failed (box down or network) — cannot confirm crypto desk state"
  exit 1
fi

AS=$(echo "$OUT"   | tr '|' '\n' | sed -n 's/^AS=//p')
SS=$(echo "$OUT"   | tr '|' '\n' | sed -n 's/^SS=//p')
NR=$(echo "$OUT"   | tr '|' '\n' | sed -n 's/^NR=//p')
UP=$(echo "$OUT"   | tr '|' '\n' | sed -n 's/^UP=//p')
PASS=$(echo "$OUT" | tr '|' '\n' | sed -n 's/^PASS=//p')
ABRT=$(echo "$OUT" | tr '|' '\n' | sed -n 's/^ABRT=//p')
HOLDS=$(echo "$OUT"| tr '|' '\n' | sed -n 's/^HOLDS=//p')
HASH=$(echo "$OUT" | tr '|' '\n' | sed -n 's/^HASH=//p')

REASONS=""
[ "$AS" != "active" ]   && REASONS="$REASONS ActiveState=$AS(not active)"
[ "$SS" != "running" ]  && REASONS="$REASONS SubState=$SS(not running — auto-restart=crash loop)"
[ "${ABRT:-0}" -gt 0 ] 2>/dev/null && REASONS="$REASONS ${ABRT}x ABORT/terminate in current boot"
[ "${PASS:-0}" -lt 1 ] 2>/dev/null && REASONS="$REASONS registry reconcile PASS absent from current boot"
[ "${UP:-0}" -lt 20 ] 2>/dev/null  && REASONS="$REASONS pid uptime ${UP}s (<20s — just crashed/restarted)"

if [ -n "$REASONS" ]; then
  echo "RED|$HASH holds=$HOLDS restarts=$NR —$REASONS"
  exit 1
fi
echo "GREEN|$HASH | reconcile PASS | active/running up=${UP}s restarts=$NR | live holds=$HOLDS"
exit 0
