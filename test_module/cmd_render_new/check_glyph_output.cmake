# verifies that 'crush render new' actually wrote a real font.h/font.c pair to disk, rather than
# empty or missing files -- the failure mode this test guards against, caused historically by a
# chain of bugs (degenerate PPI, unscaled FT_Set_Char_Size, truncated trailing bitmap byte) that
# all produced "successful" renders with no real glyph content.
# invoked as: cmake -DHEADER_FILE=<path> -DSOURCE_FILE=<path> -P check_glyph_output.cmake
foreach(_f "${HEADER_FILE}" "${SOURCE_FILE}")
        if(NOT EXISTS "${_f}")
                message(FATAL_ERROR "expected font export file does not exist: ${_f}")
        endif()
endforeach()

file(READ "${HEADER_FILE}" HEADER_CONTENT)
if(NOT HEADER_CONTENT MATCHES "extern const rend_font_t [A-Za-z0-9_]+_font;")
        message(FATAL_ERROR "'${HEADER_FILE}' doesn't look like a generated font header (no rend_font_t instance declaration found)")
endif()

file(READ "${SOURCE_FILE}" SOURCE_CONTENT)
if(NOT SOURCE_CONTENT MATCHES "glyph_table\\[")
        message(FATAL_ERROR "'${SOURCE_FILE}' doesn't look like a generated font source (no glyph_table[] found)")
endif()
# a real glyph is many packed bytes; anything this small indicates a degenerate (near-zero-size)
# render rather than real character data
file(SIZE "${SOURCE_FILE}" SOURCE_FILE_SIZE)
if(SOURCE_FILE_SIZE LESS 200)
        message(FATAL_ERROR "'${SOURCE_FILE}' is suspiciously small (${SOURCE_FILE_SIZE} bytes) -- rendering likely produced degenerate/empty glyph data")
endif()
message(STATUS "'${HEADER_FILE}' and '${SOURCE_FILE}' look OK (source is ${SOURCE_FILE_SIZE} bytes)")
