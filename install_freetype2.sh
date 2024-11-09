#!/bin/sh -e

cd `dirname $0`/module/mod_freetype/lib/freetype
ftdir=`pwd`
tmpdir=/tmp/freetype-build
rm -rf $tmpdir
mkdir -p $tmpdir

(set -x; cmake -H$ftdir \
               -B$tmpdir/build \
               -DCMAKE_BUILD_TYPE=Release \
               $build_opts)
(set -x; cmake --build $tmpdir/build \
               --config Release \
               --target install)
