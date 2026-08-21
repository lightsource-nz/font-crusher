# CrushTools.cmake
#
# CMake routines for invoking the font-crusher `crush` CLI as part of a client project's
# build, and for turning a font file into a buildable CMake target exposing the
# `light_draw_font_t` C source/header pair that `crush render new` generates.
#
# Prerequisite: a `crush` executable target must already exist -- e.g. via light_draw's
# light_draw_init_font_crusher(), or by find_package(crush MODULE) using Findcrush.cmake
# alongside this file -- before any of these routines are called.
#
# Public routines:
#   crush_invoke(...)          - low level: run an arbitrary crush subcommand as a build step
#   crush_add_font_target(...) - high level: register a font+display with a crush context and
#                                 render it into a light_draw_font_t C source/header pair,
#                                 wrapped in a buildable CMake target
#
# crush_add_font_target() runs its commands as ONE `crush console` BATCH, not one process per
# command: every crush invocation pays for a full framework boot, a context load off disk and a
# render engine started and stopped again, so the three steps a font target needs (font add,
# display add, render new) used to cost three boots where the console pays one. The batch is a
# generated script beside the context, which also leaves a human-runnable record of exactly
# what the build asked crush to do.
#
cmake_minimum_required(VERSION 3.17)

# cached, not a plain set(): CMake functions are global once defined, so crush_add_font_target()
# is callable from any directory in the consuming project -- but a plain variable is scoped to
# the directory that ran the include() and its children. a caller in a sibling directory (e.g. a
# module elsewhere in the tree from the one that pulled rend in) would find this empty and emit a
# `cmake -E copy_directory` with no source argument, failing with cmake's bare usage message
set(CRUSH_TOOLS_CONTEXT_TEMPLATE_DIR "${CMAKE_CURRENT_LIST_DIR}/context_template"
        CACHE INTERNAL "path to the crush context template bundled with font-crusher")
# cached for the same reason as the template dir above: crush_invoke() may be called from any
# directory in the consuming project, and a plain variable would be empty in a sibling one
set(CRUSH_TOOLS_RUN_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/crush_run.cmake"
        CACHE INTERNAL "path to the crush invocation wrapper bundled with font-crusher")

# crush's own logging is captured and discarded unless the invocation fails, so that a build
# shows one COMMENT line per font step rather than the hundreds of log lines a DEBUG-mode crush
# emits per render. turn this ON to see it live -- specifically if crush HANGS, since captured
# output is only replayed on exit and a step that never exits would otherwise show nothing
option(CRUSH_VERBOSE "stream crush's own output to the build console instead of capturing it" OFF)

# crush_invoke(OUTPUT <file> [<file> ...] COMMAND <arg> [<arg> ...]
#              [WORKING_DIRECTORY <dir>] [DEPENDS <file> ...] [COMMENT <text>])
#
# Runs `crush <arg>...` as a build step that produces OUTPUT. The `crush` executable is
# built automatically (as its own ExternalProject "utility" target) the first time
# anything depends on OUTPUT.
function(crush_invoke)
    cmake_parse_arguments(CI "" "WORKING_DIRECTORY;COMMENT" "OUTPUT;COMMAND;DEPENDS" ${ARGN})

    if (NOT TARGET crush)
        message(FATAL_ERROR "crush_invoke() requires the 'crush' target - call find_package(crush MODULE) or rend_init_font_crusher() first")
    endif()
    if (NOT CI_OUTPUT)
        message(FATAL_ERROR "crush_invoke() requires at least one OUTPUT file")
    endif()
    if (NOT CI_COMMAND)
        message(FATAL_ERROR "crush_invoke() requires COMMAND arguments")
    endif()
    if (NOT CI_WORKING_DIRECTORY)
        set(CI_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    if (NOT CI_COMMENT)
        string(REPLACE ";" " " _cmd_str "${CI_COMMAND}")
        set(CI_COMMENT "crush ${_cmd_str}")
    endif()

    file(MAKE_DIRECTORY ${CI_WORKING_DIRECTORY})

    # crush runs through crush_run.cmake rather than directly, so that its own log output is
    # captured and only surfaced if it fails -- see that script. the COMMENT above is what the
    # build actually shows, one line per step, like a compile line naming its source file.
    #
    # $<SEMICOLON> rather than a literal ';': the argument list has to survive as ONE argument to
    # cmake -P, and a literal semicolon here would be expanded by add_custom_command into separate
    # arguments, handing the script only the first of them. the generator expression emits the
    # separator after that expansion has happened, so the script receives a proper CMake list
    string(REPLACE ";" "$<SEMICOLON>" _ci_args "${CI_COMMAND}")
    add_custom_command(
        OUTPUT ${CI_OUTPUT}
        COMMAND ${CMAKE_COMMAND}
                -DCRUSH_EXE=$<TARGET_FILE:crush>
                -DCRUSH_WORKDIR=${CI_WORKING_DIRECTORY}
                -DCRUSH_ARGS=${_ci_args}
                -DCRUSH_VERBOSE=$<BOOL:${CRUSH_VERBOSE}>
                -P ${CRUSH_TOOLS_RUN_SCRIPT}
        WORKING_DIRECTORY ${CI_WORKING_DIRECTORY}
        DEPENDS crush ${CI_DEPENDS}
        COMMENT "${CI_COMMENT}"
        VERBATIM
    )
endfunction()

# crush_context_dir(CONTEXT_DIR <dir> OUTPUT_VAR <var>)
#
# Ensures a fresh, empty crush context exists under <dir>/.crush, seeded from a bundled
# template (since `crush context new` does not currently populate one itself). <var> is
# set to the context's context.json file, for later steps to DEPENDS on.
function(crush_context_dir)
    cmake_parse_arguments(CC "" "CONTEXT_DIR;OUTPUT_VAR" "" ${ARGN})
    if (NOT CC_CONTEXT_DIR OR NOT CC_OUTPUT_VAR)
        message(FATAL_ERROR "crush_context_dir() requires CONTEXT_DIR and OUTPUT_VAR")
    endif()

    set(_stamp ${CC_CONTEXT_DIR}/.crush/context.json)
    add_custom_command(
        OUTPUT ${_stamp}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CC_CONTEXT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CRUSH_TOOLS_CONTEXT_TEMPLATE_DIR} ${CC_CONTEXT_DIR}/.crush
        COMMENT "seeding crush context at ${CC_CONTEXT_DIR}"
        VERBATIM
    )
    set(${CC_OUTPUT_VAR} ${_stamp} PARENT_SCOPE)
endfunction()

# crush_add_font_target(<target>
#       FONT <file>
#       DISPLAY_NAME <name> DISPLAY_WIDTH <px> DISPLAY_HEIGHT <px> [DISPLAY_DIMENSION <WxH>]
#       RENDER_NAME <name> POINT_SIZE <pt> PIXEL_SIZE <px>
#       [FACE_INDEX <n>] [CONTEXT_DIR <dir>])
#
# Defines <target>, an INTERFACE library carrying the generated C source and include
# directory for the rend_font_t produced by rendering FONT (via `crush render new`)
# against the given display -- link it with target_link_libraries() from whichever
# executable/library needs the font. An explicit PIXEL_SIZE is required, since crush
# embeds it in the generated file name and CMake must know the output file names
# up front in order to declare them as build outputs.
function(crush_add_font_target TARGET_NAME)
    set(oneValueArgs FONT DISPLAY_NAME DISPLAY_WIDTH DISPLAY_HEIGHT DISPLAY_DIMENSION
                      RENDER_NAME POINT_SIZE PIXEL_SIZE FACE_INDEX CONTEXT_DIR)
    cmake_parse_arguments(CT "" "${oneValueArgs}" "" ${ARGN})

    foreach(_required FONT DISPLAY_NAME DISPLAY_WIDTH DISPLAY_HEIGHT RENDER_NAME POINT_SIZE PIXEL_SIZE)
        if (NOT DEFINED CT_${_required})
            message(FATAL_ERROR "crush_add_font_target(${TARGET_NAME}) missing required argument ${_required}")
        endif()
    endforeach()

    if (NOT CT_CONTEXT_DIR)
        set(CT_CONTEXT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.crush_context)
    endif()

    get_filename_component(_font_abs ${CT_FONT} ABSOLUTE)
    get_filename_component(_font_name ${_font_abs} NAME)
    get_filename_component(_font_base ${_font_abs} NAME_WE)
    get_filename_component(_font_ext ${_font_abs} EXT)
    string(REGEX REPLACE "^\\." "" _font_ext "${_font_ext}")

    crush_context_dir(CONTEXT_DIR ${CT_CONTEXT_DIR} OUTPUT_VAR _context_stamp)

    #   the three commands a font target needs, composed as a `crush console` SCRIPT and run
    # as one batch: one framework boot, one context load, one render engine -- where three
    # separate invocations paid all of that three times over. console stops at the first
    # failing command, so the batch fails the build exactly where a failing single step did.
    #   the font path is quoted for the console tokenizer, which strips "" quoting and
    # deliberately has no backslash escapes -- which is precisely what lets a Windows path
    # pass through it verbatim.
    set(_batch "# generated by crush_add_font_target(${TARGET_NAME}) -- do not edit.\n")
    string(APPEND _batch "# runnable by hand with `crush console <this file>` from the context directory.\n")
    set(_font_add_line "font add --local-file \"${_font_abs}\"")
    if (CT_FACE_INDEX)
        string(APPEND _font_add_line " --face-index ${CT_FACE_INDEX}")
    endif()
    string(APPEND _batch "${_font_add_line}\n")
    set(_display_add_line "display add ${CT_DISPLAY_NAME} ${CT_DISPLAY_WIDTH} ${CT_DISPLAY_HEIGHT}")
    if (CT_DISPLAY_DIMENSION)
        string(APPEND _display_add_line " --dimension ${CT_DISPLAY_DIMENSION}")
    endif()
    string(APPEND _batch "${_display_add_line}\n")
    string(APPEND _batch "render new ${CT_RENDER_NAME} ${CT_POINT_SIZE} ${CT_PIXEL_SIZE} --font ${_font_name} --display ${CT_DISPLAY_NAME}\n")

    #   file(GENERATE) rather than file(WRITE): it leaves an unchanged file untouched, so a
    # reconfigure does not bump the script's timestamp and needlessly re-run every batch
    set(_script ${CT_CONTEXT_DIR}/${TARGET_NAME}.crush)
    file(GENERATE OUTPUT ${_script} CONTENT "${_batch}")

    set(_render_dir ${CT_CONTEXT_DIR}/.crush/render/${CT_RENDER_NAME})
    set(_artifact_base "${_font_base}_${_font_ext}_${CT_PIXEL_SIZE}px_font")
    set(_out_c ${_render_dir}/${_artifact_base}.c)
    set(_out_h ${_render_dir}/${_artifact_base}.h)

    #   the context's font/display records are declared alongside the rendered pair: they are
    # products of the same batch now, and anything that used to depend on the per-step stamps
    # keeps a real output to hang on
    crush_invoke(
        OUTPUT ${_out_c} ${_out_h}
               ${CT_CONTEXT_DIR}/.crush/font.json ${CT_CONTEXT_DIR}/.crush/display.json
        COMMAND console ${_script}
        WORKING_DIRECTORY ${CT_CONTEXT_DIR}
        DEPENDS ${_context_stamp} ${_font_abs} ${_script}
        COMMENT "crush: font target ${TARGET_NAME} (add '${_font_name}', add display '${CT_DISPLAY_NAME}', render ${CT_PIXEL_SIZE}px) in one batch"
    )

    # keep the custom-command outputs' build ordering independent of whichever real
    # target happens to consume the generated sources first
    add_custom_target(${TARGET_NAME}_crush_gen DEPENDS ${_out_c} ${_out_h})

    add_library(${TARGET_NAME} INTERFACE)
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_crush_gen)
    target_sources(${TARGET_NAME} INTERFACE ${_out_c})
    target_include_directories(${TARGET_NAME} INTERFACE ${_render_dir})
    # light_draw, formerly rend: the generated source includes light_draw.h, and the stale
    # `if (TARGET rend)` from before the rename could never fire -- consumers only built
    # because they all happened to link light_draw themselves
    if (TARGET light_draw)
        target_link_libraries(${TARGET_NAME} INTERFACE light_draw)
    endif()
endfunction()
