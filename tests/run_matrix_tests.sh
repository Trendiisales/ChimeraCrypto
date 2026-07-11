#!/usr/bin/env bash
# ============================================================================
# run_matrix_tests.sh — the two PERMANENT-CI-MATRIX regression tests that were
# NOT already covered by a per-phase suite (see CRYPTO_PHASE8_ROADMAP.md
# "PERMANENTLY-AUTOMATED CI TESTS"). The other 12 matrix items map onto existing
# phase tests — this runner adds only the gaps:
#   * stale-user-stream  — a stale user-data stream HALTS entries (exits pass)
#                          until a clean StartupReconciler pass resumes.
#   * user-stream-autohalt — Phase-8G LIVE-path: the halt AUTO-ARMS on a live
#                          heartbeat lapse (no explicit trigger) via the gateway
#                          kill-switch, exits pass, clears on stream-resume +
#                          clean reconcile; SHADOW no-op (never arms).
#   * allocator-vs-legacy — the track-only allocator's would-do target vs the
#                          legacy per-sleeve sum is a fully TRACEABLE diff, and
#                          track-only emits nothing.
# Header-only compositions of existing primitives — NO engine logic added.
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"
INC="-I../include"
CXX="${CXX:-g++}"
STD="-std=c++20"
TMP="$(mktemp -d)"
rc=0

# Brew include paths (stale_user_stream pulls ExecutionGateway -> BinanceREST -> curl/openssl).
BREW_INC=""
for p in /opt/homebrew/opt/curl/include /opt/homebrew/opt/openssl@3/include \
         /usr/local/opt/curl/include /usr/local/opt/openssl@3/include; do
  [ -d "$p" ] && BREW_INC="$BREW_INC -I$p"
done

run_pos() { # name  source  extra_flags
  local name="$1" src="$2" extra="${3:-}"
  printf '── %-30s ' "$name"
  if ! $CXX $STD -O2 $INC $extra "$src" -o "$TMP/$name" 2>"$TMP/$name.build"; then
    echo "BUILD-FAIL"; sed 's/^/    /' "$TMP/$name.build"; rc=1; return
  fi
  if "$TMP/$name" >"$TMP/$name.out" 2>>"$TMP/$name.out"; then
    echo "PASS"
  else
    echo "FAIL"; sed 's/^/    /' "$TMP/$name.out"; rc=1
  fi
}

echo "=== CI-matrix supplemental suite (gaps not covered by a phase suite) ==="
run_pos "stale_user_stream"     stale_user_stream_test.cpp     "$BREW_INC"
run_pos "user_stream_autohalt"  user_stream_autohalt_test.cpp  "$BREW_INC"
run_pos "allocator_vs_legacy"   allocator_vs_legacy_test.cpp

echo "================================================"
[ $rc -eq 0 ] && echo "ALL MATRIX TESTS PASS" || echo "SOME TESTS FAILED"
rm -rf "$TMP"
exit $rc
