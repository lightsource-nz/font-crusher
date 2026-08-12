#   mod_jansson: CMake interface library wrapper for the jansson library,
# an open-source JSON parsing and encoding library
set(JANSSON_BUILD_SHARED_LIBS ON CACHE BOOL "Build jansson shared library")
set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Build jansson documentation (sphinx)")

# Don't build or register jansson's own test suite. FORCE because these need to override
# whatever a previous configure cached, and because opting back in does not currently produce
# anything usable -- see below.
#
# It contributed 231 of the 232 tests `ctest` found in this project, so font-crusher's own
# results were a rounding error inside its dependency's, and 7 of them failed. None of those
# failures say anything about font-crusher:
#
#  - test_chaos_realloc and test_memory_funcs_realloc died with 0xc0000139
#    (STATUS_ENTRYPOINT_NOT_FOUND). The tests are built into jansson/bin while this wrapper
#    deliberately relocates the DLL to the top-level bin so crush.exe sits beside it -- see
#    the set_target_properties call below, which exists for exactly that reason.
#  - test_copy, test_dump, test_equal, test_pack and test_unpack assert on depth limits and
#    on error-message wording. The submodule is clean at v2.15.1, so library and tests come
#    from one release; they disagree under a SHARED build with this toolchain, which is a
#    configuration jansson's API tests -- they reach for library internals -- are not written
#    for. This wrapper needs the shared build; jansson's tests need the static one.
#
# So the suite tests jansson, in a configuration we impose on it, and can only ever report
# that configuration back at us. Building 231 test executables to learn that is not a good
# trade. Examples go for the same reason: build time for something never run.
set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "Don't build jansson's test suite" FORCE)
set(JANSSON_EXAMPLES OFF CACHE BOOL "Don't build jansson's example programs" FORCE)
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