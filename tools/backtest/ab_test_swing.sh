#!/usr/bin/env bash
# =============================================================================
# ab_test_swing.sh -- A/B backtest for SwingEngine parameter changes.
#
# Builds and runs chimera_backtest TWICE on identical Binance H4 kline data:
#   - BEFORE: the version of SwingEngine.hpp at git HEAD
#   - AFTER:  the working-tree version (with your uncommitted changes)
#
# Workflow:
#   1. Stash uncommitted SwingEngine.hpp -> tree is at HEAD (BEFORE state)
#   2. Build chimera_backtest, run, save to backtest_out_before/
#   3. Pop stash -> tree is back at AFTER state
#   4. Rebuild, run, save to backtest_out_after/
#   5. Print summary.txt for both, plus a unified diff
#
# Pre-conditions:
#   - You must be in the chimera-git repo
#   - include/core/SwingEngine.hpp must have UNCOMMITTED changes (the fix)
#   - cmake + clang/g++ + libcurl available (Mac: brew install cmake curl)
#   - Internet access to fetch klines from api.binance.com
#
# Usage:
#   chmod +x tools/backtest/ab_test_swing.sh
#   ./tools/backtest/ab_test_swing.sh
#
# Notes:
#   --bars 1500 = 1500 H4 candles = 1500 * 4h = 250 days of history.
#   Default is all 8 symbols. Pass --symbols ethusdt to limit (faster).
# =============================================================================

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null)"
if [[ -z "$REPO_ROOT" ]]; then
    echo "[A/B] ERROR: not inside a git repo" >&2
    exit 1
fi
cd "$REPO_ROOT"

# --- pre-flight ---------------------------------------------------------------
if git diff --quiet -- include/core/SwingEngine.hpp; then
    echo "[A/B] ERROR: include/core/SwingEngine.hpp has no uncommitted changes." >&2
    echo "       Apply the fix first, then run this script. The script needs" >&2
    echo "       working-tree changes to define the AFTER state." >&2
    exit 1
fi

OUT_BEFORE="$REPO_ROOT/backtest_out_before"
OUT_AFTER="$REPO_ROOT/backtest_out_after"
BUILD_DIR="$REPO_ROOT/build_abtest"

# Allow CLI passthrough -- e.g. ./ab_test_swing.sh --bars 2000 --symbols ethusdt
EXTRA_ARGS=("$@")
if [[ ${#EXTRA_ARGS[@]} -eq 0 ]]; then
    EXTRA_ARGS=("--bars" "1500")
fi

# --- step 1: stash AFTER-fix changes (reverts working tree to HEAD = BEFORE) -
# Note: -m must come BEFORE -- per git-stash(1). With `--` first, the message
# string gets parsed as a pathspec and the stash silently does nothing.
echo "[A/B] step 1/5: stashing working-tree SwingEngine.hpp (AFTER state)"
git stash push -m "ab_test_swing temporary stash" -- include/core/SwingEngine.hpp
if ! git stash list | grep -q "ab_test_swing temporary stash"; then
    echo "[A/B] ERROR: stash failed -- aborting" >&2
    exit 1
fi

# Trap to ensure we always restore the stash on exit (success or failure).
trap '
    if git stash list | grep -q "ab_test_swing temporary stash"; then
        echo ""
        echo "[A/B] cleanup: popping stash to restore AFTER state"
        git stash pop || echo "[A/B] WARN: stash pop failed -- recover manually with git stash list / pop"
    fi
' EXIT

# --- step 2: build + run BEFORE ----------------------------------------------
echo ""
echo "[A/B] step 2/5: BEFORE -- configuring + building chimera_backtest"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release > /tmp/abtest_cmake_before.log 2>&1 \
    || { echo "[A/B] cmake configure failed -- see /tmp/abtest_cmake_before.log"; exit 1; }
cmake --build "$BUILD_DIR" --target chimera_backtest -j > /tmp/abtest_build_before.log 2>&1 \
    || { echo "[A/B] BEFORE build failed -- see /tmp/abtest_build_before.log"; exit 1; }
echo "[A/B] BEFORE: running chimera_backtest --quiet ${EXTRA_ARGS[*]} --out backtest_out_before"
"$BUILD_DIR/chimera_backtest" --quiet "${EXTRA_ARGS[@]}" --out "$OUT_BEFORE" \
    || { echo "[A/B] BEFORE run failed"; exit 1; }

# --- step 3: pop stash (restore AFTER state) ---------------------------------
echo ""
echo "[A/B] step 3/5: popping stash -- restoring AFTER state"
git stash pop > /dev/null
if git diff --quiet -- include/core/SwingEngine.hpp; then
    echo "[A/B] ERROR: stash pop did not restore changes -- aborting" >&2
    exit 1
fi
trap - EXIT  # disable cleanup trap, we've successfully restored

# --- step 4: rebuild + run AFTER ---------------------------------------------
echo ""
echo "[A/B] step 4/5: AFTER -- rebuilding"
cmake --build "$BUILD_DIR" --target chimera_backtest -j > /tmp/abtest_build_after.log 2>&1 \
    || { echo "[A/B] AFTER build failed -- see /tmp/abtest_build_after.log"; exit 1; }
echo "[A/B] AFTER: running chimera_backtest --quiet ${EXTRA_ARGS[*]} --out backtest_out_after"
"$BUILD_DIR/chimera_backtest" --quiet "${EXTRA_ARGS[@]}" --out "$OUT_AFTER" \
    || { echo "[A/B] AFTER run failed"; exit 1; }

# --- step 5: print comparison ------------------------------------------------
echo ""
echo "[A/B] step 5/5: comparison"
echo ""
echo "================================================================"
echo "BEFORE FIX (HEAD)"
echo "================================================================"
cat "$OUT_BEFORE/summary.txt" || echo "(no summary.txt produced)"
echo ""
echo "================================================================"
echo "AFTER FIX (chimera-fix-1+2+3)"
echo "================================================================"
cat "$OUT_AFTER/summary.txt" || echo "(no summary.txt produced)"
echo ""
echo "================================================================"
echo "UNIFIED DIFF (-before / +after)"
echo "================================================================"
diff -u "$OUT_BEFORE/summary.txt" "$OUT_AFTER/summary.txt" || true
echo ""
echo "================================================================"
echo "Per-trade CSVs for deeper inspection:"
echo "  BEFORE trades : $OUT_BEFORE/trades.csv"
echo "  AFTER  trades : $OUT_AFTER/trades.csv"
echo "  BEFORE equity : $OUT_BEFORE/equity.csv"
echo "  AFTER  equity : $OUT_AFTER/equity.csv"
echo "================================================================"
