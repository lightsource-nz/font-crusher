
add_executable(crush)

set(COPY_CONTEXT ${COPY_CONTEXT} CACHE BOOL "Determines whether a Crush context should be copied into the build directory")
 
# we need to actively remove any context left by previous builds
add_custom_command(
        TARGET crush POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/.crush
        COMMAND rm -r ${CMAKE_BINARY_DIR}/.crush)
if(COPY_CONTEXT)
if(NOT DEFINED COPY_CONTEXT_NAME)
        set(COPY_CONTEXT_NAME "test_context_default")
endif()
        add_custom_command(
                TARGET crush POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${TEST_RESOURCE_DIR}/${COPY_CONTEXT_NAME}
                        ${CMAKE_BINARY_DIR}/.crush)
endif()

target_include_directories(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        crush PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_link_libraries(
        crush
        PUBLIC
        libcrush
        light_core
        light_cli
)
