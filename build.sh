#!/bin/bash

# build: tmp build files
# output: final files only, for distribution
rm -rf build output
mkdir build output
mkdir output/static output/dynamic


if [ -z "$1" ]; then
	echo "Please copy sample.config.in to your.config, configure it, and then launch this script as ./build.sh your.config"
	exit 1
fi

source $1

if [ -z "$RELEASE" ] || [ -z "$PKG_URL_PREFIX" ] || [ -z "$STAGE2_URL_BASE" ]; then
	echo "Please make sure all of the following variables are defined in your config file:"
	echo "RELEASE, PKG_URL_PREFIX, STAGE2_URL_BASE"
	echo "(see sample.config.in for an example)"
	exit 2
fi

set -ex

CC=arm-vita-eabi-gcc
LD=arm-vita-eabi-gcc
AS=arm-vita-eabi-as
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS="-T payload/linker.x -nodefaultlibs -nostdlib -pie"
DEFINES="-DRELEASE=$RELEASE"
PREPROCESS="$CC -E -P -C -w -x c $DEFINES"
CFLAGS="-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=cortex-a9 -Os -mthumb $DEFINES"

echo "0) User payload"

# skipped

echo "1) Payload"

cp payload/loader.enc build/loader.enc
cp payload/payload.enc build/payload.enc
./payload/write_pkg_url.py build/payload.enc $PKG_URL_PREFIX

echo "2) Kernel ROP"

cp krop/krop.rop build/krop.rop

echo "3) User ROP"
echo "symbol stage2_url_base = \"$STAGE2_URL_BASE\";" > build/config.rop

./urop/make_rop_array.py build/loader.enc kx_loader build/kx_loader.rop
./urop/make_rop_array.py build/payload.enc second_payload build/second_payload.rop

$PREPROCESS urop/exploit.rop.in -o build/exploit.rop.in
erb build/exploit.rop.in > build/exploit.rop
roptool -s build/exploit.rop -t urop/webkit-360-pkg -o build/exploit.rop.bin >/dev/null

$PREPROCESS urop/loader.rop.in -o build/loader.rop.in
erb build/loader.rop.in > build/loader.rop
roptool -s build/loader.rop -t urop/webkit-360-pkg -o build/loader.rop.bin >/dev/null

echo "4) Webkit"
# Static website
$PREPROCESS webkit/exploit.js -DSTATIC=1 -o build/exploit.static.js
uglifyjs build/exploit.static.js -m "toplevel" > build/exploit.js
touch output/static/exploit.html
printf "<script>" >> output/static/exploit.html
cat build/exploit.js >> output/static/exploit.html
printf "</script>" >> output/static/exploit.html
./webkit/preprocess.py build/exploit.rop.bin output/static/payload.bin

# Dynamic website
$PREPROCESS webkit/exploit.js -DSTATIC=0 -o build/exploit.dynamic.js
uglifyjs build/exploit.dynamic.js -m "toplevel" > build/exploit.js
touch output/dynamic/exploit.html
printf "<noscript>Go to browser settings and check \"Enable JavaScript\", then reload this page.</noscript><script src='payload.js'></script><script>" >> output/dynamic/exploit.html
cat build/exploit.js >> output/dynamic/exploit.html
printf "</script>" >> output/dynamic/exploit.html
cp output/static/payload.bin output/dynamic/stage2.bin
./webkit/preprocess.py build/loader.rop.bin output/dynamic/payload.js
cp webkit/stage2.php output/dynamic/stage2.php
cp webkit/stage2.go output/dynamic/stage2.go
