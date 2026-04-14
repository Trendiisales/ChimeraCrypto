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

set(CHIMERA_SOURCES
    src/main.cpp
    src/live/BinanceWSFeed.cpp
    src/live/CoinbaseWSFeed.cpp
    src/live/PerpFeed.cpp
    src/live/SpotExecutor.cpp
)

# Build version
set(VERSION_HEADER "${CMAKE_SOURCE_DIR}/include/version_generated.hpp")
add_custom_target(gen_version ALL
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/GenVersion.cmake
    BYPRODUCTS ${VERSION_HEADER}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Stamping git hash into include/version_generated.hpp"
)

add_executable(chimera ${CHIMERA_SOURCES})
add_dependencies(chimera gen_version)

set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/main.cpp
    APPEND PROPERTY OBJECT_DEPENDS ${VERSION_HEADER})

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

add_custom_command(TARGET chimera POST_BUILD
    COMMAND chmod +x $<TARGET_FILE:chimera>
    COMMENT "Restoring execute permission after LTO link"
)
