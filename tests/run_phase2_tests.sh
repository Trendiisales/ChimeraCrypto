#!/usr/bin/env bash
# ============================================================================
# run_phase2_tests.sh — Phase-2 review-fix regression suite (EXCHANGE TRUTH).
# One test per defect (items 1-9). Prints PASS/FAIL per item; exit 0 iff ALL pass.
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"
INC="-I../include"
CXX="${CXX:-g++}"
STD="-std=c++20"
TMP="$(mktemp -d)"
rc=0

# Brew include paths (only needed for tests that include BinanceREST via the gateway).
BREW_INC=""
for p in /opt/homebrew/opt/curl/include /opt/homebrew/opt/openssl@3/include \
         /usr/local/opt/curl/include /usr/local/opt/openssl@3/include; do
  [ -d "$p" ] && BREW_INC="$BREW_INC -I$p"
done

run_pos() { # name  source  extra_flags
  local name="$1" src="$2" extra="${3:-}"
  printf '── %-28s ' "$name"
  if ! $CXX $STD -O2 $INC $extra "$src" -o "$TMP/$name" 2>"$TMP/$name.build"; then
    echo "BUILD-FAIL"; sed 's/^/    /' "$TMP/$name.build"; rc=1; return
  fi
  if "$TMP/$name" >"$TMP/$name.out" 2>>"$TMP/$name.out"; then
    echo "PASS"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-2 regression suite (EXCHANGE TRUTH) ==="
run_pos "1+3 exchange_ledger"    exchange_ledger_test.cpp
run_pos "2 cash_reservation"     cash_reservation_test.cpp   "$BREW_INC"
run_pos "4 exit_quantity"        exit_quantity_test.cpp      "$BREW_INC"
run_pos "5 exchange_filters"     exchange_filters_test.cpp
run_pos "6 time_sync"            time_sync_test.cpp          "$BREW_INC"
run_pos "7 order_recovery"       order_recovery_test.cpp
run_pos "8 user_stream"          user_stream_test.cpp
run_pos "9 startup_reconcile"    startup_reconcile_test.cpp  "$BREW_INC"

echo "================================================"
[ $rc -eq 0 ] && echo "ALL PHASE-2 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
