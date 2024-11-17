#!/bin/sh

zasm -u --casefold altmon.z80
../tools/combine.py -v altmon.rom:0xf800 ../ram.bin:0 -t 0xffff
