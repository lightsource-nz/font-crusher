
add_library(libcrush_font INTERFACE)

target_include_directories(
        libcrush_font INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_font INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/font.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush_font
        INTERFACE
        light_core
        light_cli
)
