
add_library(libcrush_render INTERFACE)

target_include_directories(
        libcrush_render INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_render INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/render.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush_render
        INTERFACE
        crush_render_backend
        light_core
        light_cli
)
