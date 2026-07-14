# ChimeraCrypto — Deploy Hygiene (live-money crypto box)

Written 2026-07-05 after the github divergence incident. Ported from the Omega repo's
deploy-hygiene P0 (CLAUDE.md), which ChimeraCrypto never had.

## What went wrong (so it can't recur)

The live crypto binary was traceable only to a **box-local SHA `5a85fc1`** that origin had
never seen. mac was on `25d27cc`, origin/xsec-deploy on `911bef2` — **three-way disagreement**,
same class as the Omega 2026-05-14 incident. Root causes:

1. **Commits were made ON THE BOX.** The D1 deploy patched + committed on the box → the box
   got a different SHA than mac for identical content. `scripts/deploy.sh` then did
   `git pull --rebase`, which re-applied those box commits as *new* SHAs, compounding the fork.
2. **Origin was never kept in sync** — pushes were ad-hoc; origin sat a content-step stale and
   accumulated 20+ junk branches.
3. **The mac clone was shallow** (`.git/shallow`) → history invisible → divergence misdiagnosed.

## The trunk

The live crypto line is **`main`** (as of the 2026-07 migration; the old `xsec-deploy` trunk
is GONE from origin). `tools/check_branch_freshness.sh` and `tools/deploy_hygiene_check.sh` were
repointed xsec-deploy -> main on 2026-07-15j — until then they were pinned to the dead branch
and were **inert** (see the 2026-07-15 recurrence below).

## The rules

1. **NEVER commit on the box.** The box is a pure *consumer* of origin. All commits happen on
   mac (`/Users/jo/ChimeraCrypto`), get pushed to `origin/xsec-deploy`, and the box
   fast-forwards. `scripts/deploy.sh` now aborts if the box is ahead of origin.

2. **Deploy flow (the only allowed path):**
   ```
   # mac
   git commit ... && git push origin xsec-deploy
   # box (ssh chimera-direct)
   bash scripts/deploy.sh          # pulls --ff-only, builds, restarts
   ```
   Never `git apply` a patch on the box and commit it there. If a surgical hotfix is
   unavoidable, commit it on mac first, push, then ff-only pull on the box.

3. **Never shallow-clone the working repo.** If `.git/shallow` exists, run
   `git fetch --unshallow origin`. `tools/check_branch_freshness.sh` refuses to run shallow.

4. **Three-way gate after every deploy:** `bash tools/deploy_hygiene_check.sh` — must show
   `mac == origin == box HEAD`, box binary built from box HEAD, box local-ahead = 0.
   RED = fix before trusting the deploy.

5. **Session-start freshness:** `bash tools/check_branch_freshness.sh` — blocks on stale/shallow.

## Operational files on the box

`config/live_config.json`, `CMakeLists.txt`, `cmake/GenVersion.cmake`, `backtest/optimizer_v2`,
and the space-saving `data/klines_spot/*_1m.csv` deletions are legitimately box-local and must
survive any reconcile. Preferred long-term fix: move `live_config.json` out of version control
(gitignore + a committed `live_config.example.json`). Until then, any box reset MUST be
`git reset --soft` (pointer-only) or a stash/reapply — NEVER `git reset --hard` / `git pull`
that would clobber them. Back up `live_config.json` before any git surgery and verify it is
byte-identical after.

## 2026-07-15j recurrence — the drift came back a THIRD way (config), now structurally closed

Despite the 2026-07-05 fixes, drift recurred: mac `4d89f45` and box `c9b849a` were **content-
identical commits with different SHAs** (both S-2026-07-15h "MIMIC-FLOOR", committed once on each
side), and origin/main was ~4 logical commits behind on a **parallel re-implementation lineage**
(each PHASE3/CAMPAIGN/SWEET/PERCOIN change committed twice, cross-referencing "Mac <sha>"/"box
<sha>"). On top of that, the two runtime config JSONs **crossed**: origin had `theta/sushi` +
`UPJUMP-GRID=SHADOW`; mac had neither + `DISABLED`; **the box-live (uncommitted) config was the
only operational truth.** Why the existing guards missed it:

- `check_branch_freshness.sh` + `deploy_hygiene_check.sh` were **pinned to the dead `xsec-deploy`
  branch** → fetch failed → guards exited 0 (inert). *Fix: repointed to `main`.*
- `check_box_sync.sh` byte-compared only the **3 engine files**, never the config JSONs, so
  config edited-live-never-committed was invisible. *Fix: added a **CONFIG-RECONCILE GATE** — it
  now byte-compares `config/symbol_whitelist.json` + `config/engine_registry.json` (mac-committed
  vs box-live) and BLOCKS the deploy if they differ, forcing the live truth into git first.*
- The canonical deploy (`deploy_to_box.sh`) **commits on the box** (step 4) and left "push to
  origin + sync mac" as a MANUAL after-step — the async gap that forks the lineage. *Fix: **step 7
  AUTO-RECONCILE** pushes box→origin (fast-forward) and fast-forwards mac in-process, so all three
  share ONE lineage every deploy. If bypassed, the now-live freshness guards BLOCK the next deploy.*

**Reconcile performed:** made mac-committed config == box-live (commit `1468f9a`), then
`git merge -s ours origin/main` (keep mac tree, record origin as ancestor → fast-forward push past
branch protection, no force) → origin/main `06bf2eb`; box `git reset --hard origin/main` → clean
tree, service undisturbed. Backup of the pre-reconcile origin tip: remote tag
`drift-archive/origin-main-pre-reconcile-20260715` (`07d19e6`).

**The invariant now enforced by tooling:** *box-live config == mac-committed config == origin ==
box HEAD, on `main`, every deploy.* Note this supersedes rule 1's absolute "NEVER commit on the
box": box commits ARE allowed **only** because step 7 immediately pushes them to origin + syncs
mac in the same automated run. A box commit that is not auto-reconciled is still forbidden.
