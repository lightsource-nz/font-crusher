# cmd_render_new: test package for 'crush render new', exercising a full, real render job
# (a small CC0/public-domain font -- see test_resource/fonts/TypeLightSans.ttf.LICENSE.txt --
# rendered against a freshly-added display) rather than just checking that the command runs.
#
# the previous version of this test invoked 'crush render new test_render' with only one of
# the two required positional args and no font/display, which do_cmd_render_new() does not
# validate before use (atoi()'ing a missing arg, etc.) -- and since the process always exits 0
# regardless of outcome, the test could never actually fail. this version sets up a real font
# and display first (each build's own isolated context, matching the ../cmd_font_add pattern)
# and asserts on the render job's own success/failure log output.
add_test(
        NAME cmd_render_new__reset_context
        COMMAND ${CMAKE_COMMAND}
                "-DDEST=${CMAKE_CURRENT_BINARY_DIR}/.crush"
                "-DSRC=${TEST_RESOURCE_DIR}/test_context_default"
                -P "${TEST_MODULE_DIR}/reset_context.cmake"
)
set_property(TEST cmd_render_new__reset_context PROPERTY FIXTURES_SETUP cmd_render_new_ctx)

add_test(
        NAME cmd_render_new__add_font
        COMMAND $<TARGET_FILE:crush> font add --local-file "${TEST_RESOURCE_DIR}/fonts/TypeLightSans.ttf"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__add_font PROPERTY FIXTURES_SETUP cmd_render_new_ctx)
set_property(TEST cmd_render_new__add_font PROPERTY DEPENDS cmd_render_new__reset_context)
set_property(TEST cmd_render_new__add_font PROPERTY ENV CRUSH_CONTEXT=)
set_property(TEST cmd_render_new__add_font PROPERTY PASS_REGULAR_EXPRESSION "loaded font file '.*TypeLightSans\\.ttf' successfully")

add_test(
        NAME cmd_render_new__add_display
        COMMAND $<TARGET_FILE:crush> display add test-display 64 128
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__add_display PROPERTY FIXTURES_SETUP cmd_render_new_ctx)
set_property(TEST cmd_render_new__add_display PROPERTY DEPENDS cmd_render_new__reset_context)
set_property(TEST cmd_render_new__add_display PROPERTY ENV CRUSH_CONTEXT=)

add_test(
        NAME cmd_render_new
        COMMAND $<TARGET_FILE:crush> render new test_render 14 --font TypeLightSans.ttf --display test-display
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new PROPERTY ENV CRUSH_CONTEXT=)
# as with cmd_font_add, exit code is always 0 regardless of outcome, so check the render
# job's own log output instead
set_property(TEST cmd_render_new PROPERTY PASS_REGULAR_EXPRESSION "rendering complete for job")
set_property(TEST cmd_render_new PROPERTY FAIL_REGULAR_EXPRESSION "\\[  ERROR\\]")

# the render job reporting "complete" only means FreeType didn't error -- it doesn't guarantee
# the glyph data actually written to disk is real (see check_glyph_output.cmake for the bug
# history this guards against), so check the file directly rather than trusting the log alone
add_test(
        NAME cmd_render_new__check_output_file
        COMMAND ${CMAKE_COMMAND}
                "-DGLYPH_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/A_upper.txt"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyph_output.cmake"
)
set_property(TEST cmd_render_new__check_output_file PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__check_output_file PROPERTY DEPENDS cmd_render_new)

# guards against a font (or pipeline bug) rendering visually distinct characters identically --
# see check_glyphs_differ.cmake for the history behind this check
add_test(
        NAME cmd_render_new__check_case_distinct
        COMMAND ${CMAKE_COMMAND}
                "-DFILE_A=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/a.txt"
                "-DFILE_B=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/A_upper.txt"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyphs_differ.cmake"
)
set_property(TEST cmd_render_new__check_case_distinct PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__check_case_distinct PROPERTY DEPENDS cmd_render_new)
