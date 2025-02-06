#   libcrush_common: core routines and dependencies required by
# all parts of the crush project
add_library(libcrush_common INTERFACE)

target_include_directories(
        libcrush_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        libcrush_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/common.c
        ${CMAKE_CURRENT_LIST_DIR}/src/queue.c
)

target_link_libraries(
        libcrush_common
        INTERFACE
        light_core
        mod_jansson
)
