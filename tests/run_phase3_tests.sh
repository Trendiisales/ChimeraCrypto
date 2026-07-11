#!/usr/bin/env bash
# ============================================================================
# run_phase3_tests.sh — Phase-3 review-fix regression suite (PORTFOLIO UNIFICATION).
# One test per item (15-19). Prints PASS/FAIL per item; exit 0 iff ALL pass.
# All headers are dep-free (no curl/REST), so no brew include path is needed.
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
  printf '── %-30s ' "$name"
  if ! $CXX $STD -O2 $INC "$src" -o "$TMP/$name" 2>"$TMP/$name.build"; then
    echo "BUILD-FAIL"; sed 's/^/    /' "$TMP/$name.build"; rc=1; return
  fi
  if "$TMP/$name" >"$TMP/$name.out" 2>>"$TMP/$name.out"; then
    echo "PASS"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-3 regression suite (PORTFOLIO UNIFICATION) ==="
run_pos "15 portfolio_allocator"    portfolio_allocator_test.cpp
run_pos "16 momentum_factor_cap"    momentum_factor_cap_test.cpp
run_pos "17 portfolio_risk"         portfolio_risk_test.cpp
run_pos "18 drawdown_governor"      drawdown_governor_test.cpp
run_pos "19 regime_exposure"        regime_exposure_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-3 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
