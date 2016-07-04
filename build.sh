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

$CC -c -o build/payload.o payload/payload.c $CFLAGS
$AS -o build/payload_start.o payload/payload_start.S
$LD -o build/payload.elf build/payload.o build/payload_start.o $LDFLAGS
$OBJCOPY -O binary build/payload.elf build/payload.bin

dd if=/dev/zero of=build/pad.bin bs=32 count=1
cat build/pad.bin build/payload.bin > build/payload.full
openssl enc -aes-256-ecb -in build/payload.full -out build/payload.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130

echo "2) Kernel ROP"
./krop/build_rop.py krop/rop.S build/

echo "3) User ROP"
$PREPROCESS urop/loader.rop.in -o build/loader.rop
roptool -s build/loader.rop -t webkit-360-pkg -o build/loader.rop.bin -v
$PREPROCESS urop/stage2.rop.in -o build/stage2.rop
roptool -s build/stage2.rop -t webkit-360-pkg -o build/stage2.rop.bin -v

echo "4) Webkit"
cp webkit/exploit.html output/
./webkit/preprocess.py build/loader.rop.bin output/payload.js
cp webkit/stage2.php output/
./webkit/preprocess.py build/stage2.rop.bin output/stage2-payload.php
