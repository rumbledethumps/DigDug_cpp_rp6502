#include "video.h"
#include "game.h"
#include "gfx_data.h"
#include "tables.h"
#include "xram.h"
#include <rp6502.h>
#include <string.h>

// bitmap palette indices
//  0 black, 1..8 dirt layers (dirt, pebble) x 4, 9 sky green, 10 sky blue,
//  11 sky red, 12 white, 13 yellow, 14 dig dug blue, 15 (lives colour 3)
#define BPAL_SKYBLUE 10

// per-context value -> nibble*0x11 tables
static uint8_t dirtNib[4][4];    // per layer
static uint8_t skyNib[4];
static uint8_t scoreNib[4];
static uint8_t livesNib[4];

static uint8_t dirtyList[64];
static uint8_t dirtyCount;
static uint8_t redrawAll;
static uint8_t scoreDirty;
static uint8_t palDirty;
static uint8_t blanked;

static palettes_t palettes;
static mode5_sprite_t sprites[NUM_SPRITES];

#define ROWBYTES (BITMAP_W / 2u)
#define TOP_BORDER 24            // (240 - 192) / 2

static void build_nibs(void) {
    uint8_t l, v;
    for (l = 0; l < 4; ++l) {
        dirtNib[l][0] = 0x00;
        dirtNib[l][1] = (uint8_t)((l * 2 + 1) * 0x11);
        dirtNib[l][2] = (uint8_t)(BPAL_SKYBLUE * 0x11);
        dirtNib[l][3] = (uint8_t)((l * 2 + 2) * 0x11);
    }
    skyNib[0] = 0x00;
    for (v = 1; v < 4; ++v) skyNib[v] = (uint8_t)((8 + v) * 0x11);
    scoreNib[0] = (uint8_t)(BPAL_SKYBLUE * 0x11);
    scoreNib[1] = (uint8_t)(BPAL_SKYBLUE * 0x11);
    scoreNib[2] = (uint8_t)(BPAL_SKYBLUE * 0x11);
    scoreNib[3] = (uint8_t)(12 * 0x11);
    livesNib[0] = (uint8_t)(BPAL_SKYBLUE * 0x11);
    livesNib[1] = (uint8_t)(13 * 0x11);
    livesNib[2] = (uint8_t)(14 * 0x11);
    livesNib[3] = (uint8_t)(15 * 0x11);
}

static void write_palettes(void) {
    uint8_t l, p, c;
    // bitmap palette
    palettes.bitmap[0] = NTSC555[0];
    for (l = 0; l < 4; ++l) {
        palettes.bitmap[l * 2 + 1] = NTSC555[DIRTCOLR[G.dirtScheme + l]];
        palettes.bitmap[l * 2 + 2] = NTSC555[PEBBCOLR[G.dirtScheme + l]];
    }
    palettes.bitmap[9] = NTSC555[IPALETTE[7][1]];
    palettes.bitmap[10] = NTSC555[IPALETTE[7][2]];
    palettes.bitmap[11] = NTSC555[IPALETTE[7][3]];
    palettes.bitmap[12] = NTSC555[WHITE];
    palettes.bitmap[13] = NTSC555[IPALETTE[4][1]];
    palettes.bitmap[14] = NTSC555[IPALETTE[4][2]];
    palettes.bitmap[15] = NTSC555[IPALETTE[4][3]];
    // sprite palettes 0..7 = the 7800 palettes, 8 = vegetables; entry 0 stays transparent
    for (p = 0; p < 8; ++p)
        for (c = 1; c < 4; ++c) palettes.sprites[p][c] = NTSC555[IPALETTE[p][c]];
    for (c = 0; c < 3; ++c) palettes.sprites[8][c + 1] = NTSC555[G.vegcol[c]];
    xram0_write(XRAM_PALETTES, &palettes, sizeof palettes);
}

// Draw one 8x12 character (doubled to 16x12) at bitmap pixel (x, y).
static void draw_char(uint8_t ch, unsigned x, unsigned y, const uint8_t* nib) {
    uint8_t k = CHARMAP[ch];
    const uint8_t* rows;
    unsigned addr = XRAM_BITMAP + y * ROWBYTES + (x >> 1);
    uint8_t t;
    uint8_t px[8];
    if (k == 0xFF) {
        // unknown character: draw black
        for (t = 0; t < 12; ++t) {
            xram0_set(addr, 0, sizeof px);
            addr += ROWBYTES;
        }
        return;
    }
    rows = CHARROWS + (unsigned)k * 24;
    for (t = 0; t < 12; ++t) {
        uint8_t b0 = rows[0], b1 = rows[1];
        rows += 2;
        px[0] = nib[(b0 >> 6) & 3];
        px[1] = nib[(b0 >> 4) & 3];
        px[2] = nib[(b0 >> 2) & 3];
        px[3] = nib[b0 & 3];
        px[4] = nib[(b1 >> 6) & 3];
        px[5] = nib[(b1 >> 4) & 3];
        px[6] = nib[(b1 >> 2) & 3];
        px[7] = nib[b1 & 3];
        xram0_write(addr, px, sizeof px);
        addr += ROWBYTES;
    }
}

static void draw_cell(uint8_t idx) {
    uint8_t row = (uint8_t)(idx >> 4), col = (uint8_t)(idx & 15);
    uint8_t ch = G.dirtmap[idx];
    const uint8_t* nib;
    unsigned x = 32 + (unsigned)col * 16;
    unsigned y = TOP_BORDER + 12 + (unsigned)row * 12;
    if (row > 14) return;
    if (row == 0) nib = skyNib;
    else if (row <= 3) nib = dirtNib[0];
    else if (row <= 7) nib = dirtNib[1];
    else if (row <= 11) nib = dirtNib[2];
    else nib = dirtNib[3];
    draw_char(ch, x, y, nib);
}

static void draw_digits(uint32_t value, unsigned x, uint8_t ndigits, const uint8_t* nib) {
    uint8_t digits[7];
    int8_t i;
    for (i = (int8_t)(ndigits - 1); i >= 0; --i) {
        digits[i] = (uint8_t)(value % 10);
        value /= 10;
    }
    for (i = 0; i < (int8_t)ndigits; ++i) {
        uint8_t leading = 1;
        int8_t j;
        for (j = 0; j < i; ++j) if (digits[j]) leading = 0;
        if (leading && digits[i] == 0 && i < (int8_t)(ndigits - 1)) {
            draw_char(30, x + (unsigned)i * 16, TOP_BORDER, skyNib);
        } else {
            draw_char((uint8_t)(FULL0 + 2 * digits[i]), x + (unsigned)i * 16, TOP_BORDER, nib);
        }
    }
}

static void draw_score_line(void) {
    uint8_t col, lives;
    for (col = 0; col < 16; ++col) draw_char(30, 32 + (unsigned)col * 16, TOP_BORDER, skyNib);
    draw_digits(G.score[0] / 10 * 10, 32 + 24 * 2, 6, scoreNib);
    if (G.numplayr) draw_digits(G.score[1] / 10 * 10, 32 + 88 * 2, 6, scoreNib);
    lives = G.nummen[G.playnum];
    lives = lives ? (uint8_t)(lives - 1) : 0;
    if (lives > 9) lives = 9;
    draw_char((uint8_t)(FULL0 + 2 * lives), 32 + 76 * 2, TOP_BORDER, livesNib);
}

static void clear_bitmap(void) {
    xram0_set(XRAM_BITMAP, 0, BITMAP_H * ROWBYTES);
}

static void park_sprites(void) {
    uint8_t i;
    for (i = 0; i < NUM_SPRITES; ++i) {
        sprites[i].x_pos_px = 0;
        sprites[i].y_pos_px = -16;
        sprites[i].xram_sprite_ptr = XRAM_SPRITE_IMAGES;
        sprites[i].palette_ptr = XRAM_SPRITE_PALETTES;
    }
    xram0_write(XRAM_SPRITES, sprites, sizeof sprites);
}

static const mode3_config_t bitmap_config = {
    false, false, 0, 0, BITMAP_W, BITMAP_H, XRAM_BITMAP, XRAM_BITMAP_PALETTE};

// 0xFFFF selects the built-in palette and font.
static const mode1_config_t text_config = {
    false, false, 0, 0, TEXT_COLS, TEXT_ROWS, XRAM_TEXT, 0xFFFF, 0xFFFF};

void video_init(void) {
    build_nibs();
    clear_bitmap();
    write_palettes();
    park_sprites();
    xram0_write(XRAM_BITMAP_CONFIG, &bitmap_config, sizeof bitmap_config);
    xram0_write(XRAM_TEXT_CONFIG, &text_config, sizeof text_config);
    video_text_clear();
    xreg_vga_canvas(CANVAS_320X240);
    xreg_vga_mode3(MODE3_4BPP, XRAM_BITMAP_CONFIG, 0);
    xreg_vga_mode5(MODE5_2BPP | MODE5_16X16, XRAM_SPRITES, NUM_SPRITES, 1);
    xreg_vga_mode1(MODE1_1BPP | MODE1_8X8, XRAM_TEXT_CONFIG, 2);
    dirtyCount = 0;
    redrawAll = 1;
    scoreDirty = 1;
    palDirty = 1;
    blanked = 0;
}

void video_wait_vsync(void) {
    unsigned char v = ria_vsync();
    while (ria_vsync() == v) {}
}

void video_dirty(uint8_t idx) {
    if (redrawAll) return;
    if (dirtyCount >= sizeof dirtyList) {
        redrawAll = 1;
        return;
    }
    dirtyList[dirtyCount++] = idx;
}

void video_redraw_all(void) {
    redrawAll = 1;
}

void video_palette_changed(void) {
    palDirty = 1;
}

void video_score_changed(void) {
    scoreDirty = 1;
}

void video_blank(uint8_t on) {
    if (on == blanked) return;
    blanked = on;
    // move the bitmap off screen (or back) and hide sprites
    xram0_poke16(XRAM_BITMAP_CONFIG + offsetof(mode3_config_t, y_pos_px), on ? BITMAP_H : 0);
    if (on) park_sprites();
}

// sprite list update: every visible stamp becomes one or more 16x16 chunks
static void update_sprites(void) {
    uint8_t n = 0;
    uint8_t prior, i;
    if (blanked) return;
    for (prior = 1; prior != 0xFF; --prior) {
        for (i = 0; i < 19; ++i) {
            const stamp_t* s = ALL_STAMPS[i];
            uint16_t slot;
            uint8_t chunks, c, pal;
            int sx, sy;
            unsigned img, palp;
            if (s->prior != prior || s->y == 0) continue;
            if (s->cset == 2) { slot = STAMP2_SLOT[s->ix]; chunks = STAMP2_CHUNKS[s->ix]; }
            else { slot = STAMP1_SLOT[s->ix]; chunks = STAMP1_CHUNKS[s->ix]; }
            if (slot == 0xFFFF) continue;
            pal = (uint8_t)(STMPPALW[s->ix >> 1] >> 5);
            if (s->cset == 1) pal = 8;
            palp = XRAM_SPRITE_PALETTES + (unsigned)pal * sizeof palettes.sprites[0];
            sx = 32 + (int)s->x * 2;
            sy = TOP_BORDER + 203 - (int)s->y;
            img = XRAM_SPRITE_IMAGES + slot * sizeof(sprite_image_t);
            for (c = 0; c < chunks && n < NUM_SPRITES; ++c) {
                sprites[n].x_pos_px = sx;
                sprites[n].y_pos_px = sy;
                sprites[n].xram_sprite_ptr = img;
                sprites[n].palette_ptr = palp;
                n++;
                sx += 16;
                img += sizeof(sprite_image_t);
            }
        }
    }
    // park the rest
    while (n < NUM_SPRITES) sprites[n++].y_pos_px = -16;
    xram0_write(XRAM_SPRITES, sprites, sizeof sprites);
}

void video_flush(void) {
    if (palDirty) {
        write_palettes();
        palDirty = 0;
    }
    if (redrawAll) {
        unsigned i;
        for (i = 0; i < 240; ++i) draw_cell((uint8_t)i);
        draw_score_line();
        redrawAll = 0;
        scoreDirty = 0;
        dirtyCount = 0;
    } else {
        uint8_t i;
        for (i = 0; i < dirtyCount; ++i) draw_cell(dirtyList[i]);
        dirtyCount = 0;
        if (scoreDirty) {
            draw_score_line();
            scoreDirty = 0;
        }
    }
    update_sprites();
}

// ------------------------------------------------------------------ text --
void video_text_clear(void) {
    xram0_set(XRAM_TEXT, ' ', TEXT_COLS * TEXT_ROWS);
}

void video_text(uint8_t col, uint8_t row, const char* s) {
    xram0_write(XRAM_TEXT + (unsigned)row * TEXT_COLS + col, s, strlen(s));
}

void video_text_num(uint8_t col, uint8_t row, uint32_t v, uint8_t width) {
    char buf[12];
    int8_t i = 11;
    buf[i] = 0;
    do {
        buf[--i] = (char)('0' + v % 10);
        v /= 10;
    } while (v && i > 0);
    while (i > 0 && (uint8_t)(11 - i) < width) buf[--i] = ' ';
    video_text(col, row, buf + i);
}
