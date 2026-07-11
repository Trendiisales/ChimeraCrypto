#!/usr/bin/env bash
# ============================================================================
# run_phase8a_tests.sh — Phase-8A Stage-2 regression suite (ALLOCATOR HARD-CAP
# ENFORCEMENT). The allocator moves from TRACK-ONLY to a Stage-2 SAFETY BACKSTOP:
# engines still propose qty, but govern_entry() can REDUCE/REJECT a BUY that
# genuinely breaches the symbol / momentum-factor / drawdown caps. Cash stays
# gated on portfolio_cash_usd>0 (go-live only) — NOT enforced in shadow.
#
# The KEY promotion test is the no-erroneous-rejection proof: an in-limit order
# stream through OFF (track-only) vs HARDCAP is byte-identical.
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

echo "=== Phase-8A regression suite (ALLOCATOR HARD-CAP ENFORCEMENT) ==="
run_pos "8A hardcap_enforce"        hardcap_enforce_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-8A TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
