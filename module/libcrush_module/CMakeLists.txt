
add_library(libcrush_module INTERFACE)

target_include_directories(
        libcrush_module INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_module INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/crush_module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush_module
        INTERFACE
        light_cli
)
