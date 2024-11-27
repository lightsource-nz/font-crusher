
add_library(libcrush_display INTERFACE)

target_include_directories(
        libcrush_display INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_display INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/display.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush_display
        INTERFACE
        light_core
        light_cli
)
