#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# chimera_deploy.sh — FIRST STEP IS THE FRESHNESS GUARD (S-2026-07-11).
#
# The live box runs with INTENTIONAL local/uncommitted mods, so a bare
# `git pull` here would clobber them or fail. The CANONICAL, guarded deploy is
# the Mac-side wrapper:  tools/deploy_to_box.sh  (guard -> backup -> scp -> build
# -> restart -> post-deploy hash-verify -> restore-on-fail). USE THAT.
#
# This legacy box-side path is kept for emergencies only, and now runs the
# PRE-DEPLOY FRESHNESS GUARD first so a stale-base deploy is impossible.
# (Guard needs to compare Mac vs box; if run ON the box it degrades to a warn.)
# ─────────────────────────────────────────────────────────────────────────────
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
echo "### PRE-DEPLOY FRESHNESS GUARD (stale-base = corrupted main.cpp; see S-2026-07-11) ###"
"$HERE/tools/check_box_sync.sh" || { echo "GUARD BLOCKED — re-sync then retry. NOT deployed."; exit 1; }

cd ~/ChimeraCrypto
git pull origin main
cd build && make -j$(nproc) || { echo "BUILD FAILED"; exit 1; }
cd ~/ChimeraCrypto
sudo systemctl restart chimera
sleep 16
# POST-DEPLOY HASH-VERIFY (Omega DeployHygiene): running binary must == HEAD.
want="$(git rev-parse --short HEAD)"
run="$(journalctl -u chimera --since '2 min ago' 2>/dev/null | grep -oE 'build=[0-9a-f]+' | tail -1 | cut -d= -f2)"
echo "active=$(systemctl is-active chimera) running=build=$run want=$want"
[ "$run" = "$want" ] || { echo "WARN: running binary build=$run != HEAD $want — investigate (DeployHygiene)."; exit 1; }
echo "DEPLOY-OK: chimera active, build=$run == HEAD."
