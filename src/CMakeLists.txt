include_directories(
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/libcds
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
)
