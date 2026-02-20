
add_library(governor OBJECT
    governor/CapitalAllocationGovernor.cpp
)
target_include_directories(governor PUBLIC ${CMAKE_SOURCE_DIR}/include)
