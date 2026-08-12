# crush_run.cmake
#
# Script-mode wrapper (`cmake -P`) that runs the crush CLI as a build step without letting its
# own logging reach the build console.
#
# crush is a light_framework application, and a build of it logs at whatever level its
# LIGHT_RUN_MODE selects -- for a DEBUG build that is every light_debug() and light_info() call
# on the path, which for a font render is hundreds of lines per invocation. Dumped straight into
# a build, that buries the compiler output it is interleaved with, and says nothing a build log
# reader wants: `add_custom_command`'s COMMENT already names what is being rendered, the way a
# compile line names its source file.
#
# So on success this prints nothing at all and the COMMENT is the whole story. On FAILURE
# everything captured is replayed before the fatal error, because a build step that fails
# silently is worse than one that is noisy -- the output is only uninteresting while it works.
#
# Invoked as:
#   cmake -DCRUSH_EXE=<exe> -DCRUSH_WORKDIR=<dir> -DCRUSH_ARGS=<a;b;c> [-DCRUSH_VERBOSE=ON]
#         -P crush_run.cmake

if(NOT DEFINED CRUSH_EXE)
        message(FATAL_ERROR "crush_run.cmake requires -DCRUSH_EXE=<path to crush>")
endif()
if(NOT DEFINED CRUSH_WORKDIR)
        message(FATAL_ERROR "crush_run.cmake requires -DCRUSH_WORKDIR=<working directory>")
endif()

string(REPLACE ";" " " _crush_cmd_str "crush ${CRUSH_ARGS}")

if(CRUSH_VERBOSE)
        # no capture: crush's output goes straight to the console as it is produced. this is the
        # mode to use when crush HANGS rather than fails -- a captured stream is only replayed on
        # exit, so a step that never exits shows nothing at all, and the last line it managed to
        # print is exactly the clue worth having
        message(STATUS "${_crush_cmd_str}")
        execute_process(
                COMMAND "${CRUSH_EXE}" ${CRUSH_ARGS}
                WORKING_DIRECTORY "${CRUSH_WORKDIR}"
                RESULT_VARIABLE _crush_result
        )
else()
        execute_process(
                COMMAND "${CRUSH_EXE}" ${CRUSH_ARGS}
                WORKING_DIRECTORY "${CRUSH_WORKDIR}"
                RESULT_VARIABLE _crush_result
                OUTPUT_VARIABLE _crush_stdout
                ERROR_VARIABLE _crush_stderr
        )
endif()

if(NOT _crush_result EQUAL 0)
        message("")
        message("--- ${_crush_cmd_str}")
        message("--- working directory: ${CRUSH_WORKDIR}")
        if(_crush_stdout)
                message("${_crush_stdout}")
        endif()
        if(_crush_stderr)
                message("${_crush_stderr}")
        endif()
        # a non-numeric result is execute_process reporting that it could not launch the process
        # at all (it yields a message rather than an exit code there), which is a different fault
        # from crush running and returning non-zero -- worth not disguising as an exit status
        message(FATAL_ERROR "${_crush_cmd_str} failed: ${_crush_result}")
endif()
