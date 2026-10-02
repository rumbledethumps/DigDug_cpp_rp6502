cmake_minimum_required(VERSION 3.21)

include(${CMAKE_CURRENT_LIST_DIR}/tools/rp6502.cmake)

project(DIGDUG-RP6502 C ASM)

add_executable(digdug)
rp6502_map(digdug src/xram.h "XRAM_.*")
rp6502_asset(digdug help src/help.txt)
rp6502_asset(digdug XRAM(XRAM_SPRITE_IMAGES) assets/sprites.bin)
rp6502_executable(digdug DATA default RESET default)
rp6502_web(digdug CONFIG [[
    title: 'Dig Dug',
    overlay: '',
    footer: 'Arrows or WASD to move, Space to pump, Enter to start, P to pause, M to mute.',
]])
target_sources(digdug PRIVATE
    src/main.c
    src/game.c
    src/monsters.c
    src/video.c
    src/sound.c
    src/gfx_data.c
)
target_compile_options(digdug PRIVATE -Os -Wall)

# Test patterns for checking the pixel order of the bitmap and sprites
add_executable(probe)
rp6502_map(probe src/xram.h "XRAM_.*")
rp6502_executable(probe DATA default RESET default)
target_sources(probe PRIVATE src/probe.c)
