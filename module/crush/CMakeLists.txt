
add_executable(crush)

target_include_directories(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_link_libraries(
        crush
        PUBLIC
        libcrush
        light_core
        light_cli
)
