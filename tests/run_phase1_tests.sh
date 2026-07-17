#!/usr/bin/env bash
# ============================================================================
# run_phase1_tests.sh — Phase-1 review-fix regression suite.
# Runs each fix's test; prints PASS/FAIL per item; exit 0 iff ALL pass.
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"
INC="-I../include"
CXX="${CXX:-g++}"
STD="-std=c++20"
TMP="$(mktemp -d)"
rc=0

# Brew include paths (for OrderResult in BinanceREST.hpp -> curl/openssl headers).
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
  if "$TMP/$name" >"$TMP/$name.out" 2>&1; then
    echo "PASS"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== Phase-1 regression suite ==="
run_pos riprider_parity        riprider_parity_test.cpp
run_pos runtime_mode           runtime_mode_test.cpp
run_pos http_control_auth      http_control_auth_test.cpp
run_pos execution_gateway      execution_gateway_test.cpp "$BREW_INC"
run_pos pilot_scope            pilot_scope_test.cpp "$BREW_INC"

# NEGATIVE compile test: raw execute() from strategy code MUST NOT compile.
printf '── %-28s ' "spotexecutor_private"
if $CXX $STD -fsyntax-only $INC $BREW_INC spotexecutor_private_execute.cpp \
      2>"$TMP/neg.build"; then
  echo "FAIL (raw execute() COMPILED — chokepoint regressed)"; rc=1
else
  if grep -qi "private" "$TMP/neg.build"; then
    echo "PASS (rejected: execute() is private)"
  else
    echo "PASS (compile rejected)"
  fi
fi

echo "==============================="
[ $rc -eq 0 ] && echo "ALL PHASE-1 TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
