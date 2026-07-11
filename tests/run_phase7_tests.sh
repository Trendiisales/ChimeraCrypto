#!/usr/bin/env bash
# ============================================================================
# run_phase7_tests.sh — Phase-7 (DERIVATIVES-DATA-AS-SIGNAL) regression suite.
# One test per data-supported signal (funding percentile, spot-vs-perp CVD,
# perp-spot basis) + the OBSERVATION-ONLY invariant (eval is a pure read; the
# would-be size multiplier is a size MODIFIER at most, never a new entry / short).
#
# All three filters were REJECTED by backtest/phase7_derivsignals_bt.cpp
# (funding NEUTRAL, CVD non-monotonic, basis SUSPECT) — Phase 7 ships as a pure
# OBSERVATION-ONLY recorder. These tests pin the SIGNAL MECHANICS + the
# spot-long-only / changes-nothing invariant, not a promotion claim.
# See the vault entity ChimeraReviewPhase7. Header-only, dep-free.
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

echo "=== Phase-7 regression suite (derivatives-data-as-signal — OBSERVATION-ONLY) ==="
run_pos "funding_percentile"  deriv_funding_pct_test.cpp
run_pos "cvd_divergence"      deriv_cvd_divergence_test.cpp
run_pos "basis"               deriv_basis_test.cpp
run_pos "observation_only"    deriv_observation_only_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-7 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
