#!/usr/bin/env bash
# ============================================================================
# run_phase6_tests.sh — Phase-6 (NEW long-only families) regression suite.
# One behavioural test per family (trend-pullback, compression breakout, bull-
# regime MR) + a shared NO-200DMA / breadth-gate invariant. All families are
# SHADOW OBSERVATION-ONLY (they FAILED the pick-edge control — see the vault
# entity ChimeraReviewPhase6). These tests pin the ENGINE MECHANICS (entries,
# exits, regime gate), not a promotion claim. Header-only, dep-free.
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"
INC="-I../include"
CXX="${CXX:-g++}"
STD="-std=c++20"
TMP="$(mktemp -d)"
rc=0

run_pos() {
  local name="$1" src="$2"
  printf '── %-30s ' "$name"
  if ! $CXX $STD -O2 $INC "$src" -o "$TMP/$name" 2>"$TMP/$name.build"; then
    echo "BUILD-FAIL"; sed 's/^/    /' "$TMP/$name.build"; rc=1; return
  fi
  if "$TMP/$name" >"$TMP/$name.out" 2>>"$TMP/$name.out"; then
    echo "PASS"; sed 's/^/    /' "$TMP/$name.out"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-6 regression suite (new long-only families — OBSERVATION-ONLY) ==="
run_pos "trend_pullback"      trend_pullback_test.cpp
run_pos "compression_breakout" compression_breakout_test.cpp
run_pos "bull_meanrev"        bull_meanrev_test.cpp
run_pos "no_200dma_breadth"   no_200dma_breadth_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-6 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
