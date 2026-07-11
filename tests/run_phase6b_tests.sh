#!/usr/bin/env bash
# ============================================================================
# run_phase6b_tests.sh — Phase-6b (REMAINING long-only families + item 28 two-
# stage ignition) regression suite. SALVAGE CHECK: all 7 families FAILED the
# exposure-matched pick-edge control and item 28 (tranche entry) did NOT beat the
# immediate-only RipRider parent (see wiki ChimeraReviewPhase6b + the two
# backtests). NONE is wired into production. These tests pin ENGINE MECHANICS,
# the NO-200DMA breadth invariant, and the item-28 under-deployment finding — so
# the REJECT verdicts are real results, not broken code. Header-only, dep-free.
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

echo "=== Phase-6b regression suite (remaining families + two-stage — SALVAGE, none wired) ==="
run_pos "phase6b_families"  phase6b_families_test.cpp
run_pos "phase6b_twostage"  phase6b_twostage_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-6b TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
