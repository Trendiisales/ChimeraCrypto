#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# deploy_to_box.sh — CANONICAL Mac-side deploy of the crypto engine to the LIVE box.
#
# The crypto box (chimera-direct / josgp1) runs with INTENTIONAL local/uncommitted
# mods, so the deploy is scp-based (Mac edits -> box), NOT `git pull` on the box.
# This wrapper makes a stale-base deploy IMPOSSIBLE and never leaves the live
# service broken:
#   1. FRESHNESS GUARD FIRST (check_box_sync.sh) — BLOCK if the Mac checkout has
#      drifted from the live box (the S-2026-07-11 incident).
#   2. Back up the box's current engine files (*.predeploy-bak).
#   3. scp the reconciled files, BUILD on the box (running service untouched).
#   4. If build fails -> RESTORE + report DEPLOY-FAILED (service never interrupted).
#   5. restart chimera; POST-DEPLOY HASH-VERIFY: running binary build== must equal
#      the deployed HEAD (Omega DeployHygiene). If not active OR hash mismatch ->
#      RESTORE backups + rebuild + restart the prior binary, report DEPLOY-FAILED.
#
# Usage:  DEPLOY_MSG="commit message" tools/deploy_to_box.sh
# ENV:    CHIMERA_BOX (default chimera-direct), CHIMERA_BOX_REPO (default ~/ChimeraCrypto),
#         DEPLOY_MSG (commit message — REQUIRED; the box commits the 2 files BEFORE build so
#         the stamped git hash matches what runs, since we deploy the working tree).
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail
BOX="${CHIMERA_BOX:-chimera-direct}"
BOX_REPO="${CHIMERA_BOX_REPO:-~/ChimeraCrypto}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
FILES=("src/main.cpp" "include/core/UpJumpLadderCompanion.hpp" "include/core/EdgeEngine.hpp")
MSG="${DEPLOY_MSG:?set DEPLOY_MSG to the commit message}"

echo "### 1/6 PRE-DEPLOY FRESHNESS GUARD ###"
"$HERE/tools/check_box_sync.sh" || { echo "GUARD BLOCKED — re-sync then retry. NOT deployed."; exit 1; }

echo "### 2/6 back up box engine files ###"
for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f $BOX_REPO/$f.predeploy-bak"; done

echo "### 3/6 scp working-tree files to box ###"
for f in "${FILES[@]}"; do scp -q "$HERE/$f" "$BOX:$BOX_REPO/$f"; done

echo "### 4/6 commit on box (stamps the git hash the binary will report) ###"
ssh "$BOX" "cd $BOX_REPO && git add ${FILES[*]} && git commit -m \"$MSG

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>\" >/dev/null && git rev-parse --short HEAD"
want_hash="$(ssh "$BOX" "cd $BOX_REPO && git rev-parse --short HEAD")"
echo "box HEAD now $want_hash"

echo "### 5/6 build on box (running service untouched) ###"
if ! ssh "$BOX" "cd $BOX_REPO/build && cmake .. >/dev/null 2>&1; make -j\$(nproc)" ; then
  echo "### BUILD FAILED -> restoring backups + reverting box commit (service never interrupted) ###"
  ssh "$BOX" "cd $BOX_REPO && git reset --hard HEAD~1"
  for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f.predeploy-bak $BOX_REPO/$f"; done
  echo "DEPLOY-FAILED: build error. Live service still running the prior binary."; exit 1
fi

ssh "$BOX" "sudo systemctl restart chimera && sleep 16"

echo "### 6/6 POST-DEPLOY VERIFY: active + hash == $want_hash ###"
active="$(ssh "$BOX" "systemctl is-active chimera" 2>/dev/null)"
run_hash="$(ssh "$BOX" "journalctl -u chimera --since '2 min ago' 2>/dev/null | grep -oE 'build=[0-9a-f]+' | tail -1 | cut -d= -f2")"
echo "active=$active  running build=$run_hash  want=$want_hash"
if [ "$active" != "active" ] || [ "$run_hash" != "$want_hash" ]; then
  echo "### VERIFY FAILED -> reverting box commit + restoring prior binary ###"
  ssh "$BOX" "cd $BOX_REPO && git reset --hard HEAD~1"
  for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f.predeploy-bak $BOX_REPO/$f"; done
  ssh "$BOX" "cd $BOX_REPO/build && cmake .. >/dev/null 2>&1; make -j\$(nproc) && sudo systemctl restart chimera"
  echo "DEPLOY-FAILED: service not active or hash mismatch. Prior binary restored."; exit 1
fi
echo "DEPLOY-OK: chimera active, running binary build=$run_hash == HEAD $want_hash. Verify [CLIP-INIT] shadow=1 lines next."
echo "NOTE: box committed $want_hash locally — push to origin + sync the Mac after confirming CLIP-INIT."
