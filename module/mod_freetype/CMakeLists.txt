

find_package(Freetype REQUIRED CONFIG)

add_library(mod_freetype INTERFACE)

target_include_directories(
        mod_freetype
        INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)
target_sources(
        mod_freetype
        INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)
target_link_libraries(
        mod_freetype
        INTERFACE
        freetype
)
