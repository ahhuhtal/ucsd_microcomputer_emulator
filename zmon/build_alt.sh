#!/bin/sh

zasm -u --casefold altmon.z80
zasm -u boot.z80

# combine bootloader to 0x00 and altmon to address 0x10
../tools/combine.py -v boot.rom:0x00 altmon.rom:0x10 ../rom.bin:0x00 -t 0x800
