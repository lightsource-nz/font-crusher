# cmd_font_add: test package for basic functionality of command 'crush font add'
#
# adds a small CC0/public-domain font (see test_resource/fonts/TypeLightSans.ttf.LICENSE.txt)
# to a context freshly reset (via ../reset_context.cmake) under this test module's own binary
# directory -- isolated from other test modules' contexts so parallel `ctest -j` runs can't
# collide with each other over a shared .crush directory.
add_test(
        NAME cmd_font_add__reset_context
        COMMAND ${CMAKE_COMMAND}
                "-DDEST=${CMAKE_CURRENT_BINARY_DIR}/.crush"
                "-DSRC=${TEST_RESOURCE_DIR}/test_context_default"
                -P "${TEST_MODULE_DIR}/reset_context.cmake"
)
set_property(TEST cmd_font_add__reset_context PROPERTY FIXTURES_SETUP cmd_font_add_ctx)

add_test(
        NAME cmd_font_add
        COMMAND $<TARGET_FILE:crush> font add --local-file "${TEST_RESOURCE_DIR}/fonts/TypeLightSans.ttf"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_font_add PROPERTY FIXTURES_REQUIRED cmd_font_add_ctx)
set_property(TEST cmd_font_add PROPERTY ENV CRUSH_CONTEXT=)
#   the exit code IS meaningful now, and this test relies on it as well as on the output: main()
# returns 1 when light_framework_run() reports LF_STATUS_ERROR, so CTest's own "non-zero means
# failure" already covers the command failing outright. (The note that used to be here said the
# opposite -- that the process always exits 0 because main() had no return statement and
# one-shot task failures were only logged. Both halves have since stopped being true, and
# light_cli's dispatch is not a one-shot task at all any more.)
#   the regex is still worth having on top of that: it distinguishes "the command ran and loaded
# the font" from "the process happened to exit 0", which is exactly what a stubbed-out handler
# returning success would look like
set_property(TEST cmd_font_add PROPERTY PASS_REGULAR_EXPRESSION "loaded font file '.*TypeLightSans\\.ttf' successfully")
set_property(TEST cmd_font_add PROPERTY FAIL_REGULAR_EXPRESSION "\\[  ERROR\\]")
