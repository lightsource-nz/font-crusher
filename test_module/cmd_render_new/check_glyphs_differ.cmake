# verifies two glyph dump files have different content -- guards against bugs where visually
# distinct characters end up rendering identically. this class of bug is easy to miss because
# the render job still reports success and produces non-empty files; it was originally caught
# by comparing 'a' against 'A' and noticing AVA.ttf's lowercase glyphs were near-duplicates of
# its uppercase ones (a font data issue, not a crush bug -- see TypeLightSans.ttf.LICENSE.txt).
# invoked as: cmake -DFILE_A=<path> -DFILE_B=<path> -P check_glyphs_differ.cmake
if(NOT EXISTS "${FILE_A}")
        message(FATAL_ERROR "expected glyph output file does not exist: ${FILE_A}")
endif()
if(NOT EXISTS "${FILE_B}")
        message(FATAL_ERROR "expected glyph output file does not exist: ${FILE_B}")
endif()
file(READ "${FILE_A}" CONTENT_A)
file(READ "${FILE_B}" CONTENT_B)
if(CONTENT_A STREQUAL CONTENT_B)
        message(FATAL_ERROR "glyph files '${FILE_A}' and '${FILE_B}' have identical content -- expected visually distinct characters to render differently")
endif()
message(STATUS "glyph files '${FILE_A}' and '${FILE_B}' differ, as expected")
