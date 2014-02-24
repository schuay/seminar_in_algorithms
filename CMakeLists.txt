project(seminar_in_algorithms)
cmake_minimum_required(VERSION 2.8)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(NOBLE_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/noble/libNOBLEDEMO32CPP.a")
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(NOBLE_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/noble/libNOBLEDEMO64CPP.a")
endif()

# Required by Linden's implementation.
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHE_LINE_SIZE)
add_definitions("-DINTEL -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions("-Wall -Wextra -pedantic -std=c++11 -fmessage-length=0 -fno-omit-frame-pointer")
endif()

add_subdirectory(src)
