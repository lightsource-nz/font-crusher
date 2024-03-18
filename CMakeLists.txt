cmake_minimum_required(VERSION 3.12)

add_subdirectory(lib/freetype)

project(font_crusher C)

add_executable(font_crusher)

target_include_directories(
        font_crusher
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        font_crusher
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/export.c
)
target_link_libraries(
        font_crusher
        PUBLIC
        freetype
)
