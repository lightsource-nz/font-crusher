#!/bin/bash
#   runs the cmd_render_new__skewed reproduction against the clang/ASan build in WSL.
#   the deep directory name is deliberate: the trigger is working-directory PATH LENGTH, which
# changes stored path lengths and therefore heap layout. See repro.sh for the Windows version.
#   derived from this script's own location, never hardcoded. Under WSL that resolves to the
# /mnt/... view automatically when the script is invoked through it, which is the only way this
# one is ever run. FONT_CRUSHER_PATH overrides
root="${FONT_CRUSHER_PATH:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
d="$HOME/asan-repro-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
C="$HOME/fc-asan/bin/crush"

rm -rf "$d"
mkdir -p "$d" || exit 1
cp -r "$root/test_resource/test_context_default" "$d/.crush" || exit 1
cd "$d" || exit 1
unset CRUSH_CONTEXT

#   leak detection off: it is on by default with ASan and makes every process exit non-zero,
# which hides whether the setup commands actually worked. The hunt here is for a heap
# corruption, not for leaks -- turn it back on deliberately when that is the question
export ASAN_OPTIONS=detect_leaks=0

"$C" font add --local-file "$root/test_resource/fonts/TypeLightSans.ttf" 2>&1 | grep -E "ERROR|AddressSanitizer" | head -5
echo "  font add exit=$?"
"$C" display add test-display 64 128 >/dev/null 2>&1
"$C" display add test-display-skewed 192 96 --dimension 25.4x25.4 >/dev/null 2>&1
"$C" display add po13 64 128 --dimension 17.2x32.3 >/dev/null 2>&1
"$C" render new test_render 14 19 --font TypeLightSans.ttf --display test-display >/dev/null 2>&1
echo "=== baseline render done (exit $?); now the skewed render ==="
"$C" render new test_render_skewed 14 19 --font TypeLightSans.ttf --display test-display-skewed \
        > "$d/out.txt" 2>&1
echo "skewed exit=$?"
grep -vE "^\[  DEBUG\]|^\[   INFO\]" "$d/out.txt" | head -40
