cmake_minimum_required(VERSION 3.16)
project(chimera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LWS REQUIRED libwebsockets)

# CURL — headers at arch-specific location on Ubuntu 24
find_path(CURL_INCLUDE_DIR curl/curl.h
    PATHS /usr/include/x86_64-linux-gnu /usr/include
)
find_library(CURL_LIBRARY NAMES curl
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib
)

if(NOT CURL_INCLUDE_DIR OR NOT CURL_LIBRARY)
    message(FATAL_ERROR "libcurl not found")
endif()

message(STATUS "CURL headers: ${CURL_INCLUDE_DIR}")
message(STATUS "CURL library: ${CURL_LIBRARY}")

include_directories(
    include
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${LWS_INCLUDE_DIRS}
)

set(CHIMERA_SOURCES
    src/main.cpp
    src/live/BinanceWSFeed.cpp
    src/live/CoinbaseWSFeed.cpp
    src/live/PerpFeed.cpp
    src/live/SpotExecutor.cpp
    src/core/Portfolio.cpp
    src/core/StatefulGovernor.cpp
)

# ── Build version: re-stamped on EVERY make, not just cmake configure ────────
# execute_process() only runs at configure time — running make after a new
# commit without re-running cmake leaves the hash stale. Fix: custom ALL target
# writes build_version.h on every make; main.cpp depends on it so it recompiles.
set(VERSION_HEADER "${CMAKE_BINARY_DIR}/build_version.h")

add_custom_target(gen_version ALL
    COMMAND bash -c "echo '#pragma once' > ${VERSION_HEADER} && echo '#define BUILD_VERSION "'$(git -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD 2>/dev/null || echo unknown)'"' >> ${VERSION_HEADER}"
    BYPRODUCTS ${VERSION_HEADER}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Stamping git hash into build_version.h"
)

add_executable(chimera ${CHIMERA_SOURCES})
add_dependencies(chimera gen_version)

# Force main.cpp to recompile when the version header changes
set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/main.cpp
    APPEND PROPERTY OBJECT_DEPENDS ${VERSION_HEADER})

target_include_directories(chimera PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(chimera
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARY}
    ${LWS_LIBRARIES}
    pthread
)

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

# LTO strips execute permission on the output binary — restore it after every build
add_custom_command(TARGET chimera POST_BUILD
    COMMAND chmod +x $<TARGET_FILE:chimera>
    COMMENT "Restoring execute permission after LTO link"
)
