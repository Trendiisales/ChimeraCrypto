cmake_minimum_required(VERSION 3.16)
project(chimera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LWS REQUIRED libwebsockets)

include_directories(
    include
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${LWS_INCLUDE_DIRS}
)

file(GLOB_RECURSE CHIMERA_SOURCES
    src/*.cpp
)

add_executable(chimera
    ${CHIMERA_SOURCES}
)

target_link_libraries(chimera
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${LWS_LIBRARIES}
    pthread
)

target_compile_options(chimera PRIVATE
    -O3
    -march=native
    -flto
)

set_target_properties(chimera PROPERTIES
    LINK_FLAGS "-flto"
)
