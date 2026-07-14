#!/bin/bash
# deploy_hygiene_check.sh -- three-way SHA agreement gate for the ChimeraCrypto live line.
#
# Ported from Omega's CLAUDE.md deploy-hygiene P0 (2026-05-14 three-commit-divergence
# incident) after ChimeraCrypto hit the same class of bug on 2026-07-05:
#   working tree / running binary / origin were in three-way disagreement, and the
#   running crypto binary was traceable only to a box-local SHA origin had never seen.
#
# Verifies FOUR things agree for the live trunk (xsec-deploy):
#   1. mac HEAD short hash
#   2. origin/xsec-deploy short hash
#   3. box HEAD short hash                          (ssh chimera-direct)
#   4. box binary BUILD_VERSION (last build's hash) (include/version_generated.hpp)
# and that the box has ZERO local commits ahead of origin (the divergence SOURCE:
# NEVER commit on the box -- commit on mac, push, box pulls --ff-only).
#
# Run from mac after any deploy (and at session start touching crypto deploy):
#   bash tools/deploy_hygiene_check.sh
#
# SSH form MUST be literally `ssh chimera-direct` (feedback-vps-ssh-command-form).

# LIVE TRUNK repointed xsec-deploy -> main S-2026-07-15j: the crypto live line
# moved onto main and xsec-deploy is GONE from origin, so this gate compared a
# non-existent branch (origin/xsec-deploy = "?") and its three-way HEAD agreement
# check was inert during the 2026-07-15 Mac<->box parallel-lineage drift. Now on main.
set -e
cd "$(dirname "$0")/.."

TRUNK=${TRUNK:-main}
BOX_DIR=${BOX_DIR:-/home/jo/ChimeraCrypto}
FAIL=0

echo "== ChimeraCrypto deploy-hygiene ($TRUNK) =="

git fetch origin "$TRUNK" --quiet 2>/dev/null || echo "  WARN: origin fetch failed"

MAC=$(git rev-parse --short HEAD)
ORIGIN=$(git rev-parse --short origin/$TRUNK 2>/dev/null || echo "?")

# One round-trip to the box: HEAD, last-built version, local-ahead count.
BOX_OUT=$(ssh chimera-direct "cd $BOX_DIR && \
    echo HEAD=\$(git rev-parse --short HEAD) && \
    echo VER=\$(grep -oE '\"[0-9a-f]{6,}\"' include/version_generated.hpp 2>/dev/null | tr -d '\"') && \
    echo AHEAD=\$(git rev-list --count origin/$TRUNK..HEAD 2>/dev/null || echo '?') && \
    echo ACTIVE=\$(systemctl is-active chimera 2>/dev/null || echo unknown)" 2>/dev/null)

BOX=$(echo "$BOX_OUT"   | sed -n 's/^HEAD=//p')
BOXVER=$(echo "$BOX_OUT" | sed -n 's/^VER=//p')
BOXAHEAD=$(echo "$BOX_OUT" | sed -n 's/^AHEAD=//p')
BOXACTIVE=$(echo "$BOX_OUT" | sed -n 's/^ACTIVE=//p')

printf "  %-22s %s\n" "mac HEAD"            "$MAC"
printf "  %-22s %s\n" "origin/$TRUNK"       "$ORIGIN"
printf "  %-22s %s\n" "box HEAD"            "${BOX:-UNREACHABLE}"
printf "  %-22s %s\n" "box binary version"  "${BOXVER:-?}"
printf "  %-22s %s\n" "box local-ahead"     "${BOXAHEAD:-?}"
printf "  %-22s %s\n" "chimera service"     "${BOXACTIVE:-?}"

# 1) three-way HEAD agreement (compare on the box's short length)
if [[ -n "$BOX" && "$MAC" == "$ORIGIN" && "${MAC:0:${#BOX}}" == "$BOX" ]]; then
    echo "  [PASS] mac == origin == box HEAD"
else
    echo "  [FAIL] HEADs disagree -- three-way divergence (the 2026-07-05 class)"; FAIL=1
fi

# 2) running binary was built from box HEAD
if [[ -n "$BOXVER" && "${BOX:0:${#BOXVER}}" == "${BOXVER:0:${#BOX}}" ]]; then
    echo "  [PASS] box binary built from box HEAD"
else
    echo "  [WARN] box binary version ($BOXVER) != box HEAD ($BOX) -- rebuild may be pending"
fi

# 3) box must NOT have local commits (the divergence source)
if [[ "$BOXAHEAD" == "0" ]]; then
    echo "  [PASS] box has no local commits ahead of origin"
else
    echo "  [FAIL] box is $BOXAHEAD commit(s) ahead of origin -- NEVER commit on the box."
    echo "         Reconcile: push box work to origin, or reset --soft box onto origin/$TRUNK."; FAIL=1
fi

if [[ "$FAIL" == "0" ]]; then echo "  RESULT: GREEN"; else echo "  RESULT: RED -- fix before trusting the deploy"; fi
exit $FAIL
