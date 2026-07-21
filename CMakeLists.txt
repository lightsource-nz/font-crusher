cmake_minimum_required(VERSION 3.12)

set(LIGHT_PATH "${CMAKE_CURRENT_LIST_DIR}/../light_framework_mk3" CACHE PATH "path to the root directory of the light-framework project")
get_filename_component(LIGHT_PATH "${LIGHT_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
include(${LIGHT_PATH}/external/light_preinit.cmake)

project(font-crusher C)

enable_testing()

# on Windows, DLLs must live alongside the executables that load them at
# runtime; colocate all runtime artifacts (exes + shared libs) so e.g.
# jansson's DLL is found next to crush.exe without needing PATH changes
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(${LIGHT_PATH}/ light_framework)

set(TEST_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/test_module")
set(TEST_RESOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/test_resource")

add_subdirectory(module/mod_jansson)
add_subdirectory(module/mod_freetype)
add_subdirectory(module/libcrush_common)
add_subdirectory(module/libcrush_context)
add_subdirectory(module/libcrush_display)
add_subdirectory(module/libcrush_font)
add_subdirectory(module/libcrush_module)
add_subdirectory(module/libcrush_render)
add_subdirectory(module/libcrush)
add_subdirectory(module/crush_render_backend)
add_subdirectory(module/crush)

add_subdirectory(test_module/cmd_render_new)
add_subdirectory(test_module/cmd_font_add)
