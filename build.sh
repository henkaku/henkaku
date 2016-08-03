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

if [ -z "$RELEASE" ] || [ -z "$PKG_URL_PREFIX" ] || [ -z "$STAGE2_URL_BASE" ] || [ -z "$VERSION" ]; then
	echo "Please make sure all of the following variables are defined in your config file:"
	echo "RELEASE, PKG_URL_PREFIX, STAGE2_URL_BASE, VERSION"
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

# generate version stuffs
BUILD_VERSION=$(git describe --dirty --always --tags)
BUILD_DATE=$(date)
BUILD_HOST=$(hostname)
echo "#define BUILD_VERSION \"$BUILD_VERSION\"" >> build/version.c
echo "#define BUILD_DATE \"$BUILD_DATE\"" >> build/version.c
echo "#define BUILD_HOST \"$BUILD_HOST\"" >> build/version.c
echo "#define VERSION $VERSION" >> build/version.c

# user payload is injected into web browser process
$CC -c -o build/user.o payload/user/user.c $CFLAGS
$CC -c -o build/compress.o payload/user/compress.c $CFLAGS
$LD -o build/user.elf build/user.o build/compress.o -lgcc $LDFLAGS
$OBJCOPY -O binary build/user.elf build/user.bin
xxd -i build/user.bin > build/user.h

echo "1) Payload"

# loader decrypts and lods a larger payload
$CC -c -o build/loader.o payload/loader.c $CFLAGS
$AS -o build/loader_start.o payload/loader_start.S
$LD -o build/loader.elf build/loader.o build/loader_start.o $LDFLAGS
$OBJCOPY -O binary build/loader.elf build/loader.bin

$CC -c -o build/payload.o payload/payload.c $CFLAGS
$LD -o build/payload.elf build/payload.o $LDFLAGS
$OBJCOPY -O binary build/payload.elf build/payload.bin

cat payload/pad.bin build/loader.bin > build/loader.full
# loader must be <=0x100 bytes
SIZE=$(du -sb build/loader.full | awk '{ print $1 }')
if ((SIZE>0x100)); then
	echo "loader size is $SIZE should be less or equal 0x100 bytes"
	exit -1
fi
echo "loader size is $SIZE"
truncate -s 256 build/loader.full
openssl enc -aes-256-ecb -in build/loader.full -nopad -out build/loader.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130
openssl enc -aes-128-ecb -in build/payload.bin -nopad -out build/payload.enc -K 99E4059798A0B434F9C8CF51F8A5D253

./payload/block_check.py build/loader.enc
./payload/block_check.py build/payload.enc

./payload/write_pkg_url.py build/payload.enc $PKG_URL_PREFIX

echo "2) Kernel ROP"
if [ -d "./krop" ]; then
  ./krop/build_rop.py krop/rop.S build/
else
  echo "using prebuilt krop"
  cp ./krop.rop build/krop.rop
fi

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
printf "<script src='payload.js'></script><script>" >> output/dynamic/exploit.html
cat build/exploit.js >> output/dynamic/exploit.html
printf "</script>" >> output/dynamic/exploit.html
cp output/static/payload.bin output/dynamic/stage2.bin
./webkit/preprocess.py build/loader.rop.bin output/dynamic/payload.js
cp webkit/stage2.php output/dynamic/stage2.php
cp webkit/stage2.go output/dynamic/stage2.go
