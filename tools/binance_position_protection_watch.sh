#!/bin/bash
# binance_position_protection_watch.sh — crypto twin of Omega's
# position_protection_watch.sh. Two guarantees on the live Binance book (josgp1 =
# chimera-direct):
#   1. BOT-DEATH   — the chimera engine process is running (systemctl chimera).
#   2. NO NAKED POS — every held Binance spot position has a RESTING protective
#                     SELL order at the exchange (broker-side stop survives the
#                     bot dying). Run via the read-only probe.
#
# The two together are the point: broker-side stops mean a held position stays
# protected EVEN IF the bot is dead — this watch is the alarm if either the bot
# dies OR a position is found naked (the engine's on-fill stop missed one).
#
# READ-ONLY. It reads systemctl state and runs the read-only probe (GET-only). It
# NEVER places/cancels an order (audit-read-only-never-mutate). Healing a naked
# position is the ENGINE's job; this only alerts. Distinct from Omega's watch,
# which self-heals via place_stops.py — the crypto engine now arms broker stops
# on-fill itself (ExecutionGateway), so the monitor stays observe-only.
#
# The probe is streamed over ssh to python3 (`python3 -`) so NOTHING is written to
# the box's disk — no deploy needed for this monitor to run.
set -o pipefail
HOST=${CRYPTO_HOST:-chimera-direct}
PROBE="$(cd "$(dirname "$0")" && pwd)/binance_position_protection_probe.py"
STATE=/tmp/binance_position_protection.state
LOG=/tmp/binance_position_protection.log
notify(){ /usr/bin/osascript -e "display notification \"$2\" with title \"$1\" sound name \"Basso\"" >/dev/null 2>&1; }
ts(){ date -u '+%Y-%m-%d %H:%MZ'; }

# 1. BOT-DEATH — is the chimera engine running on the box?
#    UnitFileState distinguishes the two reasons it can be inactive:
#      enabled  + inactive -> it DIED (crash / OOM / failed start)      -> alarm
#      disabled + inactive -> the OPERATOR stood it down deliberately   -> silent
#    The crypto desk is currently stopped+disabled on purpose, and alarming every
#    10 minutes about an intentional state trains the operator to ignore the
#    channel -- which is what makes a real death invisible. Log it either way.
PROBE_STATE=$(ssh -o ConnectTimeout=15 -o BatchMode=yes "$HOST" \
         'systemctl show chimera -p ActiveState -p UnitFileState --value 2>/dev/null' 2>/dev/null)
ACTIVE=$(printf '%s\n' "$PROBE_STATE" | sed -n 1p)
ENABLED=$(printf '%s\n' "$PROBE_STATE" | sed -n 2p)
if [ -z "$ACTIVE" ]; then
  echo "[$(ts)] box unreachable" >> "$LOG"; exit 0   # can't verify -> no false alarm
fi
if [ "$ACTIVE" != "active" ]; then
  if [ "$ENABLED" = "disabled" ] || [ "$ENABLED" = "masked" ]; then
    echo "[$(ts)] DESK STOOD DOWN (intentional): ActiveState=$ACTIVE UnitFileState=$ENABLED -- no alarm" >> "$LOG"
  else
    notify "🛑 CHIMERA BOT DOWN" "chimera service ActiveState=$ACTIVE (unit $ENABLED) on $HOST -- engine not managing positions. Broker stops (if armed) still protect; investigate now."
    echo "[$(ts)] BOT DOWN: ActiveState=$ACTIVE UnitFileState=$ENABLED" >> "$LOG"
  fi
fi

# 2. NAKED-POSITION — stream the read-only probe to the box and run it there so the
#    API secret never leaves the box. Exit 2 = naked position(s) found.
OUT=$(ssh -o ConnectTimeout=20 -o BatchMode=yes "$HOST" 'python3 - ' < "$PROBE" 2>/dev/null)
RC=$?
if [ -z "$OUT" ]; then
  echo "[$(ts)] probe unreachable" >> "$LOG"; exit 0
fi
if [ "$RC" -eq 2 ]; then
  notify "🚨 NAKED BINANCE POSITION" "$OUT -- held with NO resting stop. The engine on-fill stop missed one; if the bot dies this is unprotected. Investigate."
  echo "[$(ts)] NAKED: $OUT" >> "$LOG"
else
  echo "[$(ts)] OK: $OUT" >> "$LOG"
fi
