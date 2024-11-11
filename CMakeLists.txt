cmake_minimum_required(VERSION 3.5)
project(ucsd_microcomputer_emulator VERSION 1.0 LANGUAGES CXX)

add_executable(ucsd_emu src/main.cpp)

find_package(fmt)
find_package(Z80 REQUIRED Shared)
target_link_libraries(ucsd_emu Z80 fmt::fmt)
