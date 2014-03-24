# Squelch warnings in ugly external code.
set(CFLAGS_NO_WARNINGS "-Wno-unused-parameter -Wno-type-limits -Wno-overflow -Wno-format -Wno-pedantic -Wno-sign-compare -Wno-unused-but-set-variable -Wno-long-long -Wno-unused-function -Wno-implicit-function-declaration -Wno-unused-variable")

include_directories(
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/libcds
)

add_library(linden STATIC
    ${CMAKE_SOURCE_DIR}/lib/linden/common.c
    ${CMAKE_SOURCE_DIR}/lib/linden/prioq.c
    ${CMAKE_SOURCE_DIR}/lib/linden/gc/gc.c
    ${CMAKE_SOURCE_DIR}/lib/linden/gc/ptst.c
)

set_target_properties(linden PROPERTIES COMPILE_FLAGS
    "-std=c99 ${CFLAGS_NO_WARNINGS}"
)

add_library(spraylist STATIC
    ${CMAKE_SOURCE_DIR}/lib/spraylist/gc/gc.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/gc/ptst.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/fraser.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/intset.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/pqueue.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/skiplist.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/ssalloc.c
)

target_include_directories(spraylist PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/spraylist
    ${CMAKE_SOURCE_DIR}/lib/spraylist/atomic_ops/
    ${CMAKE_SOURCE_DIR}/lib/spraylist/include
)

set_target_properties(spraylist PROPERTIES COMPILE_FLAGS
    "-DLOCKFREE -DSSALLOC_USE_MALLOC ${CFLAGS_NO_WARNINGS}"
)

add_executable(pqbench
    globallock.cpp
    heap.cpp
    linden.cpp
    noble.cpp
    pqbench.cpp
    spraylist.cpp
)

target_link_libraries(pqbench
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    ${NOBLE_LIBRARIES}
    linden
    spraylist
)
