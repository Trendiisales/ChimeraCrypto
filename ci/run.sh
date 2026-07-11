#!/usr/bin/env bash
# ============================================================================
# ci/run.sh — one-command CI entry point for a clean machine.
#
# Configures (tests-only, no libwebsockets needed) -> runs the whole regression
# matrix via ctest -> fails on any red. This is the documented fallback for
# environments without the GitHub Actions runner (.github/workflows/ci.yml runs
# the same steps). For the full live-binary reproducibility check, additionally
# run  bash ci/clean_build.sh.
#
# Usage:  bash ci/run.sh
# Exit 0 iff every suite passes.
# ============================================================================
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$(pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-tests}"

echo "== ChimeraCrypto CI: configure (tests-only) =="
cmake -S "$ROOT" -B "$BUILD_DIR" -DCHIMERA_TESTS_ONLY=ON -DCMAKE_BUILD_TYPE=Release

echo "== ChimeraCrypto CI: ctest =="
cd "$BUILD_DIR"
ctest --output-on-failure
