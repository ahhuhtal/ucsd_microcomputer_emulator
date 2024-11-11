# ZMON

ZMON is the monitor program, which gets loaded to RAM address `F800h` during boot. It provides typical memory monitor features, such as loading and inpsecting, as well as the console in/out routines that are used by the CP/M BIOS. The console output routine supports scrolling as well as an ADM-3A and Kaypro II type escape sequence for cursor re-positioning. This allows running a lot of Kaypro targeted software on the UCSD microcomputer.

ZMON is loaded from EPROM by a first stage bootloader. The EPROM is damaged, so this bootloader might be lost to time.

## Detailed documentation

### B command

The `b` command initiates CP/M boot. This causes ZMON to load track 0, sector 1 = (0, 1) of any inserted floppy into memory starting at `0080h` and jump to `0080h`. Note: sector 1 is actually the first sector of each track, by some IBM convention.

Given a CP/M disk, the sector contains a further boot loader, denoted by its assembly file `CB.Z80`. This loader then loads all sectors starting from (0, 2) through (1, 26) into memory starting at `DC00h`. The sector skew used in the normal CP/M file system is not taken into account here, and the order is simply:
(0, 2) -> (0, 3) -> ... -> (0, 25) -> (0, 26) -> (1, 1) -> ... -> (1, 26)

### Other commands TODO

## Files

The files of ZMON are based on a printout of `ZMON.PRN` from 14 July 1985, that was a part of the documentation binder. `ZMON.PRN` apparently represents the listing of `ZMON.Z80` assembled by Cromemco CDOS assembler version 02.15, as denoted in the header of the printouts.

`ZMON.PRN` contains both a verbatum copy of the original assembly file as well as the opcode listing. Both of these were separately typed in. These were then used to cross-check each other for correctness.

### zmon_original.z80

Reconstruction of orignal `ZMON.Z80` file, typed in from the `ZMON.PRN` printout. It attempts to faithfully reproduce the original file, including the formatting and typos of the comments.

Unfortunately this file contains a non-standard expression, which prevents assembly with most assemblers: the ASCII byte for an apostrophe is denoted as the character literal `''''`.

### zmon.z80

Same as `zmon_original.z80`, but replaces the unsupported character literals with bare hex values.

Verified with Günter Woigk's zasm version 4.4.14 to assemble into a bit-exact copy of the opcode listing given in the `ZMON.PRN` printout.

```
zasm -u --casefold zmon.z80
```

