#!/bin/sh

# Assemble CB and ZBIOS
zasm -u --casefold zbios.z80
zasm -u --casefold cb.z80

# ZBIOS has origin at 0xf200. CPM has origin at 0xdc00
# Combine these into a single file with origin at 0xdc00
../tools/combine.py -v zbios.rom:0xf200 ../cpm/cpm22.rom:0xdc00 cpm22_bios.rom:0xdc00

