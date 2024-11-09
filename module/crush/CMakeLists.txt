
add_executable(crush)

target_include_directories(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/context.c
        ${CMAKE_CURRENT_LIST_DIR}/src/display.c
        ${CMAKE_CURRENT_LIST_DIR}/src/font.c
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/render.c
)

target_link_libraries(
        crush
        PUBLIC
        light_core
        light_cli
        mod_freetype
)
