# crush_render_backend: contains the crush rendering engine, and the background processing support
# which enables it to run in a thread or a daemon process
add_library(crush_render_backend INTERFACE)

target_include_directories(
        crush_render_backend INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        crush_render_backend INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/backend.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        crush_render_backend
        INTERFACE
        light_core
        mod_freetype
)
