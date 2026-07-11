#!/usr/bin/env bash
# ============================================================================
# run_phase4_tests.sh — Phase-4 review-fix regression suite
# (WIRING / REGISTRY / OBSERVABILITY). One test per item (20-23). Prints
# PASS/FAIL per item; exit 0 iff ALL pass. Headers are dep-free (no curl/REST).
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
    echo "PASS"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-4 regression suite (WIRING / REGISTRY / OBSERVABILITY) ==="
run_pos "20 engine_registry"   engine_registry_test.cpp
run_pos "21 gate_attribution"  gate_attribution_test.cpp
run_pos "22 realistic_fill"    realistic_fill_test.cpp
run_pos "23 data_quality"      data_quality_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-4 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
