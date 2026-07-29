# verifies that rendering the same font at the same explicit pixel size against a display with
# non-square PPI (ppi_h != ppi_v) produces a proportionally wider glyph cell than the same render
# against a square-PPI display, while the cell height stays the same. guards against
# FT_Set_Pixel_Sizes(face, 0, pixel_size)'s width=0 shorthand, which always means "same as
# height" -- silently forcing square pixels regardless of the target display's aspect ratio (see
# backend.c, worker__render_job_process()).
# invoked as: cmake -DBASELINE_SOURCE_FILE=<path> -DSKEWED_SOURCE_FILE=<path> -P check_nonsquare_ppi.cmake
foreach(_f "${BASELINE_SOURCE_FILE}" "${SKEWED_SOURCE_FILE}")
        if(NOT EXISTS "${_f}")
                message(FATAL_ERROR "expected font export file does not exist: ${_f}")
        endif()
endforeach()

file(READ "${BASELINE_SOURCE_FILE}" BASELINE_CONTENT)
file(READ "${SKEWED_SOURCE_FILE}" SKEWED_CONTENT)

if(NOT BASELINE_CONTENT MATCHES "\\.char_width = ([0-9]+),")
        message(FATAL_ERROR "'${BASELINE_SOURCE_FILE}' has no '.char_width = N,' field")
endif()
set(BASELINE_WIDTH "${CMAKE_MATCH_1}")
if(NOT BASELINE_CONTENT MATCHES "\\.char_height = ([0-9]+),")
        message(FATAL_ERROR "'${BASELINE_SOURCE_FILE}' has no '.char_height = N,' field")
endif()
set(BASELINE_HEIGHT "${CMAKE_MATCH_1}")

if(NOT SKEWED_CONTENT MATCHES "\\.char_width = ([0-9]+),")
        message(FATAL_ERROR "'${SKEWED_SOURCE_FILE}' has no '.char_width = N,' field")
endif()
set(SKEWED_WIDTH "${CMAKE_MATCH_1}")
if(NOT SKEWED_CONTENT MATCHES "\\.char_height = ([0-9]+),")
        message(FATAL_ERROR "'${SKEWED_SOURCE_FILE}' has no '.char_height = N,' field")
endif()
set(SKEWED_HEIGHT "${CMAKE_MATCH_1}")

# the skewed display's ppi_h is exactly double its ppi_v (see CMakeLists.txt), so the resolved
# horizontal pixel size -- and therefore char_width, which scales with it -- should come out
# roughly double the square-PPI baseline's, while char_height (driven only by the vertical pixel
# size, which is identical between the two renders) should stay the same. a wide tolerance is
# used on the width comparison (1.3x rather than the expected ~2x) since font hinting can shift
# the exact scaling; the point of this check is to catch the width=0 "always square" regression,
# not to pin down FreeType's exact rounding behaviour.
math(EXPR SKEWED_WIDTH_X10 "${SKEWED_WIDTH} * 10")
math(EXPR BASELINE_WIDTH_X13 "${BASELINE_WIDTH} * 13")
if(SKEWED_WIDTH_X10 LESS BASELINE_WIDTH_X13)
        message(FATAL_ERROR "skewed-display char_width (${SKEWED_WIDTH}) is not meaningfully wider than the square-PPI baseline's (${BASELINE_WIDTH}) -- expected roughly double, since the skewed display's ppi_h is 2x its ppi_v")
endif()
if(NOT SKEWED_HEIGHT STREQUAL BASELINE_HEIGHT)
        message(FATAL_ERROR "skewed-display char_height (${SKEWED_HEIGHT}) differs from the square-PPI baseline's (${BASELINE_HEIGHT}) -- expected them to match, since both renders requested the same vertical pixel size")
endif()
message(STATUS "non-square PPI compensation OK: baseline char_width=${BASELINE_WIDTH}, skewed char_width=${SKEWED_WIDTH}, both char_height=${BASELINE_HEIGHT}")
