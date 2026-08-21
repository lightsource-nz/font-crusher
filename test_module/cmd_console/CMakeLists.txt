# cmd_console: test package for `crush console`, the interactive prompt and script runner.
#
#   WHAT THIS COVERS THAT THE FRAMEWORK SUITE DOES NOT. light_cli's own test_light_cli_line
# suite already pins tokenizing, parsing, dispatch and the queue in isolation, in-process. What
# only a real `crush` process can show is the console's behaviour around them: which lines a
# script echoes, whether a failure stops it, whether the PROCESS EXIT CODE reflects that (a build
# system reads nothing else), and -- for the interactive path -- that lines handed to
# light_cli_queue_line() one per scheduler tick all still run before the session ends.
#
#   the context lives under this module's own binary directory, as in cmd_font_add. These tests
# additionally take a RESOURCE_LOCK: every crush run commits its render context at shutdown, so
# all of them write to that directory and none of them may overlap under `ctest -j`.
set(CONSOLE_SCRIPTS ${TEST_RESOURCE_DIR}/console)
set(RUN_CONSOLE ${CMAKE_CURRENT_LIST_DIR}/run_console.cmake)

add_test(
        NAME cmd_console__reset_context
        COMMAND ${CMAKE_COMMAND}
                "-DDEST=${CMAKE_CURRENT_BINARY_DIR}/.crush"
                "-DSRC=${TEST_RESOURCE_DIR}/test_context_default"
                -P "${TEST_MODULE_DIR}/reset_context.cmake"
)
set_property(TEST cmd_console__reset_context PROPERTY FIXTURES_SETUP cmd_console_ctx)

#   a directory whose name contains a space, holding a copy of the test font. basic.crush names
# it through a quoted RELATIVE path, which is the end-to-end version of what the framework suite
# checks in the tokenizer alone: the quotes group the path and are gone before it reaches
# FT_New_Face(). A path with a space in it is the normal case on Windows, not an edge one
add_test(
        NAME cmd_console__stage_font
        COMMAND ${CMAKE_COMMAND} -E copy
                "${TEST_RESOURCE_DIR}/fonts/TypeLightSans.ttf"
                "${CMAKE_CURRENT_BINARY_DIR}/font dir/TypeLightSans.ttf"
)
set_property(TEST cmd_console__stage_font PROPERTY FIXTURES_SETUP cmd_console_ctx)

#   declares one console case. ARGS is PIPE-SEPARATED rather than a CMake list, and that is not
# a style choice: an unquoted CMake argument is split on whitespace as well as on ';', so
# `ARGS console;-c;font list` would arrive as four arguments and hand crush a bare `list`.
# Splitting on '|' inside the helper keeps an argument containing a space intact, which is
# exactly what `-c "font list"` is.
function(add_console_test _name)
        cmake_parse_arguments(T "" "ARGS;INPUT;EXPECT_CODE;COUNT_PATTERN;COUNT" "MATCH;NOMATCH;ORDER" ${ARGN})
        set(_args "-DCRUSH=$<TARGET_FILE:crush>" "-DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}")
        if(T_ARGS)
                list(APPEND _args "-DARGS_PIPE=${T_ARGS}")
        endif()
        if(T_INPUT)
                list(APPEND _args "-DINPUT=${T_INPUT}")
        endif()
        if(DEFINED T_EXPECT_CODE)
                list(APPEND _args "-DEXPECT_CODE=${T_EXPECT_CODE}")
        endif()
        if(T_MATCH)
                list(APPEND _args "-DMATCH=${T_MATCH}")
        endif()
        if(T_NOMATCH)
                list(APPEND _args "-DNOMATCH=${T_NOMATCH}")
        endif()
        if(T_ORDER)
                list(APPEND _args "-DORDER=${T_ORDER}")
        endif()
        if(T_COUNT_PATTERN)
                list(APPEND _args "-DCOUNT_PATTERN=${T_COUNT_PATTERN}" "-DCOUNT=${T_COUNT}")
        endif()
        add_test(NAME ${_name} COMMAND ${CMAKE_COMMAND} ${_args} -P "${RUN_CONSOLE}")
        set_property(TEST ${_name} PROPERTY FIXTURES_REQUIRED cmd_console_ctx)
        set_property(TEST ${_name} PROPERTY RESOURCE_LOCK cmd_console_ctx)
        set_property(TEST ${_name} PROPERTY ENV CRUSH_CONTEXT=)
endfunction()

#   --command: one line, nothing read at all. The form a Makefile or a CMake custom command
# uses, and the only one where a single command's success IS the process's exit status
add_console_test(cmd_console__command_option
        ARGS "console|-c|font list"
        EXPECT_CODE 0
        MATCH "target command: 'crush font list'")

#   the same, failing. `font info` with no argument is refused by its own handler, so this is a
# command that RAN and said no -- a different thing from a line that would not parse
add_console_test(cmd_console__command_option_reports_failure
        ARGS "console|-c|font info"
        EXPECT_CODE 1
        MATCH "not enough args bound to run command")

#   a line that will not parse must fail identically as far as a caller is concerned: an unknown
# option is a typo, and a build must not carry on as though it had worked
add_console_test(cmd_console__command_option_reports_a_parse_failure
        ARGS "console|-c|font list --nosuch"
        EXPECT_CODE 1
        MATCH "no option named 'nosuch'")

#   a script file: comments and blanks produce nothing, real lines are echoed in order before
# their own output, the leading program name is accepted, and a quoted path keeps its space.
#   the COUNT is what pins the "not echoed" half. Exactly three lines in that file are commands,
# so a fourth `crush> ` would mean a comment or a blank line had been treated as one
add_console_test(cmd_console__script
        ARGS "console|${CONSOLE_SCRIPTS}/basic.crush"
        EXPECT_CODE 0
        ORDER "crush> font list;crush> crush font list;crush> font add --local-file"
        MATCH "loaded font file '.*TypeLightSans\\.ttf' successfully"
        NOMATCH "crush> #"
        #   no trailing space in the pattern: a -D value loses one in transit, so writing
        # "crush> " here would silently be matched as "crush>" anyway. Nothing else in a script
        # run emits that prefix -- the prompt itself is written only on the interactive path --
        # so counting it counts echoed command lines exactly
        COUNT_PATTERN "crush>" COUNT 3)

#   a failing script stops AT the failure by default. The third line must never be echoed,
# because it must never have been read
add_console_test(cmd_console__script_stops_at_a_failure
        ARGS "console|${CONSOLE_SCRIPTS}/stops.crush"
        EXPECT_CODE 1
        MATCH "stopping at the failed command"
        NOMATCH "crush> font info tail-marker")

#   --keep-going runs the rest and still fails the process at the end, which is what a caller
# who wants every error from one run needs
add_console_test(cmd_console__script_keep_going
        ARGS "console|--keep-going|${CONSOLE_SCRIPTS}/stops.crush"
        EXPECT_CODE 1
        MATCH "crush> font info tail-marker;command\\(s\\) failed"
        NOMATCH "stopping at the failed command")

#   'exit' ends a script early and the process still SUCCEEDS: stopping deliberately is not a
# failure, unlike stopping at an error
add_console_test(cmd_console__script_exit_builtin
        ARGS "console|${CONSOLE_SCRIPTS}/exits.crush"
        EXPECT_CODE 0
        MATCH "crush> exit"
        NOMATCH "crush> font info tail-marker")

#   a script that is not there is the caller's mistake and must be reported as one, rather than
# looking like an empty script that succeeded
add_console_test(cmd_console__missing_script_fails
        ARGS "console|${CMAKE_CURRENT_BINARY_DIR}/no-such-script.crush"
        EXPECT_CODE 1
        MATCH "could not open script")

#   stdin that is not a terminal is a script by another name: `crush console < file` has to
# behave exactly as `crush console file` does, echoing and failing the same way
add_console_test(cmd_console__stdin_pipe_behaves_as_a_script
        ARGS "console"
        INPUT ${CONSOLE_SCRIPTS}/stops.crush
        EXPECT_CODE 1
        MATCH "crush> font list"
        NOMATCH "crush> font info tail-marker")

#   the help builtin, which has to be answerable before the user knows enough to ask for
# anything else. Deliberately NOT a registered command -- `crush help` means nothing to a shell
# -- so a console session is the only place it exists to be tested.
#   note the exit code: an unknown help topic is reported and does not fail the script, which is
# the deliberate half of this. Asking about a command that is not there is not a build error
add_console_test(cmd_console__help_builtin
        ARGS "console|${CONSOLE_SCRIPTS}/help.crush"
        EXPECT_CODE 0
        MATCH "usage: crush <subcommand>;usage: crush font <subcommand>;no such command 'nosuch'")

#   THE ASYNC PATH, and the reason the console was rewritten. With --interactive it stops running
# lines inline and reads ONE per tick of its own periodic task, queueing each for light_cli's
# task to dispatch on a tick of its own, so a session waiting on a human cannot starve every
# other periodic task -- on a single-core target, including the one draining log output.
#   WHAT THAT MUST NOT COST IS A LINE. All three have to run before EOF ends the session, which
# holds only because light_cli's task is scheduled ahead of the console's own and therefore
# drains the last queued line on the same tick that the console reads EOF. The count is the
# assertion: three identical failing commands produce three identical errors, and two would mean
# the last line died with the session.
add_console_test(cmd_console__interactive_runs_every_queued_line
        ARGS "console|--interactive"
        INPUT ${CONSOLE_SCRIPTS}/session.crush
        EXPECT_CODE 0
        MATCH "type 'help' for commands;crush>;console session ended"
        COUNT_PATTERN "not enough args bound to run command" COUNT 3)

#   REGRESSION TEST FOR A REGISTRATION BUG, and it lives here because this module already has
# the harness to see it -- a whole test module for one dispatch assertion would be more file than
# fact. `display list` was registered under the name "info", so it was not a command at all: the
# parser found no child called "list", bound the token as an argument to `display` (which takes
# none), and the only evidence was a warning. Both halves are asserted, since the target line
# alone would have passed while the excess-argument warning was still being printed
add_console_test(cmd_console__dispatches_display_list
        ARGS "console|-c|display list"
        EXPECT_CODE 0
        MATCH "target command: 'crush display list'"
        NOMATCH "excess arguments supplied")

#   THE BATCHED BUILD STEP, end to end: the same font add / display add / render new
# sequence crush_add_font_target() writes into its generated script, run in one session.
# Every assertion here is a regression: "json object decode failed" is the extract format
# strings refusing the integer-typed json a same-session serialize produces (disk
# round-trips hid it -- store files are parsed with JSON_DECODE_INT_AS_REAL, so separate
# processes only ever extracted reals); "excess arguments" is the parser's to_bind counter
# left uninitialised when an option token was what moved it into its binding state; and
# "rendering failed" guards the lookups actually resolving to usable objects, since
# do_cmd_render_new reports a failed job as a failed command
add_console_test(cmd_console__runs_the_font_target_pipeline
        ARGS "console|${CONSOLE_SCRIPTS}/pipeline.crush"
        EXPECT_CODE 0
        ORDER "crush> font add;crush> display add pipeline_disp;crush> render new pipeline_render"
        NOMATCH "json object decode failed;excess arguments supplied;rendering failed")

#   an interactive session is a person, not a build step. Every line in that script fails, and
# the process must still exit 0 -- a typo costing you the session would make a prompt unusable,
# and cli_task() ignores what a queued line returns for exactly that reason
add_console_test(cmd_console__interactive_survives_failures
        ARGS "console|-i"
        INPUT ${CONSOLE_SCRIPTS}/session.crush
        EXPECT_CODE 0
        MATCH "console session ended")
