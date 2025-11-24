#!/bin/sh
set -ex
mkdir -p $PWD/install/lib/pkgconfig
cat > $PWD/install/lib/pkgconfig/libcjson.pc << EOF
Name: libcjson
Description: cJSON
Version: 1.7.19
Cflags: -I$PWD/install/include
Libs: $PWD/libcjson.a
EOF
