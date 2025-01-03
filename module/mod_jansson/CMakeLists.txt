#   mod_jansson: CMake interface library wrapper for the jansson library,
# an open-source JSON parsing and encoding library
set(JANSSON_BUILD_SHARED_LIBS ON CACHE BOOL "Build jansson shared library")
set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Build jansson documentation (sphinx)")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/jansson jansson)

add_library(mod_jansson INTERFACE)

target_sources(
        mod_jansson INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_include_directories(
        mod_jansson INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/jansson/include
)

target_link_libraries(
        mod_jansson
        INTERFACE
        jansson
)