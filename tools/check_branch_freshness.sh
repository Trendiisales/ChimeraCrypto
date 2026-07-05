#!/bin/bash
# check_branch_freshness.sh -- session-start guard against stale/divergent working state.
#
# Ported from Omega (tools/check_branch_freshness.sh) 2026-07-05 after the ChimeraCrypto
# github divergence incident:
#   - mac /Users/jo/ChimeraCrypto and the live box /home/jo/ChimeraCrypto committed the
#     SAME logical work under DIFFERENT SHAs (mac 25d27cc vs box 5a85fc1) because the D1
#     deploy patched + committed ON THE BOX instead of pulling from origin.
#   - origin/xsec-deploy sat a content-step stale; the mac clone was SHALLOW so history
#     was invisible and the divergence was misdiagnosed for a whole session.
#
# The LIVE TRUNK for crypto is `xsec-deploy`, NOT main (main is a divergent research line
# that xsec's 2026-06-14 honest revalidation tombstoned as 0/283 viable). So freshness is
# measured against origin/xsec-deploy.
#
# Override with --force or environment STALE_OK=1.
#
# Usage:
#   bash tools/check_branch_freshness.sh
#   STALE_OK=1 bash tools/check_branch_freshness.sh     # bypass
#   bash tools/check_branch_freshness.sh --force        # bypass

set -e

TRUNK=${TRUNK:-xsec-deploy}
STALE_THRESHOLD=${STALE_THRESHOLD:-15}

if [[ "$1" == "--force" || "${STALE_OK:-}" == "1" ]]; then
    echo "[check_branch_freshness] override active, skipping check"
    exit 0
fi

cd "$(dirname "$0")/.."

# Refuse a shallow clone outright -- a shallow clone HID the divergence last time.
if [[ -f .git/shallow ]]; then
    cat <<EOF

================================================================================
  SHALLOW CLONE DETECTED -- history is truncated, divergence is INVISIBLE.
================================================================================
  This is exactly what masked the 2026-07-05 divergence for a full session.
  Fix before trusting ANY git comparison:

    git fetch --unshallow origin

================================================================================
EOF
    exit 1
fi

BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "[check_branch_freshness] current branch: $BRANCH  (trunk: $TRUNK)"

git fetch origin "$TRUNK" --quiet 2>/dev/null || {
    echo "[check_branch_freshness] WARN: could not fetch origin/$TRUNK (no network?). Proceeding."
    exit 0
}

AHEAD=$(git rev-list --count origin/$TRUNK..HEAD 2>/dev/null || echo 0)
BEHIND=$(git rev-list --count HEAD..origin/$TRUNK 2>/dev/null || echo 0)

echo "[check_branch_freshness] HEAD: $AHEAD ahead, $BEHIND behind origin/$TRUNK"

# Unpushed local commits are the divergence source -- flag them loudly even below threshold.
if [[ "$AHEAD" -gt 0 ]]; then
    echo "[check_branch_freshness] NOTE: $AHEAD unpushed local commit(s). Push before any box deploy:"
    echo "    git push origin $BRANCH"
fi

if [[ "$BEHIND" -ge "$STALE_THRESHOLD" ]]; then
    cat <<EOF

================================================================================
  STALE BRANCH WARNING -- $BEHIND commits behind origin/$TRUNK (threshold $STALE_THRESHOLD)
================================================================================
  Working on '$BRANCH' this far behind WILL cause conflicts and may resurrect
  engines the live line has tombstoned (2026-06-14: 0/283 viable).

  Resolve before proceeding:
    git fetch origin
    git merge --ff-only origin/$TRUNK     # live trunk must fast-forward, never rebase-fork

  Bypass once (you know what you are doing):
    STALE_OK=1 bash tools/check_branch_freshness.sh
================================================================================
EOF
    exit 1
fi

echo "[check_branch_freshness] OK"
exit 0
