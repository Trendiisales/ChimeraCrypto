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

The live crypto line is **`xsec-deploy`**, NOT `main`. `main` is a divergent research line
(the roster-expansion program) that xsec's 2026-06-14 honest revalidation **tombstoned as
0/283 viable** (`TOMBSTONED_ROSTER_2026-06-14.md`). Do not merge `main` onto the live line
without a faithful per-engine re-backtest — its "validated"/PF rankings were fill-optimism.

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
