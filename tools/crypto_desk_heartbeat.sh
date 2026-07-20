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
  # FLAT-duration (S-2026-07-20, operator: flat-since-KILL-ALL book went unreported).
  # live_trades.csv mtime = last live fill/close write (every live SELL books there since
  # S-20f; a successful BUY flips HOLDS>0 which ends the flat state). Box-side = skew-free.
  FLATMIN=""
  if [ "${HOLDS:-1}" = "0" ] && [ -f "$HOME/ChimeraCrypto/data/live_trades.csv" ]; then
    FLATMIN=$(( ( $(date +%s) - $(stat -c %Y "$HOME/ChimeraCrypto/data/live_trades.csv") ) / 60 ))
  fi
  # DISARMED visibility (S-2026-07-20, operator: post-KILL-ALL blind window "missing these
  # breakouts ... not acceptable"). A disarmed cell CANNOT enter whatever the market does —
  # a flat desk that is also disarmed must never read as "normally flat". Counts pilot cells
  # published to crypto_companion_state.json with armed=false (retired cells excluded).
  DISARM=$(python3 -c "
import json
d=json.load(open(\"$HOME/ChimeraCrypto/data/crypto_companion_state.json\"))
legs=[l for l in d.get(\"legs\",[]) if not l.get(\"retired\")]
print(f\"{sum(1 for l in legs if not l.get('armed'))}/{len(legs)}\")" 2>/dev/null)
  cd "$HOME/ChimeraCrypto" 2>/dev/null
  HASH=$(git rev-parse --short HEAD 2>/dev/null)
  git fetch origin -q 2>/dev/null
  ORIG=$(git rev-parse --short origin/main 2>/dev/null)
  ANC=$(git merge-base --is-ancestor HEAD origin/main 2>/dev/null && echo 1 || echo 0)  # box behind==ancestor(benign) vs diverged
  RUN=$(grep -oE "build=[0-9a-f]+" logs/chimera.log 2>/dev/null | tail -1 | sed "s/build=//")
  echo "AS=$AS|SS=$SS|NR=$NR|UP=${UP:-0}|PASS=$PASS|ABRT=$ABRT|HOLDS=${HOLDS:-?}|HASH=$HASH|ORIG=$ORIG|ANC=$ANC|RUN=$RUN|FLAT=$FLATMIN|DIS=$DISARM"
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
ORIG=$(echo "$OUT" | tr '|' '\n' | sed -n 's/^ORIG=//p')
ANC=$(echo "$OUT"  | tr '|' '\n' | sed -n 's/^ANC=//p')
RUN=$(echo "$OUT"  | tr '|' '\n' | sed -n 's/^RUN=//p')
FLAT=$(echo "$OUT" | tr '|' '\n' | sed -n 's/^FLAT=//p')
DIS=$(echo "$OUT"  | tr '|' '\n' | sed -n 's/^DIS=//p')

REASONS=""
[ "$AS" != "active" ]   && REASONS="$REASONS ActiveState=$AS(not active)"
[ "$SS" != "running" ]  && REASONS="$REASONS SubState=$SS(not running — auto-restart=crash loop)"
[ "${ABRT:-0}" -gt 0 ] 2>/dev/null && REASONS="$REASONS ${ABRT}x ABORT/terminate in current boot"
[ "${PASS:-0}" -lt 1 ] 2>/dev/null && REASONS="$REASONS registry reconcile PASS absent from current boot"
[ "${UP:-0}" -lt 20 ] 2>/dev/null  && REASONS="$REASONS pid uptime ${UP}s (<20s — just crashed/restarted)"
# BUILD-MISMATCH: the exact check the Omega desk enforces (running stamp == box HEAD). This is
# the dimension this test used to miss — it caught the fork window while the heartbeat said GREEN.
# prefix-tolerant (the boot stamp may be shorter than rev-parse --short).
[ -n "$RUN" ] && [ -n "$HASH" ] && [ "${HASH#$RUN}" = "$HASH" ] && [ "${RUN#$HASH}" = "$RUN" ] \
  && REASONS="$REASONS BUILD-MISMATCH running=$RUN != box HEAD=$HASH (stale binary — rebuild+restart)"
# LINEAGE-FORK: RED only on true DIVERGENCE (box holds commits origin lacks, ANC=0). Box merely
# BEHIND origin (ANC=1, e.g. a Mac-side tooling commit not yet pulled) is BENIGN — the running
# binary is still self-consistent (running==box HEAD) and the next deploy's sync-guard ff's it.
[ -n "$ORIG" ] && [ "$HASH" != "$ORIG" ] && [ "${ANC:-1}" = "0" ] \
  && REASONS="$REASONS LINEAGE-FORK box HEAD=$HASH diverged from origin/main=$ORIG (box has commits origin lacks — reconcile, do NOT deploy on top)"

# FLAT-TOO-LONG (S-2026-07-20 operator ask: "no live capital" flat book must be REPORTED, not
# discovered by eyeballing tiles). A flat book is often legitimate — mimic-at-BE only acquires
# on a fresh confirmed up-move — so short flat is INFO on the GREEN line. Sustained flat while
# LIVE means arms are firing into a dead/rejecting path (e.g. -2010s) or no signal path at all:
#   >6h  = AMBER (investigate: grep BUY/-2010 in chimera.log, check USDT free)
#   >24h = RED   (treat as outage — live book earning nothing for a full day)
FLATNOTE=""
FLATSEV=""
if [ -n "$FLAT" ]; then
  FLATH=$(( FLAT / 60 ))
  FLATNOTE="LIVE book FLAT ${FLATH}h$((FLAT % 60))m since last live fill"
  if [ "$FLAT" -ge 1440 ] 2>/dev/null; then FLATSEV="RED"
  elif [ "$FLAT" -ge 360 ] 2>/dev/null; then FLATSEV="AMBER"
  fi
fi

# DISARMED severity (S-2026-07-20): DIS="n/N" disarmed/active cells. ALL disarmed = the desk
# structurally CANNOT enter any breakout (the exact post-KILL-ALL blind state that missed the
# 2026-07-19 75-min +170bp rally) -> AMBER floor with the recovery command, whatever FLAT says.
# Partial disarm rides the status line as info (fresh-jump cycling disarms cells transiently).
DISNOTE=""
DISSEV=""
if [ -n "$DIS" ]; then
  DN=${DIS%%/*}; DT=${DIS##*/}
  if [ "${DN:-0}" -gt 0 ] 2>/dev/null; then
    DISNOTE="cells DISARMED $DIS"
    if [ "$DN" = "$DT" ] && [ "${DT:-0}" -gt 0 ] 2>/dev/null; then
      DISSEV="AMBER"
      DISNOTE="ALL cells DISARMED $DIS — desk CANNOT enter any breakout (post-kill blind state; intentional kill test -> POST /api/rearm to resume immediately)"
    fi
  fi
fi

if [ -n "$REASONS" ]; then
  echo "RED|$HASH holds=$HOLDS restarts=$NR —$REASONS${FLATNOTE:+ — $FLATNOTE}${DISNOTE:+ — $DISNOTE}"
  exit 1
fi
if [ "$FLATSEV" = "RED" ]; then
  echo "RED|$HASH — $FLATNOTE (>24h: live book dead-flat a full day — check BUY/-2010 rejects + USDT free + signal path)${DISNOTE:+ — $DISNOTE}"
  exit 1
fi
if [ "$DISSEV" = "AMBER" ]; then
  echo "AMBER|$HASH (==origin==running) | reconcile PASS | up=${UP}s restarts=$NR | $DISNOTE${FLATNOTE:+ | $FLATNOTE}"
  exit 0
fi
if [ "$FLATSEV" = "AMBER" ]; then
  echo "AMBER|$HASH (==origin==running) | reconcile PASS | up=${UP}s restarts=$NR | $FLATNOTE (>6h: verify arms firing — grep 'BUY\\|-2010' chimera.log, check USDT free)${DISNOTE:+ | $DISNOTE}"
  exit 0
fi
echo "GREEN|$HASH (==origin==running) | reconcile PASS | active/running up=${UP}s restarts=$NR | live holds=$HOLDS${FLATNOTE:+ | $FLATNOTE (normal short-flat: mimic awaits confirmed up-move)}${DISNOTE:+ | $DISNOTE}"
exit 0
