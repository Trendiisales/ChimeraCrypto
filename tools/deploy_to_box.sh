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
# Usage:  tools/deploy_to_box.sh          (deploys src/main.cpp + the engine header)
# ENV:    CHIMERA_BOX (default chimera-direct), CHIMERA_BOX_REPO (default ~/ChimeraCrypto)
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail
BOX="${CHIMERA_BOX:-chimera-direct}"
BOX_REPO="${CHIMERA_BOX_REPO:-~/ChimeraCrypto}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
FILES=("src/main.cpp" "include/core/UpJumpLadderCompanion.hpp")

echo "### 1/5 PRE-DEPLOY FRESHNESS GUARD ###"
"$HERE/tools/check_box_sync.sh" || { echo "GUARD BLOCKED — re-sync then retry. NOT deployed."; exit 1; }

want_hash="$(git -C "$HERE" rev-parse --short HEAD)"
echo "### deploying local HEAD $want_hash to $BOX ###"

echo "### 2/5 back up box engine files ###"
for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f $BOX_REPO/$f.predeploy-bak"; done

echo "### 3/5 scp + build on box (running service untouched) ###"
for f in "${FILES[@]}"; do scp -q "$HERE/$f" "$BOX:$BOX_REPO/$f"; done
if ! ssh "$BOX" "cd $BOX_REPO/build && make -j\$(nproc)" ; then
  echo "### BUILD FAILED -> restoring backups (service was never interrupted) ###"
  for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f.predeploy-bak $BOX_REPO/$f"; done
  echo "DEPLOY-FAILED: build error. Live service still running the prior binary."; exit 1
fi

echo "### 4/5 restart chimera ###"
ssh "$BOX" "sudo systemctl restart chimera && sleep 16"

echo "### 5/5 POST-DEPLOY VERIFY: active + hash == $want_hash ###"
active="$(ssh "$BOX" "systemctl is-active chimera" 2>/dev/null)"
run_hash="$(ssh "$BOX" "journalctl -u chimera --since '2 min ago' 2>/dev/null | grep -oE 'build=[0-9a-f]+' | tail -1 | cut -d= -f2")"
echo "active=$active  running build=$run_hash  want=$want_hash"
if [ "$active" != "active" ] || [ "$run_hash" != "$want_hash" ]; then
  echo "### VERIFY FAILED -> restoring prior binary ###"
  for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f.predeploy-bak $BOX_REPO/$f"; done
  ssh "$BOX" "cd $BOX_REPO/build && make -j\$(nproc) && sudo systemctl restart chimera"
  echo "DEPLOY-FAILED: service not active or hash mismatch. Prior binary restored."; exit 1
fi
echo "DEPLOY-OK: chimera active, running binary build=$run_hash == HEAD. Verify [CLIP-INIT] shadow=1 lines next."
