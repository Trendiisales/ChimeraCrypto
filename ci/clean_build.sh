#!/usr/bin/env bash
# ============================================================================
# ci/clean_build.sh — CLEAN-BUILD REPRODUCIBILITY CHECK (roadmap P0 "build repro").
#
# Builds the live `chimera` binary from a FRESH build directory to prove the
# tree configures + compiles reproducibly from scratch (no stale CMake cache, no
# leftover objects). Prints the resolved dependency versions so a build-repro
# regression (a bumped/absent system lib) is visible.
#
# Usage:   bash ci/clean_build.sh            # full live-binary clean build
#          CHIMERA_TESTS_ONLY=1 bash ci/clean_build.sh   # harness-only (no LWS)
#
# Exit 0 iff configure + build succeed and the expected artifact exists.
#
# EXTERNAL DEPENDENCIES (hand-rolled JSON — there is NO json.hpp; the Phase-1
# review CONFIRMED the "missing third_party/json.hpp" claim a NON-issue):
#   * C++20 compiler (clang++ / g++)
#   * CMake >= 3.16
#   * OpenSSL (>= 3)          — HMAC-SHA256 request signing         [live only]
#   * libcurl                 — Binance REST                        [live only]
#   * libwebsockets           — Binance market-data / user streams  [live only]
#   * pkg-config              — locates libwebsockets               [live only]
#   Debian/Ubuntu: build-essential cmake pkg-config libssl-dev \
#                  libcurl4-openssl-dev libwebsockets-dev
#   macOS (brew):  cmake openssl@3 curl libwebsockets
# ============================================================================
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$(pwd)"

TESTS_ONLY="${CHIMERA_TESTS_ONLY:-0}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-clean}"

echo "############################################################"
echo "# ChimeraCrypto clean-build reproducibility check"
echo "# root=$ROOT  build=$BUILD_DIR  tests_only=$TESTS_ONLY"
echo "############################################################"

echo
echo "── toolchain / dependency versions ─────────────────────────"
( ${CXX:-c++} --version 2>/dev/null | head -1 ) || echo "compiler: MISSING"
( cmake --version 2>/dev/null | head -1 )       || echo "cmake: MISSING"
if [ "$TESTS_ONLY" != "1" ]; then
  ( pkg-config --modversion openssl        2>/dev/null && echo "  ^ openssl" )      || echo "openssl: (via find_package)"
  ( pkg-config --modversion libcurl        2>/dev/null && echo "  ^ libcurl" )      || echo "libcurl: (found by path)"
  ( pkg-config --modversion libwebsockets  2>/dev/null && echo "  ^ libwebsockets" )|| echo "libwebsockets: MISSING (required for live binary)"
fi

echo
echo "── fresh configure (wiping $BUILD_DIR) ─────────────────────"
rm -rf "$BUILD_DIR"
CMAKE_ARGS=(-S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release)
if [ "$TESTS_ONLY" = "1" ]; then
  CMAKE_ARGS+=(-DCHIMERA_TESTS_ONLY=ON)
fi
cmake "${CMAKE_ARGS[@]}"

if [ "$TESTS_ONLY" = "1" ]; then
  echo
  echo "── tests-only: no binary to build; verifying ctest registration ──"
  ( cd "$BUILD_DIR" && ctest -N | tail -3 )
  echo "CLEAN-BUILD (tests-only) OK"
  exit 0
fi

echo
echo "── build chimera (fresh objects) ───────────────────────────"
cmake --build "$BUILD_DIR" --target chimera -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

ART="$BUILD_DIR/chimera"
if [ -x "$ART" ]; then
  echo
  echo "── artifact ────────────────────────────────────────────────"
  ls -la "$ART"
  echo "CLEAN-BUILD OK — chimera built reproducibly from a fresh tree"
  exit 0
else
  echo "CLEAN-BUILD FAIL — expected artifact $ART not produced"
  exit 1
fi
