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
# DEPLOY_FILES env (space-separated repo-relative paths) overrides the default
# engine-file list — a deploy touching files outside the default set (e.g.
# include/live/*, config/*) must name them explicitly (S-2026-07-18h).
if [ -n "${DEPLOY_FILES:-}" ]; then
  read -r -a FILES <<< "$DEPLOY_FILES"
else
  FILES=("src/main.cpp" "include/core/MimicLadderCompanion.hpp" "include/core/EdgeEngine.hpp" "include/core/CoreTriggerEngine.hpp")
fi
MSG="${DEPLOY_MSG:?set DEPLOY_MSG to the commit message}"

echo "### 1/6 PRE-DEPLOY FRESHNESS GUARD ###"
"$HERE/tools/check_box_sync.sh" || { echo "GUARD BLOCKED — re-sync then retry. NOT deployed."; exit 1; }

echo "### 1b/6 SYNC BOX TO ORIGIN BEFORE COMMIT (fork-prevention, S-2026-07-20) ###"
# ROOT FIX for the recurring lineage fork: step 4 commits ON THE BOX, so the box's
# HEAD MUST equal origin/main first — else the new commit parents on a stale HEAD and
# forks (exactly what happened when Mac-side commits — the 2a JSON hotfix + heartbeat —
# were pushed to origin but the box never pulled them). The old flow only DETECTED the
# fork at step 7 (push rejected -> "reconcile manually"). This ENFORCES the invariant up
# front: fast-forward the box to origin (mixed reset preserves the intentional uncommitted
# working tree — NEVER --hard), or ABORT if the box has diverged. Runs before any box
# mutation, so an abort leaves the box untouched.
SYNC=$(ssh "$BOX" "cd $BOX_REPO && git fetch origin -q 2>/dev/null; BH=\$(git rev-parse HEAD); OH=\$(git rev-parse origin/main);
  if [ \"\$BH\" = \"\$OH\" ]; then echo OK-EQUAL;
  elif git merge-base --is-ancestor \"\$BH\" \"\$OH\"; then git reset --mixed origin/main >/dev/null 2>&1 && echo \"OK-FASTFWD \$(git rev-parse --short HEAD)\";
  elif git merge-base --is-ancestor \"\$OH\" \"\$BH\"; then echo \"ABORT-AHEAD \$(git rev-parse --short HEAD) vs origin \$(git rev-parse --short origin/main)\";
  else echo \"ABORT-DIVERGED \$(git rev-parse --short HEAD) vs origin \$(git rev-parse --short origin/main)\"; fi")
echo "box sync: $SYNC"
case "$SYNC" in
  OK-EQUAL|OK-FASTFWD*) : ;;
  ABORT-AHEAD*)    echo "ABORT: box HEAD is AHEAD of origin/main (unpushed box commits — a prior deploy didn't push). Run: ssh $BOX 'cd $BOX_REPO && git push origin main' then retry. NOT deployed."; exit 1 ;;
  ABORT-DIVERGED*) echo "ABORT: box and origin/main have DIVERGED (both hold commits the other lacks). Reconcile the box lineage manually (NO --hard on josgp1). NOT deployed."; exit 1 ;;
  *)               echo "ABORT: box sync check returned unexpected '$SYNC' — refusing to commit on an unverified base. NOT deployed."; exit 1 ;;
esac

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
# S-2026-07-17s FIX: the [STARTUP] build= line goes to logs/chimera.log (unit
# StandardOutput=append:...), NEVER the journal — the old journalctl grep always
# returned empty and ROLLED BACK GOOD DEPLOYS (false DEPLOY-FAILED, silent-fallback
# class: the verify read a source that cannot contain the signal). Read the log
# (last STARTUP in the file = current boot); journal kept as a fallback only.
run_hash="$(ssh "$BOX" "grep -oE 'build=[0-9a-f]+' $BOX_REPO/logs/chimera.log 2>/dev/null | tail -1 | cut -d= -f2")"
[ -n "$run_hash" ] || run_hash="$(ssh "$BOX" "journalctl -u chimera --since '2 min ago' 2>/dev/null | grep -oE 'build=[0-9a-f]+' | tail -1 | cut -d= -f2")"
echo "active=$active  running build=$run_hash  want=$want_hash"
if [ "$active" != "active" ] || [ "$run_hash" != "$want_hash" ]; then
  echo "### VERIFY FAILED -> reverting box commit + restoring prior binary ###"
  ssh "$BOX" "cd $BOX_REPO && git reset --hard HEAD~1"
  for f in "${FILES[@]}"; do ssh "$BOX" "cp -v $BOX_REPO/$f.predeploy-bak $BOX_REPO/$f"; done
  ssh "$BOX" "cd $BOX_REPO/build && cmake .. >/dev/null 2>&1; make -j\$(nproc) && sudo systemctl restart chimera"
  echo "DEPLOY-FAILED: service not active or hash mismatch. Prior binary restored."; exit 1
fi
echo "DEPLOY-OK: chimera active, running binary build=$run_hash == HEAD $want_hash. Verify [CLIP-INIT] shadow=1 lines next."

# ── 7/7 AUTO-RECONCILE (added S-2026-07-15j) ─────────────────────────────────
# The box commit at step 4 is the DRIFT GENERATOR: the old flow left "push to
# origin + sync Mac" as a MANUAL after-step, and skipping it (or re-committing the
# same files on Mac) forked the lineage (2026-07-15: box c9b849a vs Mac 4d89f45,
# identical content two SHAs). Close the gap IN-PROCESS: push box->origin now (a
# fast-forward — origin was the pre-flight base), then fast-forward the Mac so all
# three share ONE lineage before the session ends. The now-live freshness guards
# (check_branch_freshness / deploy_hygiene_check, both repointed to main) BLOCK the
# next deploy if this is ever bypassed, so a fork cannot silently compound.
echo "### 7/7 AUTO-RECONCILE box -> origin -> Mac ###"
if ssh "$BOX" "cd $BOX_REPO && git push origin main"; then
  echo "pushed box $want_hash -> origin/main"
  git -C "$HERE" fetch origin
  # Mac fast-forwards only if HEAD is now an ancestor of origin AND the deployed
  # files are the ONLY working-tree change (they already match origin -> go clean).
  if git -C "$HERE" merge-base --is-ancestor HEAD origin/main; then
    other="$(git -C "$HERE" status --porcelain -- . ':(exclude)'"${FILES[0]}" ':(exclude)'"${FILES[1]}" ':(exclude)'"${FILES[2]}" | grep -vE '^\?\?' || true)"
    if [ -z "$other" ]; then
      git -C "$HERE" reset --hard origin/main && echo "Mac fast-forwarded to origin/main $want_hash — no drift."
    else
      echo "WARN: Mac has other uncommitted tracked changes; NOT auto-resetting. Reconcile:"
      echo "  git -C $HERE stash && git -C $HERE reset --hard origin/main && git -C $HERE stash pop"
    fi
  else
    echo "WARN: Mac HEAD not an ancestor of origin/main (Mac diverged). Reconcile before next deploy."
  fi
else
  echo "WARN: box->origin push FAILED — reconcile manually NOW (else lineage forks): "
  echo "  ssh $BOX 'cd $BOX_REPO && git push origin main' ; git -C $HERE fetch origin && git -C $HERE reset --hard origin/main"
fi
