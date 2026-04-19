# Emulator for UCSD microcomputer

An emulator for a forgotten Z80-based single board CP/M computer, designed by Chris Poulos in 1985. Apparently Chris Poulos taught a microcomputer engineering class at the UCSD Extension Program in the fall of 1985.

In addition to the emulator, documentation regarding the computer is also gathered here. Most of it is from the course notes of a person who took the course back then and built the computer. The rest is from investigating the existing artifacts.

My available documentation does not have a consistent name for the machine. The PCB is labeled with "MICROCOMPUTER SYSTEM INTEGRATION (C) Chris Poulos, MSEE 6/85". The parts list of the machine just refers to it as "CP/M System". The ROM monitor program displays the message "UCSD HARDWARE DESIGN MONITOR".

## Prerequisites

- A C/C++ compiler

- cmake

- libZ80 (https://zxe.io/software/Z80)

- libfmt (https://github.com/fmtlib/fmt)

- zasm (https://github.com/Megatokio/zasm)

- python3

- screen

- git

### Ubuntu specific instructions

#### Install the repository for libZ80
```
curl -L https://zxe.io/scripts/add-zxe-apt-repo.sh | sudo sh
sudo apt update
```

#### Install required packages

```
sudo apt install build-essential cmake libfmt-dev cpmtools libz80-dev screen git
```

#### Build zasm

In a suitable directory:

```
git clone https://github.com/Megatokio/zasm.git
cd zasm
make
```
This results in the executable binary `zasm`. The emulator build system will automatically find the `zasm` binary if it is placed in a directory which is in the `PATH`. Alternatively, you can specify the location of the binary when configuring the emulator build system.

## Building the emulator

```
mkdir build
cd build
cmake ..
make
make install
```

This will build the emulator in a the subdirectory `build`. The generated executable binary is called `ucsd_emu`.

### Build options

#### Monitor

The option `MONITOR` can take the values of `ZMON` or `ALTMON`, to switch between the original `ZMON` monitor as listed in the documentation. `ALTMON` is a new version, which has somewhat enchanced terminal emulation. The default is to use `ZMON`.

Example (issue in the build directory):
```
cmake -DMONITOR=ALTMON ..
make
```

#### Zasm binary location

The option `ZASM` allows specifying the location of the Zasm assemler binary, if it was not placed to a directory in the `PATH`.

Example (issue in the build directory):
```
cmake -DZASM=~/zasm/zasm ..
make
```

## Emulator usage

Running `ucsd_emu` will load the ROM image `rom.bin` and disk image `disk.bin`, and start the emulation. This results in output similar to:
```
Terminal is at /dev/pts/4
>
```
This is a command prompt, which allows giving some commands to the emulator. Issue the command `help` to see a list of commands. The emulator is now running.

To interact with the actual terminal of the machine, first open a new terminal on your computer. The terminal should be able to show at least 82 x 27 characters. You can then attach to the terminal device using the program `screen`:
```
screen /dev/pts/XXX
```
where XXX is the PTY device indicated by the emulator.

![Screenshot of system terminal after boot](screenshot_booted.png)

## Machine basics

The computer is surprisingly capable for the amount of components it has. The main components are:

- Z80 CPU
- 64kB of RAM
- 2kB up to 64kB of ROM
- CRT9028 CRT controller
- WD2973 disk controller
- Parallel printer port
- ASCII keyboard input

The RAM is simply the entire address space of the processor.

An interesting feature is that coming out of reset, the ROM is mapped to 0000h through 1FFFh. While the ROM is mapped, any address in RAM can be written to, but any reads in the ROM range will come from the ROM.

Z80 starts execution at 0000h. This address thus must contain some type of bootloader, which copies the ZMON monitor program from ROM into F800h where it resides during execution. Unfortunately, the ROM I have has resisted recent attempts to read it, and it may be damaged.

The ROM gets unmapped from memory once any read is performed on the IO bus. The ROM will only get remapped via pressing the reset button.

## Machine usage

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

Tracks 0 and 1 are reserved. They contain CB and ZBIOS for bootable disks. The directory structure is on track 2.

## Boot description

When the `b` command is given in the monitor, the monitor will read sector 1 of track 0 into 0080 and jump into 0080. This program is called CB in the documentation.

CB then loads sectors 2 through 26 from track 0, followed by sectors 1 through 26 from track 1, all sequentially into DC00h.  This read contains both the ZBIOS program loaded at F200h, as well as the CP/M CCP loaded at C200h and BDOS loaded at E400h. A jump to F200h is then initiated, where the CP/M cold boot vector resides.

## Monitor commands

In addition to the `b` command to boot, the monitor supports at least the following commands

- `G`: Go
    - Start execution at given addess
    - Allows specifying an optional break point
    - Example: `G 0000` <- start execution at 0000h
    - Example: `G a000/a001` <- start execution at A000h, but break at A001h
- `DR`: Display registers
    - Shows the register values
    - Example: `DR` <- show all of the registers
- `DM`: Display memory
    - Show memory contents between two addresses
    - Example: `DM f800 f8ff` <- display memory from F800h up to and including F8FFh

## Assembler

The original assembler appears to have been Cromemco CDOS Z80 Assembler version 02.15.
This assembler appears to have supported some non-common instruction syntaxes that have been used in the assembly files. Namely:
- Labels without a semicolon after the name
- ADC A <- standard syntax of ADC has two operands
- OR A,80H <- standard syntax of OR takes a single operand
- AND A,7FH <- standard syntax of AND takes a single operand
- `''''` is used to denote the ASCII code of the apostrophe, i.e. 27h

Turns out that the Günter Woigk's zasm supports these peculiarities, except the last one, and can directly process most of the original files.

## References

https://www.seasip.info/Cpm/format22.html#:~:text=CP%2FM%202.2%20works%20with,track%20DEFB%20bsh%20%3BBlock%20shift.

http://www.primrosebank.net/computers/cpm/cpm_structure.htm

