
add_library(libcrush_context INTERFACE)

target_include_directories(
        libcrush_context INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_context INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/context.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        libcrush_context
        INTERFACE
        light_core
        light_cli
)
