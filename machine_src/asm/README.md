# BOOT

BOOT is the first program to be exectued, located in ROM at 0000h. Unfortunately, the EPROM of the machine could not be read successfully with my tools. The BOOT program in this repository is thus a re-write to impement the functionality. The purpose of the BOOT program is to load ZMON from ROM into RAM, and to transfer control to it.

# ZMON

ZMON is a monitor program which gets loaded to RAM address F800h during boot. It provides typical memory monitor features, such as loading and inpsecting, as well as console input and output routines that are used by the CP/M BIOS. The console output routine supports scrolling as well as an ADM-3A and Kaypro II type escape sequence for cursor re-positioning. This allows running a lot of Kaypro targeted software on the UCSD microcomputer.

## ZMON commands

### B command (boot)

The `b` command initiates system boot. This causes ZMON to load track 0, sector 1 = (0, 1) of any inserted floppy into memory at 0080h - 00FFh and then jump to 0080h. Note that sector 1 is the first sector of each track, by IBM convention. Tracks, however, are indexed starting from 0.

Given a bootable disk, sector (0, 1) contains a further boot loader program CB. CB is then responsible for loading and starting CP/M.

### G command (go)

Start execution at given addess, with an optionally specified break point address.
- Example: `G 0000` <- start execution at 0000h
- Example: `G a000/a001` <- start execution at A000h, but break at A001h

### DR command (display registers)

Shows current register values
- Example: `DR` <- show all of the registers

### DM command (display memory)

Show memory contents between two addresses
- Example: `DM f800 f8ff` <- display memory from F800h up to and including F8FFh

# CB

The CB program resides on the first sector of a disk, i.e. track 0, sector 1 (0, 1). It gets loaded and executed by ZMON. CB then loads sectors from (0, 2) to (1, 26) into memory at DC00h - F57Fh. The sector skew used in the normal CP/M file system is not taken into account here, and the sector order is simply:
(0, 2) -> (0, 3) -> ... -> (0, 26) -> (1, 1) -> ... -> (1, 26).

On a bootable disk, the sectors contain CP/M and ZBIOS.

CB then jumps to F200h, where the OS reset vector is expected to be located.

# ZBIOS

ZBIOS is an implementation of the CP/M BIOS for the computer. It provides implementations of standard functions required by CP/M at standard memory addresses. This BIOS is written for a CP/M, which resides at DC00h.

# Files

The source files for ZMON, ZBIOS and CB are based on hard copies, which are part of the documentation binder.

In particular, ZMON files are based on a printout of `ZMON.PRN` from 14 July 1985, that was a part of the documentation binder. The name of the file and the date were hand-written as a title for the printout. `ZMON.PRN` apparently represents the listing of a `ZMON.Z80` assembled by Cromemco CDOS assembler version 02.15, as denoted in the header of the printouts.

The `ZMON.PRN` printout contains both a verbatum copy of the original `ZMON.Z80` assembly file as well as the opcode listing. Both of these were separately typed in. These were then used to cross-check each other for correctness.

## zmon_original.z80

Reconstruction of the orignal `ZMON.Z80` file, typed in from the `ZMON.PRN` printout. It attempts to faithfully reproduce the original file, including the formatting and typos of the comments.

Unfortunately this file contains a non-standard expression, which prevents assembly with any modern day Z80 assembler I've tested: the character literal `''''` is used to denote the ASCII value of the apostrophe (27h).

## zmon.z80

Same as `zmon_original.z80`, but replaces the unsupported character literals with bare hex values.

Verified with Günter Woigk's zasm version 4.4.14 to assemble into a bit-exact copy of the opcode listing given in the `ZMON.PRN` printout.

```
zasm -u --casefold zmon.z80
```

## altmon.z80

A new alternative to ZMON. It does not provide any memory monitor features, but provides somewhat enhanced terminal emulation through more supported Kaypro terminal control codes. It also gives a more user friendly prompt for booting the system.

## cb.z80

Reconstruction of the original `CB.Z80` file, typed in from printout. It attempts to faithfully reproduce the original file. The printout has a hand-written title of the file name and a date, which are added as comments in the reconstructed file.

## zbios.z80

Reconstruction of the original `ZBIOS.Z80` file, typed in from printout. It attempts to faithfully reproduce the original file. The printout has a hand-written title of the file name and a date, which are added as comments in the reconstructed file.
