cmake_minimum_required(VERSION 3.12)

get_filename_component(LIGHT_PATH "${LIGHT_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
include(${LIGHT_PATH}/external/light_preinit.cmake)

project(font-crusher C)

add_subdirectory(${LIGHT_PATH}/ light_framework)

add_subdirectory(module/mod_freetype)
add_subdirectory(module/libcrush_context)
add_subdirectory(module/libcrush_display)
add_subdirectory(module/libcrush_font)
add_subdirectory(module/libcrush_module)
add_subdirectory(module/libcrush_render)
add_subdirectory(module/libcrush)
add_subdirectory(module/crush)
