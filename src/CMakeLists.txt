include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib}
)

add_executable(pqbench pqbench.cpp)
target_link_libraries(pqbench
    ${CMAKE_THREAD_LIBS_INIT}
)
