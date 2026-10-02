# Dig Dug

Dig Dug for the [Picocomputer 6502](https://picocomputer.github.io), ported
to C from the 6502 assembly of the Atari 7800 version. Dig tunnels through
the dirt, pump up the Pookas and Fygars until they pop, and drop rocks on
them.

<!-- rp6502
preset: llvm-mos/Release
publish: digdug.zip
frames: 1500
-->
[![Play Dig Dug](https://g91.github.io/DigDug_cpp_rp6502/digdug/screenshot.png)](https://g91.github.io/DigDug_cpp_rp6502/digdug/)

[Play it in your browser](https://g91.github.io/DigDug_cpp_rp6502/digdug/).

## Playing

| Action | Keyboard | Gamepad |
|---|---|---|
| Move | Arrows or WASD | D-pad or left stick |
| Pump | Space, Z or Ctrl | A, B, X or Y |
| Start | Enter | Start |
| Pause | P | |
| Mute | M | |
| Quit | Escape | |

On the title screen, Up and Down choose one or two players, and Left and
Right choose the level: Teddy is easy and Pineapple is normal. After 12
seconds on the title screen, a demo plays, and Enter or Start begins a
game. Two players take turns with the same controls.

## Building

Dig Dug builds with llvm-mos and the
[RP6502 SDK](https://picocomputer.github.io/sdk.html). Install the tools as
that page describes, then build the ROM:

```bash
$ cmake --preset llvm-mos/Release
$ cmake --build --preset llvm-mos/Release
```

The ROM is `build/llvm-mos/release/digdug.rp6502`. Run it in the emulator
with `tools/rp6502-emu build/llvm-mos/release/digdug.rp6502`, or on a
Picocomputer. In VS Code, choose an llvm-mos preset and press F5, or choose
"RP6502-WEB" to play it in a browser.

`rp6502_web()` in `CMakeLists.txt` packages the ROM into
`build/llvm-mos/release/web/digdug.zip`. Each push to `main` publishes it
to GitHub Pages: the comment above the play link names the zip, and
`.github/workflows/web.yml` builds and publishes it, with a screenshot of
the demo for the play link. See
[RP6502-WEB](https://picocomputer.github.io/web.html).

The build also makes `probe.rp6502` from `src/probe.c`. It draws test
patterns for checking the pixel order of the bitmap and sprite formats in
an emulator screenshot.

## Source

| Files | Contents |
|---|---|
| `src/main.c` | The frame loop, the title screen, the demo and the input. |
| `src/game.c`, `src/monsters.c`, `src/game.h`, `src/tables.h` | The game rules, translated from the 7800 assembly, and their tables. |
| `src/video.c` | The dirt and the score line in a bitmap, the stamps of the 7800 as sprites, and the text. |
| `src/sound.c` | The sound sequencer of the 7800, with each of its two TIA channels played on a PSG channel. |
| `src/gfx_data.c`, `assets/sprites.bin` | The 7800 graphics converted for the Picocomputer: the character set, the sprite images and the 7800 palette in RGB555. |
| `src/xram.h` | The XRAM layout, described below. |
| `src/help.txt` | Shown by HELP and INFO on a Picocomputer and in the ROM Help window of the emulator. |

The header of `src/gfx_data.h` names a generator,
`tools/gen_rp6502_gfx.py`, that is not in this repository, so
`src/gfx_data.c` and `assets/sprites.bin` are edited directly.

## Planes

The playfield is 256x192 pixels in the middle of a 320x240 canvas, with
each pixel of the 7800 two pixels wide. Plane 0 is the back and plane 2 is
the front.

| Plane | Mode | Contents |
|---|---|---|
| 0 | [Mode 3](https://picocomputer.github.io/vga.html#vga-mode-3) bitmap at 4 bpp | The dirt, the tunnels and the score line. Only the cells of the playfield that change are drawn again. |
| 1 | [Mode 5](https://picocomputer.github.io/vga.html#vga-mode-5) sprites, 16x16 at 2 bpp | Dig Dug, the monsters, the rocks, the pump, the flames and the vegetables. A 7800 stamp wider than 16 pixels takes two or three of the 40 sprites. |
| 2 | [Mode 1](https://picocomputer.github.io/vga.html#vga-mode-1) text at 1 bpp | The title screen and the messages, in the built-in font. |

## XRAM layout

`src/xram.h` holds the structures of the devices the game uses, copied from
the RIA and VGA docs, and `xram_layout_t`, which places all the XRAM data.
Each `XRAM_` name is an `offsetof()` in that structure. `rp6502_map()` reads
the names, so `CMakeLists.txt` loads `assets/sprites.bin` at
`XRAM_SPRITE_IMAGES` and no address is written twice. See
[XRAM Memory Map](https://picocomputer.github.io/sdk.html#sdk-xram-memory-map).

| Data, in order | Bytes | Notes |
|---|---|---|
| PSG | 64 | First, because the PSG must not cross a 256-byte page. |
| Palettes | 104 | 16 bitmap colors, then 9 sprite palettes of 4 colors. |
| Mode configurations | 30 | |
| Sprites | 320 | 40 sprites. |
| Keyboard, gamepads | 72 | |
| Text | 1200 | 40x30 characters. |
| Sprite images | 10624 | 166 images from `assets/sprites.bin`. |
| Bitmap | 38400 | |

The layout uses 50814 bytes and leaves 14722 free.
