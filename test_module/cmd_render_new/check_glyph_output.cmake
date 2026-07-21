# verifies that 'crush render new' actually wrote non-trivial glyph bitmap data to disk, rather
# than just creating an empty (or near-empty) file -- the failure mode this test guards against,
# caused historically by a chain of bugs (degenerate PPI, unscaled FT_Set_Char_Size, truncated
# trailing bitmap byte) that all produced "successful" renders with no real glyph content.
# invoked as: cmake -DGLYPH_FILE=<path> -P check_glyph_output.cmake
if(NOT EXISTS "${GLYPH_FILE}")
        message(FATAL_ERROR "expected glyph output file does not exist: ${GLYPH_FILE}")
endif()
file(SIZE "${GLYPH_FILE}" GLYPH_FILE_SIZE)
# a real 14pt glyph bitmap is many rows of ASCII art; anything this small indicates a
# degenerate (near-zero-size) render rather than real character data
if(GLYPH_FILE_SIZE LESS 20)
        message(FATAL_ERROR "glyph output file '${GLYPH_FILE}' is suspiciously small (${GLYPH_FILE_SIZE} bytes) -- rendering likely produced degenerate/empty glyph data")
endif()
message(STATUS "glyph output file '${GLYPH_FILE}' looks OK (${GLYPH_FILE_SIZE} bytes)")
