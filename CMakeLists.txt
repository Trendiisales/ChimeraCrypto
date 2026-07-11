cmake_minimum_required(VERSION 3.16)
project(chimera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Options ─────────────────────────────────────────────────────────────────
# CHIMERA_TESTS_ONLY: configure ONLY the regression-test harness (ctest), skip
# the live `chimera` binary and its libwebsockets/curl/openssl link. Lets a
# clean CI machine run `cmake -DCHIMERA_TESTS_ONLY=ON .. && ctest` with just a
# C++20 compiler (the per-phase suites compile their own header-only TUs and
# locate curl/openssl includes themselves when a test needs them).
option(CHIMERA_TESTS_ONLY "Build only the ctest regression harness (no live binary)" OFF)
option(BUILD_TESTING "Register the regression suites with ctest" ON)

if(NOT CHIMERA_TESTS_ONLY)
    find_package(OpenSSL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LWS REQUIRED libwebsockets)

    # CURL
    find_path(CURL_INCLUDE_DIR curl/curl.h
        PATHS /usr/include/x86_64-linux-gnu /usr/include
    )
    find_library(CURL_LIBRARY NAMES curl
        PATHS /usr/lib/x86_64-linux-gnu /usr/lib
    )
    if(NOT CURL_INCLUDE_DIR OR NOT CURL_LIBRARY)
        message(FATAL_ERROR "libcurl not found")
    endif()

    include_directories(
        include
        ${OPENSSL_INCLUDE_DIR}
        ${CURL_INCLUDE_DIR}
        ${LWS_INCLUDE_DIRS}
    )

    # ── Sources for the live binary ─────────────────────────────────────────
    # Tier-2 rewrite (2026-05-11): dropped PerpFeed + CoinbaseWSFeed — both were
    # only consumed by the deleted FundingWindow/BasisMomentum/OBI engines.
    set(CHIMERA_LIVE_SOURCES
        src/live/BinanceWSFeed.cpp
        src/live/SpotExecutor.cpp
    )

    set(CHIMERA_SOURCES
        src/main.cpp
        ${CHIMERA_LIVE_SOURCES}
    )

    # Build version stamp
    set(VERSION_HEADER "${CMAKE_SOURCE_DIR}/include/version_generated.hpp")
    add_custom_target(gen_version ALL
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/GenVersion.cmake
        BYPRODUCTS ${VERSION_HEADER}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Stamping git hash into include/version_generated.hpp"
    )

    # ── Live binary ─────────────────────────────────────────────────────────
    add_executable(chimera ${CHIMERA_SOURCES})
    add_dependencies(chimera gen_version)

    set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/main.cpp
        APPEND PROPERTY OBJECT_DEPENDS ${VERSION_HEADER})

    link_directories(${LWS_LIBRARY_DIRS})

    target_link_libraries(chimera
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CURL_LIBRARY}
        ${LWS_LIBRARIES}
        pthread
    )
    target_link_directories(chimera PRIVATE ${LWS_LIBRARY_DIRS})

    target_compile_options(chimera PRIVATE
        -O3
        -march=native
        -flto
        -Wall
        -Wextra
        -Wno-unused-parameter
    )

    set_target_properties(chimera PROPERTIES
        LINK_FLAGS "-flto"
    )

    add_custom_command(TARGET chimera POST_BUILD
        COMMAND chmod +x $<TARGET_FILE:chimera>
        COMMENT "Restoring execute permission after LTO link"
    )

    # NOTE: chimera_backtest and chimera_backtest_paper binaries removed in the
    # Tier-2 rewrite. The Python backtest pipeline in chimera_edges/ (Binance REST
    # replay) supersedes them — see HANDOFF_TIER2.md.
endif()

# ── Regression harness (ctest) ──────────────────────────────────────────────
# Each per-phase suite + the CI-matrix supplemental suite is a bash script that
# compiles + runs its own dep-free (mostly) test TUs. `run_all_tests.sh` chains
# them with a single summary. Registered individually so `ctest` reports each
# suite's pass/fail and `ctest -R phase2` can target one. `cmake --build . &&
# ctest --output-on-failure` runs the whole matrix.
if(BUILD_TESTING)
    enable_testing()
    set(_chimera_suites
        phase1  run_phase1_tests.sh
        phase2  run_phase2_tests.sh
        phase3  run_phase3_tests.sh
        phase4  run_phase4_tests.sh
        phase5  run_phase5_tests.sh
        phase6  run_phase6_tests.sh
        phase6b run_phase6b_tests.sh
        phase7  run_phase7_tests.sh
        matrix  run_matrix_tests.sh
    )
    list(LENGTH _chimera_suites _n)
    math(EXPR _last "${_n} - 1")
    foreach(_i RANGE 0 ${_last} 2)
        list(GET _chimera_suites ${_i} _name)
        math(EXPR _j "${_i} + 1")
        list(GET _chimera_suites ${_j} _script)
        add_test(NAME ${_name}
                 COMMAND bash ${CMAKE_SOURCE_DIR}/tests/${_script})
    endforeach()

    # Aggregate convenience test (the single-summary runner).
    add_test(NAME all_tests
             COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_all_tests.sh)
endif()
