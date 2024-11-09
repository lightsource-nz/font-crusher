

find_package(Freetype REQUIRED CONFIG)

add_library(mod_freetype INTERFACE)

target_link_libraries(
        mod_freetype
        INTERFACE
        freetype
)
