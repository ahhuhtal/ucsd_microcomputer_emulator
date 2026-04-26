cmake_minimum_required(VERSION 3.5)
project(ucsd_microcomputer_emulator VERSION 1.0 LANGUAGES CXX)

add_executable(ucsd_emu src/main.cpp)

find_package(fmt)
find_package(Z80 REQUIRED Shared)
target_link_libraries(ucsd_emu Z80 fmt::fmt)

set(MONITOR "ZMON" CACHE STRING "Select monitor to build (ZMON or ALTMON)")

find_program(ZASM zasm)
find_program(PYTHON python3)

option(USE_SDL "Use SDL for graphics and input" OFF)

if(USE_SDL)
    find_package(SDL2 REQUIRED)
    target_link_libraries(ucsd_emu SDL2::SDL2)
    add_compile_definitions(USE_SDL)
endif()

if(NOT ZASM)
    message(FATAL_ERROR "ZASM not found")
endif()

# source files for generating ROM
if(MONITOR STREQUAL "ZMON")
    configure_file(asm/zmon.z80 asm/monitor.z80 COPYONLY)
elseif(MONITOR STREQUAL "ALTMON")
    configure_file(asm/altmon.z80 asm/monitor.z80 COPYONLY)
else()
    message(FATAL_ERROR "Unknown monitor: ${MONITOR}")
endif()
configure_file(asm/boot.z80 asm/boot.z80 COPYONLY)

# Build ROM image. ROM is mapped at 0x0000, with a short BOOT routine at the beginning,
# followed by the monitor binary. The BOOT routine copies the monitor from 0x0010 to 0xF800.
add_custom_command(
    OUTPUT rom.bin
    COMMAND ${ZASM} -u --casefold asm/monitor.z80
    COMMAND ${ZASM} -u asm/boot.z80
    COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py -v asm/boot.rom:0x00 asm/monitor.rom:0x10 rom.bin -t 0x800
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py asm/monitor.z80 asm/boot.z80
    COMMENT "Building ROM image"
)
add_custom_target(rom DEPENDS rom.bin)

# Source files for combined CPM+ZBIOS image
configure_file(asm/zbios.z80 asm/zbios.z80 COPYONLY)

# Generate combined CP/M+ZBIOS image
# The computer uses a version of CP/M which resides at 0xDC00, while ZBIOS resides at 0xF200.
# Generate an image of the two from address 0xDC00.
add_custom_command(
    OUTPUT cpm22_zbios_dc00.bin
    COMMAND ${ZASM} -u --casefold asm/zbios.z80
    COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py -v asm/zbios.rom:0xf200 ${CMAKE_CURRENT_SOURCE_DIR}/cpm/cpm22.rom:0xdc00 cpm22_zbios_dc00.bin:0xdc00
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py asm/zbios.z80 ${CMAKE_CURRENT_SOURCE_DIR}/cpm/cpm22.rom
    COMMENT "Building combined CP/M+ZBIOS image"
)

# Source for disk loader routine
configure_file(asm/cb.z80 asm/cb.z80 COPYONLY)

# Build disk boot tracks binary.
# The disk loader routine CB is placed on track 0, sector 1.
# CB loads the data from track 0, sector 2 through track 1, sector 26 into memory at 0xDC00.
# We place the combined CP/M+ZBIOS image at those sectors.
add_custom_command(
    OUTPUT disk_boot_tracks.bin
    COMMAND ${ZASM} -u --casefold asm/cb.z80
    COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py -v asm/cb.rom:0x00 cpm22_zbios_dc00.bin:0x80 disk_boot_tracks.bin -t 6655
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/combine.py asm/cb.z80 cpm22_zbios_dc00.bin
    COMMENT "Building disk boot tracks binary"
)
add_custom_target(disk_boot_tracks DEPENDS disk_boot_tracks.bin)

# Build disk image
add_custom_command(
    OUTPUT disk.bin
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_disk_image.sh disk.bin ${CMAKE_CURRENT_SOURCE_DIR}/disk disk_boot_tracks.bin
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_disk_image.sh disk_boot_tracks.bin
    COMMENT "Building boot disk image"
)
add_custom_target(disk DEPENDS disk.bin)
add_dependencies(ucsd_emu rom disk)

# Install into a deploy directory at the source root by default
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

install(
    TARGETS ucsd_emu RUNTIME
    DESTINATION deploy
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/rom.bin ${CMAKE_CURRENT_BINARY_DIR}/disk.bin ${CMAKE_CURRENT_BINARY_DIR}/disk_boot_tracks.bin
    DESTINATION deploy
)
