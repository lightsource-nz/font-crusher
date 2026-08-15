#!/bin/bash
#   reliable reproduction of the cmd_render_new__skewed heap corruption.
#
#   the trigger is the WORKING DIRECTORY PATH LENGTH, not the display and not the geometry:
# a longer cwd lengthens the stored paths, which changes allocation sizes, which shifts heap
# layout so the corruption lands somewhere fatal. Measured: 49-char cwd passes, 76 and 104 crash.
#   note also that every `render new` adds an object to the context, so re-running in the same
# directory changes layout between runs -- each trial must start from a fresh context or the
# comparison is meaningless.
set -u
root=/c/Users/aful018/projects/c/font-crusher
d="$root/build/reprodir_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
crush="$root/build/bin/crush.exe"

rm -rf "$d"
mkdir -p "$d"
# the same reset the ctest fixture uses, rather than a hand-rolled copy
cmake -DDEST="$d/.crush" -DSRC="$root/test_resource/test_context_default" \
        -P "$root/test_module/reset_context.cmake" >/dev/null 2>&1
cd "$d" || exit 1
unset CRUSH_CONTEXT

"$crush" font add --local-file "$root/test_resource/fonts/TypeLightSans.ttf" >/dev/null 2>&1
"$crush" display add test-display 64 128 >/dev/null 2>&1
"$crush" display add test-display-skewed 192 96 --dimension 25.4x25.4 >/dev/null 2>&1
"$crush" display add po13 64 128 --dimension 17.2x32.3 >/dev/null 2>&1
"$crush" render new test_render 14 19 --font TypeLightSans.ttf --display test-display >/dev/null 2>&1

"$crush" render new test_render_skewed 14 19 --font TypeLightSans.ttf --display test-display-skewed >"$d/out.txt" 2>&1
rc=$?
grep -A 6 "PROBE" "$d/out.txt" | head -14
echo "skewed exit=$rc"
