// Hardware probe: draws known patterns so the byte/pixel order of the
// bitmap and sprite formats can be checked from an emulator screenshot.
#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>
#include "xram.h"

#define COLOR(r, g, b) (COLOR_FROM_RGB5(r, g, b) | COLOR_ALPHA_MASK)

// palette: 0 black, 1 red, 2 green, 3 blue, 4 yellow, 5 cyan, 6 magenta, 7 white ...
static const uint16_t pal[16] = {
    COLOR(0, 0, 0), COLOR(31, 0, 0), COLOR(0, 31, 0), COLOR(0, 0, 31),
    COLOR(31, 31, 0), COLOR(0, 31, 31), COLOR(31, 0, 31), COLOR(31, 31, 31),
    COLOR(15, 0, 0), COLOR(0, 15, 0), COLOR(0, 0, 15), COLOR(15, 15, 0),
    COLOR(0, 15, 15), COLOR(15, 0, 15), COLOR(15, 15, 15), COLOR(31, 15, 0)};

// top-left byte = 0x12 : tells us which nibble is the left pixel,
// then a yellow pair and a white pair
static const uint8_t corner[3] = {0x12, 0x44, 0x77};

// sprite palette (2bpp): 0 transparent, 1 red, 2 green, 3 white
static const uint16_t spal[4] = {0, COLOR(31, 0, 0), COLOR(0, 31, 0), COLOR(31, 31, 31)};

// sprite image: 16x16 2bpp = 4 bytes per row. Row 0: 0x1B,0x00,0x00,0x03
// (pixels 0..3 = 0,1,2,3 ; last pixel of row = 3). Rows 1..14: 0xFF,0x00,0x00,0xFF
// Row 15: 0xAA x4 (all colour 2 = green).
static const uint8_t spr_first[4] = {0x1B, 0x00, 0x00, 0x03};
static const uint8_t spr_mid[4] = {0xFF, 0x00, 0x00, 0xFF};
static const uint8_t spr_last[4] = {0xAA, 0xAA, 0xAA, 0xAA};

// two sprites at (32,32) and (60,40) ; a third one partly off the left edge
static const mode5_sprite_t sprites[3] = {
    {32, 32, XRAM_SPRITE_IMAGES, XRAM_SPRITE_PALETTES},
    {60, 40, XRAM_SPRITE_IMAGES, XRAM_SPRITE_PALETTES},
    {-8, 100, XRAM_SPRITE_IMAGES, XRAM_SPRITE_PALETTES}};

static const mode3_config_t bitmap_config = {
    false, false, 0, 0, BITMAP_W, BITMAP_H, XRAM_BITMAP, XRAM_BITMAP_PALETTE};

// mode 1 text: 20 columns x 2 rows at (8, 200), built-in font and palette
static const mode1_config_t text_config = {
    false, false, 8, 200, 20, 2, XRAM_TEXT, 0xFFFF, 0xFFFF};

static keyboard_t keyboard;

int main(void) {
    unsigned i;
    xram0_write(XRAM_BITMAP_PALETTE, pal, sizeof pal);
    // clear bitmap to colour 3 (blue) so black/transparent shows
    xram0_set(XRAM_BITMAP, 0x33, BITMAP_H * (BITMAP_W / 2u));
    xram0_write(XRAM_BITMAP, corner, sizeof corner);
    // a 2-pixel-wide vertical bar at x=100 (byte 50) for rows 0..239
    for (i = 0; i < BITMAP_H; ++i) xram0_poke8(XRAM_BITMAP + i * (BITMAP_W / 2) + 50, 0x77);
    xram0_write(XRAM_BITMAP_CONFIG, &bitmap_config, sizeof bitmap_config);

    xram0_write(XRAM_SPRITE_PALETTES, spal, sizeof spal);
    xram0_write(XRAM_SPRITE_IMAGES, spr_first, sizeof spr_first);
    for (i = 1; i < 15; ++i) xram0_write(XRAM_SPRITE_IMAGES + i * sizeof spr_mid, spr_mid, sizeof spr_mid);
    xram0_write(XRAM_SPRITE_IMAGES + 15 * sizeof spr_last, spr_last, sizeof spr_last);
    xram0_write(XRAM_SPRITES, sprites, sizeof sprites);

    xram0_write(XRAM_TEXT_CONFIG, &text_config, sizeof text_config);
    {
        static const char* msg = "HELLO RP6502 DIG DUG";
        xram0_write(XRAM_TEXT, msg, 20);
        for (i = 20; i < 40; ++i) xram0_poke8(XRAM_TEXT + i, 'A' + i - 20);
    }

    xreg_vga_canvas(CANVAS_320X240);
    xreg_vga_mode3(MODE3_4BPP, XRAM_BITMAP_CONFIG, 0);
    xreg_vga_mode5(MODE5_2BPP | MODE5_16X16, XRAM_SPRITES, 3, 1);
    xreg_vga_mode1(MODE1_1BPP | MODE1_8X8, XRAM_TEXT_CONFIG, 2);
    xreg_ria_keyboard(XRAM_KEYBOARD);

    // wait ~ forever, printing the keyboard state occasionally
    {
        unsigned char last = ria_vsync();
        unsigned frames = 0;
        for (;;) {
            while (ria_vsync() == last) {}
            last = ria_vsync();
            if (++frames == 60) {
                xram0_read(&keyboard, XRAM_KEYBOARD, sizeof keyboard);
                for (i = 0; i < 32; ++i) printf("%02x", keyboard.keys[i]);
                printf("\n");
                frames = 0;
            }
        }
    }
    return 0;
}
