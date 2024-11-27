# libcrush: aggregated module which exists (mostly) as a single include point for
#       all libcrush_x library modules
add_library(libcrush INTERFACE)

target_include_directories(
        libcrush INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush
        INTERFACE
        libcrush_context
        libcrush_display
        libcrush_font
        libcrush_module
        libcrush_render
)
