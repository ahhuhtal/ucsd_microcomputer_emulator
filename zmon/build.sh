#!/bin/sh

zasm -u --casefold zmon.z80
../tools/combine.py -v zmon.rom:0xf800 ../ram.bin:0 -t 0xffff
