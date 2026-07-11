#!/usr/bin/env bash
# ============================================================================
# run_all_tests.sh — ONE consolidated ChimeraCrypto regression runner.
#
# Runs every per-phase suite (Phase 1..7 + 6b) plus the CI-matrix supplemental
# suite, and prints a SINGLE pass/fail summary. Exit 0 iff every suite passes.
#
# This is the entry point wired into CMake/ctest (see CMakeLists.txt
# `enable_testing()` + `add_test`) and CI (.github/workflows/ci.yml). It is also
# runnable standalone:  bash tests/run_all_tests.sh
#
# Coverage of the 14-item "PERMANENTLY-AUTOMATED CI TESTS" matrix
# (CRYPTO_PHASE8_ROADMAP.md) — 12 map onto existing phase tests, 2 are the
# matrix supplemental suite:
#   gateway-bypass ............. P1 execution_gateway + spotexecutor_private (neg)
#   insufficient-cash collision  P2 cash_reservation
#   duplicate-symbol collision . P3 portfolio_allocator
#   partial-fill ............... P2 exchange_ledger
#   unknown-order .............. P2 order_recovery
#   restart-open-position ...... P2 startup_reconcile (position mismatch blocks)
#   restart-working-order ...... P2 startup_reconcile (adopt, no dup)
#   stale-user-stream .......... MATRIX stale_user_stream            [added]
#   mode-mismatch .............. P1 runtime_mode
#   registry-mismatch .......... P4 engine_registry
#   regime-boundary-oscillation  P3 regime_exposure (+ P5 hysteresis)
#   drawdown-ladder ............ P3 drawdown_governor
#   allocator-vs-legacy ........ MATRIX allocator_vs_legacy          [added]
#   counterfactual-realism ..... P4 realistic_fill (+ P4 gate_attribution)
# ============================================================================
set -uo pipefail
cd "$(dirname "$0")"

SUITES=(
  "Phase 1  (safety P0s)          :run_phase1_tests.sh"
  "Phase 2  (exchange truth)      :run_phase2_tests.sh"
  "Phase 3  (portfolio unify)     :run_phase3_tests.sh"
  "Phase 4  (wiring/observability):run_phase4_tests.sh"
  "Phase 5  (XSec 2.0)            :run_phase5_tests.sh"
  "Phase 6  (long-only families)  :run_phase6_tests.sh"
  "Phase 6b (remaining families)  :run_phase6b_tests.sh"
  "Phase 7  (derivatives signals) :run_phase7_tests.sh"
  "CI-matrix supplemental         :run_matrix_tests.sh"
)

rc=0
declare -a RESULTS
echo "############################################################"
echo "# ChimeraCrypto — full regression run ($(date '+%Y-%m-%d %H:%M:%S'))"
echo "############################################################"
for entry in "${SUITES[@]}"; do
  label="${entry%%:*}"
  script="${entry##*:}"
  echo
  echo ">>> $label  [$script]"
  if bash "$script"; then
    RESULTS+=("PASS  $label")
  else
    RESULTS+=("FAIL  $label")
    rc=1
  fi
done

echo
echo "############################################################"
echo "# SUMMARY"
echo "############################################################"
for r in "${RESULTS[@]}"; do echo "  $r"; done
echo "------------------------------------------------------------"
if [ $rc -eq 0 ]; then
  echo "  ALL SUITES PASS (${#SUITES[@]}/${#SUITES[@]})"
else
  npass=$(printf '%s\n' "${RESULTS[@]}" | grep -c '^PASS')
  echo "  SUITE FAILURE — ${npass}/${#SUITES[@]} passed"
fi
exit $rc
