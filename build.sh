#!/bin/bash

set -ex

rm -rf build
mkdir build

echo "1) Payload"

CC=arm-vita-eabi-gcc
LD=arm-vita-eabi-gcc
AS=arm-vita-eabi-as
OBJCOPY=arm-vita-eabi-objcopy
CFLAGS="-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=cortex-a9 -Os -mthumb"
LDFLAGS="-T payload/linker.x -nodefaultlibs -nostdlib -pie"

$CC -c -o build/payload.o payload/payload.c $CFLAGS
$AS -o build/payload_start.o payload/payload_start.S
$LD -o build/payload.elf build/payload.o build/payload_start.o $LDFLAGS
$OBJCOPY -O binary build/payload.elf build/payload.bin

dd if=/dev/zero of=build/pad.bin bs=32 count=1
cat build/pad.bin build/payload.bin > build/payload.full
openssl enc -aes-256-ecb -in build/payload.full -out build/payload.enc -K BD00BF08B543681B6B984708BD00BF0023036018467047D0F8A03043F69D1130

echo "2) Kernel ROP"
./krop/build_rop.py krop/rop.S build/


