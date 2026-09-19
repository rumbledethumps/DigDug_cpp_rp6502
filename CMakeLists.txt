cmake_minimum_required(VERSION 3.21)

# Dig Dug for the RP6502 Picocomputer (llvm-mos).
# Configure with:  cmake -S . -B build -DLLVM_MOS_PLATFORM=rp6502 -DCMAKE_TOOLCHAIN_FILE=<llvm-mos>/lib/cmake/llvm-mos-sdk/llvm-mos-toolchain.cmake
# or simply:       cmake --preset llvm-mos/Release   (see CMakePresets.json)

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)

project(DIGDUG-RP6502 C ASM)

# ---- hardware probe used while developing (draws test patterns) -----------
add_executable(probe)
rp6502_executable(probe DATA default RESET default)
target_sources(probe PRIVATE src/probe.c)

# ---- the game ---------------------------------------------------------------
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/main.c)
add_executable(digdug)
rp6502_asset(digdug 0x1A000 assets/sprites.bin)
rp6502_executable(digdug DATA default RESET default)
target_sources(digdug PRIVATE
    src/main.c
    src/game.c
    src/monsters.c
    src/video.c
    src/sound.c
    src/gfx_data.c
)
target_compile_options(digdug PRIVATE -Os -Wall)
endif()
