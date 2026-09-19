#include "video.h"
#include "game.h"
#include "gfx_data.h"
#include "tables.h"
#include <rp6502.h>

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

#define ROWBYTES 160
#define TOP_BORDER 24            // (240 - 192) / 2

static void xram_write16(unsigned addr, unsigned v) {
    RIA.addr0 = addr;
    RIA.step0 = 1;
    RIA.rw0 = (unsigned char)v;
    RIA.rw0 = (unsigned char)(v >> 8);
}

static void set_palette_entry(unsigned addr, uint8_t colour, uint8_t opaque) {
    unsigned v = NTSC555[colour];
    if (!opaque) v = 0;
    xram_write16(addr, v);
}

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
    set_palette_entry(XR_BPAL + 0, 0, 1);
    for (l = 0; l < 4; ++l) {
        set_palette_entry(XR_BPAL + (l * 2 + 1) * 2, DIRTCOLR[G.dirtScheme + l], 1);
        set_palette_entry(XR_BPAL + (l * 2 + 2) * 2, PEBBCOLR[G.dirtScheme + l], 1);
    }
    set_palette_entry(XR_BPAL + 9 * 2, IPALETTE[7][1], 1);
    set_palette_entry(XR_BPAL + 10 * 2, IPALETTE[7][2], 1);
    set_palette_entry(XR_BPAL + 11 * 2, IPALETTE[7][3], 1);
    set_palette_entry(XR_BPAL + 12 * 2, WHITE, 1);
    set_palette_entry(XR_BPAL + 13 * 2, IPALETTE[4][1], 1);
    set_palette_entry(XR_BPAL + 14 * 2, IPALETTE[4][2], 1);
    set_palette_entry(XR_BPAL + 15 * 2, IPALETTE[4][3], 1);
    // sprite palettes 0..7 = the 7800 palettes, 8 = vegetables
    for (p = 0; p < 8; ++p) {
        set_palette_entry(XR_SPAL + p * 8, 0, 0);
        for (c = 1; c < 4; ++c) set_palette_entry(XR_SPAL + p * 8 + c * 2, IPALETTE[p][c], 1);
    }
    set_palette_entry(XR_SPAL + 8 * 8, 0, 0);
    for (c = 0; c < 3; ++c) set_palette_entry(XR_SPAL + 8 * 8 + (c + 1) * 2, G.vegcol[c], 1);
}

// Draw one 8x12 character (doubled to 16x12) at bitmap pixel (x, y).
static void draw_char(uint8_t ch, unsigned x, unsigned y, const uint8_t* nib) {
    uint8_t k = CHARMAP[ch];
    const uint8_t* rows;
    unsigned addr = y * ROWBYTES + (x >> 1);
    uint8_t t;
    if (k == 0xFF) {
        // unknown character: draw black
        for (t = 0; t < 12; ++t) {
            uint8_t i;
            RIA.addr0 = addr;
            RIA.step0 = 1;
            for (i = 0; i < 8; ++i) RIA.rw0 = 0;
            addr += ROWBYTES;
        }
        return;
    }
    rows = CHARROWS + (unsigned)k * 24;
    for (t = 0; t < 12; ++t) {
        uint8_t b0 = rows[0], b1 = rows[1];
        rows += 2;
        RIA.addr0 = addr;
        RIA.step0 = 1;
        RIA.rw0 = nib[(b0 >> 6) & 3];
        RIA.rw0 = nib[(b0 >> 4) & 3];
        RIA.rw0 = nib[(b0 >> 2) & 3];
        RIA.rw0 = nib[b0 & 3];
        RIA.rw0 = nib[(b1 >> 6) & 3];
        RIA.rw0 = nib[(b1 >> 4) & 3];
        RIA.rw0 = nib[(b1 >> 2) & 3];
        RIA.rw0 = nib[b1 & 3];
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
    unsigned i;
    RIA.addr0 = XR_BITMAP;
    RIA.step0 = 1;
    for (i = 0; i < 38400u; ++i) RIA.rw0 = 0;
}

static void park_sprites(void) {
    uint8_t i;
    RIA.addr0 = XR_SPRITES;
    RIA.step0 = 1;
    for (i = 0; i < NUM_SPRITES; ++i) {
        RIA.rw0 = 0; RIA.rw0 = 0;            // x
        RIA.rw0 = 0xF0; RIA.rw0 = 0xFF;      // y = -16
        RIA.rw0 = (unsigned char)SPRITE_XRAM; RIA.rw0 = (unsigned char)(SPRITE_XRAM >> 8);
        RIA.rw0 = (unsigned char)XR_SPAL; RIA.rw0 = (unsigned char)(XR_SPAL >> 8);
    }
}

void video_init(void) {
    build_nibs();
    clear_bitmap();
    write_palettes();
    park_sprites();
    // mode 3 config
    RIA.addr0 = XR_CFG3;
    RIA.step0 = 1;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 320 & 0xFF; RIA.rw0 = 320 >> 8;
    RIA.rw0 = 240 & 0xFF; RIA.rw0 = 240 >> 8;
    RIA.rw0 = XR_BITMAP & 0xFF; RIA.rw0 = XR_BITMAP >> 8;
    RIA.rw0 = XR_BPAL & 0xFF; RIA.rw0 = XR_BPAL >> 8;
    // mode 1 text config: 40x30, built in font/palette
    RIA.addr0 = XR_CFG1;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = TEXT_COLS; RIA.rw0 = 0;
    RIA.rw0 = TEXT_ROWS; RIA.rw0 = 0;
    RIA.rw0 = XR_TEXT & 0xFF; RIA.rw0 = XR_TEXT >> 8;
    RIA.rw0 = 0xFF; RIA.rw0 = 0xFF;
    RIA.rw0 = 0xFF; RIA.rw0 = 0xFF;
    video_text_clear();
    xreg_vga_canvas(1);
    xreg_vga_mode(3, 2, XR_CFG3, 0, 0, 0);
    xreg_vga_mode(5, 1 | (1 << 3), XR_SPRITES, NUM_SPRITES, 1, 0, 0);
    xreg_vga_mode(1, 0, XR_CFG1, 2, 0, 0);
    dirtyCount = 0;
    redrawAll = 1;
    scoreDirty = 1;
    palDirty = 1;
    blanked = 0;
}

void video_wait_vsync(void) {
    unsigned char v = RIA.vsync;
    while (RIA.vsync == v) {}
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
    xram_write16(XR_CFG3 + 4, on ? 240 : 0);
    if (on) park_sprites();
}

// sprite list update: every visible stamp becomes one or more 16x16 chunks
static void update_sprites(void) {
    uint8_t n = 0;
    uint8_t prior, i;
    unsigned addr = XR_SPRITES;
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
            palp = XR_SPAL + (unsigned)pal * 8;
            sx = 32 + (int)s->x * 2;
            sy = TOP_BORDER + 203 - (int)s->y;
            img = SPRITE_XRAM + slot * 64u;
            for (c = 0; c < chunks && n < NUM_SPRITES; ++c) {
                RIA.addr0 = addr;
                RIA.step0 = 1;
                RIA.rw0 = (unsigned char)sx; RIA.rw0 = (unsigned char)(sx >> 8);
                RIA.rw0 = (unsigned char)sy; RIA.rw0 = (unsigned char)(sy >> 8);
                RIA.rw0 = (unsigned char)img; RIA.rw0 = (unsigned char)(img >> 8);
                RIA.rw0 = (unsigned char)palp; RIA.rw0 = (unsigned char)(palp >> 8);
                addr += 8;
                n++;
                sx += 16;
                img += 64;
            }
        }
    }
    // park the rest
    while (n < NUM_SPRITES) {
        RIA.addr0 = addr + 2;
        RIA.step0 = 1;
        RIA.rw0 = 0xF0; RIA.rw0 = 0xFF;
        addr += 8;
        n++;
    }
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
    unsigned i;
    RIA.addr0 = XR_TEXT;
    RIA.step0 = 1;
    for (i = 0; i < TEXT_COLS * TEXT_ROWS; ++i) RIA.rw0 = ' ';
}

void video_text(uint8_t col, uint8_t row, const char* s) {
    RIA.addr0 = XR_TEXT + (unsigned)row * TEXT_COLS + col;
    RIA.step0 = 1;
    while (*s) RIA.rw0 = (unsigned char)*s++;
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
