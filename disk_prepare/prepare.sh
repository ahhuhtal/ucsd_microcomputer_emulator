#!/bin/sh

# On the disk, the CB is on track 0 sector 1 (at address 0)
# ZBIOS and CPM follow from track 0 sector 2 (address 128), all the way to track 1 sector 26 (6656 bytes total)
../tools/combine.py -v ../zbios/cb.rom:0 ../zbios/cpm22_bios.rom:0x80 boot_tracks.bin:0x00 -t 6655

mkfs.cpm -f zen9 -b boot_tracks.bin ../disk.bin

cd contents

for name in `ls`
do
cpmcp -f zen9 ../../disk.bin $name 0:$name
done

