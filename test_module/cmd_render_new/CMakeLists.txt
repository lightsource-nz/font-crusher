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

# 19 is the pixel size (y_ppem) that FreeType itself resolves 14pt @ the display's default 96 PPI
# to -- i.e. this test now exercises the pixel_size argument with the exact value the pipeline
# would already be using for this font/size/display combination if pixel_size were left unset
add_test(
        NAME cmd_render_new
        COMMAND $<TARGET_FILE:crush> render new test_render 14 19 --font TypeLightSans.ttf --display test-display
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
# history this guards against), so check the exported font.h/font.c directly rather than
# trusting the log alone
add_test(
        NAME cmd_render_new__check_output_file
        COMMAND ${CMAKE_COMMAND}
                "-DHEADER_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/TypeLightSans_ttf_19px_font.h"
                "-DSOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/TypeLightSans_ttf_19px_font.c"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyph_output.cmake"
)
set_property(TEST cmd_render_new__check_output_file PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__check_output_file PROPERTY DEPENDS cmd_render_new)

# guards against a font (or pipeline bug) rendering visually distinct characters identically --
# see check_glyphs_differ.cmake for the history behind this check. 0x61 = 'a', 0x41 = 'A'
add_test(
        NAME cmd_render_new__check_case_distinct
        COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/TypeLightSans_ttf_19px_font.c"
                "-DGLYPH_A=0x61"
                "-DGLYPH_B=0x41"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyphs_differ.cmake"
)
set_property(TEST cmd_render_new__check_case_distinct PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__check_case_distinct PROPERTY DEPENDS cmd_render_new)

# a display with a physically square panel (25.4x25.4mm, i.e. 1x1 inch) but a non-square pixel
# resolution (192x96) has ppi_h exactly double ppi_v -- each pixel is physically twice as tall as
# it is wide. this display exists purely to exercise PPI-aspect compensation in the explicit
# pixel_size render path (see check_nonsquare_ppi.cmake below)
add_test(
        NAME cmd_render_new__add_display_skewed
        COMMAND $<TARGET_FILE:crush> display add test-display-skewed 192 96 --dimension 25.4x25.4
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__add_display_skewed PROPERTY FIXTURES_SETUP cmd_render_new_ctx)
set_property(TEST cmd_render_new__add_display_skewed PROPERTY DEPENDS cmd_render_new__reset_context)
set_property(TEST cmd_render_new__add_display_skewed PROPERTY ENV CRUSH_CONTEXT=)

# same font and same explicit pixel size (19) as the square-PPI 'cmd_render_new' render above,
# but against the skewed display -- only the display's aspect ratio differs between the two, so
# any difference in the resulting glyph cell shape is attributable to PPI-aspect compensation
add_test(
        NAME cmd_render_new__skewed
        COMMAND $<TARGET_FILE:crush> render new test_render_skewed 14 19 --font TypeLightSans.ttf --display test-display-skewed
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__skewed PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__skewed PROPERTY ENV CRUSH_CONTEXT=)
set_property(TEST cmd_render_new__skewed PROPERTY PASS_REGULAR_EXPRESSION "rendering complete for job")
set_property(TEST cmd_render_new__skewed PROPERTY FAIL_REGULAR_EXPRESSION "\\[  ERROR\\]")

add_test(
        NAME cmd_render_new__skewed_check_output_file
        COMMAND ${CMAKE_COMMAND}
                "-DHEADER_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_skewed/TypeLightSans_ttf_19px_font.h"
                "-DSOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_skewed/TypeLightSans_ttf_19px_font.c"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyph_output.cmake"
)
set_property(TEST cmd_render_new__skewed_check_output_file PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__skewed_check_output_file PROPERTY DEPENDS cmd_render_new__skewed)

# the actual regression check: compares the glyph cell exported for the skewed (non-square PPI)
# display against the square-PPI baseline rendered earlier by 'cmd_render_new' -- see
# check_nonsquare_ppi.cmake for what's asserted and why
add_test(
        NAME cmd_render_new__check_nonsquare_ppi
        COMMAND ${CMAKE_COMMAND}
                "-DBASELINE_SOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render/TypeLightSans_ttf_19px_font.c"
                "-DSKEWED_SOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_skewed/TypeLightSans_ttf_19px_font.c"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_nonsquare_ppi.cmake"
)
set_property(TEST cmd_render_new__check_nonsquare_ppi PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__check_nonsquare_ppi PROPERTY DEPENDS cmd_render_new cmd_render_new__skewed)

# real-device case: the Pico-OLED-1.3 ("po13") module used by the crossfire project -- 64x128
# pixels across a 17.2x32.3mm glass (see PO13_WIDTH/PO13_HEIGHT in crossfire's
# light_display_po13.h). its ppi_h (~94.5) and ppi_v (~100.7) are close but not equal, since the
# panel's real-world aspect ratio doesn't perfectly match its 1:2 pixel resolution ratio -- a
# smaller, less clean-cut skew than cmd_render_new__add_display_skewed's synthetic 2:1 case, and
# a good sanity check that the PPI-aspect compensation behaves on realistic measurements, not
# just a contrived ratio. pixel size 16 was chosen as a plausible on-device text size; it also
# happens to be the exact y_ppem FreeType resolves this font/size/display combination to, which
# is what fixes the output filename below
add_test(
        NAME cmd_render_new__add_display_po13
        COMMAND $<TARGET_FILE:crush> display add po13 64 128 --dimension 17.2x32.3
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__add_display_po13 PROPERTY FIXTURES_SETUP cmd_render_new_ctx)
set_property(TEST cmd_render_new__add_display_po13 PROPERTY DEPENDS cmd_render_new__reset_context)
set_property(TEST cmd_render_new__add_display_po13 PROPERTY ENV CRUSH_CONTEXT=)

add_test(
        NAME cmd_render_new__po13
        COMMAND $<TARGET_FILE:crush> render new test_render_po13 14 16 --font TypeLightSans.ttf --display po13
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_property(TEST cmd_render_new__po13 PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__po13 PROPERTY ENV CRUSH_CONTEXT=)
set_property(TEST cmd_render_new__po13 PROPERTY PASS_REGULAR_EXPRESSION "rendering complete for job")
set_property(TEST cmd_render_new__po13 PROPERTY FAIL_REGULAR_EXPRESSION "\\[  ERROR\\]")

add_test(
        NAME cmd_render_new__po13_check_output_file
        COMMAND ${CMAKE_COMMAND}
                "-DHEADER_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_po13/TypeLightSans_ttf_16px_font.h"
                "-DSOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_po13/TypeLightSans_ttf_16px_font.c"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyph_output.cmake"
)
set_property(TEST cmd_render_new__po13_check_output_file PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__po13_check_output_file PROPERTY DEPENDS cmd_render_new__po13)

add_test(
        NAME cmd_render_new__po13_check_case_distinct
        COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_FILE=${CMAKE_CURRENT_BINARY_DIR}/.crush/render/test_render_po13/TypeLightSans_ttf_16px_font.c"
                "-DGLYPH_A=0x61"
                "-DGLYPH_B=0x41"
                -P "${CMAKE_CURRENT_LIST_DIR}/check_glyphs_differ.cmake"
)
set_property(TEST cmd_render_new__po13_check_case_distinct PROPERTY FIXTURES_REQUIRED cmd_render_new_ctx)
set_property(TEST cmd_render_new__po13_check_case_distinct PROPERTY DEPENDS cmd_render_new__po13)
