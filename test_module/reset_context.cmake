# resets a .crush test context to a clean copy of test_context_default, independent of
# whether the 'crush' target happens to relink (its own PRE_LINK/POST_BUILD reset hook only
# fires on an actual rebuild, so it can't be relied on for repeatable ctest runs).
# shared by every test module that needs a known-clean context; invoked as:
#   cmake -DDEST=<path-to-.crush> -DSRC=<path-to-test_context_default> -P reset_context.cmake
file(REMOVE_RECURSE "${DEST}")
file(MAKE_DIRECTORY "${DEST}")
file(COPY "${SRC}/" DESTINATION "${DEST}")
