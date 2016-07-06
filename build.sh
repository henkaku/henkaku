#!/bin/bash

set -ex

# build: tmp build files
# output: final files only, for distribution
rm -rf build output
mkdir build output

echo "1) Payload"

CC=arm-vita-eabi-gcc
LD=arm-vita-eabi-gcc
AS=arm-vita-eabi-as
OBJCOPY=arm-vita-eabi-objcopy
CFLAGS="-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=cortex-a9 -Os -mthumb"
LDFLAGS="-T payload/linker.x -nodefaultlibs -nostdlib -pie"
PREPROCESS="$CC -E -P -C -w -x c"

# loader decrypts and lods a larger payload
$CC -c -o build/loader.o payload/loader.c $CFLAGS
$AS -o build/loader_start.o payload/loader_start.S
$LD -o build/loader.elf build/loader.o build/loader_start.o $LDFLAGS
$OBJCOPY -O binary build/loader.elf build/loader.bin

$CC -c -o build/payload.o payload/payload.c $CFLAGS
$LD -o build/payload.elf build/payload.o $LDFLAGS
$OBJCOPY -O binary build/payload.elf build/payload.bin

cat payload/pad.bin build/loader.bin > build/loader.full
openssl enc -aes-256-ecb -in build/loader.full -out build/loader.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130
openssl enc -aes-128-ecb -in build/payload.bin -out build/payload.enc -K 2975dabd59e574ddec2876d65d11089e

./payload/block_check.py build/loader.enc
./payload/block_check.py build/payload.enc

# loader must be <=0x100 bytes
SIZE=$(du -sb build/loader.enc | awk '{ print $1 }')
if ((SIZE>0x100)); then
	echo "loader size is $SIZE should be less or equal 0x100 bytes"
	exit -1
fi
echo "loader size is $SIZE"

echo "2) Kernel ROP"
./krop/build_rop.py krop/rop.S build/

echo "3) User ROP"
./urop/make_rop_array.py build/loader.enc kx_loader build/kx_loader.rop
./urop/make_rop_array.py build/payload.enc second_payload build/second_payload.rop

$PREPROCESS urop/exploit.rop.in -o build/exploit.rop.in
erb build/exploit.rop.in > build/exploit.rop
roptool -s build/exploit.rop -t webkit-360-pkg -o build/exploit.rop.bin -v >/dev/null

echo "4) Webkit"
uglifyjs webkit/exploit.js -m "toplevel" > build/exploit.js
touch output/exploit.html
printf "<script src=payload.js></script><script>" >> output/exploit.html
cat build/exploit.js >> output/exploit.html
printf "</script>" >> output/exploit.html
./webkit/preprocess.py build/exploit.rop.bin output/payload.js
