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

add_executable(pqbench
    heap.cpp
    linden.cpp
    noble.cpp
    pqbench.cpp
)

target_link_libraries(pqbench
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    ${NOBLE_LIBRARIES}
    linden
)
