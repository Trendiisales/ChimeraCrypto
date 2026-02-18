cmake_minimum_required(VERSION 3.16)
project(ChimeraCrypto_Baseline)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(chimera ${SOURCES})
