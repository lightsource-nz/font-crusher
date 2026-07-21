# verifies two glyphs' byte arrays in the generated font.c differ -- guards against bugs where
# visually distinct characters end up rendering identically. this class of bug is easy to miss
# because the render job still reports success and produces non-empty output; it was originally
# caught by comparing 'a' against 'A' and noticing AVA.ttf's lowercase glyphs were near-duplicates
# of its uppercase ones (a font data issue, not a crush bug -- see TypeLightSans.ttf.LICENSE.txt).
# invoked as: cmake -DSOURCE_FILE=<path> -DGLYPH_A=0x61 -DGLYPH_B=0x41 -P check_glyphs_differ.cmake
if(NOT EXISTS "${SOURCE_FILE}")
        message(FATAL_ERROR "expected font source file does not exist: ${SOURCE_FILE}")
endif()
file(READ "${SOURCE_FILE}" SOURCE_CONTENT)

foreach(_code "${GLYPH_A}" "${GLYPH_B}")
        if(NOT SOURCE_CONTENT MATCHES "glyph_${_code}\\[\\] = \\{([^}]*)\\}")
                message(FATAL_ERROR "'${SOURCE_FILE}' has no glyph_${_code}[] array -- expected character was not rendered")
        endif()
endforeach()

string(REGEX MATCH "glyph_${GLYPH_A}\\[\\] = \\{([^}]*)\\}" _ "${SOURCE_CONTENT}")
set(BYTES_A "${CMAKE_MATCH_1}")
string(REGEX MATCH "glyph_${GLYPH_B}\\[\\] = \\{([^}]*)\\}" _ "${SOURCE_CONTENT}")
set(BYTES_B "${CMAKE_MATCH_1}")

if(BYTES_A STREQUAL BYTES_B)
        message(FATAL_ERROR "glyph_${GLYPH_A}[] and glyph_${GLYPH_B}[] in '${SOURCE_FILE}' have identical byte content -- expected visually distinct characters to render differently")
endif()
message(STATUS "glyph_${GLYPH_A}[] and glyph_${GLYPH_B}[] differ, as expected")
