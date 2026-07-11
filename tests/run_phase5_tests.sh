#!/usr/bin/env bash
# ============================================================================
# run_phase5_tests.sh — Phase-5 (XSec 2.0) regression suite.
# One test per built-out item (24-27): composite score, hysteresis (no
# oscillation), core+challenger selection, point-in-time universe (no
# look-ahead). Prints PASS/FAIL per item; exit 0 iff ALL pass. Header is
# dep-free (stdlib only).
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"
INC="-I../include"
CXX="${CXX:-g++}"
STD="-std=c++20"
TMP="$(mktemp -d)"
rc=0

run_pos() { # name  source
  local name="$1" src="$2"
  printf '── %-28s ' "$name"
  if ! $CXX $STD -O2 $INC "$src" -o "$TMP/$name" 2>"$TMP/$name.build"; then
    echo "BUILD-FAIL"; sed 's/^/    /' "$TMP/$name.build"; rc=1; return
  fi
  if "$TMP/$name" >"$TMP/$name.out" 2>>"$TMP/$name.out"; then
    echo "PASS"; sed 's/^/    /' "$TMP/$name.out"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-5 regression suite (XSec 2.0 — items 24-27) ==="
run_pos "24 composite_score"    composite_score_test.cpp
run_pos "25 hysteresis"         hysteresis_test.cpp
run_pos "26 core_challenger"    core_challenger_test.cpp
run_pos "27 pit_universe"       pit_universe_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-5 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
