#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# check_box_sync.sh — PRE-DEPLOY FRESHNESS GUARD for the live crypto box.
#
# WHY THIS EXISTS (S-2026-07-11 incident): a deploy BLOCKED because the Mac
# ChimeraCrypto checkout had silently drifted BEHIND the LIVE box — local
# `edcc252` vs box `3e74d00`, plus uncommitted roster-finalize mods on the box the
# Mac never had. A roster patch built on the stale base FAILED to apply (context
# mismatch at main.cpp:2722) and, blindly forced, would have CORRUPTED main.cpp.
# Omega has tools/check_branch_freshness.sh; crypto had NOTHING. This is that guard.
#
# WHAT IT CHECKS:
#   • HEAD comparison (local deploy-source HEAD  vs  live box HEAD) — the fast
#     signal the incident tripped (edcc252 != 3e74d00).
#   • AUTHORITATIVE GATE — the LIVE engine files you are about to overwrite
#     (src/main.cpp + include/core/UpJumpLadderCompanion.hpp) are byte-compared
#     (sha256) Mac vs box. The box runs INTENTIONAL uncommitted mods, so HEAD
#     alone is not enough — a diff in the actual deploy surface is what corrupts a
#     patch. Files differ => BLOCK. (HEAD-differs-but-files-match, e.g. an
#     unrelated tooling commit, is reported and allowed — no cry-wolf.)
#
# MODES:  (default) BLOCK on a deploy-surface diff (exit 1).
#         WARN_ONLY=1  print drift, exit 0 (cron/liveness drift WARNING).
#         STALE_OK=1   proceed despite a diff (you KNOW they're equivalent).
# ENV:    CHIMERA_BOX (default chimera-direct), CHIMERA_BOX_REPO (default ~/ChimeraCrypto)
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail
BOX="${CHIMERA_BOX:-chimera-direct}"
BOX_REPO="${CHIMERA_BOX_REPO:-~/ChimeraCrypto}"
LOCAL_REPO="$(cd "$(dirname "$0")/.." && pwd)"
FILES=("src/main.cpp" "include/core/UpJumpLadderCompanion.hpp" "include/core/EdgeEngine.hpp")

local_head="$(git -C "$LOCAL_REPO" rev-parse --short HEAD 2>/dev/null || echo LOCAL_UNKNOWN)"
box_head="$(ssh -o ConnectTimeout=15 "$BOX" "git -C $BOX_REPO rev-parse --short HEAD" 2>/dev/null || echo BOX_UNREACHABLE)"
echo "local (Mac)  HEAD : $local_head"
echo "box   (live) HEAD : $box_head"
if [ "$box_head" = "BOX_UNREACHABLE" ]; then
  echo "WARN: could not reach box $BOX — cannot verify sync."
  [ "${WARN_ONLY:-0}" = "1" ] && exit 0
  echo "Aborting (safe default: never deploy against an unverifiable base)."; exit 1
fi
[ "$local_head" != "$box_head" ] && echo "note: HEADs differ — checking the actual deploy surface (engine files)…"

# AUTHORITATIVE: byte-compare the Mac's COMMITTED BASE (git show HEAD:file — NOT the
# working tree, which holds the new edits you are about to deploy) to the box's LIVE
# files. This answers the real question — "were my edits re-based onto the box's current
# files?" — so a legitimate new deploy (working-tree ahead of a matching base) is allowed,
# while a STALE base (my HEAD behind the box) is BLOCKED. Deploy the working tree only
# once this passes.
sha() { shasum -a 256 2>/dev/null || sha256sum; }
drift=0
for f in "${FILES[@]}"; do
  lh="$(git -C "$LOCAL_REPO" show "HEAD:$f" 2>/dev/null | sha | awk '{print $1}')"
  bh="$(ssh -o ConnectTimeout=15 "$BOX" "shasum -a 256 $BOX_REPO/$f 2>/dev/null || sha256sum $BOX_REPO/$f" 2>/dev/null | awk '{print $1}')"
  if [ -z "$lh" ]; then echo "WARN: could not read Mac HEAD:$f"; drift=1; continue; fi
  if [ -z "$bh" ]; then echo "WARN: could not hash box:$f"; drift=1; continue; fi
  if [ "$lh" != "$bh" ]; then echo "DRIFT: $f  (Mac HEAD:${lh:0:12} != box-live:${bh:0:12})"; drift=1
  else echo "base-match: $f (Mac committed base == box live)"; fi
done

if [ "$drift" = "1" ]; then
  cat <<MSG

==========================================================================
 DEPLOY BLOCKED: Mac deploy surface != the LIVE box's engine files
   (local HEAD $local_head, box HEAD $box_head)
 The box carries commits/uncommitted mods your checkout does NOT have. A
 stale-base edit/'git apply' CORRUPTS main.cpp. RE-SYNC FIRST:
   scp $BOX:$BOX_REPO/src/main.cpp                           src/main.cpp
   scp $BOX:$BOX_REPO/include/core/UpJumpLadderCompanion.hpp include/core/UpJumpLadderCompanion.hpp
 Re-apply your changes onto THOSE live files, then re-run the deploy.
==========================================================================
MSG
  [ "${WARN_ONLY:-0}" = "1" ] && { echo "WARN_ONLY=1 -> surfacing only (exit 0)."; exit 0; }
  [ "${STALE_OK:-0}"  = "1" ] && { echo "STALE_OK=1 -> proceeding despite drift."; exit 0; }
  exit 1
fi
echo "OK: Mac engine files == live box."

# ── CONFIG-RECONCILE GATE (added S-2026-07-15j) ──────────────────────────────
# ROOT CAUSE of the recurring Mac<->box drift: runtime config JSONs are edited
# LIVE on the box (whitelist tweaks, engine state flips) and NEVER committed back
# to git, so every committed snapshot is wrong and the two dev lineages guess
# differently (2026-07-15: origin had theta/sushi + UPJUMP-GRID SHADOW; Mac had
# neither/DISABLED; box-live was the only truth). The engine-file check above is
# blind to config. This gate enforces the invariant: Mac-committed config ==
# box-live config. If the box edited config live without committing it back,
# BLOCK and force a reconcile so the truth lands in git BEFORE any deploy.
CONFIG_FILES=("config/symbol_whitelist.json" "config/engine_registry.json")
cfg_drift=0
for f in "${CONFIG_FILES[@]}"; do
  lh="$(git -C "$LOCAL_REPO" show "HEAD:$f" 2>/dev/null | sha | awk '{print $1}')"
  bh="$(ssh -o ConnectTimeout=15 "$BOX" "shasum -a 256 $BOX_REPO/$f 2>/dev/null || sha256sum $BOX_REPO/$f" 2>/dev/null | awk '{print $1}')"
  if [ -z "$lh" ] || [ -z "$bh" ]; then echo "WARN: could not hash config $f"; cfg_drift=1; continue; fi
  if [ "$lh" != "$bh" ]; then echo "CONFIG-DRIFT: $f  (Mac committed:${lh:0:12} != box-live:${bh:0:12})"; cfg_drift=1
  else echo "config-match: $f (Mac committed == box live)"; fi
done
if [ "$cfg_drift" = "1" ]; then
  cat <<MSG

==========================================================================
 CONFIG DRIFT: box-live config != Mac-committed config.
 The box edited runtime config LIVE and never committed it back to git.
 This is THE recurring drift root cause. Reconcile the LIVE truth into git
 BEFORE deploying (else the deploy reverts live tuning / re-forks lineage):
   scp $BOX:$BOX_REPO/config/symbol_whitelist.json config/symbol_whitelist.json
   scp $BOX:$BOX_REPO/config/engine_registry.json  config/engine_registry.json
   git add config/*.json && git commit -m "config reconcile: box-live truth" && git push origin main
   ssh $BOX "cd $BOX_REPO && git fetch origin && git reset --hard origin/main"
==========================================================================
MSG
  [ "${WARN_ONLY:-0}" = "1" ] && { echo "WARN_ONLY=1 -> surfacing only (exit 0)."; exit 0; }
  [ "${STALE_OK:-0}"  = "1" ] && { echo "STALE_OK=1 -> proceeding despite config drift."; exit 0; }
  exit 1
fi
echo "OK: Mac engine files + config == live box. Safe to deploy."
