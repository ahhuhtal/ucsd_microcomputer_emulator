# Emulator for UCSD microcomputer

An emulator for a forgotten Z80-based single board CP/M computer, designed by Chris Poulos in 1985. Apparently Chris Poulos taught a microcomputer engineering class an the UCSD Extension Program in the fall of 1985.

In addition to the emulator, documentation regarding the computer is also gathered here. Most of it is from the course notes of a person who took the course back then and built the computer. The rest is from investigating the existing artifacts.

My available documentation does not have a consistent name for the machine. The PCB is labeled with "MICROCOMPUTER SYSTEM INTEGRATION (C) Chris Poulos, MSEE 6/85". The parts list of the machine just refers to it as "CP/M System". The ROM monitor program displays the message "UCSD HARDWARE DESIGN MONITOR".

## Machine basics

The computer is surprisingly capable for the amount of components it has.

- Z80 CPU
- 64kB of RAM
- 2kB up to 64kB of ROM
- CRT9028 CRT controller
- WD2973 disk controller
- Parallel printer port
- ASCII keyboard input

The RAM is simply the entire address space of the processor.

An interesting quirk is that coming out of reset, the ROM is mapped to 0x0000 through 0x1FFF. During this time, any address in RAM can be written to, but reads in that address range will come from the ROM.

Z80 starts execution at 0000. This address must contain some type of bootloader, which copies the ZMON monitor program from ROM into F800 where it resides during execution. Unfortunately, the ROM I have has resisted recent attempts to read it, and it may be damaged.

The ROM gets unmapped from memory omce any read is performed on the IO bus. The ROM will only get remapped via pressing the reset button.

## Usage

The machine boots into ZMON, which is a monitor program that gets loaded into F800.

The monitor prints the message "UCSD HARDWARE DESIGN MONITOR", and presents the user with a prompt. The normal command to start the boot is to press 'b' on the keyboard.

## Disk format

The disks are single-sided 8-inch soft-sectored floppies. The disk controller supports MFM, but is configured for FM only. The drives are 77 tracks total at 48 tracks per inch.

Based on ZMON/CB/ZBIOS disk routines, as well as the CP/M config (in ZBIOS), the disk format is:
- 128 bytes per sector
- 26 sectors per track
- Sector skew of 6

The total is defined as 243 kilobytes. This would imply that the last track (track 76) has only 8 sectors. These settings would appear to make the disk compatible with cpmtools zen9 configuration.

The format remains to be verified with a real disk image.

Tracks 0 and 1 are reserved. They contain CB and ZBIOS for bootable disks. Directory structure is on track 2.

## Detailed boot description

When the 'b' command is given in the monitor, the monitor will read sector 1 of track 0 into 0080 and jump into 0080. This program is called CB in the documentation.

CB then loads sectors 2 through 26 from track 0, followed by sectors 1 through 26 from track 1, all sequentially into DC00.  This read contains both the ZBIOS program loaded at F200, as well as the CP/M CCP loaded at C200 and BDOS loaded at E400. A jump to F200 is then initiated, where the CP/M cold boot vector resides.

## Monitor commands

In addition to the 'b' command to boot, the monitor supports at least the following commands

- G: Go
    - Start execution at given addess
    - Allows specifying an optional break point
    - Example: `G 0000` <- start execution at `0000h`
    - Example: `G a000/a001` <- start execution at `A000h`, but break at `A001h`
- DR: Display registers
    - Shows the register values
    - Example: `DR` <- show all of the registers
- DM: Display memory
    - Show memory contents between two addresses
    - Example: `DM f800 f8ff` <- display memory from `F800h` up to and including `F8FFh`

## Assembler

The original assembler appears to have been Cromemco CDOS Z80 Assembler version 02.15.
This assembler appears to have supported some non-common instruction syntaxes that have been used in the assembly files. Namely:
- Labels without a semicolon after the name
- ADC A <- standard syntax of ADC has two operands
- OR A,80H <- standard syntax of OR takes a single operand
- AND A,7FH <- standard syntax of AND takes a single operand
- `''''` is used to denote the ASCII code of the apostrophe, i.e. 0x27

Turns out that the Günter Vogk's zasm (see references), supports these constructs except the last one and can directly process most of the original files.

## References

https://www.seasip.info/Cpm/format22.html#:~:text=CP%2FM%202.2%20works%20with,track%20DEFB%20bsh%20%3BBlock%20shift.
http://www.primrosebank.net/computers/cpm/cpm_structure.htm
https://github.com/Megatokio/zasm
