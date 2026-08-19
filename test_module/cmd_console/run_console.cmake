#   runs `crush console` and asserts on what it produced. Exists because CTest can do neither of
# the two things these tests need: it cannot redirect a file into a test's STDIN, and its
# PASS_REGULAR_EXPRESSION is a single boolean match -- no ordering, no absence, no counting.
#
#   invoked as:
#     cmake -DCRUSH=<exe> -DWORKDIR=<dir> [-DINPUT=<file>] [-DARGS=<a;b;c>]
#           [-DEXPECT_CODE=<n>] [-DMATCH=<re;re>] [-DNOMATCH=<re;re>]
#           [-DORDER=<literal;literal>] [-DCOUNT_PATTERN=<re> -DCOUNT=<n>]
#           -P run_console.cmake
#
#   stdout and stderr are merged, deliberately: crush's log stream writes results and errors to
# both, and every assertion here is about what a person watching the console would have seen.
if(NOT DEFINED EXPECT_CODE)
        set(EXPECT_CODE 0)
endif()

set(_input_arg "")
if(DEFINED INPUT)
        if(NOT EXISTS "${INPUT}")
                message(FATAL_ERROR "input file does not exist: ${INPUT}")
        endif()
        set(_input_arg INPUT_FILE "${INPUT}")
endif()

#   ARGS_PIPE is pipe-separated because a CMake list cannot carry an argument containing a
# space without being re-split somewhere along the way -- and `-c "font list"` is precisely
# that. Splitting on '|' here is the last step before execute_process(), which takes an argument
# vector rather than a command line, so nothing re-parses it afterwards
set(_argv "")
if(DEFINED ARGS_PIPE)
        string(REPLACE "|" ";" _argv "${ARGS_PIPE}")
endif()

execute_process(
        COMMAND "${CRUSH}" ${_argv}
        WORKING_DIRECTORY "${WORKDIR}"
        ${_input_arg}
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
        RESULT_VARIABLE _code)
set(_all "${_out}${_err}")

#   the transcript goes to the log on failure ONLY. On success it is thousands of lines of
# framework boot logging, and burying a green run in it helps nobody
set(_report "exit code ${_code}\n--- output ---\n${_all}\n--- end ---")

if(NOT _code STREQUAL EXPECT_CODE)
        message(FATAL_ERROR "expected exit code ${EXPECT_CODE}, got ${_code}\n${_report}")
endif()

foreach(_re IN LISTS MATCH)
        if(NOT _all MATCHES "${_re}")
                message(FATAL_ERROR "output does not match '${_re}'\n${_report}")
        endif()
endforeach()

foreach(_re IN LISTS NOMATCH)
        if(_all MATCHES "${_re}")
                message(FATAL_ERROR "output matches '${_re}' and must not\n${_report}")
        endif()
endforeach()

#   ORDER is literal substrings, not regexes: it answers "did these things happen in this
# sequence", which is how a script's line-by-line progress is checked. string(FIND) with an
# advancing offset is what makes it an ordering check rather than a set of independent matches
if(DEFINED ORDER)
        set(_offset 0)
        set(_previous "")
        foreach(_needle IN LISTS ORDER)
                string(FIND "${_all}" "${_needle}" _at)
                if(_at EQUAL -1)
                        message(FATAL_ERROR "output never contains '${_needle}'\n${_report}")
                endif()
                if(_at LESS _offset)
                        message(FATAL_ERROR
                                "'${_needle}' appears before '${_previous}', which is the wrong order\n${_report}")
                endif()
                set(_offset ${_at})
                set(_previous "${_needle}")
        endforeach()
endif()

#   how MANY times something happened, which a boolean match cannot express -- it is the only
# way to say "all three queued lines ran" when the three produce identical output
if(DEFINED COUNT_PATTERN)
        string(REGEX MATCHALL "${COUNT_PATTERN}" _hits "${_all}")
        list(LENGTH _hits _hit_count)
        if(NOT _hit_count EQUAL COUNT)
                message(FATAL_ERROR
                        "expected ${COUNT} match(es) of '${COUNT_PATTERN}', found ${_hit_count}\n${_report}")
        endif()
endif()
