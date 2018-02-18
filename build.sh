#!/bin/bash

# build: tmp build files
# output: final files only, for distribution
rm -rf build output
mkdir build output
mkdir output/web output/offline


if [ -z "$1" ]; then
	echo "Please copy sample.config.in to your.config, configure it, and then launch this script as ./build.sh your.config"
	exit 1
fi

source $1

if [ -z "$RELEASE" ] || [ -z "$PKG_URL_PREFIX" ] || [ -z "$HENKAKU_BIN_URL" ] || [ -z "$VITASHELL_CRC32" ] || [ -z "$TAIHEN_CRC32" ] || [ -z "$HENKAKU_RELEASE" ]; then
	echo "Please make sure all of the following variables are defined in your config file:"
	echo "RELEASE, PKG_URL_PREFIX, HENKAKU_BIN_URL, VITASHELL_CRC32, TAIHEN_CRC32, HENKAKU_RELEASE"
	echo "(see sample.config.in for an example)"
	exit 2
fi

if [ -z "$BETA_RELEASE" ]; then
  BETA_RELEASE=0
fi

set -e

CC=arm-vita-eabi-gcc
LD=arm-vita-eabi-gcc
AS=arm-vita-eabi-as
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS="-T payload/linker.x -nodefaultlibs -nostdlib -pie"
DEFINES="-DRELEASE=$RELEASE"
PREPROCESS="$CC -E -P -C -w -x c $DEFINES"
CFLAGS="-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=cortex-a9 -Os -mthumb $DEFINES"

# generate version stuffs
BUILD_VERSION=$(git describe --dirty --always --tags)
BUILD_DATE=$(date)
echo "#define BUILD_VERSION \"$BUILD_VERSION\"" >> build/version.c
echo "#define BUILD_DATE \"$BUILD_DATE\"" >> build/version.c
echo "#define PKG_URL_PREFIX \"$PKG_URL_PREFIX\"" >> build/version.c
echo "#define HENKAKU_RELEASE $HENKAKU_RELEASE" >> build/version.c
echo "#define BETA_RELEASE $BETA_RELEASE" >> build/version.c
echo "#define VITASHELL_CRC32 $VITASHELL_CRC32" >> build/version.c
echo "#define TAIHEN_CRC32 $TAIHEN_CRC32" >> build/version.c
echo "#define PSN_PASSPHRASE \"$PSN_PASSPHRASE\"" >> build/version.c

echo "0) taiHEN plugin"

mkdir build/plugin
pushd build/plugin
cmake -DRELEASE=$RELEASE ../../plugin
make
popd
cp build/plugin/henkaku.skprx output/henkaku.skprx
cp build/plugin/henkaku.suprx output/henkaku.suprx
cp build/plugin/henkaku-stubs/libHENkaku_stub.a output/libHENkaku_stub.a
HENKAKU_CRC32=$(crc32 output/henkaku.skprx)
HENKAKU_USER_CRC32=$(crc32 output/henkaku.suprx)

echo "1) Installer"

echo "#define HENKAKU_CRC32 0x$HENKAKU_CRC32" >> build/version.c
echo "#define HENKAKU_USER_CRC32 0x$HENKAKU_USER_CRC32" >> build/version.c

# user payload is injected into web browser process
mkdir build/bootstrap
pushd build/bootstrap
cmake -DRELEASE=$RELEASE ../../bootstrap
make
popd
xxd -i build/bootstrap/bootstrap.self > build/bootstrap.h

echo "2) Payload"

$CC -c -o build/payload.o payload/payload.c $CFLAGS
$LD -o build/payload.elf build/payload.o $LDFLAGS
$OBJCOPY -O binary build/payload.elf build/payload.bin
PAYLOAD_SIZE=$(ls -l build/payload.bin | awk '{ print $5 }')
echo "#define PAYLOAD_SIZE $PAYLOAD_SIZE" >> build/version.c

# loader decrypts and lods a larger payload
$CC -c -o build/loader.o payload/loader.c $CFLAGS
$AS -o build/loader_start.o payload/loader_start.S
$LD -o build/loader.elf build/loader.o build/loader_start.o $LDFLAGS
$OBJCOPY -O binary build/loader.elf build/loader.bin

cat payload/pad.bin build/loader.bin > build/loader.full
# loader must be <=0x100 bytes
SIZE=$(ls -l build/loader.full | awk '{ print $5 }')
if ((SIZE>0x100)); then
	echo "loader size is $SIZE should be less or equal 0x100 bytes"
	exit -1
fi
echo "loader size is $SIZE"
dd if=/dev/zero bs=256 count=1 > build/loader.256
dd of=build/loader.256 if=build/loader.full bs=256 count=1 conv=notrunc
openssl enc -aes-256-ecb -in build/loader.256 -nopad -out build/loader.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130

echo "3) Kernel ROP"
./krop/build_rop.py krop/rop.S build/

echo "4) User ROP"
echo "symbol stage2_url_base = \"$HENKAKU_BIN_URL\";" > build/config.rop

HENKAKU_BIN_WORDS=$(./urop/exploit.py build/loader.enc build/payload.bin output/web/henkaku.bin)
./urop/loader.py "$HENKAKU_BIN_URL" output/web/henkaku.bin $HENKAKU_BIN_WORDS output/web/payload.js

echo "5) Webkit"
# Hosted version
$PREPROCESS webkit/exploit.js -DSTATIC=0 -o build/exploit.web.js
uglifyjs build/exploit.web.js -m "toplevel" > build/exploit.js
touch output/web/exploit.html
printf "<noscript>Go to browser settings and check \"Enable JavaScript\", then reload this page.</noscript><script src='payload.js'></script><script>" >> output/web/exploit.html
cat build/exploit.js >> output/web/exploit.html
printf "</script>" >> output/web/exploit.html
