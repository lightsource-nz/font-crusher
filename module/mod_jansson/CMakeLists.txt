#   mod_jansson: CMake interface library wrapper for the jansson library,
# an open-source JSON parsing and encoding library
set(JANSSON_BUILD_SHARED_LIBS ON CACHE BOOL "Build jansson shared library")
set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Build jansson documentation (sphinx)")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/jansson jansson)

# jansson's own CMakeLists.txt overrides CMAKE_RUNTIME_OUTPUT_DIRECTORY for its
# own scope; override it back on the target itself so the built DLL lands next
# to consuming executables (e.g. crush.exe) rather than a separate directory
if(WIN32)
        set_target_properties(jansson PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

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